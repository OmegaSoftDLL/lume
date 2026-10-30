# 36 — P0 real capstone acceptance slices

## Purpose and claim boundary

This increment adds executable product evidence for #337 without reusing or promoting the
arithmetic-only capstone models in `compiler/stage1/lmesh.src`. Those models remain insufficient for
SaaS, browser, kernel, or driver acceptance. The two slices below are independently runnable from
`tests/capstones/run.sh`; neither closes an umbrella issue.

## SaaS/HTTP slice: shipped only as a bounded demo

`examples/apps/p0_tenant_ledger/ledger_server.src` compiles through the real Lume toolchain to a
Linux x86-64 ELF and uses the existing native socket and durable-table primitives. It provides a
single-worker HTTP/1.1 JSON service with health, tenant-scoped balance reads, credits and debits.
Two static demonstration bearer identities are bound to tenant IDs. Cross-tenant reads and writes
are denied before storage access. Each successful mutation appends and fsyncs a real WAL snapshot;
startup recovers the latest valid committed state.

`tests/capstones/test_saas_http.py` is an independent Python client, not a Lume self-round-trip. It
exercises actual HTTP connections and raw malformed requests, verifies two tenants' data across
adversarial requests and process restarts, and checks that a torn trailing WAL frame does not change
recovered balances.

This is **not production SaaS acceptance**. Plain HTTP is the only implemented transport. There is
no TLS interoperability evidence, browser/WASM DOM artifact, complete TCP stack, timeout/backpressure
policy, multi-worker runtime, graceful shutdown, load evidence, production identity/session/secret
system, rate limiting, conditional concurrent balance update, migration, backup/restore,
observability, or deployment acceptance. Those criteria remain open in #332/#333.

## Kernel/driver slice: shipped only for x86-64 QEMU RTC

`examples/apps/p0_rtc_kernel/kernel_rtc.src` is a freestanding Lume kernel with a CMOS RTC driver.
The build emits a raw MBR/Long Mode image, not a hosted ELF. The guest waits for the RTC update
cycle, reads hardware registers through ports `0x70`/`0x71`, handles BCD/binary and 12/24-hour
formats, and reports the observed values through the existing COM1 runtime.

`tests/capstones/test_rtc_qemu.py` rejects hosted syscalls and invalid boot images, then runs
`qemu-system-x86_64` twice with different `-rtc base=` values. It requires the guest timestamps to
match the independent emulated clock settings; a fixed arithmetic result cannot pass.

This is **not complete OS or driver-ecosystem acceptance**. PCI, MMIO/block storage, interrupts in
this minimal driver, physical hardware, measured WCET, zero allocation, and the remaining #241/#297
scope are open. Existing `lume_os` evidence is separate and is not silently treated as evidence for
those missing capabilities here.

## Reproduction

```sh
tests/capstones/run.sh
```

The command requires Linux x86-64, Python 3, the repository Lume toolchain, and
`qemu-system-x86_64`. Missing QEMU is a failed check, not a skipped acceptance claim. The HTTP test
uses only Python's standard library; the kernel evidence comes from QEMU serial output and two
independently controlled hardware-clock configurations.
