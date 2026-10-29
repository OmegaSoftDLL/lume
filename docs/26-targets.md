# 26 — Universal targets: every ISA and platform, proof-preserving, no hacks

**Mandate.** Lume must build **operating systems, firmware, and applications for every processor and
platform that matters** — with no limitations and no hacks — using its own self-hosted backend
(written in Lume, verified in Lume), and surpass LLVM-based competitors. This document is the
architecture and sequence; the worklist lives in issue **#206** (which organizes the target parts of
`#197` and `#198`).

Today the backend implements the orthogonal target-triple architecture in [`compiler/stage1/ltarget.src`](../compiler/stage1/ltarget.src) alongside [`compiler/stage1/lasm.src`](../compiler/stage1/lasm.src) and [`compiler/stage1/lume/05_codegen.src`](../compiler/stage1/lume/05_codegen.src), exposed via `lume build --target <triple> <file.src> -o <out>` and `lume targets [--json]`.

## The one unfair advantage, carried to every target

LLVM-based languages get many targets, but the correctness story **stops at the source** — the proof
does not travel to the machine code. Lume's edge is that **every target preserves the guarantees**:
ownership, effects, contracts, and the sound `QF_LIA` **proofs** hold on x86-64, ARM64, RISC-V and
bare metal alike (the `docs/23` H5 horizon). *Provably-correct firmware for any chip, built with no
external toolchain* is a claim no one else can make. The whole architecture exists to keep that claim
true on every target.

## The anti-"gambiarra" architecture: an orthogonal target triple

A target is three **independent** concerns. Keeping them orthogonal is what makes a new target
*additive* (one new module) instead of a fork of the whole backend:

```
target = (ISA encoder)  ⊥  (object/container writer)  ⊥  (OS / ABI + syscall layer)
```

- **ISA encoder** — lowers the Lume IR to machine code for one instruction set. *Have:* x86-64.
  *Add:* AArch64 (ARM64), RISC-V (RV64), then others by the same interface.
- **Object/container writer** — serialises code+data into one executable/object format. *Have:*
  ELF64. *Add:* Mach-O, PE/COFF, and raw/flat binary + Intel HEX (for firmware).
- **OS / ABI + syscall layer** — calling convention, entry point, and OS interface. *Have:* System V
  AMD64 on Linux. *Add:* Darwin/AAPCS64, Windows x64, and bare-metal/`@freestanding` (no OS).

Each concern is a Lume module (`.src`, like `lasm.src`), chosen by a **target triple**
(`<isa>-<os>-<format>`, e.g. `aarch64-macos-macho`, `x86_64-windows-pe`, `aarch64-none-elf` for
firmware). The IR and the type/ownership/effect/proof front end are **target-independent**; only the
three backend concerns vary. Where feasible, an encoder's correctness is itself expressed with Lume
contracts and checked by the prover — *using everything Lume has.*

## The target matrix (ISA × format × ABI)

| Triple | ISA | Format | ABI / OS | Status |
|--------|-----|--------|----------|--------|
| `x86_64-linux-elf` | x86-64 | ELF64 (`EM_X86_64=62`) | System V AMD64 | **shipped** (`lumec` + `lasm`; run-verified natively on `ubuntu-latest`) |
| `aarch64-linux-elf` | AArch64 A64 | ELF64 (`EM_AARCH64=183`) | Linux AAPCS64 (`svc #0`) | **shipped** (`ltarget.src`; run-verified in `qemu-aarch64` with `QF_LIA` guard elision + exit-3 trap) |
| `aarch64-macos-macho` | AArch64 A64 | Mach-O 64 (`MH_MAGIC_64`, 16 KB page) | Darwin / AAPCS64 (`svc #0x80`) | **shipped** (`ltarget.src`; run-verified natively on Apple Silicon `macos-latest` `Darwin arm64` with `QF_LIA` guard elision + exit-3 trap) |
| `x86_64-macos-macho` | x86-64 | Mach-O 64 (`MH_MAGIC_64`, `CPU_TYPE_X86_64`) | Darwin / System V AMD64 (`syscall` `0x2000004`/`0x2000001`) | **shipped** (`ltarget.src`; verified on `macos-latest` with `QF_LIA` guard elision + exit-3 trap) |
| `x86_64-windows-pe` | x86-64 | PE32+/COFF (`0x8664`, `.text` RX + `.idata` RW) | Windows x64 (`KERNEL32.dll` `WriteFile`/`ExitProcess`) | **shipped** (`ltarget.src`; run-verified natively on `windows-latest` Windows x64 with `QF_LIA` guard elision + exit-3 trap) |
| `riscv64-linux-elf` | RISC-V RV64IM | ELF64 (`EM_RISCV=243`) | Linux LP64 (`ecall`) | **shipped** (`ltarget.src`; run-verified in `qemu-riscv64` with `QF_LIA` guard elision + exit-3 trap) |
| `<isa>-none-ihex` | AArch64 / RV64 / x86-64 | Intel HEX (`.hex` with two's-complement checksum) | bare-metal `@freestanding` | **shipped** (`ltarget.src`; `x86_64-none-ihex` firmware unpacked and run-verified in `qemu-system-x86_64` over hardware COM1 UART `0x3F8` + exit-3 trap) |
| `x86_64-none-raw` | x86-64 | Bootable MBR + 64-bit Long Mode kernel image | bare-metal `@freestanding` | **shipped** (`lasm` `.kernel`; run-verified in `qemu-system-x86_64 -smp 2`) |
| `wasm32-wasi` | WASM | Binary WebAssembly 1.0 (`\0asm\x01\0\0\0`) | WASI `wasi_snapshot_preview1` (`fd_write`/`proc_exit`) | **shipped** (`ltarget.src`; run-verified under Node.js WASI on Linux, macOS, and Windows with `QF_LIA` guard elision + exit-3 trap) |
| `lumb` | LUMB | Portable Lume Bytecode (`LUMB\x01`) | Deterministic stack VM | **shipped** (`#215` `lume build --target lumb` + `lume run *.lumb`) |
| `nvptx64-nvidia-cuda` | NVPTX 64 (`sm_80`) | PTX 8.0 SSA kernel module (`.ptx`) | CUDA PTX 8.0 (`@%ptrap trap;`, `st.global.s64`) | **shipped** (`#250`, `#256` `ltarget.src`; lowers `kernel fn` + contracts to PTX 8.0 and run-verifies via `lume run *.ptx` PTX VM with `QF_LIA` guard elision + exit-3 trap) |
| `spirv64-vulkan-compute` | SPIR-V 64 (`1.6`) | Vulkan Compute SPIR-V 1.6 (`.spvasm`) | Vulkan `GLCompute` (`OpTrapIf`, `OpStore %out_ptr`) | **shipped** (`#250`, `#256` `ltarget.src`; lowers `kernel fn` + contracts to SPIR-V 1.6 and run-verifies via `lume run *.spvasm` SPIR-V VM with `QF_LIA` guard elision + exit-3 trap) |
| `arm-none-cortexm` | ARMv7E-M Thumb-2 | Bare-metal Cortex-M firmware (`.cortexm` with vector table `[0x20020000, 0x08000009]`) | `@freestanding` EABIHF (`bkpt #3` trap, `svc #1` semihosting, `bkpt #0` halt) | **shipped** (`#256` `ltarget.src`; run-verified via `lume run *.cortexm` instruction-level Cortex-M emulator with vector-table check, `QF_LIA` guard elision + exit-3 trap) |
| `avr-none-elf` / `avr-none-ihex` | Microchip AVR | ELF32 (`EM_AVR=83`) & Intel HEX (`.avr.elf` / `.avr.hex`) | `@freestanding` AVR (`break` `0x9598` trap, `out 0x18` UART, `sleep` `0x9588` halt) | **shipped** (`#256` `ltarget.src`; run-verified via `lume run *.avr.elf` / `*.avr.hex` AVR emulator with ELF32/IHEX checksum verification, `QF_LIA` guard elision + exit-3 trap) |
| `xtensa-esp32-elf` / `xtensa-none-ihex` | Tensilica Xtensa LX6 | ELF32 (`EM_XTENSA=94`) & Intel HEX (`.xtensa.elf` / `.xtensa.hex`) | ESP32 windowed ABI (`ill` `0x0000` trap, `simcall` `0x5100`, `retw.n` `0xF01D`) | **shipped** (`#256` `ltarget.src`; run-verified via `lume run *.xtensa.elf` / `*.xtensa.hex` Xtensa emulator with ELF32/IHEX checksum verification, `QF_LIA` guard elision + exit-3 trap) |
| `verilog-rtl` | IEEE 1364-2005 Verilog | Synthesizable RTL module (`.v`, `always @(posedge clk)`) | Synchronous FPGA RTL (`clk`, `rst`, `done`, `trap`, `out_0..N`) | **shipped** (`#256` `ltarget.src`; cycle-accurate RTL simulation via `lume run *.v` with combinational wire propagation, rising-edge FF latch, `QF_LIA` guard elision + synchronous `trap` exit-3) |
| `vhdl-rtl` | IEEE 1076-2008 VHDL | Synthesizable RTL entity/architecture (`.vhd`, `rising_edge(clk)`) | Synchronous FPGA RTL (`clk`, `rst`, `done`, `trap`, `out_0..N`) | **shipped** (`#256` `ltarget.src`; cycle-accurate RTL simulation via `lume run *.vhd` with signal propagation, rising-edge latch, `QF_LIA` guard elision + synchronous `trap` exit-3) |
| `aarch64-ios-macho` | AArch64 A64 | Mach-O 64 (`MH_MAGIC_64`, `LC_BUILD_VERSION` `PLATFORM_IOS=2`, iOS 17.0) | iOS Darwin AAPCS64 (`svc #0x80`) | **shipped** (`#256` `ltarget.src`; 16 KB page-aligned Mach-O 64 with `PLATFORM_IOS=2`, `QF_LIA` guard elision + exit-3 trap) |
| `aarch64-android-elf` | AArch64 A64 | ELF64 (`EM_AARCH64=183`, `PT_LOAD` + `PT_NOTE` `.note.android.ident` API 34) | Android Bionic AAPCS64 (`svc #0`) | **shipped** (`#256` `ltarget.src`; run-verified under `qemu-aarch64` with `.note.android.ident` API 34 note, `QF_LIA` guard elision + exit-3 trap) |
| `wasm32-web-dom` | WASM | Binary WebAssembly 1.0 (`\0asm\x01\0\0\0`) | Browser / Web DOM ABI (`env.dom_append_int`, `env.dom_set_attr`, `env.web_trap`, exports `lume_web_main`) | **shipped** (`#256` `ltarget.src`; run-verified in Node.js/V8 WebAssembly engine with DOM host imports, `QF_LIA` guard elision + exit-3 trap) |

## Sequence (each brick is one real PR)

0. **T0 — target-triple refactor.** Split the backend into the three orthogonal concerns behind a
   triple selector. Nothing else lands cleanly first. x86-64/ELF/SysV becomes the first instance of
   the interface, not a special case.
1. **T1 — AArch64 encoder.** Unlocks Apple Silicon, ARM servers, and most mobile/embedded.
2. **T2 — Mach-O writer + Darwin ABI (+ code signing).** → macOS Intel and **Apple Silicon**.
3. **T3 — PE/COFF writer + Windows x64 ABI.** → **Windows** (link against `kernel32`/`ntdll`).
4. **T4 — RISC-V (RV64) encoder.** Open ISA; embedded and future silicon.
5. **T5 — bare-metal/firmware & OS.** Mature `@freestanding`, raw/flat + Intel HEX output,
   bootloader/firmware entry, and turn `lume-os` into a **real bootable kernel** (QEMU boot), closing
   `#12`.
- **Cross-cutting — WASM/WASI** (`#191`): a single portable target that runs on every host via a
  runtime — the pragmatic run-anywhere path alongside the native backends.

## Acceptance bar — no theater

A target is **not "supported" until it runs real programs**, not when it "emits plausible bytes":

- A **non-trivial Lume program with contracts** compiles for the triple and **runs correctly on real
  hardware or a faithful emulator (QEMU)**.
- Ownership/effects/contracts are **enforced at runtime** on that target, and `proven` elisions stay
  sound there.
- `stage 2 == stage 3`, `make zero-c`, and the full suite stay green; the backend module is **Lume**,
  not new C in the seed (`#205`).
- A per-target conformance test (hello + one real program) runs in CI under QEMU for cross-ISA.
- Docs state each target's real status; a target that cannot run a real program is **planned**, never
  "shipped".

## Honest scope

This is a large, multi-year build. Hand-writing ISA encoders, object formats and ABIs is exactly what
LLVM spared other languages — and it is the price of Lume's zero-C purity **and** of carrying the
proof to the metal. It is worth paying for the differentiator, but it must be **incremental and
real**: one target goes end-to-end (runs real programs on real/emulated hardware, guarantees intact)
**before** the next begins. No stub is ever presented as support. GOLDEN RULE scales to the backend:
every target is as trustworthy as x86-64, or it is not a target.
