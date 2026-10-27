# 26 — Universal targets: every ISA and platform, proof-preserving, no hacks

**Mandate.** Lume must build **operating systems, firmware, and applications for every processor and
platform that matters** — with no limitations and no hacks — using its own self-hosted backend
(written in Lume, verified in Lume), and surpass LLVM-based competitors. This document is the
architecture and sequence; the worklist lives in issue **#206** (which organizes the target parts of
`#197` and `#198`).

Today the backend emits **x86-64 + ELF64 only** (`compiler/stage1/lasm.src`,
`compiler/stage1/lume/05_codegen.src`). macOS, Apple Silicon, Windows, RISC-V, and bare-metal/firmware
are **not** targets yet. This is the plan to get there without turning the backend into a pile of
special cases.

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
| `x86_64-linux-elf` | x86-64 | ELF64 | System V AMD64 | **shipped** |
| `aarch64-macos-macho` | AArch64 | Mach-O | Darwin / AAPCS64 (+ signing) | planned (T1+T2) — **Apple Silicon** |
| `x86_64-macos-macho` | x86-64 | Mach-O | Darwin | planned (T2) |
| `x86_64-windows-pe` | x86-64 | PE/COFF | Windows x64 | planned (T3) |
| `aarch64-windows-pe` | AArch64 | PE/COFF | Windows ARM64 | planned (after T3) |
| `riscv64-linux-elf` | RISC-V RV64 | ELF64 | System V | planned (T4) |
| `<isa>-none-elf` / flat / ihex | any | raw/flat + Intel HEX | bare-metal `@freestanding` | planned (T5) — firmware & OS |
| `wasm32-wasi` | WASM | WASM | WASI | via `#191` — run-anywhere fallback |

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
