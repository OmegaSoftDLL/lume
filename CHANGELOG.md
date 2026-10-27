# Changelog

All notable changes to the **Lume** programming language and self-hosted toolchain are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Stage 5 (Phase 5 — `docs/25-phase5-plan.md`, `docs/20-interop.md`, `docs/22-adoption.md`, `docs/23-horizon.md`): Systems, Interop, Realtime, Proof Certificates & `lume-os` Microkernel:**
  - **Track M5 / Interop (`04_parser_decls.src`, `06b_phase3.src`, `lcaas.src`, `interop.src`):** Added `@repr(c)`, `@packed`, `@bits`, `@mmio`, `@export("C")`, `extern "c" fn` (requiring `"ffi"` effect `[E0200]` and enforcing foreign provenance taint `[A0003]` until verified via `verify(...)` or `ensures`), `pub extern "c" fn` reverse C-ABI & WAT export (`ffi_export_c_header`, `wasm_export_wat`), `lume agent bind <header.h>` AI C-header binding generator, and `std.wasm` reference sandbox model (`wasm_module_new`, `wasm_grant_cap`, `wasm_call_add`, `wasm_store32`, `wasm_load32`, `wasm_call_host_fs` requiring both `"wasm"` and `"fs"` capabilities `[E0200]`).
  - **Track K5 — Realtime, Interrupts, Structural Cycle-Budget Checker (`@bound`) & Linear Hardware Capabilities (`06b_phase3.src`, `lcaas.src`, `interop.src`, `phase5_sys_ok.src`):** Added `@interrupt(irq: N)` (`[E0200]` rejecting heap allocation or blocking I/O inside ISRs), `@bound(cycles: N)` static structural cycle-budget analyzer (`WcetSig`, emitting `"kind":"wcet"` obligations with `"status":"budget_checked"` in `lume prove` and failing with `[V0002]` when `est_cycles > bound`), and linear hardware capabilities `cap[Page]`, `cap[Port]`, `cap[Irq]` (`[O0001]` on double consumption of `cap_page_map`).
  - **Track G1/G3/H3 — Standalone Independent Proof Certificate Kernel & MCP (`lcert.src`, `./lume`):** Added `lume certify`, `lume verify-cert` (standalone zero-dependency proof-certificate verifier in `lcert.src` with cryptographic FIPS 180-4 SHA-256 tamper verification, its own S-expression parser + Fourier–Motzkin `QF_LIA` solver over $\mathbb{Z}$, and `wcet` budget verification issuing `"badge":"Lume Verified ✓"`), and `lume mcp <file.src>` Model Context Protocol server.
  - **Stage 5 Capstone (`examples/apps/lume_os/`):** Capability-based microkernel (`caps.src`, `scheduler.src`, `ipc.src`, `main.src`) with `QF_LIA`-verified page allocation, `@bound` budget-checked scheduler & `@interrupt` timer ISR, reference WASM driver, and the Stage-4 neuro-symbolic agent running in user space over capability-badged IPC.
- **Stage 4 (Phase 4 — `docs/24-phase4-plan.md`): AI, Tensors, Autodiff, Kernels, Probabilistic Programming, Datalog Rules, `extern llm fn` & Spiking Neuro:**
  - **Track N1 — Shape-Typed Tensors & `std.tensor` (`04_parser_decls.src`, `06_typecheck.src`, `06b_phase3.src`, `lcaas.src`, `tensor.src`):** Added `tensor[DType, D1, ...]` with compile-time symbolic shape unification (`[T0300]` diagnostics on mismatched `matmul`, `einsum`, `tensor_add_bias`, elementwise ops, and `if` branches) and native `std.tensor` kernels (`matmul`, `tensor_add_bias`, `tensor_relu`, `tensor_relu_backward`, `tensor_transpose`, `einsum_ij_jk`, `einsum_ij_j`).
  - **Track N2 — Differentiable Programming (`grad`, `value_and_grad`, `jvp`, `vjp`, `jit`), `QF_LIA` Gradient Verification, `kernel fn` & `std.nn` (`06b_phase3.src`, `lcaas.src`, `nn.src`):** Added `type Name = model { ... }`, compile-time symbolic/reverse-mode autodiff (`grad`, `value_and_grad`, `jvp`, `vjp`, `jit`) over the polynomial fragment (`+`, `-`, `*`, `if`, `let`, `relu`, `sq`) emitting `[N0400]` on non-polynomial ops/calls and integrated directly into `QF_LIA` (`lume prove` proves postconditions over `grad(f, x)` and elides runtime guards with `call ctfail == 0`), accelerator `kernel fn` (`@schedule`, `@autotune`, PTX/SPIR-V codegen, `[E0200]` host/heap effect rejection), and `std.nn` (`mlp2_*`, `sgd_step_mlp2`, `adam_step_mlp2`, cryptographic FIPS 180-4 SHA-256 `model_content_hash` for `safetensors` and `onnx` export).
  - **Track N3 — `std.prob`, Semi-Naive Datalog (`rule`), `extern llm fn` (`std.llm`) & Spiking `std.neuro` (`04_parser_decls.src`, `06b_phase3.src`, `lcaas.src`, `ai_runtime.src`):** Added `prob` effect (`dist_beta`, `dist_bernoulli`, `dist_normal`, `infer_mcmc`, `infer_svi`, `infer_smc`), declarative `rule` items and semi-naive Datalog fixpoint engine (`datalog_closure`), `extern llm fn` with `ensures` provenance discharge and BAML-style contract repair loop (`llm_extract_invoice_repaired`), and deterministic LIF spiking neural networks (`lif_neuron_new`, `lif_step`, `lif_run_Currents`).
  - **Stage 4 Capstones (`examples/apps/mnist/` & `examples/apps/agent/`):** Added end-to-end 2-layer MLP trainer reaching 100% accuracy (`loss 120 -> 0`) with `safetensors`/`onnx` export, and a neuro-symbolic agent combining LLM extraction + contract repair, Datalog policy reachability, Bayesian MCMC reliability, and a spiking LIF anomaly watchdog.
- **Inflection Plan (`docs/21-inflection-plan.md` — P0 & Stages A–D):** Dogfooded Phase-3 capabilities directly into the self-hosted compiler (`compiler/stage1/lume/*.src`) while preserving the `stage2 == stage3` fixpoint and Zero-C steady state:
  - **P0 (#167 / PR #171):** Root-caused and fixed inter-procedural algebraic effect handler lowering (`olowercall_fn`, `kuses_hop`, `ksubst_e`, `kbind_hop_args`, `klower_eff_ops`) across effect-polymorphic function boundaries, covered by cross-platform conformance tests.
  - **Stage A (PR #172):** Dogfooded 9 V1 `requires` / `ensures` contracts across Lexer (`numend`, `decimalend`, `decscan`), Parser (`findcloseb`, `parsevariantend`, `parsetypedecimal2`), and Typecheck (`tifn3`, `henv0`, `hbind`) passes (`compiler/stage1/compiler_contracts_ok.src`).
  - **Stage B (#173 / PR #174):** Replaced bounded sampling with a sound `QF_LIA` decision procedure over $\mathbb{Z}$ via Fourier–Motzkin variable elimination (`vqflia_prove_fn`, `vqflia_prove_law`) and constraint-driven counterexample extraction (`vpts_from_hyps`, `vfm_trace_hyps`). Only obligations genuinely decided by `QF_LIA` are marked `proven` and elide runtime guards (including `imax` dogfooded in `05_codegen.src`); nonlinear obligations remain `runtime_checked` (`compiler/stage1/qflia_ok.src`).
  - **Stage C (PR #175):** Modeled the compiler CLI I/O boundary (`07_driver.src`) as `effect CompilerIO { fn read_source() -> str }` with `lumec_drive() / {CompilerIO}` handled at the CLI entry point, while marking all 5 core passes (`lumec_lex`, `lumec_parse`, `lumec_check`, `lumec_expand`, `lumec_compile`) explicitly `/ pure` (`compiler/stage1/compiler_effects_ok.src`).
  - **Stage D (PR #176):** Annotated hot compiler data structures with `own` (`joinall`/`pairs`/`pairs2` in `05_codegen.src` and `gerase`/`gestmts`/`gestmt`/`geexpr`/`geargs`/`gerec`/`gearms` + `cfold`/`cfoldstmts`/`cfoldstmt` in `03_comptime.src`) so `lumec` exercises Perceus in-place constructor reuse (`__perceus_reuse_1` / `__perceus_reuse_2`) with zero `call alloc` in `gestmts`, `geargs`, and `cfoldstmts` (`compiler/stage1/compiler_perceus_ok.src`).
- **Compiler-as-Oracle & Intent-as-Source Synthesis Loop (`docs/19-intent-as-source.md`):**
  - Upgraded hole-guided synthesis (`06b_phase3.src`, `lcaas.src`, `levolve.src`) to generate linear and conditional AST candidates, prune refuted candidates via counterexample search, rank `QF_LIA`-proven candidates (`proven_candidates`) against `requires`/`ensures` contracts and `law` (`P34`) declarations, elide runtime contract guards on proven synthesized bodies (`compiler/stage1/intent_oracle_ok.src`), and auto-repair `A0001` holes in `lume evolve` (`evolve/proposals/0002_intent_oracle/proposal.bundle`).
- **Release Engineering & Security Automation (Lotes 1–3):**
  - Added `.github/CODEOWNERS`, `.github/pull_request_template.md`, `.github/dependabot.yml`, `.github/workflows/codeql.yml`, and `.github/workflows/release.yml` (automated self-hosted release artifacts + `SHA256SUMS`).
  - Enforced `make zero-c` and dynamic `find`-based `./lume fmt --check` inside required CI gates.

## [0.3.0] - 2026-10-26 — Stage 3: Ownership, AI-Native Authoring, Effects, Concurrency & Verification

### Added
- **Track O — Ownership, Borrow Checker, `from` Regions & Memory Tiers (`06b_phase3.src`, `ownership.src`, `own_ok.src`):**
  - Single-owner move semantics (`O0001` use-after-move, `O0002` move out of borrow) and deterministic LIFO `defer` / `errdefer`.
  - Borrow modes (`shared` default, `mut` exclusive borrow with `O0010`/`O0011`, `own` transfer with `O0012`).
  - Intra-module region inference and `pub fn ... from <param>` return-origin clauses (`O0020`, `O0021`).
  - Explicit shared/region tiers (`rc[T]`, `arc[T]`, `weak[T]`, `cell[T]`, `arena_new`/`arena_alloc`, `@gc`, `@freestanding` + `with alloc` `O0030`).
- **Track A — AI-Native Authoring (`06b_phase3.src`, `lcaas.src`, `ai_authoring_ok.src`):**
  - Typed holes (`_`, `_hole*`) with expected-type & scope inference, `lume agent fill` candidate synthesis, and `[A0001]` rejection in `pub fn`.
  - `total fn` / `pub total fn` structural and arithmetic measure termination checking (`[A0002]`).
  - Provenance & uncertainty tracking (`unverified[T]`, `llm fn`, `prob_sample`) enforcing `[A0003]` at `pub fn` returns and effectful sinks until discharged via `verify`, contract guards, `law_check`, or `evolve_approve`.
- **Track K — Effect System, Algebraic Handlers & Perceus Reuse (`06b_phase3.src`, `effects_ok.src`, `perceus_ok.src`):**
  - Effect rows (`/ {pure, io, fs, net, alloc, async, fail, panic, rand, time}` + user effects), `pub fn` effect enforcement (`[E0200]`), and `@capability` module checking.
  - Algebraic `effect Name { fn op(...) -> R }` and `with handler Name { ... } in <body>` tail-resumptive lowering.
  - Perceus in-place constructor reuse (`__perceus_reuse_1` / `__perceus_reuse_2`) mutating `own`-matched cells in place with zero heap allocation.
- **Track S — Structured Concurrency, Channels & Supervised Actors (`concurrency.src`, `actor.src`, `concurrency_ok.src`):**
  - Lexical `scope` / `spawn` and static `Send`/`Sync` checking (`[S0001]`).
  - Typed channels (`chan[T]`), deterministic seeded `select`, and data-parallel combinators (`par_for_sum_range`, `par_map_scale`, `par_sum`).
  - `std.actor` mailbox state machines, Erlang/OTP-style `supervisor` (`one_for_one`, `one_for_all`), and `std.time.logical` virtual clock.
- **Track V — Contract Verification, `lume prove` & `LAWS` (`06b_phase3.src`, `lcaas.src`, `contracts_ok.src`):**
  - First-class `requires`, `ensures`, `invariant`, `old(...)`, and `law` declarations (`P34`), static refutation with concrete counterexamples (`[V0001]`), and `lume prove` / `lume agent prove`.
- **Stage 3 Capstone (`examples/apps/job_server/`):** Multi-module concurrent job processing server exercising Tracks O, A, K, S, and V with zero `unsafe`.

## [0.2.0] - 2026-10-26 — Stage 2: Comptime, Hygienic Macros, Data/Query, One-Tool CLI, Self-Evolution & HM Types

### Added
- **Track C — Comptime (`03_comptime.src`):** Compile-time expression folding (`C1`), pure recursive user functions (`C2`), generics via `comptime T: type` (`C3`), `comptime assert` (`C4a`), and precomputed lookup tables (`C4b`).
- **Track M — Macros & Metaprogramming (`ast_meta.src`, `macro_hygiene.src`):** First-class comptime AST (`quote`, `match` over AST, `splice` — `M1`) and hygienic quasiquote (`unquote` with automatic `_hyg_` lexical alpha-renaming — `M2`).
- **Track D — Data & Queries (`decimal.src`, `table.src`, `query.src`):** Exact `decimal(p, s)` arithmetic (`D1`), typed `table[T]` & CSV ingestion (`D2`), and compile-time checked relational queries (`query_where`, `query_order`, `query_join`, `query_group` — `D3`).
- **Track T — One Tool (`./lume`, `lfmt.src`, `ldoc.src`, `ltest.src`, `lmod.src`):** Unified `./lume` CLI (`build`, `run`, `fmt`, `test`, `doc`, `check`, `pkg`, `describe`), canonical AST formatter, and SHA-256 lockfile + `P26` capability auditing.
- **Track E — Self-Evolution (`lcaas.src`, `levolve.src`):** Triple self-build (`stage2 == stage3`) + Zero-C gate (`E2`), `lume compile --json` (`E1a`), in-process compiler library API `use lcaas` + `lume agent` (`E1b`), and 6-gate governed `lume evolve` loop (`E3`).
- **Track F — Hindley–Milner Inference & Front-End Unification (`06_typecheck.src`, `f4_*.src`):** Gradual typing, exhaustiveness checking, full Hindley–Milner constraint unification (`F3c`), and unified grammar across layout, explicit-brace, and Lume-Core (`F4`).
- **Stage 2 Capstone (`examples/apps/inventory/`):** Multi-module inventory, pricing, query, and reporting application (25 `lume test` assertions).

## [0.1.0] - 2026-10-25 — Stage 1: Toolchain-Free Native Self-Hosting (`lumec` + `lasm`)

### Added
- Self-hosted compiler (`compiler/stage1/lume/01_lexer.src`–`07_driver.src`) and native x86-64 assembler + ELF64 writer (`compiler/stage1/lasm.src`) producing static Linux x86-64 binaries with zero C compiler, assembler, linker, or libc dependency.
- Disposable C11 bootstrap seed (`bootstrap/lume0`) with lexer, parser, interpreter, LBE binary AST, semantic checker (`resolve.c`), and seed x86-64/ELF64 backend.
