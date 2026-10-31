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
    (cd "$TMP" && exec "$TMP/ledger_server" >"$TMP/ledger-server.out" 2>"$TMP/ledger-server.err") &
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
./lume build tests/capstones/rtc_assertions.src -o "$TMP/rtc_assert"
./lume test tests/capstones/rtc_assertions.src | tee "$TMP/rtc-native-tests.out"
grep -q "7 passed, 0 failed" "$TMP/rtc-native-tests.out"
command -v qemu-system-x86_64 >/dev/null 2>&1

first_rc=0
timeout 20s qemu-system-x86_64 \
    -drive format=raw,file="$TMP/kernel_rtc.img" \
    -serial stdio -display none -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -rtc base=2024-06-15T12:34:56 >"$TMP/rtc-first.out" 2>"$TMP/rtc-first.err" || first_rc=$?
[ "$first_rc" -ne 124 ]

second_rc=0
timeout 20s qemu-system-x86_64 \
    -drive format=raw,file="$TMP/kernel_rtc.img" \
    -serial stdio -display none -no-reboot \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -rtc base=2031-11-22T01:02:03 >"$TMP/rtc-second.out" 2>"$TMP/rtc-second.err" || second_rc=$?
[ "$second_rc" -ne 124 ]

printf '@image %s\n@asm %s\n@first %s\n@second %s\n@first_base 2024-06-15T12:34:56\n@second_base 2031-11-22T01:02:03\n' \
    "$TMP/kernel_rtc.img" "$TMP/kernel_rtc.s" "$TMP/rtc-first.out" "$TMP/rtc-second.out" | "$TMP/rtc_assert"
