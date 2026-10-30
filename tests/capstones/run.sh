#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"
BIN_DIR=${LUME_BIN_DIR:-$ROOT/build/bin}
TMP=$(mktemp -d)
trap 'if [ -n "${SERVER_PID:-}" ]; then kill "$SERVER_PID" 2>/dev/null || true; fi; rm -rf "$TMP"' EXIT HUP INT TERM

./lume build examples/apps/p0_tenant_ledger/ledger_server.src -o "$TMP/ledger_server"
./lume build tests/capstones/saas_assertions.src -o "$TMP/saas_assert"

SERVER_PID=
start_server() {
    (cd "$TMP" && "$TMP/ledger_server" >"$TMP/ledger-server.out" 2>"$TMP/ledger-server.err") &
    SERVER_PID=$!
    attempts=0
    until (cd "$TMP" && printf 'health' | "$TMP/saas_assert" >/dev/null 2>&1); do
        attempts=$((attempts + 1))
        if [ "$attempts" -ge 200 ]; then
            cat "$TMP/ledger-server.out" "$TMP/ledger-server.err"
            return 1
        fi
        sleep 0.05
    done
}
stop_server() {
    kill "$SERVER_PID"
    server_rc=0
    wait "$SERVER_PID" || server_rc=$?
    SERVER_PID=
    [ "$server_rc" -eq 0 ] || [ "$server_rc" -eq 143 ]
}

start_server
(cd "$TMP" && printf 'phase1' | "$TMP/saas_assert")
stop_server
(cd "$TMP" && printf 'wal' | "$TMP/saas_assert")

start_server
(cd "$TMP" && printf 'phase2' | "$TMP/saas_assert")
stop_server

start_server
(cd "$TMP" && printf 'phase3' | "$TMP/saas_assert")
stop_server

./lume compile examples/apps/p0_rtc_kernel/kernel_rtc.src >"$TMP/kernel_rtc.s"
"$BIN_DIR/lasm" <"$TMP/kernel_rtc.s" >"$TMP/kernel_rtc.img"
python3 tests/capstones/test_rtc_qemu.py "$TMP/kernel_rtc.img" "$TMP/kernel_rtc.s"
