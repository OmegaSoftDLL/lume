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

- **`#230` — LLM-generation eval as a CI gate.** *(Done — `scripts/check-llm-gen-eval.sh` + `tests/llm_gen_eval/`, wired into CI.)*
- **`#214` — data track** (persist table / versioned / `money[CCY]` / `@soa`). *(Done — real on-disk WAL via `open`+`flock`+`write`+`fsync`+`close`, cross-process crash recovery, concurrent multi-process locking, `money[CCY]`, `versioned`/`@soa`.)*
- **`#210` — `lume lsp` + `lume dap`.** *(Done — live multi-turn JSON-RPC-over-stdio `lume lsp` and `lume dap` servers.)*
- **`#211` — `lume repl` + notebook/Jupyter kernel.** *(Done — interactive `lume repl` read-eval-print loop & stateful multi-message Jupyter `lume repl --kernel` protocol server.)*
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
