#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"
BIN_DIR=${LUME_BIN_DIR:-$ROOT/build/bin}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

./lume build examples/apps/p0_tenant_ledger/ledger_server.src -o "$TMP/ledger_server"
python3 tests/capstones/test_saas_http.py "$TMP/ledger_server"

./lume compile examples/apps/p0_rtc_kernel/kernel_rtc.src >"$TMP/kernel_rtc.s"
"$BIN_DIR/lasm" <"$TMP/kernel_rtc.s" >"$TMP/kernel_rtc.img"
python3 tests/capstones/test_rtc_qemu.py "$TMP/kernel_rtc.img" "$TMP/kernel_rtc.s"
