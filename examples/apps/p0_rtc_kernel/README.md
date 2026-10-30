# P0 freestanding CMOS RTC driver slice

This is a real x86-64 bare-metal acceptance increment for #337/#241/#297. It is not the hosted
arithmetic model shipped by the semantic-mesh increment. The Lume source uses `@freestanding` and
reads MC146818 CMOS RTC registers through ports `0x70`/`0x71`; serial output comes from the existing
kernel runtime. Acceptance covers the register format selected by QEMU in the two runs below.

## Build and run

From the repository root on Linux x86-64 with `qemu-system-x86_64`:

```sh
./lume compile examples/apps/p0_rtc_kernel/kernel_rtc.src >/tmp/kernel_rtc.s
build/bin/lasm </tmp/kernel_rtc.s >/tmp/kernel_rtc.img
qemu-system-x86_64 \
  -drive format=raw,file=/tmp/kernel_rtc.img \
  -serial stdio -display none -no-reboot \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -rtc base=2024-06-15T12:34:56
```

The serial line contains the year, month, day, hour, minute, second and status register observed by
the guest. The automated check builds an MBR boot image, rejects any hosted `syscall`, runs QEMU
with two different RTC bases, and verifies that each reported timestamp matches the corresponding
emulated hardware clock:

```sh
python3 tests/capstones/test_rtc_qemu.py /tmp/kernel_rtc.img /tmp/kernel_rtc.s
```

## Explicitly open

PCI enumeration, MMIO/block I/O, interrupt handling in this minimal driver, physical-hardware runs,
WCET measurement, a zero-allocation runtime, and the remaining general-purpose OS scope in #241 and
machine-checked invariant scope in #297. The existing larger `lume_os` example covers other kernel
primitives; this increment claims only the freestanding boot and RTC port-I/O behavior tested here.
