# 27 — Systems & kernels: everything Lume needs to build real bare-metal software

**Goal.** Make Lume able to build the full stack down to the metal — firmware, bootloaders, a real
bootable **microkernel**, and the systems that run on it — **for real, with no hacks, carrying
Lume's proofs to the hardware**, and surpass what C/Rust-on-LLVM offer. This document is the plan;
the worklist is the companion tracking issue. It composes with `docs/26` (universal targets) and the
`docs/23` H5 horizon.

## Two very different bars — be honest about which

- **(A) A minimal, real, bootable microkernel** — boots on QEMU/hardware, sets up interrupts,
  manages physical memory, schedules tasks, talks to a device or two, **with Lume's contracts/WCET/
  ownership proven**. Ambitious but **tractable** (years, not decades). This is what this plan targets
  and where Lume is unique.
- **(B) "A complete OS like Linux"** — thousands of drivers, filesystems, a network stack, SMP,
  POSIX, ~30M lines, 30 years. That is **not a language gap** — it is a decades + ecosystem + large-
  team effort. No language hands you "a Linux." Lume's job is to be the language you **write and
  prove** such a system in; the OS itself is a separate, enormous project built on top of (A).

The no-theater rule applies throughout: a systems brick is **not "done" until it runs on real
hardware or a faithful emulator (QEMU)** — not when it emits plausible bytes or a model.

## Today (shipped & hardware-verified on `qemu-system-x86_64 -smp 2` — Bricks K0–K9 & `#193` SMP)
Lume compiles both **Linux user-space x86-64 ELFs** and **bare-metal `@freestanding` x86-64 SMP bootable kernel images** (`lume_os.img`, `examples/apps/lume_os/kernel_qemu.src`):
- **K0 (`@freestanding` codegen) & K1 (syscall-free runtime):** When a compilation unit declares `@freestanding`, `06b_phase3.src` preserves the `OSig` (`freef == 1`) and `05_codegen.src` emits `.kernel`, initializes a physical frame arena at `0x200000..0x3F0000`, emits `runtime_krn_tail` (zero `syscall` instructions in the entire `.s` output), writes serial output to COM1 (`0x3F8`), and exits cleanly via `isa-debug-exit` (`0xF4`) + `cli; hlt`.
- **K2 (Privileged/system ISA in `lasm.src`):** `lasm.src` encodes `cli`, `sti`, `hlt`, `iretq`, `rdmsr`, `wrmsr`, `pause`, `mfence`, `outb`, `inb`, `int $imm8`, `lgdt`, `lidt`, `invlpg`, `movq %crN, %reg` / `movq %reg, %crN` (`%cr0`, `%cr2`, `%cr3`, `%cr4`, `%cr8`), `lock xaddq`, `lock cmpxchgq`, and `xchgq`.
- **K3 (Volatile MMIO):** `mmio_volatile_write64`, `mmio_volatile_read64`, `mmio_volatile_write32`, and `mmio_volatile_read32` emit direct hardware memory loads/stores + `mfence` barriers in `05_codegen.src`.
- **K4 (Real interrupts & `@interrupt` ISR):** `cpu_idt_init` populates 64-bit Interrupt Gates 14 (`__isr_pf_14` at `0x50E0`) and 32 (`__isr_hw_32` at `0x5200`) and executes `lidt idt_ptr(%rip)`; `__isr_hw_32` saves registers, invokes the WCET-checked `@interrupt(irq: 32) fn kernel_timer_isr` Lume handler, increments the hardware tick counter, sends 8259A PIC EOI (`outb $0x20, $0x20`), and returns via `iretq`.
- **K5 (Bootable x86-64 disk image in `lasm.src`) & K6 (MMU-enforced 4-level x86-64 page tables + `cap[Page]` + `#PF` vector 14):** `lasm.src` (`asmkernel`) emits a raw bootable x86-64 disk image with a 512-byte MBR bootloader (`0x7C00`) that reads 64 sectors via BIOS `INT 0x13 AH=0x42` into `0x8000`, zeroes and populates 4-level x86-64 page tables (`PML4` at `0x1000`, `PDPT` at `0x2000`, `PD` at `0x3000`, and a live 4-KB `PT` at `0x4000` attached at `PD[2]` covering virtual `0x400000..0x5FFFFF`), and transitions to 64-bit Long Mode. `cap_page_map` writes live PTEs into `0x4000` + `invlpg`, verifies physical/virtual aliasing (`0x400000` -> `0x80000`), enforces the no-double-map invariant on live hardware PTEs (`cur_pte % 2 == 1` rejected), and handles real hardware `#PF` page faults on IDT vector 14 (`__isr_pf_14` reads faulting VA from `%cr2`, demand-maps `PT[1]` at `0x401000`, flushes TLB via `invlpg`, pops the 8-byte hardware error code, and `iretq`s).
- **K7 & `#193` (Bare-metal SMP multi-core bring-up, hardware atomics/spinlocks & OS multi-process parallelism):** On bare metal (`cpu_smp_boot_ap`), the BSP maps Local APIC MMIO at `0xFEE00000`, copies the 16-bit-to-64-bit AP trampoline (`ap_tramp`) to physical `0x7000`, and sends INIT (`0x000C4500`) + SIPI (`0x000C4607`) via Local APIC ICR Low (`0xFEE00300`), waking CPU 1 (AP) on `qemu-system-x86_64 -smp 2` to enter 64-bit Long Mode, acquire a hardware spinlock (`xchgq`), execute task work on CPU 1, and signal completion via `lock xaddq`. In Linux user space (`compiler/stage1/concurrency.src`), `par_fork_scale2` and `par_map_scale_os` run concurrent OS worker processes via `sys_fork` + `sys_pipe` + `sys_wait4`.
- **K8 (Hardware timer IRQ & UART drivers) & K9 (`lume-os` context switching & ring-buffer IPC on QEMU):** Programs the 8259A Master/Slave PICs (`0x20`/`0x21`, `0xA0`/`0xA1`) and 8254 PIT Channel 0 (`0x43`/`0x40`) via `port_out8` so real hardware timer IRQ 0 interrupts wake the CPU from `sti; hlt` (`cpu_wait_timer_irq`, zero software `int $32`), and performs real hardware `%rsp`/`%rip` context switches (`__cpu_ctx_switch`) between Task A and Task B (`0x7A000`) exchanging messages over a shared-memory ring buffer at `0x5900` under hardware spinlocks.

## Sequencing & entry point (decided 2026-10-28)
This track is **in the roadmap, not parked** — it enters in the sequence below. Its only hard
prerequisite is the **existing x86-64 native backend, which already ships**, so there is no deep
technical gate; "sequence" here is priority order.

1. **In progress now:** `#191` (real FFI linking — C-ABI + WASM export). A from-scratch kernel does
   **not** use C FFI, so `#191` does **not** block this track — it merely finishes first because it is
   the current work.
2. **Then `#227` enters at K0 on x86-64.** K0→K9 proceed on the x86-64 backend that already ships;
   each brick is a QEMU-running PR (no model-as-hardware), with `stage2 == stage3`, `zero-c` and the
   suite staying green.
3. **`#206` (other ISAs) interleaves later, it is not an upfront prerequisite:** per "Per-ISA" above,
   the x86-64 kernel is built first; AArch64/RISC-V ports of K2/K3/K4 are the kernel's **second-target**
   phase, needed only after x86-64 K9. They can run in parallel but do not gate K0.
4. **`#193` is closed *by* K7, not before it:** the real parallel runtime (atomics, spinlocks, SMP
   bring-up, real scheduler) **is** the K7 deliverable, so there is no separate "do `#193` first" step.
5. **Independent of this track** (run in parallel, no gating either way): `#210` LSP/DAP, `#211`
   REPL/Jupyter, `#214` data, `#230` LLM-generation eval, `#196` adoption/distribution.

Net: `#227` enters **right after `#191`**, at K0 on x86-64, and climbs K0→K9; multi-ISA (`#206`) and
real parallelism (`#193`) fold in at K-brick boundaries rather than as upfront prerequisites.

## The bricks (K0–K9) — each a real PR, running on QEMU

- **K0 — Freestanding/no-OS codegen.** A real `@freestanding` lowering: no libc, no syscalls, a
  custom entry point, code that runs with no OS underneath. Promotes O4's *check* into actual
  bare-metal output.
- **K1 — Syscall-free runtime.** A Lume runtime that owns memory itself (a physical frame allocator,
  not `brk`) and does **no** Linux syscalls; I/O only via the hardware primitives (K3). The current
  `brk`/syscall runtime is reimplemented for bare metal.
- **K2 — Privileged/system instruction emission.** `lasm` gains the system ISA: `cli/sti`, `hlt`,
  `lgdt/lidt`, `cr0–cr4`/`cr8`, `rdmsr/wrmsr`, `in/out`, `iret`, `invlpg`, etc., plus an `asm { … }`
  intrinsic for what the compiler cannot express — per target ISA.
- **K3 — Volatile MMIO + typed hardware registers.** Real memory-mapped I/O with volatile semantics
  (no reordering/elision), replacing the `std.os.cap`/`@mmio` model with actual loads/stores.
- **K4 — Real interrupts.** IDT setup, ISR entry/exit with full register save/restore, and the
  `@interrupt` attribute **lowered to a real ISR** (not just the effect check) — with the WCET bound
  (`@bound`) proven for the handler.
- **K5 — Bootable image.** A kernel-image mode in `lasm` (physical/virtual load addresses, a layout/
  linker-script equivalent) and a boot entry (multiboot2 and/or UEFI) producing an image that a boot
  loader hands control to — not a user-space ELF.
- **K6 — Memory management.** x86-64 4-level page tables, a physical frame allocator, virtual-memory
  mapping, and `cap[Page]` made **real** (linear, with the no-double-map invariant **proven**).
- **K7 — Real SMP & concurrency.** Atomics, spinlocks, per-core state, secondary-core bring-up, and a
  real scheduler running on real cores — making `spawn`/`scope`/channels actually parallel (closes
  the sequential-simulation gap, ties `#193`), with data-race-freedom still statically enforced.
- **K8 — Drivers.** A driver model + the first real drivers: serial/UART (kernel output), the timer/
  local APIC, and the interrupt controller — volatile-safe, capability-scoped.
- **K9 — Capstone: `lume-os` that actually boots.** Turn the `lume-os` model into a kernel that
  **boots on QEMU**: serial "hello", IDT + a timer ISR (WCET-proven), physical-memory management
  (allocator invariant proven), and a scheduler running 2+ tasks (no data race proven). This is the
  living proof that Lume writes a real, proven kernel.

**Per-ISA:** K2/K3/K4 are written once per instruction set; x86-64 first, then AArch64/RISC-V (from
`docs/26`) for embedded/firmware. **Cross-cutting:** every brick is Lume (`.src`), not new C in the
frozen seed (`#205`); keeps `stage2 == stage3`, `zero-c`, and the suite green; and carries the
ownership/effect/contract/proof guarantees to the metal (H5).

## Beyond the kernel (the long tail toward a full OS)
Once (A) stands, a general-purpose OS adds, each a large track of its own: a filesystem (and a VFS),
a block/storage stack, a network stack, a process/userspace model + syscall interface, a shell and
userland, and a device-driver ecosystem. These are **not** near-term; they are the decades-scale
build that (A) enables. We track them only as direction until the kernel is real.

## The differentiator
C/Rust-on-LLVM can target bare metal, but the correctness proof **stops at the source**. Lume's edge
is a **proof-carrying kernel**: the frame allocator's invariant, the ISR's worst-case cycle bound,
and the scheduler's data-race-freedom are **proven**, in-language, down to the emitted machine code.
seL4 proved a kernel but in a separate Isabelle development over years; Lume carries the proof in the
same language that emits the code. *The first OS you can write and prove in one language* — the pitch
no one else can make, and the one that matters most where a bug is catastrophic (firmware, medical,
avionics, a bank's core).

## Acceptance (the whole track)
Each Kn ships as a green PR whose test **runs on QEMU** (or real hardware) and demonstrates the real
behaviour with its guarantees intact; `stage2 == stage3`, `zero-c`, and the full suite stay green; no
brick is a model presented as hardware. K9 boots. Then (A) is real, and Lume has earned the right to
climb toward (B).
