# 13 — Splitting the self-hosted compiler into modules (to unblock parallel work)

## Why
`compiler/stage1/lumec.src` is one monolithic Lume-Core file holding the lexer, parser,
type checker (F1/F2), comptime evaluator (C1/C2/C3), code generator, the emitted runtime, and
the driver. Every language feature edits it, so multiple contributors (tracks F, C, M, D, …)
collide on a single file. We split it so each track edits a different file and true parallel
work becomes possible.

## How: amalgamation (SQLite model), not `lmod`
The split is done by **amalgamation**, exactly like SQLite's single-file build: the editable
sources are the per-module files in `compiler/stage1/lume/NN_*.src`, and `lumec.src` is the
**generated** concatenation of them, in lexicographic (= numeric `NN`) order. `lumec.src` is no
longer a source file — it is a build artifact (git-ignored) produced by `scripts/amalgamate.sh`
(`cat compiler/stage1/lume/*.src > compiler/stage1/lumec.src`).

This was chosen over the module-system (`lmod`) route on purpose: amalgamation is a **pure
textual rearrangement** whose output is, by construction, **byte-identical** to the historical
`lumec.src`. Nothing in the compile path changes — the seed and the self-built `lumec` still
compile one flat file — so the self-host **fixpoint (stage 2 == stage 3, bit for bit) is
preserved with zero risk**. `lmod` would have altered the program fed to the compiler (bundling,
`use`-stripping, re-concatenation), putting the fixpoint at risk during a core freeze; it stays
available for *user* programs, but the compiler itself uses amalgamation.

## Module boundaries (`compiler/stage1/lume/`, in generated order)
The cut points follow the file's **actual definition order** (which is what byte-identity
requires), not an idealized lexer→parser→check→codegen order. In particular, the comptime passes
sit between the parser core and the program/type-declaration parser, the type checker sits at the
end just before the driver, and there is no separable "runtime" section — the whole runtime is a
single `runtime_tail()` function returning one asm string, in the tail of the code generator.

| File | Original lines | Contents |
|------|---------------|----------|
| `01_lexer.src`        | 1–47    | header; `lex`, token-list helpers, string-escape decode, `##` comments, decimal lexing (D1) |
| `02_parser.src`       | 48–99   | `prim`/precedence/AST constructors, `match` parsing |
| `03_comptime.src`     | 100–279 | `ceval`/`cfold`/`gerase` (C1/C2/C3), comptime calls, C3 type erasure (**tracks C/M edit here**) |
| `04_parser_decls.src` | 280–372 | `parseprogram` (programs) and named type declarations (F1) |
| `05_codegen.src`      | 373–639 | `laexpr`/`lafn`/`lgen` x86-64 codegen + `runtime_tail()` (the emitted runtime asm string) |
| `06_typecheck.src`    | 640–973 | type checker `tcheck` + type table (F1/F2) + C3 generic argument checking (**track F edits here**) |
| `07_driver.src`       | 974–975 | the driver: `print lgen(cfold(gerase(tcheck(parseprogram(readall())))))` |

The `NN_` prefix fixes each module's position in the generated file. To add or re-order a module,
add/rename a `compiler/stage1/lume/NN_*.src` file — never edit the generated `lumec.src`.

## Hard constraints (all met)
1. **Byte-identical, fixpoint-preserving.** The split is a pure refactor — moving definitions
   between files, no logic change. The gate is `cmp`: `cat compiler/stage1/lume/*.src` must equal
   the pre-split `lumec.src` with **0 bytes of difference**. (Verified at split time against
   `git show HEAD:compiler/stage1/lumec.src`.) Because the generated file is identical, the
   emitted `lumec`/`.s` is identical and `make bootstrap` stays stage2==stage3.
2. **Build-pipeline change.** `lumec.src` is regenerated before anything compiles it.
   `scripts/amalgamate.sh` is called at the start of `scripts/bootstrap.sh`, at the start of
   `bootstrap/lume0/tests/run.sh`, and in the `lume` CLI's `ensure_native_toolchain` before it
   builds `lumec` from source; `make amalgamate` (a prerequisite of `make bootstrap/test/asan`)
   runs it too.
3. **Core-quiescence (sequencing).** The split rewrites the file layout wholesale, so it runs
   under a short core freeze (F/C/M/D hold edits): split in one PR, verify byte-identical +
   fixpoint, merge, unfreeze. Thereafter each track owns a module file and the freeze is no longer
   needed.

## What a contributor does now
Edit the relevant `compiler/stage1/lume/NN_*.src` module — **not** the generated `lumec.src`.
Any build entry point regenerates `lumec.src` first; to regenerate by hand, run
`make amalgamate` (or `scripts/amalgamate.sh`). Distinct files = zero merge collisions across
tracks, which is the real unlock for 3+ parallel contributors.
