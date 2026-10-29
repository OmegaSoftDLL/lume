# 34 — Product maturity program: SaaS, Web, polyglot libraries, and operating systems

**Goal.** Make Lume an AI-first language for building complete products on every substrate: verified
native cores, real applications, and explicit interoperability with the world's mature libraries.
The horizon is **Lume for everything**, delivered one running, evidence-backed product increment at
a time.

This document registers and orders work; it does not mark any open capability as shipped. Umbrella
issues stay open until their complete capstones and acceptance criteria pass. Partial PRs use `Refs`,
never `Closes`, for those issues.

## The three-part identity

1. **Independent core.** Lume generates Lume, preserves `stage2 == stage3`, and keeps the Zero-C
   steady-state gate. The compiler and pure standard library do not acquire foreign runtime
   dependencies.
2. **Universal reach.** The same intent, contracts, proofs, and effects travel to native, bare-metal,
   browser, accelerator, and portable targets. A target claim requires execution on its real target or
   a faithful emulator, not plausible output bytes.
3. **Chameleon interoperability.** Applications can opt into C-ABI, WASM, JavaScript, or process
   boundaries to reuse mature ecosystems. Foreign dependencies are explicit, content-identified,
   capability-scoped, and excluded from any claim that the resulting application is pure Lume.

Interop is a force multiplier, not a retreat from native Lume. A component may have a native and a
foreign-backed implementation when both satisfy the same executable contract. Neither path may
silently weaken memory ownership, provenance, capability, or evidence requirements.

## Current product boundary

- The self-hosted toolchain, C-ABI import/export, minimal x86-64 bootable kernel, and multiple target
  increments are real. They are foundations, not automatic product maturity.
- Track L acceptance remains open in **#257**. TLS, complete TCP behavior, multi-worker async,
  native windowing, durable ACID storage, binary ML formats, and the remaining domain scope need real
  acceptance evidence.
- `wasm32-web-dom` currently proves a minimal DOM host ABI. Full browser applications require the
  larger browser and application platform in **#333**.
- `std.wasm` is a reference model, not real module isolation. Production interop and real WASM hosting
  are tracked in **#334**.
- `lume agent bind` has a fail-closed scalar/borrowed-string C adapter subset with explicit foreign
  provenance. General foreign-pointer ownership, callback lifetimes, and arbitrary real-world headers
  remain open in **#334**; rejected declarations do not produce a partial generated adapter.
- The bootable microkernel is not a general-purpose OS. Filesystems, userspace, drivers, networking,
  and userland remain in **#241**.

## Track registry

| Track | Product outcome | Umbrella issue |
|---|---|---|
| **E1** | Versioned, candidate-bound evidence and distinct proof/runtime/unknown results | **#330** |
| **E2** | Material-change invalidation and safe partial revalidation for intent/proof caches | **#335** |
| **A** | Machine-readable claims linked to source, tests, evidence, issues, and PRs | **#336** |
| **L** | Real standard-library domains and independent acceptance | **#257** |
| **P** | Production FFI, real WASM hosting, and mature-library capstones | **#334** |
| **S** | Secure, observable, deployable multi-tenant SaaS | **#332** |
| **W** | Complete real-browser applications and Web APIs | **#333** |
| **I** | Intent-as-source §1 baseline with spec-change regeneration and tamper rejection | **#259** |
| **N (P0)** | Semantic intent protocol, universal adapters, renewable evidence, verified component network, economic benchmark, and killer capstones | **#337** |
| **O** | General-purpose OS beyond the bootable microkernel | **#241** |

Existing compiler, proof, target, and microkernel tracks remain in `docs/33`, `docs/26`, and
`docs/27`. This program adds product-level acceptance rather than reclassifying their increments.

## Execution sequence

### Phase 0 — Machine-native inflection (P0, #337)

#337 is the primary cross-cutting track, not deferred adoption work. The next new implementation track
starts with the semantic intent protocol; the remaining P0 increments build on that shared foundation
in parallel with the product foundations below.

1. **Semantic intent protocol and transactions:** extend the shipped #259 baseline with a
   vendor-neutral negotiation of goals, contracts, effects, resources, targets, and acceptance, plus
   validated semantic edits, repair, impact, provenance, rollback, and replay.
2. **Universal verified adapters (#334):** generate library/device/API bindings with ownership,
   capabilities, provenance, and executable contract tests.
3. **Portable renewable evidence (#330, #335):** independently checkable results invalidated by
   intent, compiler, dependency, target, environment, or policy changes.
4. **Verified component network:** content-addressed intents, bindings, tests, proofs, evidence, and
   artifacts discoverable and reusable across models and vendors.
5. **Public economic benchmark (#230, #337):** real target execution measuring zero-shot success,
   tokens, time, attempts, defects, and maintenance cost, never recorded success alone. Protocol 0.1
   in `docs/35-economic-benchmark.md` is a bounded Lume-only real-execution harness; provider,
   zero-shot, token, and cost results remain `UNKNOWN` until independently measured.
6. **Reproducible killer capstones (#332, #333, #241):** one intent-to-SaaS/Web path and one
   intent-to-kernel/driver path through the same semantic and evidence fabric.

P0 implementation PRs reference #337 and the applicable component issue. The protocol starts now;
adapters, evidence, registry, and benchmarks proceed as their real dependencies allow; the capstones
close only after complete product acceptance.

### Phase 1 — Trust and traceability

1. **E1 (#330):** emit versioned evidence from real `lume evolve` evaluations, with derived
   `PASS`/`FAIL`/`UNKNOWN` results and candidate identities.
2. **E2 (#335):** invalidate intent/proof caches on material changes and propagate impact through
   dependencies. Missing freshness or graph evidence fails closed to `UNKNOWN`.
3. **A (#336):** introduce the acceptance matrix, initially covering #257. CI must reject `PASS` rows
   without runnable evidence and valid tracking links, then expand the matrix to every track here.
   The initial Track L slice lives in [`acceptance/matrix.json`](../acceptance/matrix.json), with a
   [generated human-readable view](acceptance-matrix.md).

These tracks make later product claims inspectable by agents and humans. They do not replace the
implementations or tests they reference.

### Phase 2 — Domain and interoperability foundations

Work on L and P in focused PRs, reusing mature libraries when that gives a safer and faster path:

1. Complete TCP correctness and establish interoperable TLS using official vectors and independent
   peers (#257, #334).
2. Build real HTTP service boundaries, bounded parsing, timeouts, backpressure, and multi-worker
   async execution (#257, #332).
3. Deliver durable storage, crash recovery, recursive indexes, migrations, and tenant-safe data
   access, natively or through a mature database bridge (#257, #332, #334).
4. Prove production FFI ownership/callback boundaries and real WASM resource/capability enforcement
   with adversarial tests (#334).
5. Run TLS and database capstones through foreign-library bindings and record exact artifact
   identities, platforms, and guarantees (#334).

A self-round-trip, substring format check, one-shot server, sequential scheduler, or reference model
must never close these acceptance items.

### Phase 3 — Complete SaaS

Build #332 incrementally toward one deployable multi-tenant capstone:

1. secure HTTP/TLS service and graceful concurrent runtime;
2. durable data, migrations, backup, and restore;
3. authentication, sessions, authorization, tenant isolation, and secret handling;
4. background work, structured logs, metrics, health checks, and configuration;
5. reproducible deployment and automated failure recovery;
6. complete end-to-end acceptance, including adversarial cross-tenant, malformed-input, load, crash,
   and restore tests.

Every foreign service or library is listed explicitly with its capability and trust boundary. The
capstone is not production-accepted until all claimed behavior runs in CI or a documented equivalent
real environment.

### Phase 4 — Complete Web applications

Build #333 on the real backend and polyglot boundaries:

1. run Lume-generated WASM in real browser engines;
2. support DOM nodes, strings, events, callbacks, and JavaScript lifetimes;
3. add forms, navigation, fetch, WebSockets, storage, timers, and cancellation;
4. add routing, components, state updates, assets/CSS, debugging, and packaging;
5. ship a complete browser capstone with remote API calls, loading/error states, persistence,
   responsive styling, and adversarial request/callback tests.

Node.js WASM verification remains useful but cannot substitute for real-browser acceptance.

### Phase 5 — Operating systems and intent-driven growth

- **O (#241)** grows from the real microkernel into VFS/storage, userspace/syscalls, networking,
  shell/userland, and drivers. Each subsystem is its own large track with QEMU/hardware acceptance.
- **I (#259, shipped)** supplies the initial intent/cache/proof baseline. E2 extends invalidation to
  the remaining material inputs and affected graph; N makes the protocol vendor-neutral and reusable
  across agents. Neither path may promote generated code without evidence.

OS work may proceed independently when its dependencies are real. A general-purpose OS is an
ecosystem-scale program, not a single language feature or PR.

## The adoption inflection (#337)

Lume becomes indispensable when it is objectively cheaper and more reliable for an AI to obtain
correct software through Lume than by generating and maintaining code in conventional human-centered
languages. Feature count and syntax elegance are not the adoption metric.

The decisive path is **intent in, running system plus independently checkable evidence out**:

- a vendor-neutral intent protocol describes goals, contracts, effects, resources, targets, and
  acceptance without requiring prior knowledge of Lume syntax;
- semantic transactions replace raw-text edits with validated repair, impact, provenance, and replay;
- a content-addressed component network lets any AI discover and reuse intents, bindings, tests,
  proofs, evidence, and artifacts without trusting stale or incompatible claims;
- public executable benchmarks measure zero-shot success, tokens, time, attempts, defects, and
  maintenance cost against real alternatives, including negative results;
- independently reproduced intent-to-SaaS/Web and intent-to-kernel/driver capstones demonstrate the
  same semantic and evidence fabric across radically different products.

The benchmark is **tokens/time/attempts to a real verified system**, not generated lines of code.
Recorded fixtures and self-reported success cannot establish this advantage.

## PR and acceptance contract

Every implementation PR must:

- reference the relevant umbrella issue and keep one focused change per branch/PR;
- implement the real construct in Lume or through an explicit, tested foreign boundary;
- include positive, negative, adversarial, and independently checked acceptance where applicable;
- run on every claimed platform and keep the full suite, ASan/UBSan, `stage2 == stage3`, and Zero-C
  gates green;
- update `docs/29`, the acceptance matrix when it exists, and user-facing status in the same change;
- state native versus foreign dependencies and shipped/model/planned status precisely;
- leave umbrella issues open until the entire corresponding capstone is accepted.

A roadmap or matrix PR registers work only. It never closes an implementation issue and never counts
as product evidence.
