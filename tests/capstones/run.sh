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
./lume build tests/capstones/rtc_assertions.src -o "$TMP/rtc_assert"
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
