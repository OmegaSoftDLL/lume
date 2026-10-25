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
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/sort.blk "$cgtmp/sort.c" >/dev/null 2>&1
    if cc -o "$cgtmp/sort" "$cgtmp/sort.c" 2>/dev/null && "$cgtmp/sort" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_sort_native.expected - >/dev/null; then ok "block_cc bubble sort (arrays+negatives) -> native binary"; else bad "block_cc bubble sort -> native binary"; fi
    $BIN run ../../compiler/stage1/block_cc.lume ../../compiler/stage1/forloop.blk "$cgtmp/fl.c" >/dev/null 2>&1
    if cc -o "$cgtmp/fl" "$cgtmp/fl.c" 2>/dev/null && "$cgtmp/fl" 2>/dev/null | diff -u ../../tests/conformance/stage1/block_for_native.expected - >/dev/null; then ok "block_cc for-loops (range desugar + nested) -> native binary"; else bad "block_cc for-loops -> native binary"; fi
    # a full calculator compiler WRITTEN IN the ML core, reading its source from a file
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/calc.src 2>/dev/null | diff -u ../../tests/conformance/stage1/core_compiler.expected - >/dev/null; then ok "compiler written in the ML core (lex+parse+eval)"; else bad "compiler written in the ML core"; fi
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/modtest.src 2>/dev/null | diff -u ../../tests/conformance/stage1/modtest_core.expected - >/dev/null; then ok "ML-core interpreter: modulo + gcd (regression)"; else bad "ML-core interpreter: modulo + gcd"; fi
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/esc_test.src 2>/dev/null | diff -u ../../tests/conformance/stage1/esc_test_core.expected - >/dev/null; then ok "ML-core lexer: string escapes (quote, backslash, n, t)"; else bad "ML-core lexer: string escapes"; fi
    # close the loop: the core-written compiler emits a full C program -> cc -> native binary
    $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/calc_native.src > "$cgtmp/calc.c" 2>/dev/null
    if cc -o "$cgtmp/calc" "$cgtmp/calc.c" 2>/dev/null && "$cgtmp/calc" 2>/dev/null | diff -u ../../tests/conformance/stage1/core_native.expected - >/dev/null; then ok "core-written compiler -> C -> native binary"; else bad "core-written compiler -> native"; fi
    # lumec as a file-driven compiler: read .lume -> write .c -> native binary
    $BIN run ../../compiler/stage1/lumec_main.lume ../../compiler/stage1/sample.lume "$cgtmp/s.c" >/dev/null 2>&1
    if cc -o "$cgtmp/s" "$cgtmp/s.c" 2>/dev/null && "$cgtmp/s" 2>/dev/null | diff -u ../../tests/conformance/stage1/lumec_file_native.expected - >/dev/null; then ok "lumec file -> C -> native binary"; else bad "lumec file -> C -> native binary"; fi
    # the ML-core compiler (data_codegen) lowers enum + match to C -> native binary
    $BIN run ../../compiler/stage1/data_cc.lume ../../compiler/stage1/ev.src "$cgtmp/ev.c" >/dev/null 2>&1
    if cc -o "$cgtmp/ev" "$cgtmp/ev.c" 2>/dev/null && "$cgtmp/ev" 2>/dev/null | diff -u ../../tests/conformance/stage1/ev_core_native.expected - >/dev/null; then ok "ML-core enum+match -> C -> native binary"; else bad "ML-core enum+match -> C -> native"; fi
    # the FULL core-written compiler (tokenize+parse+eval+cgen) via data_codegen -> C -> native
    $BIN run ../../compiler/stage1/data_cc.lume ../../compiler/stage1/calc.src "$cgtmp/calc2.c" >/dev/null 2>&1
    if cc -o "$cgtmp/calc2" "$cgtmp/calc2.c" 2>/dev/null && "$cgtmp/calc2" 2>/dev/null | diff -u ../../tests/conformance/stage1/calc_core_native.expected - >/dev/null; then ok "core compiler (records+fields+builtins) -> C -> native binary"; else bad "core compiler -> C -> native"; fi
    $BIN run ../../compiler/stage1/data_cc.lume ../../compiler/stage1/list.src "$cgtmp/list.c" >/dev/null 2>&1
    if cc -o "$cgtmp/list" "$cgtmp/list.c" 2>/dev/null && "$cgtmp/list" 2>/dev/null | diff -u ../../tests/conformance/stage1/list_core_native.expected - >/dev/null; then ok "ML-core lists + indexing -> C -> native binary"; else bad "ML-core lists + indexing -> C -> native"; fi
    rm -rf "$cgtmp"
fi

echo "== direct x86-64 backend: Lume -> assembly -> native binary (NO C in the pipeline)"
if [ "$(uname -s)" = "Linux" ] && [ "$(uname -m)" = "x86_64" ] && command -v as >/dev/null 2>&1 && command -v ld >/dev/null 2>&1; then
    atmp=$(mktemp -d)
    $BIN emit-asm ../../compiler/stage1/arith.lume "$atmp/a.s" 2>/dev/null
    if as -o "$atmp/a.o" "$atmp/a.s" 2>/dev/null && ld -o "$atmp/a" "$atmp/a.o" 2>/dev/null && "$atmp/a" 2>/dev/null | diff -u ../../tests/conformance/stage1/arith_native.expected - >/dev/null; then ok "x86-64 asm -> native (arithmetic, no C)"; else bad "x86-64 asm -> native (arithmetic, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/fib_sample.lume "$atmp/f.s" 2>/dev/null
    if as -o "$atmp/f.o" "$atmp/f.s" 2>/dev/null && ld -o "$atmp/f" "$atmp/f.o" 2>/dev/null && "$atmp/f" 2>/dev/null | diff -u ../../tests/conformance/stage1/fib_native.expected - >/dev/null; then ok "x86-64 asm -> native (recursion + if, no C)"; else bad "x86-64 asm -> native (recursion + if, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/loop_native.lume "$atmp/l.s" 2>/dev/null
    if as -o "$atmp/l.o" "$atmp/l.s" 2>/dev/null && ld -o "$atmp/l" "$atmp/l.o" 2>/dev/null && "$atmp/l" 2>/dev/null | diff -u ../../tests/conformance/stage1/loop_native_asm.expected - >/dev/null; then ok "x86-64 asm -> native (var + while + assignment, no C)"; else bad "x86-64 asm -> native (var + while + assignment, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_collatz.lume "$atmp/c.s" 2>/dev/null
    if as -o "$atmp/c.o" "$atmp/c.s" 2>/dev/null && ld -o "$atmp/c" "$atmp/c.o" 2>/dev/null && "$atmp/c" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_collatz_asm.expected - >/dev/null; then ok "x86-64 asm -> native (Collatz step counts and longest chain (while, if-value, %, /), no C)"; else bad "x86-64 asm -> native (Collatz step counts and longest chain (while, if-value, %, /), no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_primes.lume "$atmp/p.s" 2>/dev/null
    if as -o "$atmp/p.o" "$atmp/p.s" 2>/dev/null && ld -o "$atmp/p" "$atmp/p.o" 2>/dev/null && "$atmp/p" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_primes_asm.expected - >/dev/null; then ok "x86-64 asm -> native (trial-division primes, twin primes (nested loops, and/or/not), no C)"; else bad "x86-64 asm -> native (trial-division primes, twin primes (nested loops, and/or/not), no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_ackermann.lume "$atmp/a.s" 2>/dev/null
    if as -o "$atmp/a.o" "$atmp/a.s" 2>/dev/null && ld -o "$atmp/a" "$atmp/a.o" 2>/dev/null && "$atmp/a" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_ackermann_asm.expected - >/dev/null; then ok "x86-64 asm -> native (Ackermann small values (deep recursion, nested if/else values), no C)"; else bad "x86-64 asm -> native (Ackermann small values (deep recursion, nested if/else values), no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_power.lume "$atmp/w.s" 2>/dev/null
    if as -o "$atmp/w.o" "$atmp/w.s" 2>/dev/null && ld -o "$atmp/w" "$atmp/w.o" 2>/dev/null && "$atmp/w" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_power_asm.expected - >/dev/null; then ok "x86-64 asm -> native (exponentiation by squaring, modular power, bit-length, no C)"; else bad "x86-64 asm -> native (exponentiation by squaring, modular power, bit-length, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_mutual.lume "$atmp/m.s" 2>/dev/null
    if as -o "$atmp/m.o" "$atmp/m.s" 2>/dev/null && ld -o "$atmp/m" "$atmp/m.o" 2>/dev/null && "$atmp/m" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_mutual_asm.expected - >/dev/null; then ok "x86-64 asm -> native (mutual recursion, Hofstadter F/M, global var/let, no C)"; else bad "x86-64 asm -> native (mutual recursion, Hofstadter F/M, global var/let, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_nested.lume "$atmp/n.s" 2>/dev/null
    if as -o "$atmp/n.o" "$atmp/n.s" 2>/dev/null && ld -o "$atmp/n" "$atmp/n.o" 2>/dev/null && "$atmp/n" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_nested_asm.expected - >/dev/null; then ok "x86-64 asm -> native (nested for/while, fact/fib, comparisons, signed div/mod, no C)"; else bad "x86-64 asm -> native (nested for/while, fact/fib, comparisons, signed div/mod, no C)"; fi
    $BIN emit-asm ../../compiler/stage1/nat_elif.lume "$atmp/e.s" 2>/dev/null
    if as -o "$atmp/e.o" "$atmp/e.s" 2>/dev/null && ld -o "$atmp/e" "$atmp/e.o" 2>/dev/null && "$atmp/e" 2>/dev/null | diff -u ../../tests/conformance/stage1/nat_elif_asm.expected - >/dev/null; then ok "x86-64 asm -> native (if/elif/else value + top-level assign, no C)"; else bad "x86-64 asm -> native (if/elif/else value + top-level assign, no C)"; fi
    # a code generator WRITTEN IN LUME (data_asmgen) emits x86-64 -> native, NO C anywhere
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/nfib.src "$atmp/nf.s" >/dev/null 2>&1
    if as -o "$atmp/nf.o" "$atmp/nf.s" 2>/dev/null && ld -o "$atmp/nf" "$atmp/nf.o" 2>/dev/null && "$atmp/nf" 2>/dev/null | diff -u ../../tests/conformance/stage1/nfib_asm.expected - >/dev/null; then ok "ML-core -> x86-64 asm (Lume-written codegen) -> native, no C"; else bad "ML-core -> x86-64 asm (Lume-written codegen) -> native, no C"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/strtest.src "$atmp/st.s" >/dev/null 2>&1
    if as -o "$atmp/st.o" "$atmp/st.s" 2>/dev/null && ld -o "$atmp/st" "$atmp/st.o" 2>/dev/null && "$atmp/st" 2>/dev/null | diff -u ../../tests/conformance/stage1/strtest_asm.expected - >/dev/null; then ok "ML-core native: strings (literals, ==, print) + ints, no C"; else bad "ML-core native: strings, no C"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/ev.src "$atmp/ev.s" >/dev/null 2>&1
    if as -o "$atmp/ev.o" "$atmp/ev.s" 2>/dev/null && ld -o "$atmp/evb" "$atmp/ev.o" 2>/dev/null && "$atmp/evb" 2>/dev/null | diff -u ../../tests/conformance/stage1/ev_asm.expected - >/dev/null; then ok "ML-core native: constructors + match -> x86-64, no C"; else bad "ML-core native: constructors + match, no C"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/reclist.src "$atmp/rl.s" >/dev/null 2>&1
    if as -o "$atmp/rl.o" "$atmp/rl.s" 2>/dev/null && ld -o "$atmp/rlb" "$atmp/rl.o" 2>/dev/null && "$atmp/rlb" 2>/dev/null | diff -u ../../tests/conformance/stage1/reclist_asm.expected - >/dev/null; then ok "ML-core native: records + lists -> x86-64, no C"; else bad "ML-core native: records + lists, no C"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/calc.src "$atmp/ca.s" >/dev/null 2>&1
    if as -o "$atmp/ca.o" "$atmp/ca.s" 2>/dev/null && ld -o "$atmp/cab" "$atmp/ca.o" 2>/dev/null && "$atmp/cab" 2>/dev/null | diff -u ../../tests/conformance/stage1/calc_asm.expected - >/dev/null; then ok "FULL core compiler (calc.src) -> x86-64 native, NO C"; else bad "FULL core compiler -> native, no C"; fi
    $BIN asm ../../compiler/stage1/elf_hello.s "$atmp/eh" >/dev/null 2>&1
    if "$atmp/eh" 2>/dev/null | diff -u ../../tests/conformance/stage1/elf_hello.expected - >/dev/null; then ok "lume0 asm: x86-64 -> static ELF -> runs, no as/ld/cc"; else bad "lume0 asm: x86-64 -> static ELF, no as/ld/cc"; fi
    rm -rf "$atmp"
else
    ok "x86-64 asm backend (skipped: not linux/x86_64 or no as/ld)"
fi

# FULL no-toolchain pipeline: Lume-written codegen -> lume0 asm (our own ELF
# writer, NO as/ld/cc) -> native. Gated only on Linux x86_64 (to run the ELF).
if [ "$(uname -s)" = "Linux" ] && [ "$(uname -m)" = "x86_64" ]; then
    ntmp=$(mktemp -d)
    for prog in nfib ev strtest modtest reclist list calc; do
        case "$prog" in calc) exp=calc_asm;; modtest) exp=modtest_core;; list) exp=list_core_native;; *) exp=${prog}_asm;; esac
        $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/$prog.src "$ntmp/$prog.s" >/dev/null 2>&1
        $BIN asm "$ntmp/$prog.s" "$ntmp/$prog" >/dev/null 2>&1
        if [ -x "$ntmp/$prog" ] && "$ntmp/$prog" 2>/dev/null | diff -u ../../tests/conformance/stage1/$exp.expected - >/dev/null; then ok "no-toolchain: $prog.src -> lume0 asm -> native (NO as/ld/cc)"; else bad "no-toolchain: $prog.src -> lume0 asm -> native"; fi
    done
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_rpn.src "$ntmp/sh_rpn.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_rpn.s" "$ntmp/sh_rpn" >/dev/null 2>&1; if [ -x "$ntmp/sh_rpn" ] && "$ntmp/sh_rpn" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_rpn_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_rpn.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_rpn.src -> lume0 asm -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_tree.src "$ntmp/sh_tree.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_tree.s" "$ntmp/sh_tree" >/dev/null 2>&1; if [ -x "$ntmp/sh_tree" ] && "$ntmp/sh_tree" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_tree_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_tree.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_tree.src -> lume0 asm -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_stackvm.src "$ntmp/sh_stackvm.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_stackvm.s" "$ntmp/sh_stackvm" >/dev/null 2>&1; if [ -x "$ntmp/sh_stackvm" ] && "$ntmp/sh_stackvm" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_stackvm_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_stackvm.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_stackvm.src -> lume0 asm -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_lists.src "$ntmp/sh_lists.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_lists.s" "$ntmp/sh_lists" >/dev/null 2>&1; if [ -x "$ntmp/sh_lists" ] && "$ntmp/sh_lists" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_lists_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_lists.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_lists.src -> lume0 asm -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_str.src "$ntmp/sh_str.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_str.s" "$ntmp/sh_str" >/dev/null 2>&1; if [ -x "$ntmp/sh_str" ] && "$ntmp/sh_str" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_str_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_str.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_str.src -> lume0 asm -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_bignum.src "$ntmp/sh_bignum.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_bignum.s" "$ntmp/sh_bignum" >/dev/null 2>&1; if [ -x "$ntmp/sh_bignum" ] && "$ntmp/sh_bignum" 2>/dev/null | diff -u ../../tests/conformance/stage1/sh_bignum_asm.expected - >/dev/null; then ok "no-toolchain stress: sh_bignum.src -> lume0 asm -> native"; else bad "no-toolchain stress: sh_bignum.src -> lume0 asm -> native"; fi
    for prog in sh_args sh_show sh_heap; do
        $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/$prog.src "$ntmp/$prog.s" >/dev/null 2>&1; $BIN asm "$ntmp/$prog.s" "$ntmp/$prog" >/dev/null 2>&1
        if [ -x "$ntmp/$prog" ] && "$ntmp/$prog" 2>/dev/null | diff -u ../../tests/conformance/stage1/${prog}_asm.expected - >/dev/null; then ok "no-toolchain gap: $prog.src (7+ params / print list+record+ctor / heap grows past 64MB) -> native"; else bad "no-toolchain gap: $prog.src -> native"; fi
    done
    # heap exhaustion: under a 200MB address-space cap the native binary must stop with a clear message and exit 12 (never segfault)
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/sh_oom.src "$ntmp/sh_oom.s" >/dev/null 2>&1; $BIN asm "$ntmp/sh_oom.s" "$ntmp/sh_oom" >/dev/null 2>&1
    (ulimit -v 200000; "$ntmp/sh_oom" >"$ntmp/oom.out" 2>"$ntmp/oom.err"; echo $? >"$ntmp/oom.code")
    if [ "$(cat "$ntmp/oom.code" 2>/dev/null)" = "12" ] && diff -u ../../tests/conformance/stage1/sh_oom_asm.expected "$ntmp/oom.out" >/dev/null && grep -q "out of memory" "$ntmp/oom.err"; then ok "no-toolchain: heap exhaustion -> clear error + exit 12 (no segfault)"; else bad "no-toolchain: heap exhaustion -> clear error + exit 12"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/bigints.src "$ntmp/bi.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/bi.s" "$ntmp/bi" >/dev/null 2>&1 && "$ntmp/bi" 2>/dev/null | diff -u ../../tests/conformance/stage1/bigints_asm.expected - >/dev/null; then ok "no-toolchain: 64-bit integer literals (movabs) -> native"; else bad "no-toolchain: 64-bit integer literals"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/esc_native.src "$ntmp/en.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/en.s" "$ntmp/en" >/dev/null 2>&1 && "$ntmp/en" 2>/dev/null | diff -u ../../tests/conformance/stage1/esc_native_asm.expected - >/dev/null; then ok "no-toolchain: string escapes (\\n \\t \\\" \\\\) -> native"; else bad "no-toolchain: string escapes -> native"; fi
    # top-level `let` -> globals (visible to fns and prints, shadowed by params / let-in / pattern binds); native must equal the pinned expected AND the interpreter
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/toplevel_let.src "$ntmp/tl.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/tl.s" "$ntmp/tl" >/dev/null 2>&1 && "$ntmp/tl" 2>/dev/null | diff -u ../../tests/conformance/stage1/toplevel_let_asm.expected - >/dev/null; then ok "no-toolchain: top-level let (globals) -> native"; else bad "no-toolchain: top-level let (globals) -> native"; fi
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/toplevel_let.src 2>/dev/null | diff -u ../../tests/conformance/stage1/toplevel_let_asm.expected - >/dev/null; then ok "interpreter: top-level let (globals) == native expected"; else bad "interpreter: top-level let (globals) == native expected"; fi
    # read(): one line of stdin. native is fed via a PIPE and via a file; the interpreter (lume0 has no stdin builtin; it reads /dev/stdin) only supports a redirected file and the first line
    printf 'hello\nworld\n' >"$ntmp/in1.txt"
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/read_line.src "$ntmp/rl.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/rl.s" "$ntmp/rl" >/dev/null 2>&1 && cat "$ntmp/in1.txt" | "$ntmp/rl" 2>/dev/null | diff -u ../../tests/conformance/stage1/read_line_asm.expected - >/dev/null && "$ntmp/rl" <"$ntmp/in1.txt" 2>/dev/null | diff -u ../../tests/conformance/stage1/read_line_asm.expected - >/dev/null; then ok "no-toolchain: read() one line of stdin (pipe + file) -> native"; else bad "no-toolchain: read() one line of stdin -> native"; fi
    if $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/read_line.src <"$ntmp/in1.txt" 2>/dev/null | diff -u ../../tests/conformance/stage1/read_line_asm.expected - >/dev/null; then ok "interpreter: read() == native expected (same stdin)"; else bad "interpreter: read() == native expected"; fi
    # EOF without trailing newline, empty stdin, and an empty first line: native must equal the interpreter on each
    for inp in 'no newline' '' '\nafter' 'a\r\nb'; do
        printf "$inp" >"$ntmp/in2.txt"
        $BIN run ../../compiler/stage1/compiler_in_core.lume ../../compiler/stage1/read_line.src <"$ntmp/in2.txt" >"$ntmp/o_i.txt" 2>/dev/null
        cat "$ntmp/in2.txt" | "$ntmp/rl" >"$ntmp/o_n.txt" 2>/dev/null
        if [ -s "$ntmp/o_n.txt" ] && diff -u "$ntmp/o_i.txt" "$ntmp/o_n.txt" >/dev/null; then ok "read() edge input native == interpreter"; else bad "read() edge input native == interpreter"; fi
    done
    # several read() calls consume successive lines (native only: pinned by hand, the interpreter cannot keep a stdin cursor)
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/read_lines.src "$ntmp/rls.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/rls.s" "$ntmp/rls" >/dev/null 2>&1 && printf 'one\ntwo\nthree\nfour' | "$ntmp/rls" 2>/dev/null | diff -u ../../tests/conformance/stage1/read_lines_asm.expected - >/dev/null; then ok "no-toolchain: successive read() calls -> native (pinned)"; else bad "no-toolchain: successive read() calls -> native"; fi
    $BIN run ../../compiler/stage1/data_asm_cc.lume ../../compiler/stage1/lclex.src "$ntmp/lx.s" >/dev/null 2>&1
    if $BIN asm "$ntmp/lx.s" "$ntmp/lx" >/dev/null 2>&1 && "$ntmp/lx" 2>/dev/null | diff -u ../../tests/conformance/stage1/lclex_asm.expected - >/dev/null; then ok "self-host R4: core lexer (written in Lume-Core) -> native"; else bad "self-host R4: core lexer -> native"; fi
    rm -rf "$ntmp"
else
    ok "no-toolchain ELF pipeline (skipped: not linux/x86_64)"
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

echo "== resolve: undefined names are caught at check time"
for src in tests/resolve/bad_*.lume; do
    [ -f "$src" ] || continue
    code=$(sed -n '1s/^# expect: //p' "$src")
    if $BIN check "$src" --json 2>/dev/null | grep -q "\"code\":\"$code\""; then ok "$src ($code)"; else bad "$src (expected $code)"; $BIN check "$src" --json; fi
done
for src in tests/resolve/ok_*.lume; do
    [ -f "$src" ] || continue
    if $BIN check "$src" >/dev/null 2>e.txt; then ok "$src (clean)"; else bad "$src (should resolve clean)"; sed 's/^/      /' e.txt | head -4; fi
done

echo "== positive parser cases (must parse clean)"
for src in tests/parser/ok_*.lume; do
    [ -f "$src" ] || continue
    # parser ok-tests assert PARSING only (bare free identifiers are expected); use parse, not check
    if $BIN parse "$src" >/dev/null 2>e.txt; then ok "$src"; else bad "$src"; sed 's/^/      /' e.txt | head -4; fi
done

rm -f e.txt
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
