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

> **Implementation status.** This section and Track I describe the *specified design*. What exists today: `extern "c"` is parsed and effect/provenance-checked (`"ffi"` effect, `[A0003]`), `lume agent bind` emits binding JSON, and `std.wasm` is an in-language **model** of a capability-scoped sandbox. **Not implemented:** native linking (no dynamic ELF/PLT/GOT, no `[ffi]` handling), real C-ABI call lowering to foreign symbols, real WASM module hosting, and reverse C-ABI/WASM export (#11, #186).

1. **`extern "c"` declarations.** Declare a foreign function/type with its C signature; the backend
   emits the call in the System V AMD64 convention (`lasm` already emits native x86-64, so this is an
   extension of existing codegen, not a new backend).
   ```lume
   extern "c" { fn sqlite3_open(path: cstr, out: ptr[ptr]) -> int }
   ```
2. **Linking** *(planned, not implemented).* `lume pkg` gains a C-dependency declaration and links against a system `.so` (dynamic:
   ELF dynamic section + PLT/GOT, which `lasm`'s ELF writer is extended to emit) or `.a` (static).
3. **WASM host** *(planned; today only a model).* A `wasm` capability runs a WASM module in-process with an explicit import surface, so
   the sandbox sees only the capabilities Lume grants it (no ambient `net`/`fs`).

## AI-first binding generation (the Lume advantage)

Rust has `bindgen`, Zig has `@cImport`; Lume has **the model**. Point `lume agent` at a C header (or
docs) and it **generates the `extern "c"` declarations and a safe, idiomatic Lume wrapper**
automatically — reviewed and proven like any Lume code. "Use any C library" becomes: *show the header,
the AI writes the proven binding.* This is interop at the speed of AI, and it is why interop fits a
language built for AI.

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

Interop is two-way *(planned; not implemented)*: Lume also **exposes a C ABI** (`pub extern "c" fn …`) and can compile to a `.so` /
`.a` / WASM module, so other languages consume Lume. A Lume library becomes usable from Python, Node,
Rust, Go — broadening adoption without those ecosystems needing to know Lume.

## Track I — interop (Phase 4/5)

This is **not core** and lands after the language is systems-capable (ownership, effects, concurrency
from Phase 3). Bricks:

- *Status: I1 parse+check only; I2, I5 and the real I4 host are not implemented; I3 emits binding JSON (#11).*
- **I1 — `extern "c"` declarations + System V AMD64 call lowering** (codegen + `lasm` call sites).
- **I2 — Linking**: dynamic (`.so`, ELF dynamic section/PLT/GOT) and static (`.a`), wired into `lume pkg`.
- **I3 — AI binding generator**: `lume agent bind <header.h>` → `extern "c"` + safe wrapper (JSON/`lcaas`).
- **I4 — WASM host + sandbox**: run a WASM module with a capability-scoped import surface.
- **I5 — Reverse export**: `pub extern "c"` + compile Lume to `.so`/`.a`/WASM for other languages.
- **Cross-cutting**: every brick gated by capability (P26) + effect (P14) + A3 provenance; each a green
  PR with positive/negative tests; the pure core, `stage 2 == stage 3`, and the Zero-C gate stay green.

## Bottom line

Lume does not need to rewrite the world, and it does not need to borrow another language's ecosystem
to exist — it already self-hosts independently. Interop is the pragmatic bridge: **speak the C ABI and
host WASM, generate the bindings with the AI, and guard every foreign value with the same
provenance/capability model built for AI output.** The world's libraries enrich Lume while the core
stays pure, reproducible, and safe.
