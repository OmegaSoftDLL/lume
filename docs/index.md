# Lume — the AI-first programming language

**Written by AI, proven by the compiler, reviewed by humans.** Lume self-hosts with **no C in the
loop** (`stage 2 == stage 3`, bit-identical), carries a **sound `QF_LIA` proof engine** so contracts
can be `proven` and their runtime guards elided, and treats correctness as something the machine
**demonstrates**, not something you hope for.

> **Honest status.** Early and pre-1.0. The compiler, Phases 1–5 (ownership, effects, concurrency,
> contracts/proofs, AI-numerics, systems/interop), the inflection (the compiler dogfooding its own
> features) and P33 software protection are implemented and self-hosting. Native codegen today is
> **Linux x86-64 only**; macOS/Apple Silicon, Windows, RISC-V and bare-metal are planned (not built).
> Some features are models or stubs clearly labelled as such, and verification/trust-layer hardening
> is ongoing. See the open issues for exactly what is shipped vs. planned.

## Start here
- [README](https://github.com/OmegaSoftDLL/lume) — overview, the ten pillars, a taste, quickstart.
- [05 — Vision](05-vision.md) · [02 — Principles (P1–P35)](02-principles.md) · [03 — Specification](03-specification.md)
- [08 — AI-first](08-ai-first.md) — why the primary user is a model, not a person.

## How it builds itself
- [11 — Self-hosting plan](11-self-hosting-plan.md) · [14 — Zero-C](14-zero-c.md) · [04 — Roadmap (Stages 0–5)](04-roadmap.md)

## The capabilities
- [16 — Phase 3 plan](16-phase3-plan.md) (ownership, effects, concurrency, verification)
- [21 — Inflection](21-inflection-plan.md) (the compiler dogfoods its own contracts/effects/Perceus)
- [24 — Phase 4 plan](24-phase4-plan.md) (tensors, autodiff, prob, rules, llm) · [25 — Phase 5 plan](25-phase5-plan.md) (systems, interop, realtime, lume-os)
- [06 — Software protection](06-software-protection.md) (P33 anti-reverse-engineering)

## For AI authors
- [32 — Build real software in Lume (onboarding for any AI)](32-ai-authoring.md) · [30 — The Lume Book](30-lume-book.md) · [08 — AI-first](08-ai-first.md)

## Build order
- [29 — Worklist execution order](29-worklist-order.md) — the ordered open backlog (agy's sequence).

## The north star
- [19 — Intent as source / the compiler as an oracle](19-intent-as-source.md)
- [20 — Interop](20-interop.md) · [22 — Adoption](22-adoption.md) · [23 — Horizon](23-horizon.md) · [26 — Universal targets](26-targets.md)

## Reference
- [00 — Catalog of ~260 languages](00-catalog.md) · [01 — Survey](01-survey.md) · [07 — Competitive scan](07-competitive.md)

*Dual-licensed MIT OR Apache-2.0. Contributions: see [`AGENTS.md`](https://github.com/OmegaSoftDLL/lume/blob/main/AGENTS.md) and [`CONTRIBUTING.md`](https://github.com/OmegaSoftDLL/lume/blob/main/CONTRIBUTING.md).*

## The plan to build anything
- [33 — Frontiers, multipliers & order to build anything with AI](33-build-anything-plan.md)

