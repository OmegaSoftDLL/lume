# 10 — Stage 1 status: what the Lume-written compiler covers (honest inventory)

Stage 1's exit criterion is the **triple self-build**: `lume0` runs `lumec` to compile `lumec`,
which compiles itself, bit-identical. Reaching that fixpoint means the Lume-written `lumec` must
compile the *entire* language `lumec` is written in. That is a large, multi-session effort. This
document is the honest running inventory of how far the self-hosted front-end has come, so no one
mistakes the current milestone for the fixpoint.

## Written in Lume, running on `lume0` today

The compiler toolchain in `compiler/stage1/`, all authored in Lume:

| Piece | Module(s) | Covers |
|---|---|---|
| Line lexer | `token`, `lexer` | identifiers/keywords, ints, strings, operators (incl. multi-char), comments, positions |
| Layout lexer | `layout` | the above **plus INDENT/DEDENT** for indented blocks, bracket-depth suppression |
| Expression/function parser | `ast`, `parser` | `let`, expressions with precedence, calls, **functions**, **`if`/then/else** |
| Block parser | `block_ast`, `block_parser` | **`while`**, **`if`/`else` blocks**, assignment, `print`, nested blocks (via layout) |
| ADT parser | `data_ast`, `data_parser` | **constructors with payloads**, **`match`** with nested patterns and bindings |
| Checker | `check` | undefined variable/function, wrong call arity, string-in-arithmetic -> diagnostics |
| Evaluators | `eval`, `block_eval`, `data_eval` | tree-walking interpreters for each subset (incl. tagged values for ADTs) |
| Code generators | `codegen`, `block_codegen` | AST -> C for expressions/functions/`if` and for `while`/`if`/assignment |
| Driver | `lumec`, `lumec_main` | lex -> parse -> check -> codegen; a file-driven CLI (`read_file`/`write_file`/`env.args`) |

Plus, in the C bootstrap, a **direct x86-64 backend** (`asmgen.c`): `lume0 emit-asm` lowers the AST to
assembly (no C in the pipeline) for arithmetic, `let`, functions, recursion and `if`; assembled with
`as`/`ld` into a native binary.

Programs that compile today, to a native binary: integer arithmetic and `let`; user functions and
recursion (e.g. `fib`); `if`/`while` iterative programs (e.g. iterative factorial). Programs that
run under the Lume-written interpreters today: all of the above plus algebraic data types and
pattern matching.

## What the fixpoint still needs (not yet done)

To compile `lumec` itself, the Lume-written compiler still needs, roughly in order:
- a single unified front-end (today there are subset-specific parsers/evaluators to keep each
  increment small and tested) — merge them into one grammar and one AST;
- the value types `lumec` uses: strings, lists, maps, records with named fields, optionals/results;
- codegen for `match`, enums/records, closures, and string/list operations;
- a type checker and the ownership/borrow checker (Stage 3 features `lumec` can be written without,
  if `lumec.lume` is kept to a GC-free-by-value subset);
- `lumec` rewritten so it only uses features `lumec` can compile, then the triple self-build.

Each of these is a real body of work. The staged, test-first approach here deliberately trades
"finish self-hosting fast" for "every step compiles, runs and is covered by conformance tests."

## Status

- [x] Lexing (line + layout), parsing (expressions, functions, blocks, ADTs), checking, evaluation,
      and C codegen for growing subsets — all written in Lume and run by `lume0`.
- [x] A file-driven `lumec` CLI that compiles a Lume subset to C -> native.
- [x] A direct x86-64 backend in the bootstrap (no C) for the arithmetic/function/recursion subset.
- [x] The ML-core (constructors, match, records, lists, strings, builtins) lowers to C AND to
      x86-64 via a codegen written in Lume (`data_asmgen.lume`); a full core-written compiler
      (`calc.src`) runs natively.
- [x] A seed assembler + ELF64 writer (`lume0 asm`): the whole pipeline — Lume-written codegen,
      then `lume0 asm` — produces a native ELF with **no C and no external as/ld/cc**.
- [x] `lume0 check` is no longer parse-only: it does static **name resolution** (R0200), basic
      **literal type checking** against annotations (T0001), and **duplicate** parameter/function
      detection (D0001/D0002). `lume0 describe` lists what `check` catches.
- [x] **Self-hosted `lumec` — triple self-build reached.** The Lume-Core compiler
      `compiler/stage1/lumec.src` (lexer + program parser + x86-64 codegen, incl. `match`,
      records, strings, constructors, `##` comments and string escapes) compiles its own
      **verbatim** source bit-identically and stably: `lumec` → `gen1`, `asm(gen1)` = `lumec2`
      → `gen2`, `asm(gen2)` = `lumec3` → `gen3`, with `gen1 == gen2 == gen3`. The self-compiled
      `lumec2` compiles every conformance program correctly, no C in the target pipeline.
- [ ] Lexical scoping (use-before-def / out-of-scope), fuller type checking (call arity, match
      exhaustiveness, operator types), effects/ownership/contract enforcement.
- [~] Delete the last C — **zero-C loop proven.** `compiler/stage1/lasm.src` is an x86-64
      assembler + ELF64 writer in Lume-Core (byte-level `emit`/`flush` runtime). Steady state
      uses no C: `lumec` self-compiles → asm; `lasm` assembles → a `lumec` that reproduces
      itself bit-identically and compiles programs correctly. Remaining before `bootstrap/` is
      removed: speed up `assemble()` (slow on large inputs), add `movabs`, and ship a
      Lume-produced seed so the first binaries don't need the C seed either.

This file is updated as the inventory grows.
