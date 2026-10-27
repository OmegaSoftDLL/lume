# Phase 5 Plan — Machine Level, Interop (Track I), Realtime/WCET, `lume-os` Microkernel & Proof Certificates

**Status:** Implemented & Active (Stage 5 of `docs/04-roadmap.md`, `docs/20-interop.md`, `docs/22-adoption.md` G1/G3, `docs/23-horizon.md` H1–H5)
**Prerequisite:** Phase 4 (Tensors, Autodiff, GPU Kernels, Probabilistic/Rules/LLM/Neuro, PR #181)

---

## 1. Scope & Architecture

Phase 5 completes the Lume language roadmap (`docs/04-roadmap.md` §Stage 5), the universal C/WASM interop layer (`docs/20-interop.md` Track I1–I5), and the proof-certificate & MCP oracle distribution primitives (`docs/22-adoption.md` G1/G3, `docs/23-horizon.md` H3):

1. **Track M5 / Interop (`I1–I5`, `P16`, `P26`, `A3`):**
   - Low-level attributes & blocks: `@repr(c)`, `@packed`, `@bits(N)`, `@mmio(addr)`, `@export("C")`, `unsafe`, inline `asm(...)`, `import c "header.h"`, `extern "c" fn`, and `pub extern "c" fn`.
   - **I1 (`extern "c"` + System V AMD64 ABI):** `extern "c" fn` calls infer the `"ffi"` effect (`[E0200]` if unhandled) and mark returned values with **`foreign` / `unverified` provenance (`[A0003]`)** until discharged via `verify(...)` or a proven `ensures` contract.
   - **I2 (Linking metadata):** `[ffi]` link declarations in `lume.toml` and ELF dynamic/static symbol descriptors in `std.ffi` (`compiler/stage1/interop.src`).
   - **I3 (`lume agent bind <header.h>`):** AI-first C header binding generator (`compiler/stage1/lcaas.src`) emitting structured JSON with `extern "c" fn` declarations and provenance-verified Lume wrappers.
   - **I4 (`std.wasm` Reference Sandbox Model + Capability Gate):** Deterministic reference semantics and capability-sandbox model (`wasm_module_new`, `wasm_grant_cap`, `wasm_call_add`, `wasm_store32`, `wasm_load32`, `wasm_call_host_fs` in `compiler/stage1/interop.src`) paired with compile-time effect and module `@capability` enforcement in `06b_phase3.src` and `lcaas.src` (`wasm_call_host_fs` requires both `"wasm"` and `"fs"` capabilities, failing with `[E0200]` if `"fs"` is not granted to the module).
   - **I5 (Reverse C-ABI & WASM Export):** `pub extern "c" fn` / `@export("C")` and `ffi_export_c_header` / `wasm_export_wat` so Python, Node, Rust, and Go can call proven Lume cores (`docs/22-adoption.md` Pillar A / G2).
2. **Track K5 — Realtime, Interrupts, Structural Cycle-Budget Checker (`@bound`) & Linear Hardware Capabilities (`std.os.cap`):**
   - `@interrupt(irq: N)` and `interrupt` / `realtime` effects: compile-time enforcement in `06b_phase3.src` and `lcaas.src` rejecting `alloc`, `io`, `net`, `fs`, and `async` inside interrupt service routines (`[E0200]`).
   - `@bound(cycles: N)`: static structural AST cycle-budget analyzer integrated into `lume prove` (`06b_phase3.src` + `lcaas.src`), emitting `"kind":"wcet"` obligations with `"status":"budget_checked"` when the structural cycle cost `est_cycles <= bound` (distinguished from mathematical `QF_LIA` `"status":"proven"` obligations, since a structural AST cost model does not model microarchitectural pipelines, caches, or `idivq` variable latency), or `[V0002]` `"status":"refuted"` when exceeded or recursive.
   - Linear hardware capabilities `cap[Page]`, `cap[Port]`, `cap[Irq]` tracked by the Phase 3 ownership checker (`[O0001]` on double consumption) and `platform` / `app` top-level items.
3. **Track O5 — `lume-os` Microkernel (`examples/apps/lume_os/`) & Proof Certificates / MCP (`G1`, `G3`, `H3`):**
   - **`compiler/stage1/lcert.src` (Standalone Independent Proof-Certificate Verifier):**
     - `lume certify <file.src>` (`lcaas.src`) emits a certificate artifact JSON containing cryptographic 256-bit SHA-256 digests (`source_hash` and `certificate_hash`), S-expression `QF_LIA` proof terms `(implies (and ...) <goal>)`, and `wcet_budget(est <= bound)` terms.
     - `lume verify-cert <cert.json | file.src>` invokes `compiler/stage1/lcert.src` — a standalone, zero-dependency verifier with **no compiler imports (`use lcaas` is not used)** that independently verifies the FIPS 180-4 SHA-256 tamper-evidence hash, parses each S-expression proof term, re-runs its own Fourier–Motzkin `QF_LIA` solver over $\mathbb{Z}$ on `not(term)` without trusting the stored `"status"`, and checks `"kind":"wcet"` `"status":"budget_checked"` bounds (`"badge":"Lume Verified ✓"`).
     - `lume mcp <file.src>`: JSON-RPC 2.0 MCP server exposing `lume_check`, `lume_prove`, `lume_fill`, `lume_bind`, and `lume_certify`.
   - **Stage 5 Exit-Criterion Capstone (`examples/apps/lume_os/`):**
     - Capability-based microkernel (`caps.src`, `scheduler.src`, `ipc.src`, `main.src`) with `QF_LIA`-proven page-table and scheduler contracts (0 runtime `ctfail` guards), a `@bound(cycles: 64)` budget-checked scheduler, a `@interrupt(irq: 32)` timer ISR, and isolated user-space execution of the Stage 4 neuro-symbolic agent.

---

## 2. Diagnostic Codes Introduced in Phase 4 & Phase 5

| Code | Pass | Condition |
|---|---|---|
| `N0400` | `06b_phase3.src`, `lcaas.src` | Symbolic autodiff (`grad`, `value_and_grad`, `jvp`, `vjp`) applied to a function containing non-polynomial operators (`/`, `%`, comparisons) or unhandled function calls |
| `A0003` | `06b_phase3.src`, `lcaas.src` | Untrusted `extern "c" fn` (foreign) return value flows into a `pub fn` return or trusted sink without `verify(...)` or `ensures` discharge |
| `E0200` | `06b_phase3.src`, `lcaas.src` | Unhandled `"ffi"` / `"wasm"` / `"fs"` / `"net"` effect or missing module `@capability`, or `@interrupt(irq: N)` handler invoking `alloc`, `io`, `net`, `fs`, or `async` |
| `O0001` | `06b_phase3.src`, `lcaas.src` | Linear hardware capability (`cap[Page]`, `cap[Port]`, `cap[Irq]`) consumed more than once |
| `V0002` | `06b_phase3.src`, `lcaas.src` | Function structural cycle cost exceeds `@bound(cycles: N)` (`wcet` budget refuted) |

---

## 3. Non-Negotiable Invariants

1. **Triple Bootstrap (`stage2 == stage3`):** All Phase 5 compiler extensions in `04_parser_decls.src` and `06b_phase3.src` compile cleanly through `lume0` -> `stage1` -> `stage2` -> `stage3` with byte-identical ELF binaries.
2. **Zero-C Steady State:** All Phase 5 runtime, interop sandbox, WCET, proof-certificate, and `lume-os` modules are written in 100% pure Lume.
3. **Sound Proof Elision:** `lume-os` kernel memory and scheduler functions with `QF_LIA`-proven contracts compile with `call ctfail == 0` in the emitted x86-64 assembly.
