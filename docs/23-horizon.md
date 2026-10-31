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
- **Foothold.** V1–V3 contracts + the sound `QF_LIA` prover (Stage B), compositional verification (`@mode compose`), and LTL `temporal law` (`always` $\Box$, `eventually` $\Diamond$, `until` $\mathcal{U}$, `leads_to` $\leadsto$ proved by `QF_LIA` inductive invariants and ranking functions, with `[V0005]` multi-step trace & Büchi lasso counterexamples and standalone `lcert.src` certificate verification, `#198`); the effect system (K) tracks what crosses boundaries.
- **Remaining horizon.** Full branching-time CTL* / $\mu$-calculus over concurrent actor message graphs and global heap separation-logic frame inference across arbitrary pointer graphs.

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

- **H1 — Compositional Multi-Module & LTL Temporal Proof (`lume prove --compositional <mod_a.src> <mod_b.src>`, `temporal law`, `@mode compose`, `#198`):**
  1. **Compositional & Relational Law Verification:** Combines producer and consumer modules, verifies cross-module `requires`/`ensures` call obligations and relational/algebraic `law` invariants via `QF_LIA`, and reports `"compositional_verified": true` (or `[V0001]` / `[V0004]` diagnostics on failure).
  2. **LTL Temporal Logic Verification (`always` $\Box$, `eventually` $\Diamond$, `until` $\mathcal{U}$, `leads_to` $\leadsto$):** First-class `temporal law <name>(s: int) = ...` declarations in `compiler/stage1/lume/04_parser_decls.src`, `compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/lcaas.src`, and `compiler/stage1/lcert.src` prove infinite-horizon state-transition traces:
     - `always(init, step(s), inv)` ($\Box P$): proves inductive invariance (`init => inv` base case and `inv => let _s_next = step(s) in inv[s := _s_next]` inductive step in `QF_LIA`, with `If`-branch case splitting) plus bounded trajectory simulation, refuting invariant violations with `[V0005]` and a concrete multi-step state trace (`"trace: t=0: s=5 -> t=1: s=2 -> t=2: s=-1 (violates always invariant at t=2)"`).
     - `eventually(init, step(s), goal[, rank])` ($\Diamond Q$) & `leads_to(init, step(s), trigger, goal[, rank])` ($P \leadsto Q$): proves termination/liveness via a well-founded ranking measure (`not_goal => rank(s) > 0` and `not_goal => let _s_next = step(s) in rank(_s_next) < rank(s)` in `QF_LIA`) plus Büchi lasso cycle detection over state trajectories, refuting stuttering fixed points or cycles with `[V0005]` (`"lasso/diverging trace: ... (lasso cycle at s=1 while eventually goal false)"`).
     - `until(init, step(s), hold, goal)` ($P\ \mathcal{U}\ Q$): proves both prior-to-goal invariant preservation (`init => (goal or hold)` and `(not_goal and hold) => let _s_next = step(s) in (hold(_s_next) or goal(_s_next))`) and eventual goal reachability (`eventually`).
     - **Independent Proof Kernel (`compiler/stage1/lcert.src`):** `lume certify` emits compound temporal proof terms `(temporal-always <base> <step>)`, `(temporal-eventually <bound> <dec>)`, and `(temporal-until <hbase> <hstep> <bound> <dec>)`, and `lume verify-cert` independently re-verifies every sub-implication in `lcert.src` (`"kernel_verified": true`, `"badge": "Lume Verified ✓"`).
- **H2 — Anti-Rot Dependency Proof-Delta Verification (`lume pkg check-rot <old_dep.src> <new_dep.src> <downstream.src>`, `@mode check-rot`):** Detects semantic rot when a dependency upgrade weakens or breaks a contract relied upon by a downstream module, failing at compile time with `[V0003]`.
- **H3 — Minimal Independent Proof Kernel (`compiler/stage1/lcert.src`):** `lume verify-cert` re-derives Fourier–Motzkin `QF_LIA` proofs and FIPS-180-4 SHA-256 digests in a standalone Lume kernel (`#183`). The earlier Lean 4 sidecar was removed during the Lume-only migration; no current Lean mechanisation is claimed.
- **H4 — Learned ⊕ Proven Seam Verification (`@mode compose`, `[V0004]`):** Tracks `extern llm fn` and probabilistic calls across module boundaries and enforces that every learned output is discharged through a `verify(...)` contract guard before flowing into a proven function (`"seam_verified": true`, `[V0004]` on unverified flow).
- **H5 — Freestanding Multi-ISA Bare-Metal & Real-Time WCET Verification (`compiler/stage1/ltarget.src`, `examples/apps/lume_os/boot_x86_64.s`):** Emits native executables and freestanding Intel HEX (`IHEX`) / raw binaries across `x86-64`, `AArch64`, `RISC-V 64`, `WASM32`, and `LUMB` (`#206`), boots `lume-os` under `qemu-system-x86_64` (`#227`), and enforces `@bound(cycles: N)` WCET limits statically (`[V0002]`).
