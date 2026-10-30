#!/bin/sh
# Validate the real-execution economic benchmark harness and its negative gates (#337).
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

"$ROOT/lume" build "$ROOT/tests/economic_benchmark/assertions.src" -o "$TMP/economic_assert"

"$ROOT/lume" economy \
    --output "$TMP/results.json" \
    --artifacts "$TMP/artifacts" \
    --self-test >"$TMP/cli-summary.json"

if "$ROOT/lume" mesh bench >"$TMP/legacy-bench.out" 2>"$TMP/legacy-bench.err"; then
    echo "check-economic-benchmark: legacy fixed-metrics command unexpectedly passed" >&2
    exit 1
fi
grep -q "use 'lume economy'" "$TMP/legacy-bench.err"

printf '@path %s\n@cli %s\n@base %s\n@root %s\n' \
    "$TMP/results.json" "$TMP/cli-summary.json" "$TMP" "$ROOT" | "$TMP/economic_assert"
