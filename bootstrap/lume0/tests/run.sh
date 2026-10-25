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

echo "== codegen: a Lume-written transpiler emits C that compiles and runs (structure -> bits)"
if command -v cc >/dev/null 2>&1; then
    cgtmp=$(mktemp -d)
    $BIN run ../../compiler/stage1/codegen_demo.lume > "$cgtmp/out.c" 2>/dev/null
    if cc -o "$cgtmp/prog" "$cgtmp/out.c" 2>/dev/null && "$cgtmp/prog" 2>/dev/null | diff -u ../../tests/conformance/stage1/codegen_native.expected - >/dev/null; then ok "codegen -> C -> native binary"; else bad "codegen -> C -> native binary"; fi
    $BIN run ../../compiler/stage1/fn_codegen_demo.lume > "$cgtmp/fn.c" 2>/dev/null
    if cc -o "$cgtmp/fnprog" "$cgtmp/fn.c" 2>/dev/null && "$cgtmp/fnprog" 2>/dev/null | diff -u ../../tests/conformance/stage1/fn_codegen_native.expected - >/dev/null; then ok "codegen (functions + if) -> native binary"; else bad "codegen (functions + if) -> native binary"; fi
    # block language (loops, if) -> C -> native binary
    $BIN run ../../compiler/stage1/block_codegen_demo.lume > "$cgtmp/blk.c" 2>/dev/null
    if cc -o "$cgtmp/blk" "$cgtmp/blk.c" 2>/dev/null && "$cgtmp/blk" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_codegen_native.expected - >/dev/null; then ok "block language (loops) -> native binary"; else bad "block language (loops) -> native binary"; fi
    # block language with FUNCTIONS + recursion -> C -> native binary
    $BIN run ../../compiler/stage1/block_fn_demo.lume > "$cgtmp/bf.c" 2>/dev/null
    if cc -o "$cgtmp/bf" "$cgtmp/bf.c" 2>/dev/null && "$cgtmp/bf" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_fn_native.expected - >/dev/null; then ok "block language (functions+recursion) -> native binary"; else bad "block language (functions) -> native binary"; fi
    # block_cc as a file-driven CLI: compile a .blk source file -> C -> native
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/sample.blk "$cgtmp/bcc.c" >/dev/null 2>&1
    if cc -o "$cgtmp/bcc" "$cgtmp/bcc.c" 2>/dev/null && "$cgtmp/bcc" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_cc_native.expected - >/dev/null; then ok "block_cc file compiler (recursion+loops) -> native binary"; else bad "block_cc file compiler -> native binary"; fi
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/sample2.blk "$cgtmp/bcc2.c" >/dev/null 2>&1
    if cc -o "$cgtmp/bcc2" "$cgtmp/bcc2.c" 2>/dev/null && "$cgtmp/bcc2" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_cc2_native.expected - >/dev/null; then ok "block_cc (modulo + and/or) -> native binary"; else bad "block_cc (modulo + and/or) -> native binary"; fi
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/fizz.blk "$cgtmp/fz.c" >/dev/null 2>&1
    if cc -o "$cgtmp/fz" "$cgtmp/fz.c" 2>/dev/null && "$cgtmp/fz" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_fizz_native.expected - >/dev/null; then ok "block_cc FizzBuzz (strings+nested if) -> native binary"; else bad "block_cc FizzBuzz -> native binary"; fi
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/sieve.blk "$cgtmp/sv.c" >/dev/null 2>&1
    if cc -o "$cgtmp/sv" "$cgtmp/sv.c" 2>/dev/null && "$cgtmp/sv" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_sieve_native.expected - >/dev/null; then ok "block_cc Sieve (arrays+nested loops) -> native binary"; else bad "block_cc Sieve -> native binary"; fi
    # a full calculator compiler WRITTEN IN the ML core, reading its source from a file
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/calc.src 2>/dev/null | diff -u ../../tests/conformance/stage1/core_compiler.expected - >/dev/null; then ok "compiler written in the ML core (lex+parse+eval)"; else bad "compiler written in the ML core"; fi
    # close the loop: the core-written compiler emits a full C program -> cc -> native binary
    $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/calc_native.src > "$cgtmp/calc.c" 2>/dev/null
    if cc -o "$cgtmp/calc" "$cgtmp/calc.c" 2>/dev/null && "$cgtmp/calc" 2>/dev/null | diff -u ../../tests/conformance/stage1/core_native.expected - >/dev/null; then ok "core-written compiler -> C -> native binary"; else bad "core-written compiler -> native"; fi
    # lumec as a file-driven compiler: read .lume -> write .c -> native binary
    $BIN run ../../compiler/stage1/lumec_main.lume ../../compiler/stage1/sample.lume "$cgtmp/s.c" >/dev/null 2>&1
    if cc -o "$cgtmp/s" "$cgtmp/s.c" 2>/dev/null && "$cgtmp/s" 2>/dev/null | diff -u ../../tests/conformance/stage1/lumec_file_native.expected - >/dev/null; then ok "lumec file -> C -> native binary"; else bad "lumec file -> C -> native binary"; fi
    rm -rf "$cgtmp"
fi

echo "== direct x86-64 backend: Lume -> assembly -> native binary (NO C in the pipeline)"
if [ "$(uname -s)" = "Linux" ] && [ "$(uname -m)" = "x86_64" ] && command -v as >/dev/null 2>&1 && command -v ld >/dev/null 2>&1; then
    atmp=$(mktemp -d)
    $BIN emit-asm ../../compiler/stage1/arith.lume "$atmp/a.s" 2>/dev/null
    if as -o "$atmp/a.o" "$atmp/a.s" 2>/dev/null && ld -o "$atmp/a" "$atmp/a.o" 2>/dev/null && "$atmp/a" 2>/dev/null | diff -u ../../tests/conformance/stage1/arith_native.expected - >/dev/null; then ok "x86-64 asm -> native (arithmetic, no C)"; else bad "x86-64 asm -> native (arithmetic, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/fib_sample.lume "$atmp/f.s" 2>/dev/null
    if as -o "$atmp/f.o" "$atmp/f.s" 2>/dev/null && ld -o "$atmp/f" "$atmp/f.o" 2>/dev/null && "$atmp/f" 2>/dev/null | diff -u ../../tests/conformance/stage1/fib_native.expected - >/dev/null; then ok "x86-64 asm -> native (recursion + if, no C)"; else bad "x86-64 asm -> native (recursion + if, no C)"; fi
    rm -rf "$atmp"
else
    ok "x86-64 asm backend (skipped: not linux/x86_64 or no as/ld)"
fi

echo "== AI-first: `lume0 describe` emits valid JSON"
if command -v python3 >/dev/null 2>&1; then
    if $BIN describe | python3 -c 'import json,sys; json.load(sys.stdin)' 2>/dev/null; then ok "describe --json"; else bad "describe --json"; fi
fi

echo "== Lume-in-Lume: Stage-1 programs run under lume0"
for exp in ../../tests/conformance/stage1/*.expected; do
    [ -f "$exp" ] || continue
    base=$(basename "$exp" .expected)
    src="../../compiler/stage1/$base.lume"
    [ -f "$src" ] || continue    # expected files without a matching .lume (e.g. native-binary tests) are handled elsewhere
    if $BIN run "$src" 2>/dev/null | diff -u "$exp" - >/dev/null; then ok "compiler/stage1/$base.lume"; else bad "compiler/stage1/$base.lume"; $BIN run "$src" 2>/dev/null | diff -u "$exp" - | head -20; fi
done

echo "== LBE: emit to binary AST and run it with no re-parse (output must match text run)"
tmpd=$(mktemp -d)
for exp in ../../tests/conformance/stage0/*.expected; do
    [ -f "$exp" ] || continue
    base=$(basename "$exp" .expected)
    src="$EX/$base.lume"
    $BIN emit "$src" "$tmpd/$base.lbe" 2>/dev/null
    if $BIN run "$tmpd/$base.lbe" 2>/dev/null | diff -u "$exp" - >/dev/null; then ok "lbe $base"; else bad "lbe $base"; fi
done
rm -rf "$tmpd"

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
