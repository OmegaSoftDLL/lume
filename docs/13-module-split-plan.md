# 13 — Splitting the self-hosted compiler into modules (to unblock parallel work)

## Why
`compiler/stage1/lumec.src` is one monolithic Lume-Core file holding the lexer, parser,
type checker (F1/F2), comptime evaluator (C1/C2/C3), code generator, the emitted runtime, and
the driver. Every language feature edits it, so multiple contributors (tracks F, C, M, D, …)
collide on a single file. T3 gave Lume a module system (`lmod`): we use it to split the compiler
into modules so each track edits a different file and true parallel work becomes possible.

## Feasibility (confirmed against `lmod.src`)
`lmod` bundles a stdin stream of `@module <name> … @end` blocks plus an `@entry … @end`, resolving
`use <mod>` in topological order, deduplicating diamonds, rejecting cycles/missing modules, and
emitting a **single self-contained Lume-Core bundle** (it strips `use` lines and library-level demo
`print`s, then concatenates module bodies). So a split compiler = N module files, assembled by
`lmod` into one bundle that `lumec` + `lasm` compile exactly as today.

## Proposed module boundaries (`compiler/stage1/lume/`)
- `lexer.src` — `lex`, token helpers, string-escape decode, `##` comments.
- `parser.src` — `prim`/precedence/`match`/`parseprog`, type declarations (F1), annotations (F2).
- `check.src` — `tcheck` + type table (F1/F2), generic type env.
- `comptime.src` — `ceval`/`cfold`/`gerase` (C1/C2/C3). **← track C/M edit here.**
- `codegen.src` — `laexpr`/`lafn`/`lgen` (x86-64). **← track D codegen edits here.**
- `runtime.src` — `runtime_tail` (the asm string) + `abin`/`areg`/`slicen`/`emit`/`flush`.
- `lumec.src` — the driver: `use` the above + `print lgen(cfold(gerase(tcheck(parseprogram(readall())))))`.

## Hard constraints
1. **Byte-identical, fixpoint-preserving.** The split is a pure refactor — moving definitions between
   files, no logic change. The `lmod` bundle must be an equivalent program, so the resulting `lumec`
   is **byte-identical** to the pre-split one (verify: `cmp` the new `lumec`/`.s` against the old,
   and `make bootstrap` stays stage2==stage3). Watch: definition order and the exact `runtime_tail`
   string must be preserved.
2. **Build-pipeline change.** Today the pipeline compiles `lumec.src` directly. After the split it
   must first `lmod`-bundle the modules, then compile the bundle. Update `scripts/bootstrap.sh`,
   `run.sh`, and the `lume` CLI accordingly. Note the bootstrap order: `lmod` is itself built by
   `lumec`; the seed (`data_asm_cc`) can compile the bundled driver, and `lmod` is built once — keep
   this acyclic and documented.
3. **Core-quiescence (sequencing).** The split rewrites `lumec.src` wholesale, so it **cannot run in
   parallel with any other core edit** (e.g. track M editing comptime). Execute it only when the core
   is frozen: after the in-flight core PR (agy's M) lands, announce a short "core freeze" (F/C/M/D hold
   `lumec.src` edits), split in one PR, verify byte-identical + fixpoint, merge, unfreeze. Thereafter
   each track owns a module file and the freeze is no longer needed.

## Migration (one PR, when the core is frozen)
1. Create `compiler/stage1/lume/{lexer,parser,check,comptime,codegen,runtime}.src` by moving the
   corresponding definition groups out of `lumec.src` verbatim; `lumec.src` becomes the driver with
   `use`s.
2. Add the bundle step (`lmod`) to the build; confirm the bundled program compiles **byte-identically**
   to the pre-split `lumec` (seed-built and self-built) and that `make bootstrap` holds stage2==stage3.
3. Wire a `run.sh` guard so the split stays byte-identical over time.

After this, tracks F/C/M/D stop colliding on one file — the real unlock for 3+ parallel contributors.
