# Documentation and delivery tracking index

Snapshot: 2026-10-29, baseline `2e3ccfc`. This index records a metadata and documentation review; it is not proof that every historical implementation meets its specification. Draft PRs contain execution plans, not implementations. Each item remains open until its full applicable scope, proofs, regression tests, and required CI gates are complete.

## Delivery standard

Real end-to-end behavior is mandatory: no mocks, placeholders, fixed results or simulated success in place of the feature. Establish sound, independently checked proofs for applicable guarantees, exercise positive, adversarial and negative cases, and require green CI. Existing partial implementations and closed issues require evidence before being counted as complete.

## Open implementation tracks and plan PRs

| Scope | Tracking issue | Draft plan PR |
|---|---|---|
| P35 example coverage and strict enforcement | [#269](https://github.com/OmegaSoftDLL/lume/issues/269) | [#270](https://github.com/OmegaSoftDLL/lume/pull/270) |
| Capability-gated compile-time file embedding | [#271](https://github.com/OmegaSoftDLL/lume/issues/271) | [#272](https://github.com/OmegaSoftDLL/lume/pull/272) |
| Derive table schemas from nominal row types | [#273](https://github.com/OmegaSoftDLL/lume/issues/273) | [#274](https://github.com/OmegaSoftDLL/lume/pull/274) |
| Binary WASM hosting and capability sandbox | [#275](https://github.com/OmegaSoftDLL/lume/issues/275) | [#276](https://github.com/OmegaSoftDLL/lume/pull/276) |
| Submit Lume to GitHub Linguist after maturity gate | [#277](https://github.com/OmegaSoftDLL/lume/issues/277) | [#278](https://github.com/OmegaSoftDLL/lume/pull/278) |
| GPU hardware dispatch and CPU/GPU capstone | [#279](https://github.com/OmegaSoftDLL/lume/issues/279) | [#280](https://github.com/OmegaSoftDLL/lume/pull/280) |
| Complete adoption and distribution acceptance | [#281](https://github.com/OmegaSoftDLL/lume/issues/281) | [#282](https://github.com/OmegaSoftDLL/lume/pull/282) |
| Complete software protection and secrets requirements | [#283](https://github.com/OmegaSoftDLL/lume/issues/283) | [#284](https://github.com/OmegaSoftDLL/lume/pull/284) |
| Semantic edits, effect replay and full provenance | [#285](https://github.com/OmegaSoftDLL/lume/issues/285) | [#286](https://github.com/OmegaSoftDLL/lume/pull/286) |
| Complete horizon proof and portability goals | [#287](https://github.com/OmegaSoftDLL/lume/issues/287) | [#288](https://github.com/OmegaSoftDLL/lume/pull/288) |
| Prover theories and independent benchmark acceptance | [#289](https://github.com/OmegaSoftDLL/lume/issues/289) | [#290](https://github.com/OmegaSoftDLL/lume/pull/290) |
| Universal targets independent execution and proof coverage | [#291](https://github.com/OmegaSoftDLL/lume/issues/291) | [#292](https://github.com/OmegaSoftDLL/lume/pull/292) |
| Remaining language and runtime roadmap features | [#293](https://github.com/OmegaSoftDLL/lume/issues/293) | [#294](https://github.com/OmegaSoftDLL/lume/pull/294) |
| Complete AI provider and Stage 4 roadmap acceptance | [#295](https://github.com/OmegaSoftDLL/lume/issues/295) | [#296](https://github.com/OmegaSoftDLL/lume/pull/296) |
| Stage 5 OS capstone and machine-checked invariants | [#297](https://github.com/OmegaSoftDLL/lume/issues/297) | [#298](https://github.com/OmegaSoftDLL/lume/pull/298) |
| Reconcile Stage 1 checker and assembler checklist | [#299](https://github.com/OmegaSoftDLL/lume/issues/299) | [#300](https://github.com/OmegaSoftDLL/lume/pull/300) |
| Reconcile documentation status and normative core | [#301](https://github.com/OmegaSoftDLL/lume/issues/301) | [#302](https://github.com/OmegaSoftDLL/lume/pull/302) |
| Standard library & domains — net stack, async runtime, GUI, database, media, real ML | [#257](https://github.com/OmegaSoftDLL/lume/issues/257) | [#303](https://github.com/OmegaSoftDLL/lume/pull/303) |
| Intent-as-source §1 — intent is the source, code is a proven cache (docs/19 §1 flagship) | [#259](https://github.com/OmegaSoftDLL/lume/issues/259) | [#304](https://github.com/OmegaSoftDLL/lume/pull/304) |
| General-purpose OS beyond the microkernel: filesystem/VFS, storage, network stack, userspace/syscalls, shell/userland, driver ecosystem (docs/27 'Beyond the kernel') | [#241](https://github.com/OmegaSoftDLL/lume/issues/241) | [#305](https://github.com/OmegaSoftDLL/lume/pull/305) |
| Complete autonomous self-growth and governance | [#306](https://github.com/OmegaSoftDLL/lume/issues/306) | [#307](https://github.com/OmegaSoftDLL/lume/pull/307) |
| Complete language specification conformance | [#308](https://github.com/OmegaSoftDLL/lume/issues/308) | [#309](https://github.com/OmegaSoftDLL/lume/pull/309) |
| Incremental compilation and feedback performance | [#310](https://github.com/OmegaSoftDLL/lume/issues/310) | [#311](https://github.com/OmegaSoftDLL/lume/pull/311) |
| Structured LBE constrained decoding and compact format | [#312](https://github.com/OmegaSoftDLL/lume/issues/312) | [#313](https://github.com/OmegaSoftDLL/lume/pull/313) |
| Reconcile historical backend and seed parity limitations | [#314](https://github.com/OmegaSoftDLL/lume/issues/314) | [#315](https://github.com/OmegaSoftDLL/lume/pull/315) |

## Documentation inventory

All tracked documentation and grammar paths at the baseline are listed below. For requirements without an explicit specialized track, specification coverage is tracked in [#308](https://github.com/OmegaSoftDLL/lume/issues/308), and conflicting or obsolete documentation in [#301](https://github.com/OmegaSoftDLL/lume/issues/301). Historical survey material and reserved edition features must be classified explicitly, not silently discarded.

- `.github/ISSUE_TEMPLATE/bug_report.md`
- `.github/ISSUE_TEMPLATE/feature_request.md`
- `.github/PULL_REQUEST_TEMPLATE.md`
- `.github/pull_request_template.md`
- `AGENTS.md`
- `CHANGELOG.md`
- `CODE_OF_CONDUCT.md`
- `CONTRIBUTING.md`
- `README.md`
- `SECURITY.md`
- `bootstrap/README.md`
- `contrib/linguist/README.md`
- `contrib/linguist/samples/Lume/README.md`
- `docs/00-catalog.md`
- `docs/01-survey.md`
- `docs/02-principles.md`
- `docs/03-specification.md`
- `docs/04-roadmap.md`
- `docs/05-vision.md`
- `docs/06-software-protection.md`
- `docs/07-competitive.md`
- `docs/08-ai-first.md`
- `docs/09-lbe.md`
- `docs/10-stage1-status.md`
- `docs/11-self-hosting-plan.md`
- `docs/12-phase2-plan.md`
- `docs/13-module-split-plan.md`
- `docs/14-data-track-handoff.md`
- `docs/14-zero-c.md`
- `docs/15-phase2-status.md`
- `docs/16-phase3-plan.md`
- `docs/17-frontend-unification.md`
- `docs/19-intent-as-source.md`
- `docs/20-interop.md`
- `docs/21-inflection-plan.md`
- `docs/22-adoption.md`
- `docs/23-horizon.md`
- `docs/24-phase4-plan.md`
- `docs/25-phase5-plan.md`
- `docs/26-targets.md`
- `docs/27-systems-and-kernels.md`
- `docs/28-delivery-rules.md`
- `docs/29-worklist-order.md`
- `docs/30-lume-book.md`
- `docs/31-rfc-and-governance.md`
- `docs/32-ai-authoring.md`
- `docs/33-build-anything-plan.md`
- `docs/PROJECT_STATE.md`
- `docs/execution-plans/docs-reconcile.md`
- `docs/index.md`
- `docs/install.sh`
- `examples/README.md`
- `examples/apps/inventory/README.md`
- `formal/lean/README.md`
- `grammar/lume.ebnf`
- `tests/conformance/README.md`
- `tests/llm_gen_eval/system_prompt.md`

## Historical issue inventory

The following lists every issue returned by GitHub at the snapshot, including closed issues. Closure is history, not independent proof of acceptance; residual scope belongs in the open tracks above.

- [#167](https://github.com/OmegaSoftDLL/lume/issues/167) ? closed: Effect dispatch across a function boundary miscompiles (platform-dependent output / UB) — masked by capstone rewrite in #165
- [#173](https://github.com/OmegaSoftDLL/lume/issues/173) ? closed: Inflection Stage B: sound QF_LIA decision procedure so 'proven' can elide guards (replace v0 sampling)
- [#183](https://github.com/OmegaSoftDLL/lume/issues/183) ? closed: Proof certificate kernel does not verify independently (rubber-stamps compiler status) + toy hash mislabeled sha256
- [#184](https://github.com/OmegaSoftDLL/lume/issues/184) ? closed: Symbolic autodiff silently returns 0 for non-polynomial ops (division/mod/calls) — no diagnostic
- [#185](https://github.com/OmegaSoftDLL/lume/issues/185) ? closed: WCET @bound is a heuristic budget, not sound cycle-accurate WCET — overclaimed as proven
- [#186](https://github.com/OmegaSoftDLL/lume/issues/186) ? closed: std.wasm 'capability-scoped sandbox' is a runtime library model, not enforced isolation — overclaimed
- [#188](https://github.com/OmegaSoftDLL/lume/issues/188) ? closed: URGENT: implement P33 software protection (anti-RE) for generated artifacts — plan exists in docs/06, zero implemented
- [#191](https://github.com/OmegaSoftDLL/lume/issues/191) ? closed: Interop is parse/check-only — implement real FFI linking + C-ABI/WASM export (Track I)
- [#192](https://github.com/OmegaSoftDLL/lume/issues/192) ? closed: Phase 4 AI-numerics are stubs — make jit/kernel/prob/datalog/safetensors real
- [#193](https://github.com/OmegaSoftDLL/lume/issues/193) ? closed: Track S runs sequentially — implement a real parallel runtime or keep the 'simulation' label
- [#194](https://github.com/OmegaSoftDLL/lume/issues/194) ? closed: lume evolve governance differs from docs/04 — unauthenticated approval, fixpoint gate isn't stage2==stage3
- [#195](https://github.com/OmegaSoftDLL/lume/issues/195) ? closed: Intent-as-source oracle incomplete — implement witness/repair/impact queries (docs/19 §2)
- [#196](https://github.com/OmegaSoftDLL/lume/issues/196) ? closed: Post-Phase-5 adoption & distribution (docs/22 G-series): installer, packaging/registry, certificates, playground — implementation tracking
- [#197](https://github.com/OmegaSoftDLL/lume/issues/197) ? closed: Specified-but-unimplemented language features (docs/03): traits, closures, refinement types, async/await, rc/arc, shard, AArch64/RISC-V/WASM, let-polymorphism
- [#198](https://github.com/OmegaSoftDLL/lume/issues/198) ? closed: Horizon tracking (docs/23 H1–H5 + docs/19 §3–§5): compositional/temporal proof, anti-rot, portable kernel, learned⊕proven, metal targets
- [#203](https://github.com/OmegaSoftDLL/lume/issues/203) ? closed: lume evolve must DRIVE the evolution — real autonomous self-evolution, no theater (needs #194 real Ed25519 auth)
- [#204](https://github.com/OmegaSoftDLL/lume/issues/204) ? closed: Port the lume CLI from a shell script to Lume (self-host the control plane)
- [#205](https://github.com/OmegaSoftDLL/lume/issues/205) ? closed: Freeze the C seed: new primitives go in Lume (lasm.src), not new seed C
- [#206](https://github.com/OmegaSoftDLL/lume/issues/206) ? closed: Universal targets: build OSes, firmware & apps for every ISA/platform (x86-64 ✓, ARM64/Apple Silicon, Windows, RISC-V, bare-metal) — proof-preserving, no hacks
- [#208](https://github.com/OmegaSoftDLL/lume/issues/208) ? closed: P35: emit a missing-test diagnostic for untested pub surface (docs/02, not enforced)
- [#209](https://github.com/OmegaSoftDLL/lume/issues/209) ? closed: AI-author substrate (docs/08/09): content-addressed per-definition hashing + constrained-decoding grammar + projectional editing
- [#210](https://github.com/OmegaSoftDLL/lume/issues/210) ? closed: lume lsp + lume dap: language server & debug adapter (docs/04 Stage 1, not built)
- [#211](https://github.com/OmegaSoftDLL/lume/issues/211) ? closed: lume repl + notebook/Jupyter kernel with first-class tables (docs/04 Stage 2, not built)
- [#212](https://github.com/OmegaSoftDLL/lume/issues/212) ? closed: Comptime generics over declared types/containers — last open C4 sub-item (docs/12)
- [#213](https://github.com/OmegaSoftDLL/lume/issues/213) ? closed: Mechanise lume-core in Lean: semantics + type-soundness as the trust anchor (docs/04 Stage 5, docs/05 §2.10)
- [#214](https://github.com/OmegaSoftDLL/lume/issues/214) ? closed: Data track beyond D1-D3: persist table / versioned / money[CCY] / @soa (docs/05 §2.6, docs/14)
- [#215](https://github.com/OmegaSoftDLL/lume/issues/215) ? closed: Untracked planned items: signal[T]/promise[T], lumb bytecode target, Linguist submission (decide: implement/fold/drop)
- [#217](https://github.com/OmegaSoftDLL/lume/issues/217) ? closed: Write a Lume syntax grammar (tree-sitter/TextMate) + Linguist-submission prep
- [#222](https://github.com/OmegaSoftDLL/lume/issues/222) ? closed: Validate ed25519_verify against the official RFC 8032 §7.1 test vectors (pin the verifier to the standard)
- [#227](https://github.com/OmegaSoftDLL/lume/issues/227) ? closed: Systems & kernels (K0–K9, docs/27): everything Lume needs to build firmware & an OS from scratch — proof-carrying, runs on QEMU
- [#230](https://github.com/OmegaSoftDLL/lume/issues/230) ? closed: LLM-generation eval as a CI gate (tokens/attempts to produce compiling+proven Lume)
- [#241](https://github.com/OmegaSoftDLL/lume/issues/241) ? open: General-purpose OS beyond the microkernel: filesystem/VFS, storage, network stack, userspace/syscalls, shell/userland, driver ecosystem (docs/27 'Beyond the kernel')
- [#250](https://github.com/OmegaSoftDLL/lume/issues/250) ? closed: Kernel codegen is a fixed-op placeholder (no real body lowering / no GPU exec) — #192 closed it prematurely, docs/24 mis-tracks to closed #8
- [#255](https://github.com/OmegaSoftDLL/lume/issues/255) ? closed: Prover reach beyond QF_LIA — the proof-power multiplier (SMT · arrays/quantifiers · separation · refinement/dependent · termination · temporal)
- [#256](https://github.com/OmegaSoftDLL/lume/issues/256) ? closed: Universal reach — run on every substrate (GPU real, MCU, FPGA/HDL, mobile, web), proofs preserved
- [#257](https://github.com/OmegaSoftDLL/lume/issues/257) ? open: Standard library & domains — net stack, async runtime, GUI, database, media, real ML
- [#258](https://github.com/OmegaSoftDLL/lume/issues/258) ? closed: lume evolve as a self-growth engine — the AI grows the language's missing pieces, proven
- [#259](https://github.com/OmegaSoftDLL/lume/issues/259) ? open: Intent-as-source §1 — intent is the source, code is a proven cache (docs/19 §1 flagship)
- [#262](https://github.com/OmegaSoftDLL/lume/issues/262) ? closed: Network install one-liner + self-update from GitHub Releases + multi-platform release artifacts (install like rustup/Go)
- [#269](https://github.com/OmegaSoftDLL/lume/issues/269) ? open: P35 example coverage and strict enforcement
- [#271](https://github.com/OmegaSoftDLL/lume/issues/271) ? open: Capability-gated compile-time file embedding
- [#273](https://github.com/OmegaSoftDLL/lume/issues/273) ? open: Derive table schemas from nominal row types
- [#275](https://github.com/OmegaSoftDLL/lume/issues/275) ? open: Binary WASM hosting and capability sandbox
- [#277](https://github.com/OmegaSoftDLL/lume/issues/277) ? open: Submit Lume to GitHub Linguist after maturity gate
- [#279](https://github.com/OmegaSoftDLL/lume/issues/279) ? open: GPU hardware dispatch and CPU/GPU capstone
- [#281](https://github.com/OmegaSoftDLL/lume/issues/281) ? open: Complete adoption and distribution acceptance
- [#283](https://github.com/OmegaSoftDLL/lume/issues/283) ? open: Complete software protection and secrets requirements
- [#285](https://github.com/OmegaSoftDLL/lume/issues/285) ? open: Semantic edits, effect replay and full provenance
- [#287](https://github.com/OmegaSoftDLL/lume/issues/287) ? open: Complete horizon proof and portability goals
- [#289](https://github.com/OmegaSoftDLL/lume/issues/289) ? open: Prover theories and independent benchmark acceptance
- [#291](https://github.com/OmegaSoftDLL/lume/issues/291) ? open: Universal targets independent execution and proof coverage
- [#293](https://github.com/OmegaSoftDLL/lume/issues/293) ? open: Remaining language and runtime roadmap features
- [#295](https://github.com/OmegaSoftDLL/lume/issues/295) ? open: Complete AI provider and Stage 4 roadmap acceptance
- [#297](https://github.com/OmegaSoftDLL/lume/issues/297) ? open: Stage 5 OS capstone and machine-checked invariants
- [#299](https://github.com/OmegaSoftDLL/lume/issues/299) ? open: Reconcile Stage 1 checker and assembler checklist
- [#301](https://github.com/OmegaSoftDLL/lume/issues/301) ? open: Reconcile documentation status and normative core
- [#306](https://github.com/OmegaSoftDLL/lume/issues/306) ? open: Complete autonomous self-growth and governance
- [#308](https://github.com/OmegaSoftDLL/lume/issues/308) ? open: Complete language specification conformance
- [#310](https://github.com/OmegaSoftDLL/lume/issues/310) ? open: Incremental compilation and feedback performance
- [#312](https://github.com/OmegaSoftDLL/lume/issues/312) ? open: Structured LBE constrained decoding and compact format
- [#314](https://github.com/OmegaSoftDLL/lume/issues/314) ? open: Reconcile historical backend and seed parity limitations

## Pull request inventory

The following lists every PR returned by GitHub at the snapshot, including merged and closed PRs. Unmerged draft plans are not feature delivery.

- [#1](https://github.com/OmegaSoftDLL/lume/pull/1) ? merged: Stage 0 foundation: spec v0.1, grammar, roadmap, examples, C lexer, CI
- [#2](https://github.com/OmegaSoftDLL/lume/pull/2) ? merged: Stage 0 parser, dual license (MIT/Apache-2.0), anti-reverse-engineering doc
- [#3](https://github.com/OmegaSoftDLL/lume/pull/3) ? merged: Stage 0 interpreter: lume0 runs real Lume programs
- [#4](https://github.com/OmegaSoftDLL/lume/pull/4) ? merged: Stage 0: finish std coverage + first Lume-in-Lume tokenizer
- [#5](https://github.com/OmegaSoftDLL/lume/pull/5) ? merged: Stage 1 groundwork: a compiler written in Lume + local module loading
- [#6](https://github.com/OmegaSoftDLL/lume/pull/6) ? merged: Stage 1 Lume lexer (in Lume) + competitive scan improvements
- [#7](https://github.com/OmegaSoftDLL/lume/pull/7) ? merged: Stage 1: Lume parser (tokens -> AST) in Lume
- [#8](https://github.com/OmegaSoftDLL/lume/pull/8) ? merged: Stage 1: full lex->parse->eval pipeline written in Lume
- [#9](https://github.com/OmegaSoftDLL/lume/pull/9) ? merged: AI-first: Lume is authored by AI, reviewed by humans (+ lume0 describe)
- [#10](https://github.com/OmegaSoftDLL/lume/pull/10) ? merged: LBE: AI emits structure (binary AST), compiler emits the bits
- [#11](https://github.com/OmegaSoftDLL/lume/pull/11) ? merged: Stage 1: code generator written in Lume (AST -> C -> native binary)
- [#12](https://github.com/OmegaSoftDLL/lume/pull/12) ? merged: Stage 1: Lume-written compiler gains functions and if (-> native binary)
- [#13](https://github.com/OmegaSoftDLL/lume/pull/13) ? merged: Stage 1: a static checker written in Lume
- [#14](https://github.com/OmegaSoftDLL/lume/pull/14) ? merged: Stage 1: lumec driver in Lume (lex -> parse -> check -> codegen)
- [#15](https://github.com/OmegaSoftDLL/lume/pull/15) ? merged: Stage 1: lumec as a file-driven command-line compiler (in Lume)
- [#16](https://github.com/OmegaSoftDLL/lume/pull/16) ? merged: Stage 1: direct x86-64 backend (Lume -> assembly -> native, no C)
- [#17](https://github.com/OmegaSoftDLL/lume/pull/17) ? merged: Stage 1: x86-64 backend gains functions, calls and if (recursion, no C)
- [#18](https://github.com/OmegaSoftDLL/lume/pull/18) ? merged: Stage 1: layout-aware lexer in Lume (INDENT/DEDENT) + pop builtin
- [#19](https://github.com/OmegaSoftDLL/lume/pull/19) ? merged: Stage 1: block-structured language (loops, if/else) parsed and run in Lume
- [#20](https://github.com/OmegaSoftDLL/lume/pull/20) ? merged: Stage 1: block language (loops, if/else) -> C -> native binary
- [#21](https://github.com/OmegaSoftDLL/lume/pull/21) ? merged: Stage 1: algebraic data types + pattern matching, in Lume
- [#22](https://github.com/OmegaSoftDLL/lume/pull/22) ? merged: docs: honest Stage 1 status inventory
- [#23](https://github.com/OmegaSoftDLL/lume/pull/23) ? merged: Stage 1: recursive functions over ADTs (a small ML core) in Lume
- [#24](https://github.com/OmegaSoftDLL/lume/pull/24) ? merged: Stage 1: strings in the ML core (literals, concat, string match arms)
- [#25](https://github.com/OmegaSoftDLL/lume/pull/25) ? merged: Stage 1: lists in the ML core (literals, indexing, count)
- [#26](https://github.com/OmegaSoftDLL/lume/pull/26) ? merged: Stage 1: records with named fields in the ML core
- [#27](https://github.com/OmegaSoftDLL/lume/pull/27) ? merged: Stage 1: if/then/else, string equality, and a meta-circular interpreter
- [#28](https://github.com/OmegaSoftDLL/lume/pull/28) ? merged: Stage 1: string ops + and/or + full comparisons in the ML core (+ a scanner)
- [#29](https://github.com/OmegaSoftDLL/lume/pull/29) ? merged: Stage 1: a real tokenizer written in the ML-core language
- [#30](https://github.com/OmegaSoftDLL/lume/pull/30) ? merged: Stage 1: a complete calculator compiler written in the ML core
- [#31](https://github.com/OmegaSoftDLL/lume/pull/31) ? merged: Stage 1: the core compiler generates code too (all four phases in the core)
- [#32](https://github.com/OmegaSoftDLL/lume/pull/32) ? merged: Stage 1: a compiler written in the core emits C that builds to a native binary
- [#33](https://github.com/OmegaSoftDLL/lume/pull/33) ? merged: Stage 1: progress guards in the core parser
- [#34](https://github.com/OmegaSoftDLL/lume/pull/34) ? merged: perf: lazy param snapshot (only when ensures/old is present) — ~7x on recursion
- [#35](https://github.com/OmegaSoftDLL/lume/pull/35) ? merged: Stage 1: block compiler gains functions + return + calls (-> native)
- [#36](https://github.com/OmegaSoftDLL/lume/pull/36) ? merged: Stage 1: block_cc -- imperative compiler as a CLI (file -> native)
- [#37](https://github.com/OmegaSoftDLL/lume/pull/37) ? merged: Stage 1: modulo + compound and/or in the block language
- [#38](https://github.com/OmegaSoftDLL/lume/pull/38) ? merged: Stage 1: string literals + FizzBuzz in the block language (-> native)
- [#39](https://github.com/OmegaSoftDLL/lume/pull/39) ? merged: Stage 1: arrays in the block language (Sieve of Eratosthenes -> native)
- [#40](https://github.com/OmegaSoftDLL/lume/pull/40) ? merged: Stage 1: unary minus + bubble sort (block language -> native)
- [#41](https://github.com/OmegaSoftDLL/lume/pull/41) ? merged: Stage 1: elif in the block language
- [#42](https://github.com/OmegaSoftDLL/lume/pull/42) ? merged: Stage 1: for-loops in the block language
- [#43](https://github.com/OmegaSoftDLL/lume/pull/43) ? merged: Native x86-64 backend: var + while + assignment (no C)
- [#44](https://github.com/OmegaSoftDLL/lume/pull/44) ? merged: Native backend: for-loops, and/or/not, value if/elif/else
- [#45](https://github.com/OmegaSoftDLL/lume/pull/45) ? merged: test: bateria nativa x86-64 (6 programas, sem C)
- [#46](https://github.com/OmegaSoftDLL/lume/pull/46) ? merged: Fix native backend: flat if/elif/else value + top-level assignment
- [#47](https://github.com/OmegaSoftDLL/lume/pull/47) ? merged: ML-core codegen: lower enum constructors + match
- [#48](https://github.com/OmegaSoftDLL/lume/pull/48) ? merged: ML-core codegen: records + field access + builtins (full compiler goes native)
- [#49](https://github.com/OmegaSoftDLL/lume/pull/49) ? merged: ML-core codegen: lists + indexing (core feature-complete to native)
- [#50](https://github.com/OmegaSoftDLL/lume/pull/50) ? merged: Lume-written native codegen: ML-core -> x86-64 machine code (no C)
- [#51](https://github.com/OmegaSoftDLL/lume/pull/51) ? merged: Fix ML-core interpreter: modulo operator was missing
- [#52](https://github.com/OmegaSoftDLL/lume/pull/52) ? merged: Native codegen: boxed values + brk heap allocator
- [#53](https://github.com/OmegaSoftDLL/lume/pull/53) ? merged: Native codegen: strings (literals, equality, printing), no C
- [#54](https://github.com/OmegaSoftDLL/lume/pull/54) ? merged: Native codegen: constructors + match -> x86-64 (no C)
- [#55](https://github.com/OmegaSoftDLL/lume/pull/55) ? merged: Native codegen: records + lists -> x86-64 (no C)
- [#56](https://github.com/OmegaSoftDLL/lume/pull/56) ? merged: Native codegen: full core compiler (calc.src) -> x86-64, no C
- [#57](https://github.com/OmegaSoftDLL/lume/pull/57) ? merged: Seed: x86-64 assembler + static ELF writer (drop as/ld), first brick
- [#58](https://github.com/OmegaSoftDLL/lume/pull/58) ? merged: lume0 asm: full x86-64 encoder — whole pipeline runs with NO C and NO as/ld
- [#59](https://github.com/OmegaSoftDLL/lume/pull/59) ? merged: check: real name resolution — first semantic pass (AI-first loop made real)
- [#60](https://github.com/OmegaSoftDLL/lume/pull/60) ? merged: check: basic literal type checking against annotations (T0001)
- [#61](https://github.com/OmegaSoftDLL/lume/pull/61) ? merged: check: duplicate parameter and duplicate top-level function detection
- [#62](https://github.com/OmegaSoftDLL/lume/pull/62) ? merged: describe + status: reflect the real check and no-toolchain pipeline
- [#63](https://github.com/OmegaSoftDLL/lume/pull/63) ? merged: docs: AGENTS.md + PROJECT_STATE.md (full handoff for any agent)
- [#64](https://github.com/OmegaSoftDLL/lume/pull/64) ? merged: chore: professionalize the repo + show Lume as the language
- [#65](https://github.com/OmegaSoftDLL/lume/pull/65) ? merged: check: call arity checking for top-level functions (A0001/A0002)
- [#66](https://github.com/OmegaSoftDLL/lume/pull/66) ? merged: docs: professional README (badges, 100% Lume, honest status)
- [#67](https://github.com/OmegaSoftDLL/lume/pull/67) ? merged: docs: register the future Linguist submission task
- [#68](https://github.com/OmegaSoftDLL/lume/pull/68) ? merged: check: control-flow context (break/continue in loops, return in functions)
- [#69](https://github.com/OmegaSoftDLL/lume/pull/69) ? merged: check: assignment to an immutable (let/const) binding (M0001)
- [#70](https://github.com/OmegaSoftDLL/lume/pull/70) ? merged: describe: list all check diagnostics
- [#71](https://github.com/OmegaSoftDLL/lume/pull/71) ? merged: check: unreachable match arm after a catch-all (U0001)
- [#72](https://github.com/OmegaSoftDLL/lume/pull/72) ? merged: test: harden semantic checker coverage (64 resolve cases)
- [#73](https://github.com/OmegaSoftDLL/lume/pull/73) ? merged: docs: Phase 1 self-hosting plan (docs/11)
- [#74](https://github.com/OmegaSoftDLL/lume/pull/74) ? merged: check: unreachable code after return/break/continue (U0002)
- [#75](https://github.com/OmegaSoftDLL/lume/pull/75) ? merged: test: native-backend stress corpus (6 ML-core programs)
- [#76](https://github.com/OmegaSoftDLL/lume/pull/76) ? merged: check: M0001 also covers const (not just let)
- [#77](https://github.com/OmegaSoftDLL/lume/pull/77) ? merged: Fix native backend: 64-bit integer literals truncated to 32 bits
- [#78](https://github.com/OmegaSoftDLL/lume/pull/78) ? merged: seed: run on a large stack for deep recursion
- [#79](https://github.com/OmegaSoftDLL/lume/pull/79) ? merged: Fix three gaps in the native x86-64 backend (>6 params, print of data, heap overflow)
- [#80](https://github.com/OmegaSoftDLL/lume/pull/80) ? merged: Lex string escapes in the ML-core layout lexer (G2)
- [#81](https://github.com/OmegaSoftDLL/lume/pull/81) ? merged: Fix codegen: escape control chars in emitted string literals
- [#82](https://github.com/OmegaSoftDLL/lume/pull/82) ? merged: check: fix record/map key false positive + duplicate-key check (D0003)
- [#83](https://github.com/OmegaSoftDLL/lume/pull/83) ? merged: Core: native top-level let (G3) and read() builtin (G1)
- [#84](https://github.com/OmegaSoftDLL/lume/pull/84) ? merged: self-host R4: core lexer written in Lume-Core runs native (no C)
- [#85](https://github.com/OmegaSoftDLL/lume/pull/85) ? merged: self-host R5 (brick 1): core expression parser in Lume-Core, native
- [#86](https://github.com/OmegaSoftDLL/lume/pull/86) ? merged: self-host R5 (brick 2): function & constructor calls in core parser
- [#87](https://github.com/OmegaSoftDLL/lume/pull/87) ? merged: self-host R5 (brick 3): string literals in the core parser
- [#88](https://github.com/OmegaSoftDLL/lume/pull/88) ? merged: self-host R5 (brick 4): if/then/else and let/in in core parser
- [#89](https://github.com/OmegaSoftDLL/lume/pull/89) ? merged: self-host R5 (brick 5): field access in the core parser
- [#90](https://github.com/OmegaSoftDLL/lume/pull/90) ? merged: self-host R5 (brick 6): record literals in the core parser
- [#91](https://github.com/OmegaSoftDLL/lume/pull/91) ? merged: self-host R5 (brick 7): parse `and`/`or` in the core parser
- [#92](https://github.com/OmegaSoftDLL/lume/pull/92) ? merged: docs(readme): record the self-hosting rewrite (R-series) in Status
- [#93](https://github.com/OmegaSoftDLL/lume/pull/93) ? merged: docs(state): record the O(n^2) memory pitfall and the O(1) fixes
- [#94](https://github.com/OmegaSoftDLL/lume/pull/94) ? merged: self-host R5: parse full programs (fn + print) in the core parser
- [#95](https://github.com/OmegaSoftDLL/lume/pull/95) ? merged: self-host R6: a native self-hosted codegen (lumec.src), no C in the target pipeline
- [#96](https://github.com/OmegaSoftDLL/lume/pull/96) ? merged: self-host R6b: lumec codegen for strings, records, field access and constructors
- [#97](https://github.com/OmegaSoftDLL/lume/pull/97) ? merged: docs(readme): Status reflects the native self-hosted codegen (R6)
- [#98](https://github.com/OmegaSoftDLL/lume/pull/98) ? merged: self-host R6: match expressions in lumec (parse + x86-64 codegen)
- [#99](https://github.com/OmegaSoftDLL/lume/pull/99) ? merged: self-host R6c: lumec reads its source from stdin (file compiler)
- [#100](https://github.com/OmegaSoftDLL/lume/pull/100) ? merged: self-host: decode string escapes → lumec reaches the self-compilation fixpoint
- [#101](https://github.com/OmegaSoftDLL/lume/pull/101) ? merged: self-host: close the triple self-build on the verbatim lumec.src
- [#102](https://github.com/OmegaSoftDLL/lume/pull/102) ? merged: docs: record the self-host fixpoint (triple self-build reached)
- [#103](https://github.com/OmegaSoftDLL/lume/pull/103) ? merged: self-host: lasm — an x86-64 assembler + ELF64 writer in Lume-Core (slice 1)
- [#104](https://github.com/OmegaSoftDLL/lume/pull/104) ? merged: self-host: buffered emit + prove the zero-C self-host loop (lumec | lasm, no lume0 asm)
- [#105](https://github.com/OmegaSoftDLL/lume/pull/105) ? merged: docs: the zero-C steady-state loop is proven
- [#106](https://github.com/OmegaSoftDLL/lume/pull/106) ? merged: perf(lasm): assemble the whole compiler in ~16s (was ~4m40s), byte-identical
- [#107](https://github.com/OmegaSoftDLL/lume/pull/107) ? merged: lasm: 64-bit immediates (movabs) — full instruction coverage
- [#108](https://github.com/OmegaSoftDLL/lume/pull/108) ? merged: build: staged self-verifying bootstrap (make bootstrap), lume0 frozen as Stage 0
- [#109](https://github.com/OmegaSoftDLL/lume/pull/109) ? merged: docs(readme): Stage 1 self-hosting complete + known limitations
- [#110](https://github.com/OmegaSoftDLL/lume/pull/110) ? merged: feat(lumec): top-level expression statements → byte-perfect lasm + lookahead match delimiting
- [#111](https://github.com/OmegaSoftDLL/lume/pull/111) ? merged: lasm: real GAS local-label resolution (1f/1b nearest) — superior to the C oracle
- [#112](https://github.com/OmegaSoftDLL/lume/pull/112) ? merged: perf(runtime): length-aware slicen — assemble the compiler in ~2.8s (was ~16s)
- [#113](https://github.com/OmegaSoftDLL/lume/pull/113) ? merged: docs(readme): close the Phase-1 refinements — no open limitations
- [#114](https://github.com/OmegaSoftDLL/lume/pull/114) ? merged: docs(readme): rewrite the Status section (consolidate, fix terminology)
- [#115](https://github.com/OmegaSoftDLL/lume/pull/115) ? merged: ci(E2): gate the self-host fixpoint — run `make bootstrap` on every PR
- [#116](https://github.com/OmegaSoftDLL/lume/pull/116) ? merged: feat(lumec): comptime seed — compile-time evaluation folding to a constant (Phase 2, brick C1)
- [#117](https://github.com/OmegaSoftDLL/lume/pull/117) ? merged: docs: Phase 2 plan (docs/12) — comptime flagship + F/C/M/D/T/E tracks
- [#118](https://github.com/OmegaSoftDLL/lume/pull/118) ? merged: feat(cli): T1 — lume single CLI (build/run/fmt/test/doc) + native ldoc & ltest
- [#119](https://github.com/OmegaSoftDLL/lume/pull/119) ? merged: feat(lumec): comptime C2 — evaluate calls to pure user functions at compile time
- [#120](https://github.com/OmegaSoftDLL/lume/pull/120) ? merged: feat(data): add exact decimal values and arithmetic
- [#121](https://github.com/OmegaSoftDLL/lume/pull/121) ? merged: feat(lumec): F1 — named type/enum/record declarations + a gradual, AI-first checker
- [#122](https://github.com/OmegaSoftDLL/lume/pull/122) ? merged: feat(fmt): T2 — canonical Lume-Core AST formatter (lfmt.src)
- [#123](https://github.com/OmegaSoftDLL/lume/pull/123) ? merged: feat(lumec): F2 — optional type annotations on params/returns + gradual arg-type checking
- [#124](https://github.com/OmegaSoftDLL/lume/pull/124) ? merged: feat(pkg): T3 — modules (use bundling) + package manager (lmod.src, lume pkg)
- [#125](https://github.com/OmegaSoftDLL/lume/pull/125) ? merged: feat(lumec): C3 — generics via comptime type parameters (erased at runtime)
- [#126](https://github.com/OmegaSoftDLL/lume/pull/126) ? merged: docs: module-split plan (docs/13) — split lumec.src via lmod for parallel tracks
- [#127](https://github.com/OmegaSoftDLL/lume/pull/127) ? merged: feat(data): add schema-typed CSV tables
- [#128](https://github.com/OmegaSoftDLL/lume/pull/128) ? merged: docs(readme): "What works today" — showcase the shipped Phase-2 pillars
- [#129](https://github.com/OmegaSoftDLL/lume/pull/129) ? merged: feat(lumec): M1 — expose the AST as a first-class comptime value
- [#130](https://github.com/OmegaSoftDLL/lume/pull/130) ? merged: refactor: split lumec.src into modules (amalgamation) to unblock parallel tracks
- [#131](https://github.com/OmegaSoftDLL/lume/pull/131) ? merged: feat(data): add typed table query operations
- [#132](https://github.com/OmegaSoftDLL/lume/pull/132) ? merged: feat(lumec): M2 — hygienic macros and quasiquote (unquote + capture avoidance)
- [#133](https://github.com/OmegaSoftDLL/lume/pull/133) ? merged: ci(zero-c): name and prove the zero-C steady state adversarially
- [#134](https://github.com/OmegaSoftDLL/lume/pull/134) ? merged: docs: record completed D1-D3 handoff
- [#135](https://github.com/OmegaSoftDLL/lume/pull/135) ? merged: feat(lumec): C4a — comptime assert (compile-time checks, no runtime trace)
- [#136](https://github.com/OmegaSoftDLL/lume/pull/136) ? merged: feat(lumec): F3a — local return-type inference (gradual, toward HM)
- [#137](https://github.com/OmegaSoftDLL/lume/pull/137) ? merged: docs: keep future data scope open
- [#138](https://github.com/OmegaSoftDLL/lume/pull/138) ? merged: feat(cli): E1a — compiler-as-a-service (lume compile --json/--run)
- [#139](https://github.com/OmegaSoftDLL/lume/pull/139) ? merged: feat(lumec): C4b — comptime precomputed tables (generating fn folded away)
- [#140](https://github.com/OmegaSoftDLL/lume/pull/140) ? merged: feat(e1b): add in-process compiler-as-a-service library (lcaas) and lume agent query protocol
- [#141](https://github.com/OmegaSoftDLL/lume/pull/141) ? merged: feat(lumec): F3b — argument inference for int/str (gradual, additive)
- [#142](https://github.com/OmegaSoftDLL/lume/pull/142) ? merged: feat(lumec): F3 — infer let-bound int/str variables for argument checks
- [#143](https://github.com/OmegaSoftDLL/lume/pull/143) ? merged: feat(lumec): F4 slice 1 — core accepts surface named enum payloads
- [#144](https://github.com/OmegaSoftDLL/lume/pull/144) ? merged: test(lumec): harden F2/F3 — lock zero-false-positive on valid mixed-annotation code
- [#145](https://github.com/OmegaSoftDLL/lume/pull/145) ? closed: feat(lumec): F4 slice 2 — string interpolation ("hi {name}")
- [#146](https://github.com/OmegaSoftDLL/lume/pull/146) ? merged: feat(lumec): F3 — extend gradual inference to nominal (enum/record) types
- [#147](https://github.com/OmegaSoftDLL/lume/pull/147) ? merged: feat(e3): add lume evolve self-evolution engine (use levolve) and governance loop
- [#148](https://github.com/OmegaSoftDLL/lume/pull/148) ? merged: feat(lumec): F3c — structural field-access checking on anonymous records
- [#149](https://github.com/OmegaSoftDLL/lume/pull/149) ? merged: feat(lumec): F3c — infer record field types and check them as arguments
- [#150](https://github.com/OmegaSoftDLL/lume/pull/150) ? merged: docs: Phase 2 status capstone — shipped vs honestly open
- [#151](https://github.com/OmegaSoftDLL/lume/pull/151) ? closed: feat(lumec): F3c — Hindley-Milner inference engine (unification) for unannotated parameters
- [#152](https://github.com/OmegaSoftDLL/lume/pull/152) ? merged: feat(lumec): F4 slice 3 — surface elif chains desugar to the core's nested If AST
- [#153](https://github.com/OmegaSoftDLL/lume/pull/153) ? merged: feat(phase2): add Stage 2 capstone app (examples/apps/inventory) and Phase 3 plan
- [#154](https://github.com/OmegaSoftDLL/lume/pull/154) ? merged: feat(f3c): Hindley-Milner unification engine for unannotated functions
- [#155](https://github.com/OmegaSoftDLL/lume/pull/155) ? merged: docs: F4 front-end unification plan (handoff reference)
- [#156](https://github.com/OmegaSoftDLL/lume/pull/156) ? merged: feat(f4): unify surface layout, explicit-brace, and core inline front-ends
- [#157](https://github.com/OmegaSoftDLL/lume/pull/157) ? merged: docs(phase3): add Track A — AI-native authoring (holes, totality, provenance)
- [#158](https://github.com/OmegaSoftDLL/lume/pull/158) ? merged: feat(stage1): Phase 3 Track O ownership, borrow checker, regions & memory tiers
- [#159](https://github.com/OmegaSoftDLL/lume/pull/159) ? merged: feat(stage1): Phase 3 Track A AI-native authoring (typed holes, total fn, provenance)
- [#160](https://github.com/OmegaSoftDLL/lume/pull/160) ? merged: docs(readme): update Status — Phase 2 complete, Phase 3 in progress
- [#161](https://github.com/OmegaSoftDLL/lume/pull/161) ? merged: feat(phase3): Track K effect system, algebraic handlers & Perceus reuse (K1-K3)
- [#162](https://github.com/OmegaSoftDLL/lume/pull/162) ? merged: docs: interop plan (docs/20) — C-ABI + WASM, AI bindings, provenance-guarded
- [#163](https://github.com/OmegaSoftDLL/lume/pull/163) ? merged: feat(phase3): Track S structured concurrency, channels & supervised actors (S1-S3)
- [#164](https://github.com/OmegaSoftDLL/lume/pull/164) ? merged: chore(governance): add CODEOWNERS and PR template
- [#165](https://github.com/OmegaSoftDLL/lume/pull/165) ? merged: feat(phase3): Track V contract verification (lume prove & LAWS) + Stage 3 Capstone (job_server)
- [#166](https://github.com/OmegaSoftDLL/lume/pull/166) ? merged: ci(security): add Dependabot, CodeQL & mandatory lume fmt --check / zero-c gates (Lote 2)
- [#168](https://github.com/OmegaSoftDLL/lume/pull/168) ? merged: ci(deps): bump actions/checkout from 4 to 7
- [#169](https://github.com/OmegaSoftDLL/lume/pull/169) ? merged: ci(deps): bump github/codeql-action from 3 to 4
- [#170](https://github.com/OmegaSoftDLL/lume/pull/170) ? merged: docs: inflection plan — dogfooding Lume into the compiler
- [#171](https://github.com/OmegaSoftDLL/lume/pull/171) ? merged: fix(effects): lower inter-procedural effect handler calls across function boundaries (#167)
- [#172](https://github.com/OmegaSoftDLL/lume/pull/172) ? merged: feat(stage-a): dogfood V1 requires/ensures contracts into Lexer, Parser, and Typecheck passes
- [#174](https://github.com/OmegaSoftDLL/lume/pull/174) ? merged: feat(stage1): graduate verifier to sound QF_LIA decision procedure (Inflection Stage B, closes #173)
- [#175](https://github.com/OmegaSoftDLL/lume/pull/175) ? merged: feat(compiler): dogfood algebraic effects in 07_driver CompilerIO boundary (Inflection Stage C)
- [#176](https://github.com/OmegaSoftDLL/lume/pull/176) ? merged: feat(compiler): dogfood own and Perceus in-place reuse in lumec (Inflection Stage D)
- [#177](https://github.com/OmegaSoftDLL/lume/pull/177) ? merged: ci(release): add CHANGELOG.md and self-hosted release workflow (Lote 3)
- [#178](https://github.com/OmegaSoftDLL/lume/pull/178) ? merged: feat(oracle): hole-guided QF_LIA + law synthesis loop (docs/19-intent-as-source.md)
- [#179](https://github.com/OmegaSoftDLL/lume/pull/179) ? merged: docs: adoption plan (docs/22) — AI-first go-to-market
- [#180](https://github.com/OmegaSoftDLL/lume/pull/180) ? merged: docs: horizon (docs/23) — proving whole systems that cannot rot
- [#181](https://github.com/OmegaSoftDLL/lume/pull/181) ? merged: feat(phase4): Stage 4 AI, tensors, autodiff, kernels, prob, rules, llm & spiking neuro
- [#182](https://github.com/OmegaSoftDLL/lume/pull/182) ? merged: feat(phase5): implement Stage 5 Systems, Interop, Realtime WCET, Proof Certificates & lume-os
- [#187](https://github.com/OmegaSoftDLL/lume/pull/187) ? merged: docs(readme): refresh status to Phases 0–5 complete + inflection (honest)
- [#189](https://github.com/OmegaSoftDLL/lume/pull/189) ? merged: fix(audit): independent QF_LIA proof certificate kernel + FIPS 180-4 SHA-256 + autodiff [N0400] + WCET budget_checked + wasm host fs gate (#183, #184, #185, #186)
- [#190](https://github.com/OmegaSoftDLL/lume/pull/190) ? merged: docs(honesty): state every feature at its real level (shipped / model / planned)
- [#199](https://github.com/OmegaSoftDLL/lume/pull/199) ? merged: fix(seed): close CodeQL command-injection (elf.c) + snprintf overflow (interp.c)
- [#200](https://github.com/OmegaSoftDLL/lume/pull/200) ? merged: feat(protect): implement P33 software protection Bricks W1-W5 (#188)
- [#201](https://github.com/OmegaSoftDLL/lume/pull/201) ? merged: feat(evolve,oracle): cryptographic evolve governance (#194) & witness/repair/impact queries (#195)
- [#202](https://github.com/OmegaSoftDLL/lume/pull/202) ? merged: docs(honesty): lume evolve 'signature' is forgeable, not cryptographic auth (#194)
- [#207](https://github.com/OmegaSoftDLL/lume/pull/207) ? merged: docs: universal-targets architecture (docs/26) — every ISA/platform, proof-preserving
- [#218](https://github.com/OmegaSoftDLL/lume/pull/218) ? merged: feat(caas): add P35 public test coverage warnings
- [#219](https://github.com/OmegaSoftDLL/lume/pull/219) ? merged: docs: landing page (docs/index.md) for GitHub Pages
- [#220](https://github.com/OmegaSoftDLL/lume/pull/220) ? merged: feat(evolve,cli,seed): RFC 8032 Ed25519 auth & apply loop (#194, #203), self-hosted lcli.src (#204), and C seed freeze (#205)
- [#221](https://github.com/OmegaSoftDLL/lume/pull/221) ? merged: feat(editor): add Lume TextMate grammar and Linguist prep
- [#223](https://github.com/OmegaSoftDLL/lume/pull/223) ? merged: feat(comptime,evolve): C4c generics over declared containers (#212) and RFC 8032 Section 7.1 vectors (#222)
- [#224](https://github.com/OmegaSoftDLL/lume/pull/224) ? merged: feat(data,tooling,ai): Track D4 persistence/money/soa (#214), LSP/DAP (#210), REPL/Jupyter kernel (#211), AI substrate (#209), and survey cleanup (#215)
- [#225](https://github.com/OmegaSoftDLL/lume/pull/225) ? merged: docs(honesty): #224 lsp/dap/repl are one-shot & persist is a model (not servers/durable)
- [#226](https://github.com/OmegaSoftDLL/lume/pull/226) ? merged: docs: systems & kernels plan (docs/27) — everything Lume needs for real bare-metal
- [#228](https://github.com/OmegaSoftDLL/lume/pull/228) ? merged: docs: delivery hardening rule — no-theater checklist for every PR (AGENTS + docs/28)
- [#229](https://github.com/OmegaSoftDLL/lume/pull/229) ? merged: feat(stage4,stage5): real jit specialisation, kernel/c-export AST lowering, prob/datalog & Lean 4 proofs (#192, #213, #191)
- [#231](https://github.com/OmegaSoftDLL/lume/pull/231) ? merged: docs(07): GitHub name-homonym scan, positioning, and LLM-generation eval gap (#230)
- [#232](https://github.com/OmegaSoftDLL/lume/pull/232) ? merged: docs(27): kernel/OS track sequencing & entry point (#227)
- [#233](https://github.com/OmegaSoftDLL/lume/pull/233) ? merged: docs(29): ordered execution plan for the open worklist (agy's sequence)
- [#234](https://github.com/OmegaSoftDLL/lume/pull/234) ? merged: feat(tooling,data,eval): durable WAL persist, live LSP/DAP stdio servers, interactive REPL/Jupyter & LLM-gen eval gate (#214, #210, #211, #230)
- [#235](https://github.com/OmegaSoftDLL/lume/pull/235) ? merged: docs(11): reconcile seed-C policy with docs/14 — frozen off the loop, never deleted
- [#236](https://github.com/OmegaSoftDLL/lume/pull/236) ? merged: feat(interop): implement real FFI linking & C-ABI/WASM export (#191)
- [#237](https://github.com/OmegaSoftDLL/lume/pull/237) ? merged: feat(kernel,concurrency): K0-K9 bare-metal QEMU kernel (#227) & OS parallel runtime (#193)
- [#238](https://github.com/OmegaSoftDLL/lume/pull/238) ? merged: feat(targets): orthogonal universal targets backend T0-T5 & LUMB VM (#206, #215)
- [#239](https://github.com/OmegaSoftDLL/lume/pull/239) ? merged: feat(core): implement #197 core language features and #215 concurrency/target items
- [#240](https://github.com/OmegaSoftDLL/lume/pull/240) ? merged: docs(honesty): relabel targets & kernel to their real level (run-verified vs emit-only)
- [#242](https://github.com/OmegaSoftDLL/lume/pull/242) ? merged: docs(28): harden the rule — deliver the real construct, not a mock of it
- [#243](https://github.com/OmegaSoftDLL/lume/pull/243) ? merged: feat(systems,targets): bare-metal K6-K9 + SMP (#227, #193) & target run-verification (#206)
- [#244](https://github.com/OmegaSoftDLL/lume/pull/244) ? merged: docs(honesty): label rc/arc/weak & promise/signal as MODELS, not the typed features (#197/#215)
- [#245](https://github.com/OmegaSoftDLL/lume/pull/245) ? merged: feat(compiler): implement compiler-level rc[T]/arc[T]/weak[T] and promise[T]/signal[T] (#197, #215)
- [#246](https://github.com/OmegaSoftDLL/lume/pull/246) ? merged: feat(adoption,horizon): implement G0-G9 adoption & H1-H5/docs-19 horizon (#196, #198)
- [#247](https://github.com/OmegaSoftDLL/lume/pull/247) ? merged: docs(28): forbid fit-to-expected; require per-observation assertions
- [#248](https://github.com/OmegaSoftDLL/lume/pull/248) ? merged: docs(23): relabel H1 'temporal' → relational (QF_LIA can't express temporal logic)
- [#249](https://github.com/OmegaSoftDLL/lume/pull/249) ? merged: fix(lcaas): reword 'temporal laws' compose error → 'relational laws'
- [#251](https://github.com/OmegaSoftDLL/lume/pull/251) ? merged: docs: fix audit-found inaccuracies (kernel #8→#250; stale 'not implemented' notes)
- [#252](https://github.com/OmegaSoftDLL/lume/pull/252) ? merged: docs(readme): refresh Status to verified current state
- [#253](https://github.com/OmegaSoftDLL/lume/pull/253) ? merged: docs(28): §7 — the hard line: zero placeholders, everything documented runs, exhaustive tests
- [#254](https://github.com/OmegaSoftDLL/lume/pull/254) ? merged: docs(32): AI-authoring onboarding guide (install + edit→check→prove→fix)
- [#260](https://github.com/OmegaSoftDLL/lume/pull/260) ? merged: feat(kernel,temporal,codegen): SSA PTX/SPIR-V lowering & VM exec, LTL temporal logic & [C0001] (#250, #198)
- [#261](https://github.com/OmegaSoftDLL/lume/pull/261) ? merged: docs(33): the plan to build anything with AI — frontiers, multipliers, order
- [#263](https://github.com/OmegaSoftDLL/lume/pull/263) ? merged: feat(install): network one-liner installer (GitHub Releases, Pages-served, self-updating)
- [#264](https://github.com/OmegaSoftDLL/lume/pull/264) ? merged: feat(release): publish multi-platform release archives with CI run-verification (#262)
- [#265](https://github.com/OmegaSoftDLL/lume/pull/265) ? merged: fix(release): use exact bootstrap/lume0 C sources in release.yml and preserve cross-built lume0 (#262)
- [#266](https://github.com/OmegaSoftDLL/lume/pull/266) ? merged: feat(prover): implement Multiplier M1 prover reach beyond QF_LIA (#255)
- [#267](https://github.com/OmegaSoftDLL/lume/pull/267) ? merged: feat(evolve,cli): multi-file self-growth engine across target, prover & stdlib domains (#258)
- [#268](https://github.com/OmegaSoftDLL/lume/pull/268) ? merged: feat(target): universal reach across GPU, MCU, FPGA/HDL, mobile, and web (#256)
- [#270](https://github.com/OmegaSoftDLL/lume/pull/270) ? draft: plan: P35 example coverage and strict enforcement
- [#272](https://github.com/OmegaSoftDLL/lume/pull/272) ? draft: plan: Capability-gated compile-time file embedding
- [#274](https://github.com/OmegaSoftDLL/lume/pull/274) ? draft: plan: Derive table schemas from nominal row types
- [#276](https://github.com/OmegaSoftDLL/lume/pull/276) ? draft: plan: Binary WASM hosting and capability sandbox
- [#278](https://github.com/OmegaSoftDLL/lume/pull/278) ? draft: plan: Submit Lume to GitHub Linguist after maturity gate
- [#280](https://github.com/OmegaSoftDLL/lume/pull/280) ? draft: plan: GPU hardware dispatch and CPU/GPU capstone
- [#282](https://github.com/OmegaSoftDLL/lume/pull/282) ? draft: plan: Complete adoption and distribution acceptance
- [#284](https://github.com/OmegaSoftDLL/lume/pull/284) ? draft: plan: Complete software protection and secrets requirements
- [#286](https://github.com/OmegaSoftDLL/lume/pull/286) ? draft: plan: Semantic edits, effect replay and full provenance
- [#288](https://github.com/OmegaSoftDLL/lume/pull/288) ? draft: plan: Complete horizon proof and portability goals
- [#290](https://github.com/OmegaSoftDLL/lume/pull/290) ? draft: plan: Prover theories and independent benchmark acceptance
- [#292](https://github.com/OmegaSoftDLL/lume/pull/292) ? draft: plan: Universal targets independent execution and proof coverage
- [#294](https://github.com/OmegaSoftDLL/lume/pull/294) ? draft: plan: Remaining language and runtime roadmap features
- [#296](https://github.com/OmegaSoftDLL/lume/pull/296) ? draft: plan: Complete AI provider and Stage 4 roadmap acceptance
- [#298](https://github.com/OmegaSoftDLL/lume/pull/298) ? draft: plan: Stage 5 OS capstone and machine-checked invariants
- [#300](https://github.com/OmegaSoftDLL/lume/pull/300) ? draft: plan: Reconcile Stage 1 checker and assembler checklist
- [#302](https://github.com/OmegaSoftDLL/lume/pull/302) ? draft: plan: Reconcile documentation status and normative core
- [#303](https://github.com/OmegaSoftDLL/lume/pull/303) ? draft: plan: Standard library and domains execution plan
- [#304](https://github.com/OmegaSoftDLL/lume/pull/304) ? draft: plan: Intent as source execution plan
- [#305](https://github.com/OmegaSoftDLL/lume/pull/305) ? draft: plan: General-purpose OS execution plan
- [#307](https://github.com/OmegaSoftDLL/lume/pull/307) ? draft: plan: Complete autonomous self-growth and governance
- [#309](https://github.com/OmegaSoftDLL/lume/pull/309) ? draft: plan: Complete language specification conformance
- [#311](https://github.com/OmegaSoftDLL/lume/pull/311) ? draft: plan: Incremental compilation and feedback performance
- [#313](https://github.com/OmegaSoftDLL/lume/pull/313) ? draft: plan: Structured LBE constrained decoding and compact format
- [#315](https://github.com/OmegaSoftDLL/lume/pull/315) ? draft: plan: Reconcile historical backend and seed parity limitations
