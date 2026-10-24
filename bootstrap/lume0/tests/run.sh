#!/usr/bin/env sh
# Stage 0 test runner: golden token dumps + every example must lex cleanly + negative cases.
set -u
cd "$(dirname "$0")/.."
BIN=./lume0
pass=0; fail=0
ok()   { pass=$((pass+1)); printf '  ok   %s\n' "$1"; }
bad()  { fail=$((fail+1)); printf '  FAIL %s\n' "$1"; }

echo "== golden token dumps"
for src in tests/lexer/*.lume; do
    exp="${src%.lume}.tokens"
    [ -f "$exp" ] || continue
    if $BIN tokens "$src" 2>/dev/null | diff -u "$exp" - >/dev/null; then ok "$src"; else bad "$src"; $BIN tokens "$src" | diff -u "$exp" - | head -20; fi
done

echo "== examples lex without errors"
for src in ../../examples/*.lume ../../examples/future/*.lume; do
    if $BIN check "$src" >/dev/null 2>&1; then ok "$src"; else bad "$src"; $BIN check "$src"; fi
done

echo "== negative cases (must report the expected code)"
for src in tests/lexer/bad_*.lume; do
    code=$(sed -n '1s/^# expect: //p' "$src")
    if $BIN check "$src" --json 2>/dev/null | grep -q "\"code\":\"$code\""; then ok "$src ($code)"; else bad "$src (expected $code)"; $BIN check "$src" --json; fi
done

echo "== JSON output is well-formed"
if command -v python3 >/dev/null 2>&1; then
    if $BIN tokens tests/lexer/indent.lume --json | python3 -c 'import json,sys; json.load(sys.stdin)'; then ok "tokens --json"; else bad "tokens --json"; fi
fi

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
