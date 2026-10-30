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

1. **`#191` — real interop (FFI linking + C-ABI / WASM export).** *(Done — System V AMD64 `extern "c" fn` unboxing/boxing trampolines and `@export("C")` / `pub extern "c" fn` C-ABI entrypoints in `05_codegen.src`, dynamic ELF64 `PT_INTERP`/`PT_DYNAMIC`/`DT_NEEDED`/PLT/GOT emission in `lasm.src`, `[ffi] libs` + `[E0200]` capability check in `lmod.src`, `lume build --link <lib.so|lib.a>` and `-o <out.a|out.so|out.wat|out.wasm>` + `--emit-c-header`, and `[I0002]` WASM fragment diagnostics in `lcaas.src`/`interop.src`.)*
   `#191` is **not [HARD] for the kernel** — a from-scratch kernel uses no C FFI.

2. **`#227` — systems & kernels, K0→K9 on x86-64** (`docs/27`). *(Done — `@freestanding` zero-syscall codegen & physical frame allocator at `0x200000` (K0/K1), system ISA `cli`/`sti`/`hlt`/`iretq`/`lgdt`/`lidt`/`invlpg`/`movq %crN`/`outb`/`inb`/`int $imm8`/`lock xaddq`/`lock cmpxchgq` and `.kernel` MBR + 4-level x86-64 page-table Long Mode bootloader in `lasm.src` (K2/K5/K6), volatile MMIO & hardware 64-bit IDT + `@interrupt` ISR lowering in `05_codegen.src` (K3/K4), real OS multi-process parallelism (`sys_fork` + `sys_pipe` + `sys_wait4`) & hardware atomics/spinlocks in `concurrency.src` (K7, closing `#193`), COM1 16550A UART driver (K8), and `examples/apps/lume_os/kernel_qemu.src` booting in `qemu-system-x86_64` in CI (K9).)*

3. **`#206` — universal targets** (ARM64/Apple Silicon, Windows/PE, RISC-V, bare-metal). *(Done — orthogonal target-triple architecture `(ISA encoder) ⊥ (container writer) ⊥ (OS/ABI layer)` in `compiler/stage1/ltarget.src`: `T0` triple selector & `lume targets [--json]`, `T1` 2-pass AArch64 A64 32-bit instruction encoder verified in `qemu-aarch64`, `T2` 16 KB page-aligned Mach-O 64 container writer + Darwin AAPCS64 ABI (`aarch64-macos-macho`), `T3` PE32+/COFF container writer + Windows x64 ABI (`x86_64-windows-pe`), `T4` 2-pass RISC-V RV64IM 32-bit instruction encoder verified in `qemu-riscv64` (`riscv64-linux-elf`), `T5` bare-metal Intel HEX (`.hex`) firmware writer with verified checksum (`<isa>-none-ihex`) + bootable raw kernel (`x86_64-none-raw`), and `LUMB\x01` portable bytecode container + VM (`lumb`, closing the `lumb` brick of `#215`), with `QF_LIA` `proven` guard elision and native target-ISA exit-3 runtime contract traps.)*

4. **`#197` — specified-but-unimplemented core features** (traits, closures, refinement types,
   async/await, rc/arc, shard, let-polymorphism; the AArch64/RISC-V/WASM backend items belong with
   `#206`). *(Done — `trait`/`impl` declarations & static method dispatch (`[T0400]`/`[T0401]`), first-class closures with lexical capture (`cl_convert_prog`) & `Send` checking (`[S0001]`), refinement types `type Name = Base where <pred>` and inline `x: T where <pred>` / `-> T where <pred>` wired into `QF_LIA` `proven` guard elision, `[V0001]` refutation, and exit-3 runtime guards, `async fn`/`.await`/`block_on` with `[E0200]` effect checking, `rc[T]`/`arc[T]` (`atomic_xadd64` hardware `lock xaddq`)/`weak[T]` backed by shared heap control blocks with destructor invalidation and `[O0001]` use-after-drop checking, `shard` data-parallel construct (`[S0001]`/`[E0200]`), and F3c Hindley–Milner `let`-polymorphism (`Forall`/`hgeneralize`/`hinstantiate`, `[T0500]`).)*

## Parallel tracks (no gating either way — pick up by priority, any time)

- **`#230` — LLM-generation eval as a CI gate.** *(Done — `scripts/check-llm-gen-eval.sh` + `tests/llm_gen_eval/`, wired into CI.)*
- **`#214` — data track** (persist table / versioned / `money[CCY]` / `@soa`). *(Done — real on-disk WAL via `open`+`flock`+`write`+`fsync`+`close`, cross-process crash recovery, concurrent multi-process locking, `money[CCY]`, `versioned`/`@soa`.)*
- **`#210` — `lume lsp` + `lume dap`.** *(Done — live multi-turn JSON-RPC-over-stdio `lume lsp` and `lume dap` servers.)*
- **`#211` — `lume repl` + notebook/Jupyter kernel.** *(Done — interactive `lume repl` read-eval-print loop & stateful multi-message Jupyter `lume repl --kernel` protocol server.)*
- **`#196` — adoption & distribution** (installer, registry, playground, certificates). Needed for
  real-world use; the playground also helps the AI-authoring loop.
- **`#215` — untracked planned items** (`signal[T]`/`promise[T]`, `lumb` bytecode, Linguist). *(Done — `promise_new`/`promise_resolve`/`promise_await` and `signal_new`/`signal_get`/`signal_set`/`signal_version` in `compiler/stage1/concurrency.src`, `LUMB\x01` portable bytecode container + VM (`lume build --target lumb`, `lume run <file.lumb>`) in `compiler/stage1/ltarget.src`, and `.gitattributes` + `contrib/linguist/` TextMate grammar & samples staged and validated by `.github/workflows/linguist-grammar.yml`.)*

## Horizon (last — research-grade, after the spine stands)

- **`#198` — horizon tracking** (compositional/temporal proof, anti-rot, portable kernel,
  learned⊕proven, metal targets). Depends on a solid proven core + a real kernel; it is the frontier,
  not near-term.

## Open product maturity program (`docs/34`)

1. **`#337` — P0 machine-native inflection.** Start the semantic intent protocol immediately, then
   universal verified adapters (#334), portable renewable evidence (#330/#335), the content-addressed
   component network, public economic benchmarks, and independent SaaS/Web + kernel/driver capstones.
   The bounded Lume-only real-execution benchmark protocol 0.1 is in `docs/35`; public model/token/cost
   measurements and fair alternative-language arms remain open. The bounded real HTTP/WAL and QEMU
   RTC capstone slices are in `docs/36`; TLS, browser DOM, full SaaS, PCI/block, and general OS
   acceptance remain open. This is the primary track, not post-product adoption work.
2. **`#330` — versioned evolve evidence.** Candidate-bound, derived gate/obligation results; already
   in flight and directly feeds the P0 evidence protocol.
3. **`#335` — material-change invalidation.** Extend the shipped #259 spec-change regeneration to
   dependencies, toolchain, target, environment, and dependency-graph revalidation. **[HARD]** Builds
   on the evidence identities and result model from `#330`.
4. **`#336` — machine-readable acceptance matrix.** Start with the reopened `#257` claims, then cover
   every product track. **[HARD for claim promotion]** Missing runnable evidence remains `UNKNOWN`.
   The initial slice is [`acceptance/matrix.json`](../acceptance/matrix.json), rendered in
   [`acceptance-matrix.md`](acceptance-matrix.md).
5. **`#257` + `#334` — real domains and universal adapters.** TLS/TCP, HTTP, async, storage, FFI,
   generated bindings, and real WASM hosting proceed in focused parallel PRs under the P0 protocol.
   The shipped binding subset is limited to `int64_t` scalars and borrowed `const char *` parameters,
   preserves foreign returns as `unverified[int]`, and rejects unsupported ownership/callback boundaries
   with `[I0003]`; broader adapter maturity and real WASM hosting remain open.
6. **`#332` — production SaaS capstone.** **[HARD]** Requires the applicable secure-networking,
   concurrent-runtime, data, and interop increments from `#257`/`#334`.
7. **`#333` — real Web applications.** Real browsers, DOM/events/fetch, application architecture, and
   a complete capstone. It integrates with `#332` when the backend capstone is available; basic
   browser/DOM increments need not wait for SaaS completion.
8. **`#259` — shipped intent baseline.** Preserve its spec-change regeneration and tamper rejection
   while #337 generalizes the protocol and #335 broadens freshness.
9. **`#241` — general-purpose OS.** Grow independently from the shipped microkernel into
   VFS/storage, userspace/syscalls, networking, userland, and drivers; the P0 kernel/driver capstone
   uses the applicable real increments.

Every partial PR uses `Refs` for these umbrella issues. Documentation and matrix rows do not close
implementation issues.

## Open integrity corrections

- **`#340` — full-content certificate and AST identities.** Remove summarized/truncated input from
  producer/verifier SHA-256, with NIST 56-byte vectors and independent long-source `hashlib` checks.
  This feeds the evidence and verified-component architecture tracked in `#337`.

## One-line dependency summary
`#337` is P0: semantic intent + universal adapters + renewable evidence + verified components +
economic benchmarks + killer capstones. `#330`/`#335`/`#336` supply trust; `#257`/`#334` supply real
domains and bridges; `#332`/`#333` and `#241` supply product proof; shipped #259 is the intent baseline.
