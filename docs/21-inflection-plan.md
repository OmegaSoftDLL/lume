# 21 — Inflection plan: dogfooding Lume into the compiler

Phase 3 created the capabilities (Tracks **O, A, K, S, V**). The inflection point is **not**
"Phase 3 is done" — it is the moment the compiler itself is built **with** those capabilities.
Today `compiler/stage1/lume/*.src` is written in pure Lume-Core (`fn`/`let`/`if`/`match`/records)
and uses **zero** Phase-3 features. This document is the staged plan that closes that gap. It is
deliberate and ordered; do **not** rush Phase 4/5 language work ahead of it.

The direction this climbs toward is the north star in `docs/19-intent-as-source.md`
(intent-as-source, the compiler as an oracle) and the Phase-3 footholds in `docs/16-phase3-plan.md`
(Track A holes/`total`/provenance, Track V `lume prove`).

## Non-negotiable invariants (GOLDEN RULE) — hold on every PR below

- `make bootstrap` (stage 2 == stage 3, bit-identical), `make zero-c`, and the oracle
  byte-equivalence test stay green.
- **No Lume feature is used inside the compiler until it is proven sound by a conformance test.**
  Do not build the compiler on a feature that has an open soundness bug.
- One brick per PR, each with a regression test. The frozen-oracle rule for the core lexer/parser
  still applies (`docs/17`): mirror a core change in `data_parser.lume` in the same PR, or do not
  touch the core.
- Each dogfood PR must leave the compiler at least as correct and clear as before — the feature has
  to earn its place, never just demonstrate itself.

## P0 — Prerequisite before ANY effect use in the compiler: close #167 (✅ Done in PR #171)

Effect dispatch across a function-call boundary miscompiles (platform-dependent output / UB; see
issue #167). Effects are **not** a trustworthy foundation until:

1. the effect-handler lowering is root-caused and fixed (`olowercall_fn` / `kuses_hop` / `ksubst_e` / `kbind_hop_args` / `klower_eff_ops`), and
2. a conformance test calls an effect-polymorphic `fn` from inside a `with handler` (`step(c) + step(c) == 210`) and asserts
   **byte-identical** asm/output on `ubuntu-latest` and `macos-latest`.

Until #167 is green, **no compiler pass may use effects**.

## Stage A — First dogfood (safest; start here): contracts on existing passes, behaviour unchanged (✅ Done)

Add `requires` / `ensures` / `invariant` to existing compiler passes to encode invariants they
already rely on — **no logic change**. This is the safest first fold: it uses only V1
`runtime_checked` guards, with no dependence on prover soundness (Stage B) and no effects (Stage C).

Concrete targets:

- **Lexer (`01_lexer.src`)** — positional/precondition invariants (`numend`, `decimalend`, `decscan`).
- **Parser (`02_parser.src`, `04_parser_decls.src`)** — delimiter bounds & well-formedness postconditions on produced nodes (`findcloseb`, `parsevariantend`, `parsetypedecimal2`).
- **Typecheck (`06_typecheck.src`)** — environment & substitution invariants (`tifn3`, `henv0`, `hbind`).

**Acceptance:** guards encode real invariants; `stage 2 == stage 3` still bit-identical; `zero-c`
green; a regression test per contract; `lume prove` reports them `runtime_checked` (never falsely
`proven`). This is "Lume verifying itself" with essentially no risk surface.

## Stage B — Graduate the verifier from v0 sampling to a sound fragment

Replace the fixed 2–3-point enumeration (`cs_vpts_a/b/c`) with a real decision procedure for the
linear-integer fragment (QF_LIA). Only when an obligation is genuinely **decided** may it be
`proven` and elide its runtime guard; everything else stays `runtime_checked`. This is the gate that
lets proofs replace tests inside the compiler — until it lands, Stage A contracts stay
runtime-checked (correct, just not zero-overhead). This is the operational foothold for the
`docs/19` north star.

## Stage C — Effects in the compiler (only after #167 is closed)

Model the one place the compiler does I/O — the driver (`07_driver`) / CLI boundary — as an effect.
Keep the footprint minimal: the pure passes are pure; **leave them pure**. Smallest effect surface,
highest signal that effects hold under self-hosting.

## Stage D — Ownership / Perceus in the compiler (later; after A–C are stable)

Annotate hot data structures (token lists, AST cells) with `own` to exercise Perceus in-place reuse
in the compiler itself — a performance dogfood. Only after the correctness stages are solid.

## Critical path and ownership

- **Order:** `#167` → **Stage A** → **Stage B** are the critical path; **C** and **D** follow.
- **Who builds:** `agy` + `codex` (16 GB hosts that run the self-hosted toolchain); split work to
  avoid collisions as in the F3c / E1 coordination.
- **Review:** every PR touching soundness, effects, or `prove` is reviewed with doubled scrutiny
  before merge — a green CI does not by itself prove soundness (it did not catch #167).

## Explicitly NOT now

- Do not effect-ify the pure passes.
- Do not rely on `proven` elision before Stage B.
- Do not start Phase 4/5 language work ahead of Stages A–B.
