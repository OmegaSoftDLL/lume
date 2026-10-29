#!/usr/bin/env python3
"""Validate and render Lume's machine-readable acceptance matrix."""

import argparse
import json
import os
from pathlib import Path, PurePosixPath
import re
import sys
import urllib.error
import urllib.request

SCHEMA = "lume-acceptance-matrix/0.1"
RESULTS = {"PASS", "FAIL", "UNKNOWN"}
IMPLEMENTATION_STATES = {"shipped", "partial", "model", "planned"}
ISSUE_STATES = {"OPEN", "CLOSED"}
PR_STATES = {"OPEN", "CLOSED", "MERGED"}
CLAIM_ID = re.compile(r"^L(?:\.[A-Z0-9_]+)+$")
ROOT_KEYS = {"schema", "repository", "scope", "claims"}
SCOPE_KEYS = {"track", "issue", "source", "boundary"}
CLAIM_KEYS = {
    "id", "claim", "implementation_state", "result", "issues",
    "pull_requests", "source", "evidence", "notes",
}
EVIDENCE_KEYS = {"id", "command", "artifacts", "expected"}
REFERENCE_KEYS = {"number", "expected_state"}


def no_duplicate_keys(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON key: {key}")
        value[key] = item
    return value


def is_text(value):
    return isinstance(value, str) and bool(value.strip())


def check_keys(value, required, path, errors):
    if not isinstance(value, dict):
        errors.append(f"{path}: must be an object")
        return False
    for key in sorted(required - value.keys()):
        errors.append(f"{path}: missing required key '{key}'")
    for key in sorted(value.keys() - required):
        errors.append(f"{path}: unknown key '{key}'")
    return True


def check_artifact(path_text, repo_root, path, errors):
    if not is_text(path_text):
        errors.append(f"{path}: artifact must be a non-empty string")
        return
    artifact = PurePosixPath(path_text)
    if artifact.is_absolute() or "\\" in path_text or ".." in artifact.parts:
        errors.append(f"{path}: artifact must be a safe repository-relative path: {path_text}")
        return
    if not repo_root.joinpath(*artifact.parts).is_file():
        errors.append(f"{path}: artifact does not exist: {path_text}")


def check_references(value, kind, path, errors):
    if not isinstance(value, list) or not value:
        errors.append(f"{path}: must contain at least one {kind} reference")
        return
    seen = set()
    for index, reference in enumerate(value):
        ref_path = f"{path}[{index}]"
        if not check_keys(reference, REFERENCE_KEYS, ref_path, errors):
            continue
        number = reference["number"]
        if isinstance(number, bool) or not isinstance(number, int) or number <= 0:
            errors.append(f"{ref_path}: {kind} number must be a positive integer")
        elif number in seen:
            errors.append(f"{ref_path}: duplicate {kind} reference #{number}")
        else:
            seen.add(number)
        states = ISSUE_STATES if kind == "issue" else PR_STATES
        if reference["expected_state"] not in states:
            errors.append(f"{ref_path}: invalid expected {kind} state")


def validate_matrix(matrix_path, repo_root, online=False):
    errors = []
    try:
        with matrix_path.open(encoding="utf-8") as stream:
            matrix = json.load(stream, object_pairs_hook=no_duplicate_keys)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        return [f"{matrix_path}: {exc}"], None

    if not check_keys(matrix, ROOT_KEYS, "matrix", errors):
        return errors, None
    if matrix["schema"] != SCHEMA:
        errors.append(f"matrix.schema: expected {SCHEMA}")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", matrix["repository"]):
        errors.append("matrix.repository: expected owner/repository")
    if check_keys(matrix["scope"], SCOPE_KEYS, "matrix.scope", errors):
        if matrix["scope"]["track"] != "L":
            errors.append("matrix.scope.track: this schema slice requires Track L")
        if isinstance(matrix["scope"]["issue"], bool) or not isinstance(matrix["scope"]["issue"], int):
            errors.append("matrix.scope.issue: must be an integer")
        for key in ("source", "boundary"):
            if not is_text(matrix["scope"][key]):
                errors.append(f"matrix.scope.{key}: must be a non-empty string")
        check_artifact(matrix["scope"]["source"], repo_root, "matrix.scope.source", errors)

    claims = matrix["claims"]
    if not isinstance(claims, list) or not claims:
        errors.append("matrix.claims: must be a non-empty array")
        return errors, None

    seen_ids = set()
    for index, claim in enumerate(claims):
        path = f"matrix.claims[{index}]"
        if not check_keys(claim, CLAIM_KEYS, path, errors):
            continue
        claim_id = claim["id"]
        if not is_text(claim_id) or not CLAIM_ID.fullmatch(claim_id):
            errors.append(f"{path}.id: invalid stable Track L claim ID")
            claim_id = path
        elif claim_id in seen_ids:
            errors.append(f"{path}.id: duplicate claim ID {claim_id}")
        else:
            seen_ids.add(claim_id)
        path = f"claim {claim_id}"
        for key in ("claim", "notes"):
            if not is_text(claim[key]):
                errors.append(f"{path}.{key}: must be a non-empty string")
        if claim["implementation_state"] not in IMPLEMENTATION_STATES:
            errors.append(f"{path}.implementation_state: invalid state")
        if claim["result"] not in RESULTS:
            errors.append(f"{path}.result: expected PASS, FAIL, or UNKNOWN")
        check_references(claim["issues"], "issue", f"{path}.issues", errors)
        check_references(claim["pull_requests"], "pull request", f"{path}.pull_requests", errors)
        if isinstance(matrix["scope"], dict) and isinstance(claim["issues"], list):
            scope_issue = matrix["scope"].get("issue")
            if scope_issue not in [ref.get("number") for ref in claim["issues"] if isinstance(ref, dict)]:
                errors.append(f"{path}.issues: missing scope issue #{scope_issue}")

        if not isinstance(claim["source"], list) or not claim["source"]:
            errors.append(f"{path}.source: must contain at least one source artifact")
        else:
            for source_index, source in enumerate(claim["source"]):
                check_artifact(source, repo_root, f"{path}.source[{source_index}]", errors)

        evidence = claim["evidence"]
        if not isinstance(evidence, list):
            errors.append(f"{path}.evidence: must be an array")
        else:
            if claim["result"] in {"PASS", "FAIL"} and not evidence:
                errors.append(f"{path}.evidence: {claim['result']} requires at least one runnable evidence entry")
            evidence_ids = set()
            for evidence_index, item in enumerate(evidence):
                evidence_path = f"{path}.evidence[{evidence_index}]"
                if not check_keys(item, EVIDENCE_KEYS, evidence_path, errors):
                    continue
                if not is_text(item["id"]):
                    errors.append(f"{evidence_path}.id: must be a non-empty string")
                elif item["id"] in evidence_ids:
                    errors.append(f"{evidence_path}.id: duplicate evidence ID in claim")
                else:
                    evidence_ids.add(item["id"])
                if not is_text(item["command"]):
                    errors.append(f"{evidence_path}.command: runnable evidence requires a command")
                if not is_text(item["expected"]):
                    errors.append(f"{evidence_path}.expected: must describe the expected observation")
                if not isinstance(item["artifacts"], list) or not item["artifacts"]:
                    errors.append(f"{evidence_path}.artifacts: must contain test/evidence artifacts")
                else:
                    for artifact_index, artifact in enumerate(item["artifacts"]):
                        check_artifact(artifact, repo_root, f"{evidence_path}.artifacts[{artifact_index}]", errors)

    if online and not errors:
        errors.extend(validate_online(matrix))
    return errors, matrix


def github_json(repository, kind, number):
    url = f"https://api.github.com/repos/{repository}/{kind}/{number}"
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "lume-acceptance-matrix"}
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.load(response)
    except (OSError, ValueError, urllib.error.URLError) as exc:
        raise ValueError(f"cannot validate {url}: {exc}") from exc


def validate_online(matrix):
    errors = []
    references = {"issues": {}, "pulls": {}}
    for claim in matrix["claims"]:
        for reference in claim["issues"]:
            references["issues"].setdefault(reference["number"], set()).add(reference["expected_state"])
        for reference in claim["pull_requests"]:
            references["pulls"].setdefault(reference["number"], set()).add(reference["expected_state"])
    for kind, by_number in references.items():
        for number, expected_states in sorted(by_number.items()):
            try:
                payload = github_json(matrix["repository"], kind, number)
            except ValueError as exc:
                errors.append(str(exc))
                continue
            if payload.get("number") != number:
                errors.append(f"{kind}/{number}: API returned a different reference")
                continue
            if kind == "issues" and "pull_request" in payload:
                errors.append(f"issues/{number}: reference is a pull request, not an issue")
                continue
            actual = payload.get("state", "").upper()
            if kind == "pulls" and payload.get("merged_at") is not None:
                actual = "MERGED"
            if actual not in expected_states:
                expected = "/".join(sorted(expected_states))
                errors.append(f"{kind}/{number}: stale state, expected {expected}, got {actual}")
    return errors


def md(value):
    return str(value).replace("|", "\\|").replace("\n", " ")


def render_markdown(matrix):
    repository = matrix["repository"]
    scope = matrix["scope"]
    counts = {result: sum(claim["result"] == result for claim in matrix["claims"]) for result in sorted(RESULTS)}
    lines = [
        "# Track L acceptance matrix",
        "",
        "<!-- Generated by scripts/check-acceptance-matrix.py; edit acceptance/matrix.json instead. -->",
        "",
        f"Schema: `{matrix['schema']}`. Source: [`{scope['source']}`](../{scope['source']}).",
        "",
        md(scope["boundary"]),
        "",
        "## Result semantics",
        "",
        "- **PASS**: the cited runnable evidence supports the exact, narrowly scoped claim.",
        "- **FAIL**: cited runnable evidence contradicts the claim.",
        "- **UNKNOWN**: evidence is missing, insufficient, not independently checked where required, or not established for the current revision.",
        "- Implementation state (`shipped`, `partial`, `model`, or `planned`) is separate from evidence result.",
        "",
        f"Current declared results: **{counts['PASS']} PASS**, **{counts['FAIL']} FAIL**, **{counts['UNKNOWN']} UNKNOWN**. These counts are not a maturity score.",
        "",
        "| Claim ID | Implementation | Result | Scoped claim | Evidence | Tracking |",
        "|---|---|---|---|---|---|",
    ]
    for claim in matrix["claims"]:
        evidence = ", ".join(f"`{md(item['id'])}`" for item in claim["evidence"]) or "none"
        issues = ", ".join(f"[#{ref['number']}](https://github.com/{repository}/issues/{ref['number']})" for ref in claim["issues"])
        pulls = ", ".join(f"[#{ref['number']}](https://github.com/{repository}/pull/{ref['number']})" for ref in claim["pull_requests"])
        lines.append(
            f"| `{claim['id']}` | {claim['implementation_state']} | **{claim['result']}** | {md(claim['claim'])} | {evidence} | {issues}; {pulls} |"
        )
    lines.extend(["", "## Sources, evidence, and boundaries", ""])
    for claim in matrix["claims"]:
        lines.extend([
            f"### `{claim['id']}`",
            "",
            "Source: " + ", ".join(f"`{source}`" for source in claim["source"]) + ".",
            "",
        ])
        if claim["evidence"]:
            for item in claim["evidence"]:
                lines.extend([
                    f"Evidence `{item['id']}`:",
                    "",
                    "```sh",
                    item["command"],
                    "```",
                    "",
                    f"Expected: {md(item['expected'])}",
                    "",
                    "Artifacts: " + ", ".join(f"`{artifact}`" for artifact in item["artifacts"]) + ".",
                    "",
                ])
        else:
            lines.extend(["Evidence: **UNKNOWN — no sufficient runnable acceptance evidence is registered.**", ""])
        lines.extend([f"Boundary: {md(claim['notes'])}", ""])
    lines.extend([
        "## Consumer commands",
        "",
        "```sh",
        "python3 scripts/check-acceptance-matrix.py --self-test --check-doc",
        "python3 scripts/check-acceptance-matrix.py --online",
        "```",
        "",
        "The first command validates local artifacts and negative fixtures and checks this generated view. The second additionally verifies issue/PR existence and expected state through the GitHub API. Runnable evidence commands are executed by the existing project test jobs; this validator does not treat a declared command as an executed result.",
        "",
    ])
    return "\n".join(lines)


def self_test(repo_root, matrix_path):
    expected = {
        "invalid_missing_artifact.json": "artifact does not exist",
        "invalid_missing_evidence.json": "PASS requires at least one runnable evidence entry",
        "invalid_issue_reference.json": "issue number must be a positive integer",
        "invalid_pr_reference.json": "pull request number must be a positive integer",
        "invalid_result.json": "expected PASS, FAIL, or UNKNOWN",
    }
    fixture_dir = repo_root / "tests" / "acceptance_matrix"
    errors = []
    for filename, message in expected.items():
        fixture = fixture_dir / filename
        fixture_errors, _ = validate_matrix(fixture, repo_root)
        if not fixture_errors:
            errors.append(f"{filename}: invalid fixture silently passed")
        elif not any(message in error for error in fixture_errors):
            errors.append(f"{filename}: expected error containing '{message}', got {fixture_errors}")
        else:
            print(f"ok: rejected {filename}")
    if errors:
        raise ValueError("\n".join(errors))


def main():
    repo_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", type=Path, default=repo_root / "acceptance" / "matrix.json")
    parser.add_argument("--doc", type=Path, default=repo_root / "docs" / "acceptance-matrix.md")
    parser.add_argument("--online", action="store_true", help="validate references through the GitHub API")
    parser.add_argument("--self-test", action="store_true", help="prove negative fixtures are rejected")
    parser.add_argument("--check-doc", action="store_true", help="require the generated Markdown view to be current")
    parser.add_argument("--write-doc", action="store_true", help="regenerate the Markdown view")
    args = parser.parse_args()

    errors, matrix = validate_matrix(args.matrix, repo_root, args.online)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"ok: {len(matrix['claims'])} claims validated" + (" with GitHub references" if args.online else ""))

    if args.self_test:
        try:
            self_test(repo_root, args.matrix)
        except ValueError as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
    if args.write_doc or args.check_doc:
        rendered = render_markdown(matrix)
        if args.write_doc:
            args.doc.write_text(rendered, encoding="utf-8", newline="\n")
            print(f"ok: generated {args.doc}")
        else:
            try:
                current = args.doc.read_text(encoding="utf-8")
            except OSError as exc:
                print(f"error: cannot read generated view: {exc}", file=sys.stderr)
                return 1
            if current != rendered:
                print(f"error: {args.doc} is stale; run scripts/check-acceptance-matrix.py --write-doc", file=sys.stderr)
                return 1
            print(f"ok: generated view is current: {args.doc}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
