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
| **F4 — front-end unification** | slice 1: the core grammar accepts the surface's **named enum payloads**, normalized to the same AST (byte-identical codegen) | 🟡 started |
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

2. **F4 — layout and one shared parser.** Slice 1 (named payloads) is additive and safe. The
   remaining surface features — indentation/layout as a block form, string interpolation — cannot
   be added to the **core** parser alone: the self-host byte-equivalence test compares the
   self-hosted compiler against the frozen ML-core oracle (`data_parser.lume`), so any core-parser
   change that the oracle does not mirror diverges by construction. Interpolation was attempted in
   the core and correctly rejected by that gate (it is a surface-layer feature that should desugar
   to the core's `+`/`str` AST). Unifying the two front-ends into one grammar is therefore a
   deliberate core-plus-oracle effort, not an additive slice.

## Bottom line

All five structural tracks (**C, M, D, T, E**), the Stage 2 capstone (`examples/apps/inventory/`),
and Track **F**'s type declarations, gradual checker, and Hindley–Milner core unification engine
(**F1–F3c**) are complete and proven in CI, with full front-end grammar unification (**F4**) scoped
honestly above. Nothing is faked: the triple self-build fixpoint is green, and every claim above is
backed by a CI check.
