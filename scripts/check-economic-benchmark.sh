#!/bin/sh
# Validate the real-execution economic benchmark harness and its negative gates (#337).
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

if [ -n "${PYTHON:-}" ]; then
    PYTHON_BIN=$PYTHON
elif command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN=python
else
    echo "check-economic-benchmark: Python 3.8+ is required" >&2
    exit 1
fi

"$ROOT/lume" economy \
    --output "$TMP/results.json" \
    --artifacts "$TMP/artifacts" \
    --self-test >"$TMP/cli-summary.json"

if "$ROOT/lume" mesh bench >"$TMP/legacy-bench.out" 2>"$TMP/legacy-bench.err"; then
    echo "check-economic-benchmark: legacy fixed-metrics command unexpectedly passed" >&2
    exit 1
fi
grep -q "use 'lume economy'" "$TMP/legacy-bench.err"

"$PYTHON_BIN" - "$TMP/results.json" "$TMP/cli-summary.json" "$ROOT" <<'PY'
import hashlib
import json
from pathlib import Path
import sys

report_path = Path(sys.argv[1])
cli_summary = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
root = Path(sys.argv[3])
report = json.loads(report_path.read_text(encoding="utf-8"))
assert report["schema"] == "lume-economic-benchmark/0.1"
assert cli_summary["schema"] == report["schema"]
assert cli_summary["attempts"] == report["summary"]["attempts_total"]
assert report["evaluation_kind"] == "lume_only_harness_conformance"
assert report["source_origin"] == "repository_fixture"
assert report["provider"] == {"id": "UNKNOWN", "model": "UNKNOWN"}
for field in (
    "zero_shot_success",
    "end_to_end_wall_time_ms",
    "input_tokens",
    "output_tokens",
    "total_tokens",
    "generation_cost_usd",
    "verification_cost_usd",
    "maintenance_cost_usd",
):
    assert report["metrics"][field] == "UNKNOWN", field
assert report["harness_self_test"]["outcome"] == "PASS"
assert report["setup"]["outcome"] == "PASS"
assert report["summary"]["attempts_total"] == 9
assert report["summary"]["attempts_passed"] == 4
assert report["summary"]["attempts_failed"] == 5
assert report["summary"]["defects_observed"] == 5

tasks = {task["task_id"]: task for task in report["tasks"]}
assert tasks["clamp_nonnegative"]["attempts_to_pass"] == 1
assert tasks["safe_deposit_repair"]["attempts_to_pass"] == 2
assert tasks["safe_deposit_repair"]["defects_observed"] == 1
expected_failures = {
    "negative_compile_failure": "compile",
    "negative_proof_failure": "proof",
    "negative_execution_failure": "execution",
}
for task_id, failed_stage in expected_failures.items():
    attempt = tasks[task_id]["attempts"][0]
    assert attempt["outcome"] == "FAIL", task_id
    assert attempt["failed_stage"] == failed_stage, task_id
    assert attempt["stages"][failed_stage]["status"] == "FAIL", task_id
assert tasks["negative_proof_failure"]["attempts"][0]["stages"]["compile"]["status"] == "PASS"
assert tasks["negative_execution_failure"]["attempts"][0]["stages"]["proof"]["status"] == "PASS"

maintenance = tasks["deposit_fee_spec_change"]
initial, changed = maintenance["revisions"]
assert maintenance["outcome"] == "PASS"
assert initial["outcome"] == "PASS"
assert changed["attempts_to_pass"] == 2
assert changed["attempts"][0]["outcome"] == "FAIL"
assert changed["attempts"][0]["failed_stage"] == "execution"
assert changed["attempts"][0]["stages"]["execution"]["stdout_mismatch"] is True
assert changed["attempts"][1]["outcome"] == "PASS"
assert initial["attempts"][0]["candidate_sha256"] == changed["attempts"][0]["candidate_sha256"]
assert initial["attempts"][0]["stages"]["execution"]["actual_stdout_sha256"] != changed["attempts"][0]["stages"]["execution"]["expected_stdout_sha256"]
maintenance_metrics = maintenance["maintenance"]
assert maintenance_metrics["attempts_executed"] == 2
assert maintenance_metrics["defects_observed"] == 1
assert maintenance_metrics["verification_wall_time_ms"] >= 0
for field in (
    "generation_wall_time_ms",
    "human_wall_time_ms",
    "input_tokens",
    "output_tokens",
    "total_tokens",
    "cost_usd",
):
    assert maintenance_metrics[field] == "UNKNOWN", field

attempts = [attempt for task in report["tasks"] for attempt in task["attempts"]]
assert report["summary"]["attempts_total"] == len(attempts)
assert report["summary"]["attempts_passed"] == sum(a["outcome"] == "PASS" for a in attempts)
assert report["summary"]["attempts_failed"] == sum(a["outcome"] == "FAIL" for a in attempts)
assert report["summary"]["defects_observed"] == sum(len(a["defects"]) for a in attempts)
for task in report["tasks"]:
    assert task["verification_wall_time_ms"] >= 0
    for attempt in task["attempts"]:
        derived_outcome = "PASS" if all(
            stage["status"] == "PASS" for stage in attempt["stages"].values()
        ) else "FAIL"
        assert attempt["outcome"] == derived_outcome, attempt["candidate"]
        assert attempt["wall_time_ms"] >= 0
        assert len(attempt["candidate_sha256"]) == 64
        for stage in attempt["stages"].values():
            assert stage["wall_time_ms"] >= 0
            for stream, log in stage.get("logs", {}).items():
                log_path = report_path.parent / log
                assert log_path.is_file(), log
                digest = hashlib.sha256(log_path.read_bytes()).hexdigest()
                assert stage["log_sha256"][stream] == digest, log

legacy_fields = (
    "zero_shot_pass_rate_pct",
    "lume_intent_tokens",
    "baseline_raw_text_tokens",
    "token_saving_pct",
    "baseline_attempts_to_green",
    "lume_escaped_defects",
    "baseline_escaped_defects",
    "maintenance_spec_change_tokens",
    "baseline_maintenance_tokens",
)
mesh_source = (root / "compiler" / "stage1" / "lmesh.src").read_text(encoding="utf-8")
assert not any(field in mesh_source for field in legacy_fields)
print("economic benchmark: real execution, spec-change maintenance, and fail-closed gates passed")
PY
