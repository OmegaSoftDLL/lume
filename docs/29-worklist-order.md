# 29 — Worklist execution order (for any agent, agy first)

The single ordered plan for the **open** worklist. It exists so work happens in a sound order, not
issue-number order. Two kinds of ordering are marked explicitly:

- **[HARD]** — a real technical dependency: the earlier item must exist first or the later one cannot
  be built.
- **[PRIORITY]** — a judgment call about what is worth doing first; reorderable by the user without a
  technical penalty.

Rule: **when an issue closes or a new one opens, update this file in the same change.** Each item
keeps its own tracking issue; this file only orders them. Everything still obeys the delivery-hardening
rule (`docs/28`): no brick is "done" until the real thing runs, with its guarantees intact.

## The spine (do roughly in this order)

1. **`#191` — real interop (FFI linking + C-ABI / WASM export).** *In progress now.* Finishes first
   because it is the current work and it unblocks the "compatible with C/C++/Rust/Go/WASM" goal.
   `#191` is **not [HARD] for the kernel** — a from-scratch kernel uses no C FFI.

2. **`#227` — systems & kernels, K0→K9 on x86-64** (`docs/27`). **[PRIORITY]** — the flagship
   proof-carrying kernel, the pitch no one else can make. Its only **[HARD]** prerequisite is the
   x86-64 native backend, **which already ships**, so it enters **right after `#191`**. Internal order
   is fixed by `docs/27` (K0 freestanding → … → K9 boots on QEMU). Note: **K7 closes `#193`** (real
   parallel runtime) and **K0 is the freestanding codegen** that `#206`'s bare-metal target then uses.

3. **`#206` — universal targets** (ARM64/Apple Silicon, Windows/PE, RISC-V, bare-metal). Split by
   dependency:
   - **`#206a` hosted targets (ARM64 macOS, Windows/PE):** independent of the kernel — **[PRIORITY]**,
     may run in parallel with the spine whenever there is bandwidth.
   - **`#206b` bare-metal / RISC-V embedded:** **[HARD]** on `#227`-K0 (freestanding codegen) and on the
     per-ISA K2/K3/K4 work — so it is the kernel's **second-target** phase, after x86-64 K9.

4. **`#197` — specified-but-unimplemented core features** (traits, closures, refinement types,
   async/await, rc/arc, shard, let-polymorphism; the AArch64/RISC-V/WASM backend items belong with
   `#206`). **[PRIORITY]** — language completeness. Runs as the **top parallel track alongside the
   spine**, not as a gate; but if kernel work hits a missing core primitive, that primitive is pulled
   forward ahead of the rest of `#197`.

## Parallel tracks (no gating either way — pick up by priority, any time)

- **`#230` — LLM-generation eval as a CI gate.** Do **early**: it guards against syntax/surface
  regressions that make Lume harder for a model to generate, and it serves the AI-first north star.
- **`#214` — data track** (persist table / versioned / `money[CCY]` / `@soa`). Feature track; independent.
- **`#210` — `lume lsp` + `lume dap`.** Developer/agent experience; independent.
- **`#211` — `lume repl` + notebook/Jupyter kernel.** DX; independent.
- **`#196` — adoption & distribution** (installer, registry, playground, certificates). Needed for
  real-world use; the playground also helps the AI-authoring loop.
- **`#215` — untracked planned items** (`signal[T]`/`promise[T]`, `lumb` bytecode, Linguist). **Triage
  first** (cheap): decide implement / fold into `#197`/`#206` / drop, then act.

## Horizon (last — research-grade, after the spine stands)

- **`#198` — horizon tracking** (compositional/temporal proof, anti-rot, portable kernel,
  learned⊕proven, metal targets). Depends on a solid proven core + a real kernel; it is the frontier,
  not near-term.

## One-line dependency summary
`#191` (now) → `#227` K0..K9 on x86-64 [flagship] → `#206b` bare-metal/other-ISA kernel ports.
`#193` folds into K7; `#206a` hosted targets and `#197` core features run in parallel as top priority;
`#230`/`#214`/`#210`/`#211`/`#196`/`#215` are independent parallel tracks; `#198` is the horizon.
