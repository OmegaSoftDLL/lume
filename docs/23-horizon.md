# 23 — Horizon: from proving pieces to proving systems that cannot rot

The roadmap — five phases, the inflection (`docs/21`), interop (`docs/20`), adoption (`docs/22`) —
takes Lume to something almost nothing in the world has: **the AI writes, the compiler proves, it is
reproducible, it interoperates, it is adopted.** This document is the horizon *beyond* that: what it
takes to go from "an excellent verified AI-first language" to **changing what software is**.

Be honest about the register: these are research-grade, multi-year directions, not next sprints. But
none are magic — each is anchored in something Lume already has, and each is the star a later phase
climbs toward. Lume today proves **pieces**; the horizon is proving **the whole system, across time,
in a form that outlives Lume itself**.

## The keystone

If one thing defines the horizon, it is **H1 + H3 together**: proofs that **compose across the whole
system and across time**, emitted into a **tiny, eternal trusted kernel**. That combination produces
something that does not exist today — software whose correctness is **not tested, not hoped for, but
demonstrated**, and whose demonstration **anyone can verify, alone, forever, trusting no one**. Add
proven self-evolution (`lume evolve`) and you get the first **autonomous software organism**: it
improves itself, proves each improvement, ships it, and explains it — and every claim is checkable
from a thousand-line root.

## H1 — Compositional & temporal proof: from proven functions to proven systems
- **Dream.** State a **system-level, time-spanning** property — *"no user's balance is ever negative,
  on every path, forever"* — and have it proven end to end, across modules and across the FFI
  boundary, not function by function.
- **Foothold.** V1–V3 contracts + the sound `QF_LIA` prover (Stage B); the effect system (K) already
  tracks what crosses boundaries.
- **Missing.** Compositional verification (proof-carrying modules that compose), **temporal** logic
  over effect traces (safety *and* liveness: "always", "eventually"), refinement types, and a frame
  discipline so local proofs lift to global ones. This is the largest leap — "proven functions" →
  "proven systems".

## H2 — Software that does not rot
- **Dream.** Code today decays silently: a dependency changes, a platform shifts, an assumption
  breaks and nothing tells you. Lume software should **know the moment its premise breaks**.
- **Foothold.** Provenance (A3), reproducible `stage 2 == stage 3`, content-addressing (P32),
  per-obligation proofs.
- **Missing.** **Proof-deltas across versions** (when a dependency's proven property changes, every
  downstream proof re-checks and names exactly what fell), and **proof-aware dependencies** so an
  upgrade that breaks a relied-upon guarantee is a compile-time event, not a production incident. An
  immune system against bit-rot.

## H3 — A portable, minimal trusted kernel: proofs that outlive Lume
- **Dream.** Twenty years from now, anyone, in any language, verifies a Lume proof with a checker
  small enough to read in an afternoon — trusting nothing about us.
- **Foothold.** The sound prover and the certificate sketch (`docs/22` G3).
- **Missing.** A **minimal trusted proof kernel** (de Bruijn-small, independently re-implementable)
  and an **exportable proof format**: Lume emits full proof terms, not just a verdict, and the kernel
  — not Lume — is the root of trust. This decouples correctness from the tool entirely.

## H4 — The fusion of the learned and the proven
- **Dream.** AI/probabilistic components and proven-deterministic components **compose with
  guarantees**: *"this model's output is always validated by a proven contract before it touches
  trusted state,"* with verified bounds on the probabilistic parts.
- **Foothold.** `llm fn` / `prob` + provenance (A3): foreign/learned values are born `unverified` and
  cannot flow into trusted code until discharged.
- **Missing.** A real **probabilistic-effect + proof-boundary calculus**: distributions as
  first-class, verified bounds on learned outputs, and a principled contract at the learned↔proven
  seam. The bridge between software that learns and software that is proven — which no language has.

## H5 — Down to the metal, up to the catastrophic
- **Dream.** The language you would write a pacemaker, a central bank's ledger, a kernel, or a smart
  contract in — and **prove** it. Where a bug is catastrophe and a proof is priceless.
- **Foothold.** Zero-C, direct x86-64, `@freestanding` (O4), the ownership/effect models.
- **Missing.** Mature freestanding/no-runtime support, a **formally sound memory & concurrency model**
  (not just checked — proven), and targets beyond x86-64 (ARM, RISC-V, GPU/accelerators, WASM) — each
  with the proof guarantees preserved across the backend.

## The autonomous software organism

Compose the horizons and the shape appears: a system that **synthesises** from intent (`docs/19`),
**proves** each change sound (H1), **cannot silently rot** (H2), ships a **certificate anyone can
check forever** (H3), **safely absorbs learned components** (H4), runs **on the bare metal of the
highest-stakes domains** (H5), and **evolves itself** under a proven, human-approved loop
(`lume evolve`). It improves, proves, ships, and explains — and you audit every claim from a
thousand-line kernel. That is not a better language; it is a new kind of artifact.

## Non-negotiables (the horizon dies without these)

Soundness is sacred. A single false "proven" — shipped as a certificate, relied on by a downstream
proof, trusted by a human who stopped reading — would do more damage than every bug Lume ever
prevented. So, forever: `proven` means *sound*, never sampling; every proof is **independently
checkable** by the minimal kernel (#183 landed a standalone Fourier–Motzkin checker + FIPS-180-4 SHA-256 that re-derives each proof independently of the compiler); `stage 2 == stage 3`, `zero-c`, and byte-equivalence stay green.
The GOLDEN RULE scales from the compiler to the ecosystem to the horizon.

## Relationship to the plan

This sits **above** adoption (`docs/22`) and **beyond** the five phases. It does not reorder the near
work — finish the language, the inflection, interop, and adoption first. It is the direction every
one of those climbs toward, and the reason the guarantees must stay pristine on the way: each phase
should leave Lume a step closer to proving **the whole system, across time, in a form that outlives
it**. The near plan earns the right to the horizon; the horizon tells the near plan what not to
compromise.

## Shipped Horizon Foundations (`#198`, `#213`, `#206`, `#227`)

- **H1 (partial) — Compositional Multi-Module & Relational Law Verification (`lume prove --compositional <mod_a.src> <mod_b.src>`, `@mode compose`):** Combines producer and consumer modules, verifies cross-module `requires`/`ensures` call obligations and **relational/algebraic** `law` invariants via `QF_LIA`, and reports `"compositional_verified": true` (or `[V0001]` / `[V0004]` diagnostics on failure). **Not temporal:** `QF_LIA` is quantifier-free linear arithmetic and cannot express temporal logic (LTL/CTL over traces/time); real **temporal** proof stays the open part of H1 (`#198`; this doc's §H1 still lists temporal logic as *Missing*). The shipped mechanism is sound for what it is — just not temporal.
- **H2 — Anti-Rot Dependency Proof-Delta Verification (`lume pkg check-rot <old_dep.src> <new_dep.src> <downstream.src>`, `@mode check-rot`):** Detects semantic rot when a dependency upgrade weakens or breaks a contract relied upon by a downstream module, failing at compile time with `[V0003]`.
- **H3 — Minimal Independent Proof Kernel & Mechanised Metatheory (`compiler/stage1/lcert.src`, `formal/lean/LumeCore.lean`):** `lume verify-cert` re-derives Fourier–Motzkin `QF_LIA` proofs and FIPS-180-4 SHA-256 digests in a standalone kernel (`#183`), and `formal/lean/LumeCore.lean` mechanises Lume-Core typing, progress, preservation, linear ownership (`O0001`), capability containment (`E0200`), and `QF_LIA` soundness in Lean 4 with zero `sorry`/`axiom` (`#213`).
- **H4 — Learned ⊕ Proven Seam Verification (`@mode compose`, `[V0004]`):** Tracks `extern llm fn` and probabilistic calls across module boundaries and enforces that every learned output is discharged through a `verify(...)` contract guard before flowing into a proven function (`"seam_verified": true`, `[V0004]` on unverified flow).
- **H5 — Freestanding Multi-ISA Bare-Metal & Real-Time WCET Verification (`compiler/stage1/ltarget.src`, `examples/apps/lume_os/boot_x86_64.s`):** Emits native executables and freestanding Intel HEX (`IHEX`) / raw binaries across `x86-64`, `AArch64`, `RISC-V 64`, `WASM32`, and `LUMB` (`#206`), boots `lume-os` under `qemu-system-x86_64` (`#227`), and enforces `@bound(cycles: N)` WCET limits statically (`[V0002]`).
