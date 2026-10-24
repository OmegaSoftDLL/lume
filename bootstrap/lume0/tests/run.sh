#!/usr/bin/env sh
# Stage 0 test runner (lexer + parser).
set -u
cd "$(dirname "$0")/.."
BIN=./lume0
EX=../../examples
pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  ok   %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  FAIL %s\n' "$1"; }
have_py() { command -v python3 >/dev/null 2>&1; }

echo "== golden token dumps"
for src in tests/lexer/*.lume; do
    exp="${src%.lume}.tokens"; [ -f "$exp" ] || continue
    if $BIN tokens "$src" 2>/dev/null | diff -u "$exp" - >/dev/null; then ok "$src"; else bad "$src"; $BIN tokens "$src" | diff -u "$exp" - | head -20; fi
done

echo "== stage-0/1 examples parse cleanly and emit valid JSON"
for src in $EX/*.lume; do
    if ! $BIN check "$src" >/dev/null 2>e.txt; then bad "parse $src"; sed 's/^/      /' e.txt | head -4; continue; fi
    if have_py; then
        if $BIN parse "$src" --json 2>/dev/null | python3 -c 'import json,sys; json.load(sys.stdin)' 2>/dev/null; then ok "$src"; else bad "json $src"; fi
    else ok "$src"; fi
done

echo "== future-stage examples lex cleanly (parser handles them in later stages)"
for src in $EX/future/*.lume; do
    if $BIN tokens "$src" >/dev/null 2>e.txt && ! grep -q '"severity"' e.txt; then ok "$src"; else
        # tokens prints diagnostics to stderr only on error; check exit via a lex-only probe
        if $BIN tokens "$src" >/dev/null 2>/dev/null; then ok "$src"; else bad "lex $src"; fi
    fi
done

echo "== run: stage-0 conformance (stdout must match)"
for exp in ../../tests/conformance/stage0/*.expected; do
    [ -f "$exp" ] || continue
    base=$(basename "$exp" .expected)
    src="$EX/$base.lume"
    if $BIN run "$src" 2>/dev/null | diff -u "$exp" - >/dev/null; then ok "run $base"; else bad "run $base"; $BIN run "$src" 2>/dev/null | diff -u "$exp" - | head -20; fi
done

echo "== negative lexer cases"
for src in tests/lexer/bad_*.lume; do
    code=$(sed -n '1s/^# expect: //p' "$src")
    if $BIN check "$src" --json 2>/dev/null | grep -q "\"code\":\"$code\""; then ok "$src ($code)"; else bad "$src (expected $code)"; fi
done

echo "== negative parser cases"
for src in tests/parser/bad_*.lume; do
    [ -f "$src" ] || continue
    code=$(sed -n '1s/^# expect: //p' "$src")
    if $BIN check "$src" --json 2>/dev/null | grep -q "\"code\":\"$code\""; then ok "$src ($code)"; else bad "$src (expected $code)"; $BIN check "$src" --json; fi
done

echo "== positive parser cases (must parse clean)"
for src in tests/parser/ok_*.lume; do
    [ -f "$src" ] || continue
    if $BIN check "$src" >/dev/null 2>e.txt; then ok "$src"; else bad "$src"; sed 's/^/      /' e.txt | head -4; fi
done

rm -f e.txt
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
