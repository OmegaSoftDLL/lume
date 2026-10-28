# 33 — The plan to build anything with AI (frontiers, multipliers, order)

**Goal.** Let the AI build **anything** in Lume — any software, on any substrate — with every artifact
**real and proven** (never a placeholder or mock, per `docs/28` §7). "Anything imaginable" is an
asymptote no language reaches by pre-building features; Lume's bet is a **universal, proof-carrying
substrate** plus **AI self-growth**, so the model evolves the language toward each target instead of a
human building every one.

## The frontiers (each a tracked, decomposed, incremental track)

| Track | Frontier | Issue |
|---|---|---|
| **M1** | **Prover reach beyond QF_LIA** — nonlinear/bitvectors/arrays/quantifiers (SMT), separation logic, refinement/dependent types, termination, temporal (folds #198) | **#255** |
| **M2** | **`lume evolve` as a self-growth engine** — the AI proposes→proves→lands large features (targets, prover theories, stdlib), gated and real | **#258** |
| **R** | **Universal reach** — real GPU (folds #250), MCUs, FPGA/HDL, mobile, web — run-verified, proofs preserved | **#256** |
| **L** | **Standard library & domains** — net stack, async runtime, GUI, database, media, real ML | **#257** |
| **I** | **Intent-as-source §1** — intent is the source, code is a proven cache (`docs/19` §1) | **#259** |
| **O** | **General-purpose OS** on the microkernel — incremental, uses R+L | **#241** |

## Why two of them are *multipliers* (do them first)
- **M1 (prover reach)** is what makes "build anything" mean "build anything **proven**". QF_LIA proves a
  narrow fragment; without richer theories the AI can generate code but not verify it. Every other
  track ships proof-carrying only to the extent M1 reaches.
- **M2 (evolve engine)** is what makes the breadth tracks (R, L) tractable **without a human building
  each piece**: the AI evolves the language toward the target, each change real + proven + gated. This
  is the unique, AI-first lever no other language has.

With M1 + M2 in place, R and L stop being an endless human to-do list and become territory the AI
grows on demand — and I (intent-as-source) becomes the loop the user drives at the top.

## Order (dependency, not a hard gate everywhere)
1. **M1 prover reach** (#255) — in parallel with M2; underpins proof-carrying everything.
2. **M2 evolve engine** (#258) — so the AI can then drive R and L itself.
3. **R universal reach** (#256) and **L stdlib/domains** (#257) — grown via M2, proven via M1; the GPU
   slice (#250) is the first R brick.
4. **I intent-as-source §1** (#259) — the top-level AI loop, ties M1 (re-prove on intent change).
5. **O general-purpose OS** (#241) — built incrementally on R + L once the kernel (done, `docs/27`) stands.

## The one rule over all of it (`docs/28` §7)
No placeholder, stub, mock, fixed output, or one-shot ships in a feature's path — not even labelled; if
it isn't real it's an open issue referenced as working nowhere. Everything documented must **run**,
tested exhaustively and re-tested every change; closing an issue needs the real, run-verified
deliverable; large tracks ship incrementally, each increment real. "Build anything" is earned one
proven brick at a time — never claimed.
