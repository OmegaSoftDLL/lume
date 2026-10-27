# 22 — Adoption: making Lume the indispensable substrate for AI-authored software

The five phases + the verification/provenance protection build the **language**. **None of them
build adoption.** A technically superior language with no distribution dies — the graveyard is full
of languages better than Rust and Go that nobody uses. So the work *after* the language is done
changes nature: from building a compiler to building **distribution**. This document is that plan,
to execute once Phases 1–5 and the correctness protection (provenance A3 + contracts V + sound
`QF_LIA` proof) have landed.

> **Status note.** **G1 (MCP)**: `lume mcp` exposes `lume_check`, `lume_prove`, `lume_fill`, `lume_witness`, `lume_repair`, `lume_impact`, `lume_bind`, and `lume_certify` (`lume agent witness|repair|impact`, #195 fixed). **G3 (certificate)**: `lume certify`/`verify-cert` verify certificates **independently** (a standalone Fourier–Motzkin `QF_LIA` kernel re-derives each proof term + FIPS-180-4 SHA-256 hashing, #183 fixed). **G8 (`lume evolve`)**: `lume evolve` enforces 6 gates (including `proofs_ok`, `benchmark_ok`, and `stage2 == stage3` fixpoint) plus cryptographic SHA-256 maintainer signature verification (#194 fixed). Remaining distribution tracks (G0/G2/G4/G5/G6/G7) are tracked under #196.

It deliberately does **not** replay the human-first playbook (installer → LSP → registry → book).
Lume's user is not primarily a human typing; it is an **AI generating**. That single fact lets us
reconceive distribution, onboarding, virality, and even the artifact of programming itself.

## The one unfair advantage

Lume is the only toolchain that produces **code carrying a machine-checked proof that it matches its
spec**, with **provenance** on every value, **reproducible bit-for-bit** from source, with **no C to
trust** — and authored by an AI. Every move below weaponises that one asset. If a move does not, cut
it.

The pitch, in one sentence:

> *The AI wrote it, and the compiler **proved** it does what the spec says — so the human trusts it
> without reading every line.*

In a world where everyone generates code with AI, emitting **unverified** Python/JS becomes the
reckless option next to emitting **proven** Lume. That is the wedge.

## Three pillars (the out-of-the-box core)

### Pillar A — Don't make them switch. Infiltrate.
Nobody migrates a codebase to a new language. So we never ask. A proven Lume **core** drops *into*
existing stacks via the reverse C-ABI / WASM export (`docs/20` I5): the AI generates the critical 5%
— the payment path, the parser, the crypto, the state machine — in Lume, **proves it**, and exposes
it to Python / Node / Rust / Go. "Prove the part that matters; keep the rest." Zero-migration, the
way TypeScript was zero-migration — but for *correctness*, not types.

### Pillar B — Distribute through the AI, not the human.
Lume's distribution channel is the **agent's toolbelt**, not a download page. The compiler-as-oracle
(`docs/19`) is exposed as a tool every AI coding agent calls: *prove this, fill this hole, give me an
input that breaks this contract, what's the smallest repair, what breaks if I change this type.* The
language spreads **agent-to-agent**: once an agent can prove its own output, it prefers to, and the
habit propagates. Onboarding is not "learn the syntax" — the human states **intent**; the AI + oracle
synthesise and prove. The learning curve is "how to state provable intent," not grammar.

### Pillar C — Make the proof the product.
Code today ships with tests (hope). Lume code ships with a **proof you can verify independently**.
That proof becomes the *unit of virality*: a signed **"Lume Verified ✓"** certificate you embed in a
PR, a package, a release, a smart contract — the padlock-icon moment for code correctness. Registries
rank by proof coverage; code review shows a **proof diff**, not a text diff; a dependency's
verification status is a social and competitive signal no npm/cargo can show.

## The sequenced plan (execute after the language is complete)

Each track: goal · foothold (what already exists to build on) · acceptance.

### G0 — Single zero-C binary + one-command install *(foundation; fast)*
- **Goal.** `curl … | sh` drops one static `lume` binary; `lume self update` manages versions;
  Homebrew / apt / Scoop packages.
- **Foothold.** Self-hosted, zero-C: a single static binary is already achievable and is itself a
  marketing asset (Go's single-binary story, but with a *provable* bootstrap).
- **Acceptance.** Fresh machine → working `lume` in one command, no C toolchain, reproducible hash.

### G1 — Oracle-as-a-service + MCP tool *(Pillar B; the priority)*
- **Goal.** Ship `lume` as an **MCP server** exposing `prove`, `fill`, `compile --json`, and the
  oracle queries (`witness`, `repair`, `impact`). Any agent (Claude Code, codex, …) calls it as a
  tool and gets back structured proofs and counterexamples.
- **Foothold.** `lcaas` / `lume agent` already speak JSON in/out; `docs/19` oracle is the engine.
- **Acceptance.** An agent, with zero Lume knowledge, calls the tool and ships a proven function.
  **This is the move that makes Lume indispensable for programming with AI.**

### G2 — Verified sidecar / zero-migration export *(Pillar A)*
- **Goal.** A proven Lume core compiles to `.so` / `.a` / WASM and is called from Python / Node /
  Rust / Go, carrying its certificate (G3).
- **Foothold.** `docs/20` Track I5 (reverse `pub extern "c"` + WASM export).
- **Acceptance.** A Python and a Node demo each call a proven Lume function and display its proof.

### G3 — The proof certificate *(Pillar C; the viral unit)*
- **Goal.** `lume certify` emits a signed, **independently verifiable** artifact per proven
  obligation: source content-hash + contracts + the proof (or counterexample) + a reproducible-build
  attestation (stage2 == stage3). A `lume verify <cert>` checks it offline, needing no trust in us.
- **Foothold.** Sound `QF_LIA` prover (Stage B), content-addressing (P32), reproducible bootstrap,
  Lote 3 signing.
- **Acceptance.** A "Lume Verified ✓" badge renders in a README/PR; a third party verifies it offline.

### G4 — Registry ranked by proof coverage + verified dependency graph *(Pillar C)*
- **Goal.** Public registry where every package shows **% proven**, its proof obligations, and its
  certificate; you can view your whole dependency tree's verification status.
- **Foothold.** `lume pkg` exists.
- **Acceptance.** Publish/consume a package; the dependency graph shows per-node proof coverage — a
  supply-chain + correctness signal no other registry offers.

### G5 — Proof-diff code review bot *(Pillar C)*
- **Goal.** A GitHub app that, on every (AI-authored) PR, posts the **proof delta** — which proven
  properties this change preserves, strengthens, or breaks — instead of only a text diff.
- **Foothold.** `docs/19` #3 (semantic AST-patch edits), the oracle's `impact` query.
- **Acceptance.** A PR that breaks a proven invariant is flagged by the bot before a human reads it.

### G6 — Reimagined onboarding + playground *(Pillar B)*
- **Goal.** A web playground where the shareable link is a **proof**, not just a snippet; a "Lume
  Book" that teaches **stating provable intent**, not syntax; first contact is a dialogue with the
  oracle.
- **Foothold.** `lume compile --json`, the oracle.
- **Acceptance.** A newcomer, in the browser, states intent and gets proven code + a shareable proof
  link — without installing anything.

### G7 — Supply-chain-security narrative + killer demo *(Pillar C; owns a market)*
- **Goal.** Position Lume where its guarantees are worth most: **supply-chain security** (post
  xz/SolarWinds) — the only toolchain with a **provable, zero-C, trusting-trust-free bootstrap** —
  and a high-stakes fintech/security demo: *"the AI wrote this payment handler, the compiler proved it
  can never go negative, and the build is reproducible bit-for-bit."* Plus a public **verified-
  correctness leaderboard** ranking AI agents by how much proven-correct Lume they emit.
- **Foothold.** zero-C gate, reproducible bootstrap, sound prover; the security/fintech domain is our
  home turf.
- **Acceptance.** One demo a skeptic cannot dismiss, and a living benchmark the AI-coding field cites.

### G8 — The self-evolving flywheel *(unique; no other language can tell this story)*
- **Goal.** The standard library and optimisations evolve via **`lume evolve` proven proposals**;
  releases are AI-authored, proof-gated, signed. *"The first language that improves itself and ships
  the improvement — each change carrying a proof."*
- **Foothold.** `lume evolve` (E3), the 6-gate loop, Lote 3 release automation.
- **Acceptance.** A stdlib improvement lands via a proven, human-approved evolve proposal, end to end.

### G9 — Community, governance, docs *(consolidation)*
- RFC process, a published semver + stability policy, the Lume Book, and the conformance suite as the
  executable spec.

## Critical path and the flywheel

**Order:** G0 → **G1 (oracle/MCP)** → G3 (certificate) → G2 (sidecar) → G4 / G5 (proof-as-product) →
G6 / G7 (onboarding + narrative) → G8 / G9.

**The flywheel:** AI emits Lume → compiler **proves** it → a **certificate** ships → humans **trust**
it without re-reading → more agents embed the **oracle** → more **proven packages** in the registry →
**verified dependency graphs** become the norm → emitting unverified code looks reckless → more AI
emits Lume. Each turn makes the next cheaper. The asset compounds; the moat is the proof.

## Non-negotiables

The guarantees are the product, not marketing copy. They must stay **true**, or the whole pitch
collapses: `stage 2 == stage 3` bit-identical, `zero-c`, and the oracle byte-equivalence stay green;
`proven` always means *sound* (never sampling — see Stage B); every certificate must be verifiable by
a third party with no trust in us. A single false "proven" shipped as a certificate would do more
damage than a hundred bugs. GOLDEN RULE scales to the ecosystem.

## Explicitly not now

Start G-track work only after Phases 1–5 and the verification protection are complete. Do not dilute
the proof guarantee to chase a feature checkbox. Do not build the human-first registry/LSP before the
AI-first oracle/MCP (G1) — the agent toolbelt is the channel, not the download page.
