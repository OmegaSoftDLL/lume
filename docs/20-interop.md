# 20 — Interop: using the whole world's libraries without losing Lume's core

**Goal.** Let Lume reuse the existing universe of libraries — C, C++, Rust, Go, Zig, Swift, and
anything that compiles to WebAssembly — **without** rewriting them and **without** compromising
Lume's pure, zero-C, reproducible, capability-safe core. Interop is an **opt-in layer**, never part
of the self-hosted core; a program that uses it declares the capability and accepts the trade-off.

## The key insight: speak one lingua franca, not every language

There is **no stable ABI for rich types** (C++ templates, Rust/Go generics, Go's runtime/GC), so you
cannot consume another language's rich type system directly. But **every systems language can expose
a C ABI** (`extern "C"`, `#[no_mangle] extern "C"`, Go `-buildmode=c-shared`, Zig, Swift's C interop).
So the reach comes from **two universal bridges**, not from N language-specific importers:

| Bridge | Reaches | Strengths | Costs |
|--------|---------|-----------|-------|
| **C ABI + linking** | everything exposing a C interface (C, C++, Rust, Go, Zig, Swift, …) | native speed, full system access | unsafe, unsandboxed, breaks source-reproducibility |
| **WebAssembly sandbox** | everything that compiles to WASM (Rust, C/C++, Go, …) | **sandboxed, capability-controlled, portable, deterministic** | call overhead, not every library targets WASM |

"Compatible with everything" means **everything that exposes a C ABI or compiles to WASM** — roughly
all that matters. Rich foreign types are always crossed through a C or WASM boundary; that is a
property of the world's missing universal ABI, not a Lume limitation.

## How it works in Lume

> **Implementation status (`#191`).**
> - **Shipped (I1, I2, I3, I5):**
>   - **I1 (`extern "c" fn` + System V AMD64 call lowering):** `05_codegen.src` lowers `extern "c" fn` calls to System V AMD64 unboxing/boxing trampolines (`__lume_ffi_<sym>`), paired with compile-time `"ffi"` effect checking (`[E0200]`) and foreign return provenance checking (`[A0003]`).
>   - **I2 (Dynamic & Static Linking):** `lasm.src` (`asmdyn`) emits dynamically linked ELF64 binaries (`PT_INTERP`, `PT_DYNAMIC`, `DT_NEEDED`, `.dynsym`, `.dynstr`, `.got.plt`, `.rela.dyn` `R_X86_64_GLOB_DAT`, and PLT stubs) directly from `.need` / `.extern` / `@link("<soname>")`, and `lume build --link <lib.so|lib.a>` + `[ffi] libs = [...]` in `lume.toml` (enforcing `"ffi"` in `[capabilities] allow` `[E0200]`) link dynamic `.so` and static `.a` libraries end-to-end.
>   - **I3 (`lume adapt c <header.h>` / `lume agent bind <header.h>`):** Generates N-parameter `extern "c" fn` declarations and `pub fn foreign_<name>` adapters for strict single-line prototypes with `int64_t` returns, `int64_t` parameters, borrowed `const char *` strings, and adjacent `const char *name, int64_t name_len` read-only buffer pairs. Buffer wrappers derive the C length from `count(name)` and do not expose a caller-supplied length. Adapter returns remain `unverified[int]`; malformed declarations, duplicate symbols/parameters, mutable buffers, callbacks, variadics, and other ABI types fail closed with `[I0003]` and no generated source.
>   - **I5 (Reverse C-ABI & WASM Export):** `pub extern "c" fn` and `@export("C")` emit System V AMD64 C-ABI entrypoints (`.globl <name>`, lazy Lume heap initialization when entered from a foreign `main`, callee-saved register preservation, argument boxing, and return unboxing). `lume build -o <out.a|out.so>` builds static/shared libraries callable from C with generated `.h` headers (`--emit-c-header`), and `lume build -o <out.wat|out.wasm>` / `lume agent c-export` compiles the scalar `i64` fragment to `.wat` and `\0asm` `.wasm` modules, rejecting unsupported constructs with `[I0002]`.
> - **Model (I4 `std.wasm`):** `std.wasm` (`compiler/stage1/interop.src`) is an in-language reference model of linear-memory bounds, instruction fuel, and host capability flags paired with compile-time `[E0200]` capability enforcement (`wasm_call_host_fs` requires `"wasm"` and `"fs"`); in-process binary WASM VM hosting remains **planned**.

1. **`extern "c"` declarations (`shipped`).** Declare a foreign function with its C signature; `05_codegen.src`
   emits the unboxing/boxing trampoline in the System V AMD64 convention and `lasm` emits the ELF relocation / PLT call.
   ```lume
   @link("libc.so.6")
   extern "c" fn puts(s: str) -> int
   ```
2. **Linking (`shipped`).** `lume.toml` `[ffi] libs = [...]` (requiring `"ffi"` in `[capabilities] allow`) and `lume build --link <lib.so|lib.a>` link against `.so` (via `lasm`'s dynamic ELF64 writer: `PT_INTERP`, `PT_DYNAMIC`, `DT_NEEDED`, `.dynsym`, `.dynstr`, `.got.plt`, `.rela.dyn`, PLT) or static `.a` archives.
3. **WASM host (`model` / `planned`).** `std.wasm` models linear memory, fuel, and capability grants in Lume and enforces `"wasm"`/`"fs"`/`"net"` capabilities at compile time (`[E0200]`); executing arbitrary binary `.wasm` modules in-process remains planned.

## AI-first binding generation (the Lume advantage)

Rust has `bindgen`, Zig has `@cImport`; Lume preserves an explicit boundary contract. Run
`lume adapt c <header.h> -o <adapter.src>` to generate `extern "c"` declarations plus explicit
`foreign_<name>` adapters, or use `--json` (equivalently `lume agent bind`) for the machine-readable
manifest. The accepted fragment contains `int64_t` scalars, borrowed NUL-terminated `const char *`
strings, and adjacent `const char *name, int64_t name_len` read-only buffer pairs. A sized-buffer
adapter accepts only the Lume string and passes `count(string)` as the C length, so a caller cannot
provide an inconsistent length. The pointer is borrowed for the call only; no ownership transfer or
post-call retention is inferred.

Every generated return remains `unverified[int]` and requires validation before trusted use. JSON
manifests include the header's real SHA-256, per-parameter ownership/extent metadata, the `ffi`
capability, and foreign return provenance. These checks establish the generated call boundary, not
the foreign library's internal memory safety or its compliance with `const` and no-retention
contracts.

Parsing is deliberately strict: one complete prototype per line, `int64_t` returns, no macros or
conditional preprocessing, no duplicate symbols or parameter names, and no trailing tokens. Any
error rejects the whole header with `[I0003]`, produces no partial source, returns a nonzero CLI
status, and leaves an existing `-o` file unchanged. Mutable/output buffers, arbitrary pointers,
callbacks, variadics, other C types, HTTP/OpenAPI adapters, and adaptive MMIO remain open in #334.

## Safety model: interop reuses the A3 provenance mechanism

Foreign code can crash, leak, and ignore Lume's ownership and effects, so every boundary is guarded by
the machinery Lume already has:

- **Capability + effect.** An FFI/WASM call lives in an `ffi` / `wasm` effect (P14) and requires the
  capability to be granted in `lume.toml` (P26), exactly as I/O is. No ambient foreign access.
- **Provenance (A3).** A value returned from foreign code is born **`unverified` / `foreign`** in its
  type. By the A3 rule it **cannot flow into trusted or proven Lume code** (a `pub` boundary, a
  `LAWS`-guarded region, an effectful sink) until it is validated by a contract (`V1`), a `LAWS` gate
  (`V3`), or a human via `lume evolve`. The feature built for LLM output is exactly what makes foreign
  output safe: foreign == unverified.
- **`unsafe` boundary (P16).** Raw pointer/ABI operations live behind `unsafe`, visible and auditable.

## The purity boundary (be honest)

FFI with native linking **breaks** Lume's zero-C, reproducible-from-source, self-contained guarantees:
the program now depends on external binaries present at build/run time. Therefore:

- **The self-hosted core, std, and any `pure`/verified code stay interop-free** — the `stage 2 == stage
  3` fixpoint and the Zero-C gate are untouched.
- **A program that uses interop declares it** (`[capabilities] allow = ["ffi"]` / `["wasm"]`) and the
  compiler marks it as no longer pure/reproducible-from-source. You get both worlds, clearly separated:
  proven pure Lume for the core, interop to tap the world when it is pragmatic.
- **WASM is the intended default** for untrusted/third-party reuse (sandboxed + capability-scoped);
  native C ABI is for trusted, performance-critical, or system-level libraries. **Caveat:** today `std.wasm`
  is only an in-language model with no real module isolation, so do not rely on it to sandbox untrusted code
  until real WASM hosting lands (#186).

Optional, Zig-style: a `ffi/c` capability could bundle a C front-end to compile C *source*
dependencies directly (`@cImport`-like). Powerful, but it reintroduces C into that program's build, so
it is strictly opt-in and never in the pure core.

## The reverse direction

Interop is two-way (`shipped`, `#191`): Lume **exposes a C ABI** (`pub extern "c" fn …` / `@export("C")`) and compiles to `.so` /
`.a` / `.wat` / `.wasm` modules alongside a generated `.h` header (`lume build -o libfoo.so|libfoo.a|foo.wat|foo.wasm`), so C, Python, Node,
Rust, and Go can call verified Lume libraries directly.

## Track I — interop (Phase 4/5)

This is **not core** and lands after the language is systems-capable (ownership, effects, concurrency
from Phase 3). Bricks:

- *Status: I1, I2, I3, and I5 are **shipped** (`#191`); I4 `std.wasm` is an in-language capability/fuel **model** paired with compile-time `[E0200]` capability enforcement.*
- **I1 — `extern "c"` declarations + System V AMD64 call lowering** (`shipped`: `05_codegen.src` unboxing/boxing trampolines + `lasm` `.extern`).
- **I2 — Linking** (`shipped`: dynamic `.so` via `lasm` `PT_INTERP`/`PT_DYNAMIC`/`DT_NEEDED`/PLT/GOT and static `.a` via `lume build --link` + `[ffi] libs` in `lume.toml`).
- **I3 — AI binding generator** (`shipped` for the explicit scalar/borrowed-string/sized-read-only-buffer subset: `lume adapt c <header.h>` or `lume agent bind <header.h>` → N-parameter `extern "c" fn` + `foreign_<name>` adapters returning `unverified[int]`; unsupported ownership/callback boundaries fail closed with `[I0003]`).
- **I4 — WASM host + sandbox** (`model`: `std.wasm` capability/fuel/bounds reference model + compile-time `[E0200]` capability gate).
- **I5 — Reverse export** (`shipped`: `pub extern "c" fn` / `@export("C")` + `lume build -o <out.so|out.a|out.wat|out.wasm>` + `--emit-c-header` + `[I0002]` WASM fragment check).
- **Cross-cutting**: every brick gated by capability (P26) + effect (P14) + A3 provenance; each a green
  PR with positive/negative tests; the pure core, `stage 2 == stage 3`, and the Zero-C gate stay green.

## Bottom line

Lume does not need to rewrite the world, and it does not need to borrow another language's ecosystem
to exist — it already self-hosts independently. Interop is the pragmatic bridge: **speak the C ABI and
host WASM, generate the bindings with the AI, and guard every foreign value with the same
provenance/capability model built for AI output.** The world's libraries enrich Lume while the core
stays pure, reproducible, and safe.
