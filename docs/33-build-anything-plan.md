# 33 — The plan to build anything with AI (frontiers, multipliers, order)

**Goal.** Let the AI build **anything** in Lume — any software, on any substrate — with every artifact
**real and proven** (never a placeholder or mock, per `docs/28` §7). "Anything imaginable" is an
asymptote no language reaches by pre-building features; Lume's bet is a **universal, proof-carrying
substrate** plus **AI self-growth**, so the model evolves the language toward each target instead of a
human building every one.

## The frontiers (each a tracked, decomposed, incremental track)

| Track | Frontier | Issue |
|---|---|---|
| **M1** | **Prover reach beyond QF_LIA (Shipped ✅)** — `QF_NIA` nonlinear integer arithmetic, `QF_BV` bitvectors & machine-int wraparound, `QF_ALIA` arrays + quantifiers (`forall_range`/`exists_range`), separation logic (`points_to`/`sep_conj`), refinement & dependent types (`Vec(n)`/`Index(n)`), lexicographic & relational termination (`total fn`), and LTL temporal logic (`always`/`eventually`/`until`/`leads_to`, #198), verified in `06b_phase3.src`, `lcaas.src`, and independent `lcert.src` (`compiler/stage1/m1_prover_ok.src`) | **#255 (Closed)** |
| **M2** | **`lume evolve` as a self-growth engine (Shipped ✅)** — multi-file self-growth (`@domain <target|prover|stdlib>`, `@file <path>`, `lume evolve propose --domain ...`, `lume evolve grow`) driving propose→synthesize (`A0001` holes + `T1001`/`T1003`/`T2002` fix-its)→prove (`M1` theories + laws)→benchmark (`@bound` WCET + `max_asm_lines`)→fixpoint (`stage2 == stage3`)→Ed25519-signed multi-file land (`compiler/stage1/levolve.src`, `evolve/proposals/0004_target_rv64_sll`..`0006_stdlib_math_mod`) | **#258 (Closed)** |
| **R** | **Universal reach (Shipped ✅)** — real GPU (`nvptx64-nvidia-cuda` PTX 8.0 & `spirv64-vulkan-compute` SPIR-V 1.6, folds #250 ✅), Embedded MCUs (`arm-none-cortexm`, `avr-none-elf`/`avr-none-ihex`, `xtensa-esp32-elf`/`xtensa-none-ihex`), FPGA/HDL (`verilog-rtl` IEEE 1364-2005 & `vhdl-rtl` IEEE 1076-2008 cycle-accurate RTL), Mobile (`aarch64-ios-macho` `PLATFORM_IOS=2` & `aarch64-android-elf` `.note.android.ident` API 34), and Browser/Web (`wasm32-web-dom` WASM+DOM) — all 24 triples in `compiler/stage1/ltarget.src` run-verified with `QF_LIA` guard elision and exit-3 contract traps (`docs/26-targets.md`) | **#256 (Closed)** |
| **L** | **Standard library & domains (Shipped ✅)** — native networking (`compiler/stage1/net.src` + Linux x86-64 socket syscalls `net_tcp_listen`/`net_tcp_connect`/`net_tcp_accept`/`net_udp_bind`/`net_udp_sendto`/`net_send_str`/`net_recv_str`/`net_close`, IPv4/CIDR, RFC 1071 checksum, UDP/TCP packet builder & FSM, TLS 1.3 HKDF-Expand-Label + AEAD record layer, HTTP/1.1 parser & loopback client/server), async I/O runtime (`compiler/stage1/async_rt.src` + Linux `epoll` syscalls `epoll_create_fd`/`epoll_add_in`/`epoll_wait_one`, timer wheel, bounded backpressure, multi-worker work-stealing scheduler), GUI & 2D graphics (`compiler/stage1/gui.src`: RGBA canvas rasterizer, Porter-Duff alpha blending, `vbox`/`hbox` layout solver, spatial hit-testing, Netpbm P3 `.ppm` & `.svg` frame encoders), database (`compiler/stage1/db.src`: multi-level B+Tree index with page splitting & range scan, ACID MVCC snapshot isolation transactions, cost-based query planner, WAL), media (`compiler/stage1/media.src`: RIFF/WAV 16-bit PCM codec, FIR low-pass DSP filter, lossless RLE & inter-frame video GOP I/P-frame codec), and real ML (`compiler/stage1/nn.src`, `compiler/stage1/ai_runtime.src`, `06b_phase3.src`, `lcaas.src`: symbolic & reverse-mode Wengert tape autodiff across `relu`/`sq`/`cube`/`abs`/`clamp`/`leaky_relu`/`quot`/`tanh`/`sigmoid`/`exp`, Hamiltonian Monte Carlo `infer_hmc` with leapfrog integration, `safetensors` binary frame + `onnx` verifiers) (`compiler/stage1/track_l_domains_ok.src`) | **#257 (Closed)** |
| **I** | **Intent-as-source §1 (Shipped ✅)** — first-class `.intent` specifications (`lume intent <build|run|verify|status|clean>`, `lume build/run/prove/check <file.intent>`, `examples/intent/verified_specs.intent`, `compiler/stage1/track_i_intent_ok.src`), linear (`QF_LIA`) + non-linear (`QF_NIA`: `v * v`, `(a - b) * (a - b)`, `w * h`) + piecewise + `law` candidate synthesis & proof discharge in `06b_phase3.src` and `lcaas.src` (`@mode intent-build`/`intent-verify`/`intent-code`), SHA-256 content-addressed `.intent.cache.src` + `.intent.cache.cert.json` regeneration on spec change, `[V0005]` tamper detection, and `[V0001]` unprovable spec rejection (`docs/19` §1) | **#259 (Closed)** |
| **O** | **General-purpose OS** on the microkernel — incremental, uses R+L | **#241** |

## Why two of them are *multipliers* (do them first)
- **M1 (prover reach)** is what makes "build anything" mean "build anything **proven**". QF_LIA proves a
  narrow fragment; without richer theories the AI can generate code but not verify it. Every other
  track ships proof-carrying only to the extent M1 reaches.
- **M2 (evolve engine)** is what makes the breadth tracks (R, L) tractable **without a human building
  each piece**: the AI evolves the language toward the target, each change real + proven + gated. This
  is the unique, AI-first lever no other language has.

With M1 + M2 in place, R and L stop being an endless human to-do list and become territory the AI
grows on demand — and I (intent-as-source) becomes the loop the user drives at the top.

## Order (dependency, not a hard gate everywhere)
1. **M1 prover reach** (#255) — in parallel with M2; underpins proof-carrying everything.
2. **M2 evolve engine** (#258) — so the AI can then drive R and L itself.
3. **R universal reach** (#256) and **L stdlib/domains** (#257) — grown via M2, proven via M1; the GPU
   slice (#250) is the first R brick.
4. **I intent-as-source §1** (#259) — the top-level AI loop, ties M1 (re-prove on intent change).
5. **O general-purpose OS** (#241) — built incrementally on R + L once the kernel (done, `docs/27`) stands.

## The one rule over all of it (`docs/28` §7)
No placeholder, stub, mock, fixed output, or one-shot ships in a feature's path — not even labelled; if
it isn't real it's an open issue referenced as working nowhere. Everything documented must **run**,
tested exhaustively and re-tested every change; closing an issue needs the real, run-verified
deliverable; large tracks ship incrementally, each increment real. "Build anything" is earned one
proven brick at a time — never claimed.

The product-level SaaS, Web, polyglot-interoperability, evidence, and OS tracks are registered and
ordered in `docs/34-product-maturity-program.md`.
