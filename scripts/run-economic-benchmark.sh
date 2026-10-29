#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ -n "${PYTHON:-}" ]; then
    PYTHON_BIN=$PYTHON
elif command -v python3 >/dev/null 2>&1; then
    PYTHON_BIN=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON_BIN=python
else
    echo "run-economic-benchmark: Python 3.8+ is required" >&2
    exit 1
fi

exec "$PYTHON_BIN" "$ROOT/scripts/economic_benchmark.py" "$@"
