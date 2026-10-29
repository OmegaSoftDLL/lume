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

sh "$ROOT/scripts/run-economic-benchmark.sh" \
    --output "$TMP/results.json" \
    --artifacts "$TMP/artifacts" \
    --self-test

"$PYTHON_BIN" - "$TMP/results.json" <<'PY'
import json
from pathlib import Path
import sys

report_path = Path(sys.argv[1])
report = json.loads(report_path.read_text(encoding="utf-8"))
assert report["schema"] == "lume-economic-benchmark/0.1"
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
assert report["summary"]["attempts_total"] == 6
assert report["summary"]["attempts_passed"] == 2
assert report["summary"]["attempts_failed"] == 4
assert report["summary"]["defects_observed"] == 4

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

for task in report["tasks"]:
    assert task["verification_wall_time_ms"] >= 0
    for attempt in task["attempts"]:
        assert attempt["wall_time_ms"] >= 0
        assert len(attempt["candidate_sha256"]) == 64
        for stage in attempt["stages"].values():
            assert stage["wall_time_ms"] >= 0
            for log in stage.get("logs", {}).values():
                assert (report_path.parent / log).is_file(), log
print("economic benchmark: real-execution positive, repair, and negative gates passed")
PY
