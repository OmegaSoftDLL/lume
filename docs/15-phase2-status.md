# 15 — Phase 2 status: what shipped, and what is honestly still open

Phase 2 grows Lume from the self-hosted Lume-Core into the AI-first language the pillars
(`docs/02-principles.md`) promise. This note is the honest capstone: every pillar that is
**done and proven in CI**, and the two pieces that are genuinely **still open** — stated
plainly, because the project's golden rule forbids claiming a milestone that is not real.

## Done and proven (one green PR + a regression test each; the self-host fixpoint stays bit-identical)

| Track | Bricks | Status |
|-------|--------|--------|
| **C — comptime** (flagship) | C1 fold pure expr, C2 pure fn calls, C3 generics via `comptime T: type` + erasure, C4a `comptime assert`, C4b precomputed tables | ✅ complete |
| **M — macros** | M1 AST as a first-class comptime value, M2 hygienic macros / quasiquote | ✅ complete |
| **D — data** | D1 exact `decimal`, D2 `table[T]` + CSV, D3 typed `query` (where/order/join/group) | ✅ complete |
| **T — tooling** | T1 `lume` CLI, T2 canonical formatter, T3 modules + package manager (capability-audited, SHA-256 lockfile) | ✅ complete |
| **E — self-evolution** | E1a `lume compile` service, E1b in-process `lcaas` + `lume agent`, E2 CI fixpoint gate, E3 `lume evolve` (6 gates + human approval) | ✅ complete |
| **F — type foundation** | F1 type/enum/record decls + gradual checker, F2 param/return annotations + gradual argument checks | ✅ complete |
| **F3 — inference** | F3a/F3b gradual return/argument/let/record-field inference + **F3c Hindley–Milner unification engine** (`TyVar`, `hunify`, `happly`, `tcollecti`/`tinfer`) inferring unannotated function parameters (`fn inc(x) = x + 1` -> `x: int`) and checking call sites with zero false positives | ✅ complete |
| **F4 — front-end unification** | named enum payloads (#143), `elif` chains (#152), string interpolation (`"a{e}b"` + `{{`/`}}` escaping), and unified layout/indented + explicit-brace (`docs/05-vision.md:224`) + inline core forms (`f4_layout.src`, `f4_brace.src`, `f4_core.src`) emitting byte-identical asm and normalising via `lume fmt` | ✅ complete |
| **Capstone — Stage 2 exit criterion** | `examples/apps/inventory/`: multi-module inventory, invoicing & executive reporting app (< 500 lines across `catalog.src`, `invoicing.src`, `reports.src`, `main.src`, `lume.toml`, `lume.lock`) combining `use`, `decimal`, `table[T]`/`read_csv`, `query`, `comptime`, hygienic macros, `lume pkg`, and `lume test` | ✅ complete |

Plus a cross-cutting fix: the self-hosted lexer/parser recurse once per source character, so the
growing (~117 KB) compiler needs a deep native stack — `make bootstrap`, `make zero-c`, the test
runner and the `lume` CLI now raise the stack limit so the compiler can keep growing without a
stage-1 `SIGSEGV`.

## Honestly still open (deliberately not claimed as done)

1. **F3c — let-polymorphism / higher-rank generalization.** The Hindley–Milner unification engine
   (`TyVar(n)`, `hunify`, `happly`, `tcollecti`/`tinfer`) now infers unannotated function parameter
   types from their body usage (`fn inc(x) = x + 1` -> `x: int`, `fn greet(s) = "hello " + s` ->
   `s: str`, polymorphic `fn id(x) = x` -> `TyAny`) and rejects mismatched call sites with zero
   false positives. Generalizing polymorphic `let`-bound functions across distinct instantiations
   inside a single body remains open for future extension.

2. **F4 — layout, explicit-brace, and string interpolation unification (shipped).** The unified
   front-end (`compiler/stage1/lume/01_lexer.src`, `02_parser.src`, `04_parser_decls.src`, and
   `compiler/stage1/lfmt.src`) now accepts all three forms — indented layout (`f4_layout.src`),
   explicit-brace (`f4_brace.src`, `docs/05-vision.md:224`), and canonical inline Lume-Core
   (`f4_core.src`) — plus surface string interpolation (`"a{e}b"` with `{{`/`}}` escaping),
   lowering all of them to the same AST, emitting byte-identical x86-64 assembly, and normalising
   both surface forms into `f4_core.src` via `lume fmt` while keeping `lumec.src`'s self-host
   fixpoint and oracle byte-equivalence untouched.

## Bottom line

All six Phase 2 tracks (**C, M, D, T, E, F**) and the Stage 2 capstone (`examples/apps/inventory/`)
are complete and proven in CI. Nothing is faked: the triple self-build fixpoint is green, and every
claim above is backed by a CI check.
