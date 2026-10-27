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
   - **I3 (`lume agent bind <header.h>`):** AI-first C header binding generator (`compiler/stage1/lcert.src`) emitting structured JSON with `extern "c" fn` declarations and provenance-verified Lume wrappers.
   - **I4 (`std.wasm` Sandbox Host):** Deterministic, capability-scoped WebAssembly sandbox (`wasm_module_new`, `wasm_grant_cap`, `wasm_call_add`, `wasm_call_mem`, `wasm_fuel_left`) gated by the `"wasm"` effect.
   - **I5 (Reverse C-ABI & WASM Export):** `pub extern "c" fn` / `@export("C")` and `ffi_export_c_header` / `wasm_export_wat` so Python, Node, Rust, and Go can call proven Lume cores (`docs/22-adoption.md` Pillar A / G2).
2. **Track K5 — Realtime, Interrupts, Static WCET & Linear Hardware Capabilities (`std.os.cap`):**
   - `@interrupt(irq: N)` and `interrupt` / `realtime` effects: compile-time enforcement in `06b_phase3.src` and `lcaas.src` rejecting `alloc`, `io`, `net`, `fs`, and `async` inside interrupt service routines (`[E0200]`).
   - `@bound(cycles: N)`: static Worst-Case Execution Time (WCET) analyzer integrated into `lume prove` (`06b_phase3.src` + `lcaas.src`), emitting `"kind":"wcet"` proof obligations (`"status":"proven"` when `est_cycles <= bound`, or `[V0002]` `"status":"refuted"` when exceeded or recursive).
   - Linear hardware capabilities `cap[Page]`, `cap[Port]`, `cap[Irq]` tracked by the Phase 3 ownership checker (`[O0001]` on double consumption) and `platform` / `app` top-level items.
3. **Track O5 — `lume-os` Microkernel (`examples/apps/lume_os/`) & Proof Certificates / MCP (`G1`, `G3`, `H3`):**
   - **`compiler/stage1/lcert.src`:**
     - `lume certify <file.src>` and `lume verify-cert <file.src>`: emits and independently verifies a self-contained proof certificate JSON (`"badge":"Lume Verified ✓"`, `QF_LIA` + `WCET` obligations, source hash, and `stage2==stage3` attestation) via a minimal de Bruijn-style proof kernel.
     - `lume mcp <file.src>`: JSON-RPC 2.0 MCP server exposing `lume_check`, `lume_prove`, `lume_fill`, `lume_bind`, and `lume_certify`.
   - **Stage 5 Exit-Criterion Capstone (`examples/apps/lume_os/`):**
     - Capability-based microkernel (`caps.src`, `scheduler.src`, `ipc.src`, `main.src`) with `QF_LIA`-proven page-table and IPC capability invariants (0 runtime `ctfail` guards), a `@bound(cycles: 64)` WCET-proven scheduler, a `@interrupt(irq: 32)` timer ISR, and isolated user-space execution of the Stage 4 neuro-symbolic agent.

---

## 2. Diagnostic Codes Introduced in Phase 5

| Code | Pass | Condition |
|---|---|---|
| `A0003` | `06b_phase3.src`, `lcaas.src` | Untrusted `extern "c" fn` (foreign) return value flows into a `pub fn` return or trusted sink without `verify(...)` or `ensures` discharge |
| `E0200` | `06b_phase3.src`, `lcaas.src` | Unhandled `"ffi"` / `"wasm"` effect, or `@interrupt(irq: N)` handler invoking `alloc`, `io`, `net`, `fs`, or `async` |
| `O0001` | `06b_phase3.src`, `lcaas.src` | Linear hardware capability (`cap[Page]`, `cap[Port]`, `cap[Irq]`) consumed more than once |
| `V0002` | `06b_phase3.src`, `lcaas.src` | Function worst-case cycle estimate exceeds `@bound(cycles: N)` (`WCET` refuted) |

---

## 3. Non-Negotiable Invariants

1. **Triple Bootstrap (`stage2 == stage3`):** All Phase 5 compiler extensions in `04_parser_decls.src` and `06b_phase3.src` compile cleanly through `lume0` -> `stage1` -> `stage2` -> `stage3` with byte-identical ELF binaries.
2. **Zero-C Steady State:** All Phase 5 runtime, interop sandbox, WCET, proof-certificate, and `lume-os` modules are written in 100% pure Lume.
3. **Sound Proof Elision:** `lume-os` kernel memory and scheduler functions with `QF_LIA`-proven contracts compile with `call ctfail == 0` in the emitted x86-64 assembly.
