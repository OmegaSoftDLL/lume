# Phase 5 Plan — Machine Level, Interop (Track I), Realtime/WCET, `lume-os` Microkernel & Proof Certificates

**Status:** Implemented & Active, with the honest limits flagged inline below (Track I1/I2/I3/I5 real FFI linking & C-ABI/WASM export shipped in `#191`; Track K0–K9 bare-metal x86-64 `lume-os` kernel image booting in QEMU shipped in `#227`; `std.wasm` is a reference model; `@bound` is a structural AST cycle-budget checker; certificates are independently verified by `lcert.src` over FIPS 180-4 SHA-256 and `QF_LIA` S-expressions, and unsigned) (Stage 5 of `docs/04-roadmap.md`, `docs/20-interop.md`, `docs/22-adoption.md` G1/G3, `docs/23-horizon.md` H1–H5)
**Prerequisite:** Phase 4 (Tensors, Autodiff, GPU Kernels, Probabilistic/Rules/LLM/Neuro, PR #181)

---

## 1. Scope & Architecture

Phase 5 completes the Lume language roadmap (`docs/04-roadmap.md` §Stage 5), the universal C/WASM interop layer (`docs/20-interop.md` Track I1–I5), and the proof-certificate & MCP oracle distribution primitives (`docs/22-adoption.md` G1/G3, `docs/23-horizon.md` H3):

1. **Track M5 / Interop (`I1–I5`, `P16`, `P26`, `A3`, `#191`):**
   - Low-level attributes & blocks: `@repr(c)`, `@packed`, `@bits(N)`, `@mmio(addr)`, `@export("C")`, `@link("<soname>")`, `unsafe`, inline `asm(...)`, `import c "header.h"`, `extern "c" fn`, and `pub extern "c" fn`.
   - **I1 (`extern "c"` + System V AMD64 ABI, `shipped`):** `extern "c" fn` calls infer the `"ffi"` effect (`[E0200]` if unhandled), mark returned values with **`foreign` / `unverified` provenance (`[A0003]`)** until discharged via `verify(...)` or a proven `ensures` contract, and lower in `05_codegen.src` to `.extern <sym>` + `__lume_ffi_<sym>` System V AMD64 unboxing/boxing trampolines.
   - **I2 (Dynamic & Static Linking, `shipped`):** `lasm.src` (`asmdyn`) emits dynamically linked ELF64 executables (`PT_INTERP`, `PT_DYNAMIC`, `DT_NEEDED`, `.dynsym`, `.dynstr`, `.got.plt`, `.rela.dyn` `R_X86_64_GLOB_DAT`, and PLT stubs) when `.need`/`.extern` directives are present while keeping static ELF output byte-identical otherwise. `lmod.src` and `lume build --link <lib.so|lib.a>` support `[ffi] libs = [...]` in `lume.toml` and enforce that `"ffi"` is granted in `[capabilities] allow` (`[E0200]`).
   - **I3 (`lume agent bind <header.h>`, `shipped` for an explicit scalar/borrowed-string subset):** AI-first C header binding generator (`compiler/stage1/lcaas.src`) emitting structured JSON with N-parameter `extern "c" fn` declarations and provenance-preserving `foreign_<name>` adapters. The accepted ABI is single-line `int64_t` functions with `int64_t` or borrowed `const char *` parameters; generated public returns are `unverified[int]`. Unsupported pointers, mutable buffers, callbacks, variadics, malformed declarations, and empty headers produce `[I0003]` and no generated source.
   - **I4 (`std.wasm` Reference Sandbox Model + Capability Gate, `model`):** An in-language *model* of a capability-scoped WebAssembly sandbox (`wasm_module_new`, `wasm_grant_cap`, `wasm_call_add`, `wasm_store32`, `wasm_load32`, `wasm_call_host_fs` in `compiler/stage1/interop.src`) paired with compile-time effect and module `@capability` enforcement in `06b_phase3.src` and `lcaas.src` (`wasm_call_host_fs` requires both `"wasm"` and `"fs"` capabilities, failing with `[E0200]` if `"fs"` is not granted to the module, #186). Embedding a binary WASM VM remains *planned*.
   - **I5 (Reverse C-ABI & WASM Export, `shipped`):** `pub extern "c" fn` / `@export("C")` emit System V AMD64 C-ABI entrypoints in `05_codegen.src` (`.globl <name>`, lazy 64MB heap initialization when called from a foreign C `main`, callee-saved register preservation, argument boxing, and return unboxing). `lume build -o <out.a|out.so|out.wat|out.wasm>` and `lume agent c-export` emit C `.h` headers (with real parameter types and contract comments), static `.a` archives, shared `.so` libraries callable from C, and WebAssembly `.wat`/`.wasm` modules over the scalar `i64` fragment (rejecting unsupported constructs with `[I0002]`).
2. **Track K5 — Realtime, Interrupts, Structural Cycle-Budget Checker (`@bound`) & Linear Hardware Capabilities (`std.os.cap`):**
   - `@interrupt(irq: N)` and `interrupt` / `realtime` effects: compile-time enforcement in `06b_phase3.src` and `lcaas.src` rejecting `alloc`, `io`, `net`, `fs`, and `async` inside interrupt service routines (`[E0200]`).
   - `@bound(cycles: N)`: a static structural AST cycle-budget estimator (not a hardware-accurate WCET analysis, #185) integrated into `lume prove` (`06b_phase3.src` + `lcaas.src`), emitting `"kind":"wcet"` obligations with `"status":"budget_checked"` when the structural cycle cost `est_cycles <= bound` (distinguished from mathematical `QF_LIA` `"status":"proven"` obligations, since a structural AST cost model does not model microarchitectural pipelines, caches, or `idivq` variable latency), or `[V0002]` `"status":"refuted"` when exceeded or recursive.
   - Linear hardware capabilities `cap[Page]`, `cap[Port]`, `cap[Irq]` tracked by the Phase 3 ownership checker (`[O0001]` on double consumption) and `platform` / `app` top-level items.
3. **Track O5 — `lume-os` Microkernel (`examples/apps/lume_os/`) & Proof Certificates / MCP (`G1`, `G3`, `H3`):**
   - **`compiler/stage1/lcert.src` (Standalone Independent Proof-Certificate Verifier):**
     - `lume certify <file.src>` (`lcaas.src`) emits a certificate artifact JSON containing cryptographic 256-bit SHA-256 digests (`source_hash` and `certificate_hash`), S-expression `QF_LIA` proof terms `(implies (and ...) <goal>)`, and `wcet_budget(est <= bound)` terms. Both producer and verifier hash the complete input; #340 adds NIST 56-byte and long-input vectors plus independent `hashlib` checks of real certificate source/certificate hashes.
     - `lume verify-cert <cert.json | file.src>` invokes `compiler/stage1/lcert.src` — a standalone, zero-dependency verifier with **no compiler imports (`use lcaas` is not used)** that independently verifies the FIPS 180-4 SHA-256 tamper-evidence hash, parses each S-expression proof term, re-runs its own Fourier–Motzkin `QF_LIA` solver over $\mathbb{Z}$ on `not(term)` without trusting the stored `"status"`, and checks `"kind":"wcet"` `"status":"budget_checked"` bounds (`"badge":"Lume Verified ✓"`, #183). **Honest limit:** certificates carry a SHA-256 tamper digest, not an asymmetric cryptographic signature.
     - `lume mcp <file.src>`: JSON-RPC 2.0 MCP server exposing `lume_check`, `lume_prove`, `lume_fill`, `lume_bind`, and `lume_certify`.
   - **Stage 5 Exit-Criterion Capstone (`examples/apps/lume_os/`, `shipped` on x86-64 QEMU in `#227`):**
     - Includes both the cross-platform user-space harness (`main.src`) and the real `@freestanding` bare-metal x86-64 kernel image (`kernel_qemu.src` -> `lume_os.img`) that boots in `qemu-system-x86_64` with zero Linux syscalls (`grep -c syscall == 0`), 4-level page tables (`CR3=0x1000`), physical frame allocator at `0x200000`, hardware 64-bit IDT + `int $32` + `@interrupt` + `iretq`, volatile MMIO, `lock xaddq` + spinlocks, COM1 (`0x3F8`) serial UART, and `QF_LIA`-proven allocator/scheduler/IPC services (`call ctfail == 0`).

---

## 2. Diagnostic Codes Introduced in Phase 4 & Phase 5

| Code | Pass | Condition |
|---|---|---|
| `N0400` | `06b_phase3.src`, `lcaas.src` | Symbolic autodiff (`grad`, `value_and_grad`, `jvp`, `vjp`) applied to a function containing non-polynomial operators (`/`, `%`, comparisons) or unhandled function calls |
| `A0003` | `06b_phase3.src`, `lcaas.src` | Untrusted `extern "c" fn` (foreign) return value flows into a `pub fn` return or trusted sink without `verify(...)` or `ensures` discharge |
| `E0200` | `06b_phase3.src`, `lcaas.src` | Unhandled `"ffi"` / `"wasm"` / `"fs"` / `"net"` effect or missing module `@capability`, or `@interrupt(irq: N)` handler invoking `alloc`, `io`, `net`, `fs`, or `async` |
| `O0001` | `06b_phase3.src`, `lcaas.src` | Linear hardware capability (`cap[Page]`, `cap[Port]`, `cap[Irq]`) consumed more than once |
| `V0002` | `06b_phase3.src`, `lcaas.src` | Function structural cycle cost exceeds `@bound(cycles: N)` (`wcet` budget refuted) |
| `I0002` | `lcaas.src`, `interop.src` | Function exported to WebAssembly (`c-export` / `-o *.wat|*.wasm`) uses a construct outside the supported WASM scalar `i64` fragment |
| `I0003` | `lcaas.src` | C binding declaration is outside the supported `int64_t` / borrowed `const char *` subset, has undefined pointer/buffer/callback ownership, is malformed, or contains no supported function |

---

## 3. Non-Negotiable Invariants

1. **Triple Bootstrap (`stage2 == stage3`):** All Phase 5 compiler extensions in `04_parser_decls.src` and `06b_phase3.src` compile cleanly through `lume0` -> `stage1` -> `stage2` -> `stage3` with byte-identical ELF binaries.
2. **Zero-C Steady State:** All Phase 5 runtime, interop sandbox, WCET, proof-certificate, and `lume-os` modules are written in 100% pure Lume.
3. **Sound Proof Elision:** `lume-os` kernel memory and scheduler functions with `QF_LIA`-proven contracts compile with `call ctfail == 0` in the emitted x86-64 assembly.
