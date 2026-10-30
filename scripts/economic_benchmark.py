#!/usr/bin/env python3
"""Execute Lume's versioned economic benchmark protocol without network access."""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import signal
import subprocess
import sys
import time

SCHEMA = "lume-economic-benchmark/0.1"
SUITE_SCHEMA = "lume-economic-benchmark-suite/0.1"
SUPPORTED_TARGET = "x86_64-linux-elf"
ROOT = Path(__file__).resolve().parent.parent
DIAGNOSTIC_CODE = re.compile(rb"\[([A-Z][0-9]{4})\]")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run real Lume compilation, proof, and native execution attempts."
    )
    parser.add_argument(
        "--suite",
        type=Path,
        default=ROOT / "tests" / "economic_benchmark" / "suite.json",
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path)
    parser.add_argument("--setup-timeout", type=float, default=900.0)
    parser.add_argument("--compile-timeout", type=float, default=120.0)
    parser.add_argument("--proof-timeout", type=float, default=120.0)
    parser.add_argument("--execution-timeout", type=float, default=10.0)
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="Check fixture outcomes; never interpret them as model scores.",
    )
    args = parser.parse_args()
    for name in ("setup_timeout", "compile_timeout", "proof_timeout", "execution_timeout"):
        if getattr(args, name) <= 0:
            parser.error("--%s must be positive" % name.replace("_", "-"))
    args.suite = args.suite.resolve()
    args.output = args.output.resolve()
    args.artifacts = (
        args.artifacts.resolve()
        if args.artifacts
        else args.output.with_suffix(".artifacts")
    )
    return args


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_value(args):
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=str(ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return "UNKNOWN"
    if result.returncode != 0:
        return "UNKNOWN"
    return result.stdout.decode("utf-8").strip()


def relative_display(path, base):
    return Path(os.path.relpath(str(path), str(base))).as_posix()


def run_process(command, timeout, stdout_path, stderr_path):
    started = time.monotonic_ns()
    timed_out = False
    try:
        process = subprocess.Popen(
            command,
            cwd=str(ROOT),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=(os.name == "posix"),
        )
    except OSError as error:
        stdout_path.write_bytes(b"")
        stderr_path.write_bytes(str(error).encode("utf-8"))
        return {
            "status": "FAIL",
            "exit_code": "UNKNOWN",
            "timed_out": False,
            "wall_time_ms": (time.monotonic_ns() - started) // 1000000,
            "launch_error": str(error),
        }
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        if os.name == "posix":
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        else:
            process.kill()
        stdout, stderr = process.communicate()
    stdout_path.write_bytes(stdout)
    stderr_path.write_bytes(stderr)
    return {
        "status": "PASS" if process.returncode == 0 and not timed_out else "FAIL",
        "exit_code": "UNKNOWN" if timed_out else process.returncode,
        "timed_out": timed_out,
        "wall_time_ms": (time.monotonic_ns() - started) // 1000000,
    }


def diagnostic_codes(* payloads):
    codes = set()
    for payload in payloads:
        for code in DIAGNOSTIC_CODE.findall(payload):
            codes.add(code.decode("ascii"))
    return sorted(codes)


def not_run_stage():
    return {"status": "NOT_RUN", "wall_time_ms": 0}


def add_log_evidence(result, stdout_path, stderr_path, report_base):
    result["logs"] = {
        "stdout": relative_display(stdout_path, report_base),
        "stderr": relative_display(stderr_path, report_base),
    }
    result["log_sha256"] = {
        "stdout": sha256_file(stdout_path),
        "stderr": sha256_file(stderr_path),
    }


def proof_summary(stdout_path):
    try:
        data = json.loads(stdout_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, ValueError):
        return False, {}, ["proof output was not valid JSON"]
    proofs = data.get("proofs")
    if not isinstance(proofs, list):
        return False, {}, ["proof output did not contain a proofs array"]
    counts = {}
    malformed = False
    for proof in proofs:
        if not isinstance(proof, dict) or not isinstance(proof.get("status"), str):
            malformed = True
            continue
        status = proof["status"]
        counts[status] = counts.get(status, 0) + 1
    errors = []
    if data.get("ok") is not True:
        errors.append("proof command did not report ok=true")
    if not proofs:
        errors.append("no proof obligations were reported")
    if malformed:
        errors.append("malformed proof obligation")
    non_proven = sorted(status for status in counts if status != "proven")
    if non_proven:
        errors.append("non-proven obligations: " + ", ".join(non_proven))
    return not errors, counts, errors


def defect_for(stage, stage_result, proof_errors, stdout, stderr):
    if stage_result.get("timed_out"):
        kind = "timeout"
    elif stage == "compile":
        kind = "nonzero_exit" if stage_result.get("exit_code") != 0 else "artifact_missing"
    elif stage == "proof":
        if any("no proof obligations" in error for error in proof_errors):
            kind = "missing_obligations"
        elif any("non-proven obligations" in error for error in proof_errors):
            kind = "not_fully_proven"
        elif stage_result.get("exit_code") != 0:
            kind = "refuted_or_rejected"
        else:
            kind = "protocol_error"
    elif stage_result.get("exit_code") != 0:
        kind = "nonzero_exit"
    else:
        kind = "stdout_mismatch"
    defect = {"stage": stage, "kind": kind}
    codes = diagnostic_codes(stdout, stderr)
    if codes:
        defect["diagnostic_codes"] = codes
    if proof_errors:
        defect["details"] = proof_errors
    return defect


def run_attempt(suite_dir, revision, attempt, attempt_number, args, attempt_dir, report_base):
    source = (suite_dir / attempt["path"]).resolve()
    try:
        source.relative_to(suite_dir)
    except ValueError:
        raise ValueError("candidate path escapes the suite directory: %s" % attempt["path"])
    if not source.is_file():
        raise ValueError("candidate does not exist: %s" % source)
    source_rel = source.relative_to(suite_dir).as_posix()
    attempt_dir.mkdir(parents=True, exist_ok=True)
    binary = attempt_dir / "program"
    if binary.exists():
        binary.unlink()
    stages = {}
    defects = []

    compile_stdout = attempt_dir / "compile.stdout"
    compile_stderr = attempt_dir / "compile.stderr"
    compile_result = run_process(
        [str(ROOT / "lume"), "build", "--target", SUPPORTED_TARGET, str(source), "-o", str(binary)],
        args.compile_timeout,
        compile_stdout,
        compile_stderr,
    )
    compile_result["command"] = [
        "lume", "build", "--target", SUPPORTED_TARGET, source_rel, "-o", "<artifact>"
    ]
    add_log_evidence(compile_result, compile_stdout, compile_stderr, report_base)
    if compile_result["status"] == "PASS" and not binary.is_file():
        compile_result["status"] = "FAIL"
        compile_result["artifact_missing"] = True
    if binary.is_file():
        compile_result["artifact_sha256"] = sha256_file(binary)
    stages["compile"] = compile_result

    proof_errors = []
    if compile_result["status"] == "PASS":
        proof_stdout = attempt_dir / "proof.stdout"
        proof_stderr = attempt_dir / "proof.stderr"
        proof_result = run_process(
            [str(ROOT / "lume"), "prove", "--json", str(source)],
            args.proof_timeout,
            proof_stdout,
            proof_stderr,
        )
        proof_ok, proof_counts, proof_errors = proof_summary(proof_stdout)
        proof_result["proof_counts"] = proof_counts
        proof_result["validation_errors"] = proof_errors
        if proof_result["status"] == "PASS" and not proof_ok:
            proof_result["status"] = "FAIL"
        proof_result["command"] = ["lume", "prove", "--json", source_rel]
        add_log_evidence(proof_result, proof_stdout, proof_stderr, report_base)
        stages["proof"] = proof_result
    else:
        stages["proof"] = not_run_stage()
        defects.append(
            defect_for(
                "compile",
                compile_result,
                [],
                compile_stdout.read_bytes(),
                compile_stderr.read_bytes(),
            )
        )

    if stages["proof"]["status"] == "PASS":
        execution_stdout = attempt_dir / "execution.stdout"
        execution_stderr = attempt_dir / "execution.stderr"
        execution_result = run_process(
            [str(binary)], args.execution_timeout, execution_stdout, execution_stderr
        )
        expected_stdout = revision["expected_stdout"].encode("utf-8")
        actual_stdout = execution_stdout.read_bytes()
        execution_result["expected_stdout_sha256"] = hashlib.sha256(expected_stdout).hexdigest()
        execution_result["actual_stdout_sha256"] = hashlib.sha256(actual_stdout).hexdigest()
        if execution_result["status"] == "PASS" and actual_stdout != expected_stdout:
            execution_result["status"] = "FAIL"
            execution_result["stdout_mismatch"] = True
        execution_result["command"] = ["<artifact>"]
        add_log_evidence(execution_result, execution_stdout, execution_stderr, report_base)
        stages["execution"] = execution_result
        if execution_result["status"] != "PASS":
            defects.append(
                defect_for(
                    "execution",
                    execution_result,
                    [],
                    actual_stdout,
                    execution_stderr.read_bytes(),
                )
            )
    else:
        stages["execution"] = not_run_stage()
        if stages["proof"]["status"] != "NOT_RUN":
            proof_result = stages["proof"]
            defects.append(
                defect_for(
                    "proof",
                    proof_result,
                    proof_errors,
                    (attempt_dir / "proof.stdout").read_bytes(),
                    (attempt_dir / "proof.stderr").read_bytes(),
                )
            )

    outcome = "PASS" if all(stage["status"] == "PASS" for stage in stages.values()) else "FAIL"
    failed_stage = None
    for name in ("compile", "proof", "execution"):
        if stages[name]["status"] == "FAIL":
            failed_stage = name
            break
    return {
        "attempt": attempt_number,
        "candidate": source_rel,
        "candidate_sha256": sha256_file(source),
        "outcome": outcome,
        "failed_stage": failed_stage,
        "wall_time_ms": sum(stage["wall_time_ms"] for stage in stages.values()),
        "defects": defects,
        "stages": stages,
    }


def check_attempt_self_test(task_id, expected_attempts, actual_attempts):
    errors = []
    if len(expected_attempts) != len(actual_attempts):
        return ["%s: attempt count differed from self-test expectation" % task_id]
    for expected, actual in zip(expected_attempts, actual_attempts):
        expectation = expected["self_test"]
        if actual["outcome"] != expectation["outcome"]:
            errors.append(
                "%s attempt %d: expected %s, got %s"
                % (task_id, actual["attempt"], expectation["outcome"], actual["outcome"])
            )
        if actual["failed_stage"] != expectation["failed_stage"]:
            errors.append(
                "%s attempt %d: expected failed_stage %s, got %s"
                % (
                    task_id,
                    actual["attempt"],
                    expectation["failed_stage"],
                    actual["failed_stage"],
                )
            )
    return errors


def check_self_test(tasks):
    errors = []
    for task in tasks:
        definition = task["definition"]
        if "revisions" in definition:
            for expected_revision, actual_revision in zip(
                definition["revisions"], task["revisions"]
            ):
                errors.extend(
                    check_attempt_self_test(
                        task["task_id"] + "/" + actual_revision["revision_id"],
                        expected_revision["attempts"],
                        actual_revision["attempts"],
                    )
                )
        else:
            errors.extend(
                check_attempt_self_test(task["task_id"], definition["attempts"], task["attempts"])
            )
    return errors


def run_revision(suite_dir, task, revision, args, report_base, revision_id=None):
    started = time.monotonic_ns()
    result = {"attempts": []}
    first_pass = None
    for index, attempt in enumerate(revision["attempts"], 1):
        attempt_dir = args.artifacts / task["id"]
        if revision_id is not None:
            attempt_dir = attempt_dir / revision_id
        attempt_dir = attempt_dir / ("attempt-%d" % index)
        attempt_result = run_attempt(
            suite_dir, revision, attempt, index, args, attempt_dir, report_base
        )
        if revision_id is not None:
            attempt_result["revision"] = revision_id
        result["attempts"].append(attempt_result)
        if attempt_result["outcome"] == "PASS":
            first_pass = index
            break
    result["attempts_executed"] = len(result["attempts"])
    result["attempts_to_pass"] = first_pass
    result["defects_observed"] = sum(
        len(attempt["defects"]) for attempt in result["attempts"]
    )
    result["verification_wall_time_ms"] = (time.monotonic_ns() - started) // 1000000
    result["outcome"] = "PASS" if first_pass is not None else "FAIL"
    return result


def prepare_toolchain(args, report_base):
    setup_dir = args.artifacts / "setup"
    setup_dir.mkdir(parents=True, exist_ok=True)
    steps = {}
    commands = [
        ("describe", [str(ROOT / "lume"), "describe", "--json"]),
        (
            "proof",
            [
                str(ROOT / "lume"),
                "prove",
                "--json",
                str(ROOT / "tests" / "economic_benchmark" / "tasks" / "clamp_nonnegative" / "attempt1.src"),
            ],
        ),
    ]
    for name, command in commands:
        stdout_path = setup_dir / (name + ".stdout")
        stderr_path = setup_dir / (name + ".stderr")
        result = run_process(command, args.setup_timeout, stdout_path, stderr_path)
        add_log_evidence(result, stdout_path, stderr_path, report_base)
        steps[name] = result
        if result["status"] != "PASS":
            diagnostics = stderr_path.read_text(encoding="utf-8", errors="replace")
            raise SystemExit(
                "economic benchmark toolchain setup failed at %s:\n%s" % (name, diagnostics)
            )
    return {
        "outcome": "PASS",
        "wall_time_ms": sum(step["wall_time_ms"] for step in steps.values()),
        "steps": steps,
    }


def main():
    args = parse_args()
    if sys.version_info < (3, 8):
        raise SystemExit("economic benchmark requires Python 3.8 or newer")
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise SystemExit("economic benchmark protocol 0.1 requires native Linux x86-64")
    try:
        suite_bytes = args.suite.read_bytes()
        suite = json.loads(suite_bytes.decode("utf-8"))
    except (OSError, UnicodeError, ValueError) as error:
        raise SystemExit("cannot read benchmark suite: %s" % error)
    if suite.get("schema") != SUITE_SCHEMA:
        raise SystemExit("unsupported benchmark suite schema")
    if suite.get("target") != SUPPORTED_TARGET:
        raise SystemExit("suite target does not match the protocol's native execution target")
    if not isinstance(suite.get("tasks"), list) or not suite["tasks"]:
        raise SystemExit("benchmark suite must contain tasks")

    suite_dir = args.suite.parent
    report_base = args.output.parent
    repository = {
        "commit": git_value(["rev-parse", "HEAD"]),
        "dirty": git_value(["status", "--porcelain"]) != "",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.artifacts.mkdir(parents=True, exist_ok=True)
    setup = prepare_toolchain(args, report_base)
    run_started = time.monotonic_ns()
    tasks = []
    for task in suite["tasks"]:
        task_started = time.monotonic_ns()
        if "revisions" in task:
            if len(task["revisions"]) != 2:
                raise SystemExit("spec-change maintenance tasks require exactly two revisions")
            task_result = {
                "task_id": task["id"],
                "attempts": [],
                "revisions": [],
                "definition": task,
            }
            for revision in task["revisions"]:
                revision_result = run_revision(
                    suite_dir, task, revision, args, report_base, revision["id"]
                )
                revision_result["revision_id"] = revision["id"]
                task_result["revisions"].append(revision_result)
                task_result["attempts"].extend(revision_result["attempts"])
            initial, changed = task_result["revisions"]
            task_result["attempts_executed"] = len(task_result["attempts"])
            task_result["attempts_to_pass"] = None
            task_result["defects_observed"] = sum(
                len(attempt["defects"]) for attempt in task_result["attempts"]
            )
            task_result["maintenance"] = {
                "from_revision": initial["revision_id"],
                "to_revision": changed["revision_id"],
                "attempts_executed": changed["attempts_executed"],
                "attempts_to_pass": changed["attempts_to_pass"],
                "defects_observed": changed["defects_observed"],
                "verification_wall_time_ms": changed["verification_wall_time_ms"],
                "generation_wall_time_ms": "UNKNOWN",
                "human_wall_time_ms": "UNKNOWN",
                "input_tokens": "UNKNOWN",
                "output_tokens": "UNKNOWN",
                "total_tokens": "UNKNOWN",
                "cost_usd": "UNKNOWN",
            }
            task_result["outcome"] = (
                "PASS"
                if all(revision["outcome"] == "PASS" for revision in task_result["revisions"])
                else "FAIL"
            )
        else:
            task_result = run_revision(suite_dir, task, task, args, report_base)
            task_result["task_id"] = task["id"]
            task_result["definition"] = task
        task_result["verification_wall_time_ms"] = (
            time.monotonic_ns() - task_started
        ) // 1000000
        tasks.append(task_result)

    attempts = [attempt for task in tasks for attempt in task["attempts"]]
    report = {
        "schema": SCHEMA,
        "evaluation_kind": "lume_only_harness_conformance",
        "source_origin": "repository_fixture",
        "suite": {
            "id": suite["id"],
            "schema": suite["schema"],
            "sha256": hashlib.sha256(suite_bytes).hexdigest(),
            "target": suite["target"],
        },
        "repository": repository,
        "setup": setup,
        "environment": {
            "system": platform.system(),
            "machine": platform.machine(),
            "platform": platform.platform(),
            "python": platform.python_version(),
        },
        "provider": {"id": "UNKNOWN", "model": "UNKNOWN"},
        "metrics": {
            "zero_shot_success": "UNKNOWN",
            "end_to_end_wall_time_ms": "UNKNOWN",
            "input_tokens": "UNKNOWN",
            "output_tokens": "UNKNOWN",
            "total_tokens": "UNKNOWN",
            "generation_cost_usd": "UNKNOWN",
            "verification_cost_usd": "UNKNOWN",
            "maintenance_cost_usd": "UNKNOWN",
        },
        "summary": {
            "tasks_total": len(tasks),
            "tasks_passed": sum(task["outcome"] == "PASS" for task in tasks),
            "tasks_failed": sum(task["outcome"] == "FAIL" for task in tasks),
            "attempts_total": len(attempts),
            "attempts_passed": sum(attempt["outcome"] == "PASS" for attempt in attempts),
            "attempts_failed": sum(attempt["outcome"] == "FAIL" for attempt in attempts),
            "defects_observed": sum(len(attempt["defects"]) for attempt in attempts),
            "verification_wall_time_ms": (time.monotonic_ns() - run_started) // 1000000,
        },
        "timeouts_seconds": {
            "setup": args.setup_timeout,
            "compile": args.compile_timeout,
            "proof": args.proof_timeout,
            "execution": args.execution_timeout,
        },
        "claim_boundaries": [
            "Repository fixtures validate the harness and are not model generations.",
            "No zero-shot, cross-model, cross-language, superiority, or cost result is established.",
            "Wall time covers local verification only; generation and human maintenance time are unavailable.",
            "Only native Linux x86-64 execution is measured by protocol 0.1.",
        ],
        "tasks": tasks,
    }
    report["generated_at"] = datetime.datetime.now(datetime.timezone.utc).isoformat()

    self_test_errors = check_self_test(tasks) if args.self_test else []
    if args.self_test:
        report["harness_self_test"] = {
            "outcome": "PASS" if not self_test_errors else "FAIL",
            "errors": self_test_errors,
        }
    for task in report["tasks"]:
        definition = task.pop("definition")
        if not args.self_test:
            revisions = definition.get("revisions", [definition])
            for revision in revisions:
                for attempt in revision["attempts"]:
                    attempt.pop("self_test", None)
    temporary = args.output.with_name(args.output.name + ".tmp")
    temporary.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(str(temporary), str(args.output))
    print(json.dumps({
        "schema": SCHEMA,
        "output": str(args.output),
        "attempts": len(attempts),
        "defects_observed": report["summary"]["defects_observed"],
        "harness_self_test": report.get("harness_self_test", {"outcome": "NOT_RUN"}),
    }, sort_keys=True))
    if self_test_errors:
        for error in self_test_errors:
            print("self-test: " + error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
