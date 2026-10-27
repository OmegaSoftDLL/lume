# 04 — Roadmap: from zero to a self-evolving language

Lume is built in **stages**. Each stage has a deliverable, an exit criterion that is a test, and a
list of spec sections it implements. Nothing from a later stage blocks an earlier one; later stages
are macro libraries over the core wherever possible (P23).

Bootstrap rule (P27): **no third language stays in the loop.** Stage 0 is a small C interpreter
that exists to run Stage 1 once. From Stage 1 on, Lume is written in Lume.

---

## Stage 0 — `lume0`: the disposable bootstrap interpreter (C)

**Goal:** run Lume programs written in the core subset (spec §1–§9) well enough to execute the
Stage 1 compiler's source.

**Why C:** builds on every machine with no dependencies; readable by anyone; it is the language of
every existing kernel, so the reproducibility root is as low-level and universal as it gets. It is
*not* the implementation language of Lume — it is the seed. Once Stage 1 compiles itself, `lume0`
is frozen; it stays in `bootstrap/` for anyone who wants to rebuild Lume from scratch (the
"trusting trust" defence).

**Deliverables** (`bootstrap/lume0/`):
1. `lexer.c` — UTF-8 source → tokens, with `NEWLINE/INDENT/DEDENT`, all literal forms of §1.4,
   diagnostics `L*`.
2. `parser.c` — tokens → AST for §2–§9 (modules, `use`, `type record/enum/error`, `fn` with
   `requires/ensures`, traits/impls (basic), statements, expressions, `match`, closures,
   comprehensions, interpolation). Diagnostics `P*`.
3. `check.c` — name resolution, basic type inference (locals, literals, calls), exhaustiveness of
   `match` on enums, arity/named-argument checking. Diagnostics `T*` (a subset). *No* ownership
   checking in Stage 0 (it interprets by value with reference counting internally; safety is
   checked by Stage 1).
4. `interp.c` — tree-walking interpreter: values (int/float/decimal/bool/char/str/list/map/set/
   tuple/record/enum/closure/optional/error), UFCS, `?`, `fail`, `defer`, `match`, contracts
   (`requires`/`ensures`/`invariant` checked at runtime), string interpolation, ranges,
   comprehensions, `using`, `assert`, `panic`, `todo`.
5. `std0/` — a minimal prelude in Lume (`Option`/`Result` helpers, `Iter` adaptors, `Show`) plus
   builtins in C: `io.println/print/read_line`, `fs.read/write`, `str.*`, `math.*`, `list.*`,
   `map.*`, `env.args`, `process.exit`, `time.now`.
6. `lume0` CLI: `lume0 run file.lume`, `lume0 tokens`, `lume0 parse --json`, `lume0 check --json`.
7. Tests: `tests/conformance/stage0/*.lume` with expected output; `make test`.

**Exit criterion:** `lume0 run compiler/lumec.lume -- compiler/lumec.lume` produces a working
`lumec` (Stage 1), and every Stage-0 conformance test passes.

**Size budget:** ≤ 8,000 lines of C. If it grows past that, features move to Stage 1.

---

## Stage 1 — `lumec`: the real compiler, written in Lume

**Goal:** a self-hosting compiler for the core subset plus the Stage-1 features, producing native
binaries.

**Deliverables** (`compiler/`), all in Lume:
1. `lexer.lume`, `parser.lume`, `ast.lume` — complete surface syntax (§1–§9 plus reserved syntax
   parsed and rejected with "not yet implemented" diagnostics), the explicit-brace form, the
   formatter (`lume fmt`) as the normative pretty-printer of the AST.
2. `resolve.lume`, `types.lume` — full local inference (bidirectional), generics with
   monomorphisation, traits with associated types/defaults, `dyn`, derive macros for
   `Eq/Ord/Hash/Show/Clone/Default`.
3. `effects.lume` — effect inference and checking (no user handlers yet), capability manifest.
4. `core.lume` — desugaring of the surface to `lume-core` IR (§16); `lume core` prints it.
5. `interp.lume` — the core interpreter, used for `comptime`, the REPL and the debug tier.
6. `codegen/` — `lume-core` → **C** as the first native backend (portable, fast to write, uses
   the system C compiler; also how Stage 1 produces its own first binary). Then a direct
   machine-code backend (**x86-64 and AArch64**, via our own small backend in Lume — no LLVM
   dependency for the debug tier) **[1.5]**.
7. `lume` CLI: `new build run test fmt doc repl core expand`, `--json` diagnostics (§17),
   `lume agent` (compiler-as-a-service over stdio JSON-RPC: parse/check/symbols/fix-its/holes).
8. `std/` — the standard library core: `core, io, fs, fmt, str, collections, math, decimal, time,
   rand, json, regex, test, bench`.
8b. **Tooling for the AI-author loop and the C ecosystem (ship with the first compiler):** `lume lsp`
    (language server) and `lume dap` (debug adapter) so agents get interactive diagnostics; `import c
    "header.h"` and `@export("C")` for FFI and incremental migration. (Scan: NanoLang, Carbon, Zig, Mojo.)
8c. **P35 — mandatory tests:** the compiler reports a `pub` function with no test/example (warning in
    debug, error under `--strict`).
9. Diagnostics style guide (`docs/diagnostics.md`) and the first 100 diagnostics written to it.

**Exit criterion (the triple build):** `lume0` runs `lumec` to compile `lumec` → binary **A**;
**A** compiles `lumec` → **B**; **B** compiles `lumec` → **C**; `B == C` byte for byte. All Stage
0–1 conformance tests pass under **C**. `lume0` is then frozen.

**Performance gate:** debug build of the compiler's own largest module < 1 s; `lume run` of a
500-line script < 200 ms end-to-end on the reference laptop (the 2012 MacBook Air this project
started on).

---

## Stage 2 — Data, comptime, macros, package manager

1. **`comptime` + hygienic `macro`** (§12), `when`, `embed`, type providers (`schema`).
   From here, new syntax is added as macros in `std` whenever possible (P23).
2. **`record`/`table`/`index`/`query`/`relation`** (§11) as macros + `std.data` engine: in-memory
   columnar tables, vectorised query plans, CSV/JSON/Parquet I/O, `persist` (an embedded ACID
   store with a write-ahead log), adapters (SQLite first, then PostgreSQL, DuckDB).
3. `decimal`, `date/time/duration/money` literals and arithmetic (`std.decimal`, `std.time`).
4. **`lume pkg`** *(built today: per-file SHA-256 lockfile + capability audit only; the content-addressed store, signed packages, registry protocol and AST-hash definitions below are **planned**, not built)*: content-addressed store (`~/.lume/store/<hash>`), `lume.toml` written in the
   restricted Lume config subset, lockfiles, enforced semantic versioning via API diff, capability
   grants per dependency (capabilities declared in package metadata and checked, scan: Austral),
   signed packages, a registry protocol (self-hostable). **Content-address definitions by AST hash**
   (scan: Unison) so tests and proofs are cached per definition and renames are non-breaking.
5b. **`LAWS` (P34):** project-level invariants compiled to obligations checked on every build.
5. `shape` structural types, multiple-dispatch trait parameters, properties, `report`.
6. *(planned, not built)* REPL with tables as first-class values (the dot prompt reborn), notebooks protocol (Jupyter kernel).
7. **Exit criterion:** a complete business application (inventory + invoicing + reports) in
   `examples/apps/inventory/` under 1,500 lines, with tests, built only with `lume`.

## Stage 3 — Ownership, concurrency, effects with handlers, GC regions

1. **Ownership and borrow checker** on `lume-core` with region inference and the `from` clause;
   `rc/arc/cell/weak`; `Drop`; `Send/Sync` inference. Diagnostics `O*` with fix suggestions.
2. **Structured concurrency** (§10): `scope`, `spawn`, `chan`, `select`, work-stealing runtime,
   `par for/map/reduce`; `std.actor` with supervisors; `std.time.logical`.
3. **Effect handlers** (`effect`/`with handler`), `async` as an effect (no colouring), `yield`
   generators via handlers; `sample`/`observe`/`infer` are handlers too (scan: Koka). Add
   **Perceus-style reuse analysis** so functional updates happen in place without a GC (scan: Koka).
4. `@gc` regions (a precise, incremental collector scoped to declared types).
5. Allocator contexts (`with alloc`), `freestanding`-ready `std.collections`.
6. `lume prove` v0: contracts on the pure subset discharged with an SMT solver (Z3/cvc5 through FFI);
   `requires/ensures` become proof obligations where possible, runtime checks otherwise.
7. **Exit criterion:** an HTTP server + worker pool + actor-based job system in `examples/apps/`
   with zero `unsafe`, passing a stress test with the race detector finding nothing (because the
   type system already did).

## Stage 4 — AI: tensors, autodiff, kernels, probability, rules, LLM functions

1. `tensor[T, dims…]` with symbolic shapes in the type checker (§14); `std.tensor` CPU kernels
   (SIMD via `std.simd`), BLAS via FFI.
2. **Autodiff as an IR transformation** on `lume-core`: `grad/vjp/jvp/vmap` over pure functions
   with control flow; `jit` through the native backend.
3. `kernel fn` → GPU: first via SPIR-V/Vulkan compute and CUDA PTX emitted from `lume-core`;
   `@schedule`, `@autotune`; later an MLIR bridge **[4.5]** for TPU/NPU dialects.
4. `model` types, `std.nn` (layers, optimisers, data loaders), safetensors/ONNX import-export.
5. `prob` effect + `std.prob` (MCMC, SVI, SMC handlers).
6. `rule` blocks → Rete/semi-naive Datalog over `table`s (`std.rules`).
7. `extern llm fn` + `std.llm`: schema-constrained decoding, **plus a schema-aligned parsing fallback
   for providers without constrained decoding** (scan: BAML); retries and a repair loop driven by the
   same JSON diagnostics; budgets, caching, provider handlers (local models through llama.cpp FFI
   first — this project values running offline); **an eval + record/replay mocking framework** so
   `test llm` runs deterministic regression suites over prompts and model versions.
8. `std.neuro`: spiking networks as actors on logical time.
9. **Exit criteria:** (a) MNIST trained end-to-end in pure Lume on CPU and GPU from the same
   source; (b) an agent in `examples/apps/agent/` with typed tools, `table` memory, an `llm fn`
   planner, supervision, and an eval suite — all in one program.

## Stage 5 — Machine level, operating system, verification

1. `@repr/@packed/@bits/@mmio`, `asm`, `import c "header.h"`, `@export("C")`, cross-compilation
   matrix (x86-64, AArch64, RISC-V; Linux, Windows, macOS, WASI, freestanding).
2. `interrupt` and `realtime` effects; `@bound`; WCET estimation in `lume prove`.
3. Capability types (`std.os.cap`), the **platform/application** split (`platform`/`app` items).
4. **`lume-os`**: a capability-based microkernel in Lume for x86-64 and RISC-V (QEMU first):
   boot, memory, scheduler, IPC by typed channels, drivers as platforms, a Lume REPL as the
   shell with the filesystem and process table as `table`s, software-isolated processes for
   verified-safe Lume apps, a GPU-aware model service.
5. `lume-core` mechanised in Lean (semantics + type soundness); the verified core becomes the
   trust anchor for `lume prove` and for self-evolution.
6. **Exit criteria (NOT met — planned):** the `lume-os` capstone is a deterministic user-space model of kernel services, with no boot, QEMU or hardware; the target is that `lume-os` boots in QEMU, runs the Stage 4 agent example as a user app, and
   the kernel's memory manager and scheduler have machine-checked proofs of their contracts.

---

## Stage 2+ in parallel — `lume evolve`: self-evolution and its governance

The language is "ready" when `lumec` compiles itself (Stage 1 exit). From that moment Lume evolves
through its own tooling. This is the part of the project that is genuinely new, so it is specified
as carefully as the type system.

### What can evolve
1. **The compiler and standard library** (`compiler/`, `std/`): any change.
2. **The surface language**: new sugar, implemented as macros in `std` (P23) — no core change.
3. **`lume-core`**: only with an RFC, a spec change and updated proofs (rare, human-approved).
4. **The specification and this roadmap**: as RFCs.

### How a change is proposed
A proposal is a **Lume program** (`evolve/proposals/<id>/proposal.lume`) that:
- states intent (`## doc` + a `spec` block with examples and property tests that must pass);
- uses `lume agent` (the compiler as a service) to transform source trees — never raw text edits;
- declares which principles (`02-principles.md`) it serves and which it strains.

Proposals may be written by humans or by AI models (`lume evolve propose <bundle>` or `lume evolve propose "<goal>"`). The flow gives the model the structured compiler API, the spec, the diagnostics and the test results of its
attempts (the write → compile → read-errors → fix loop, P1/P30/P31).

### The gates (all automatic, all required)
> **Implemented today:** six gates in `compiler/stage1/levolve.src` and `./lume` — `compile`, `conformance` (including static `QF_LIA` `proofs_ok` contract/law verification), `api_compat`, `capability`, `budget` (including `benchmark_ok` `@bound` cycle-budget verification and assembly line budgets), and `fixpoint` (`stage2 == stage3` multi-stage codegen fixpoint + binary equality check when bootstrap binaries are present).

1. **Conformance**: every test in `tests/conformance/` passes for the edition.
2. **Triple self-build**: `A → B → C`, `B == C` byte-identical, from `lume0` as the root.
3. **Proofs**: `lume prove` on `std` contracts and the `lume-core` Lean development still check.
4. **Benchmarks**: no regression beyond the configured budget (`evolve/budgets.lume`); compile-time
   and runtime.
5. **API diff**: breaking changes to `pub` items require a new edition and a migrator.
6. **Size budget** (P24): keyword count and spec length within budget; adding requires removing.
7. **Capability audit**: no new effects granted to `std` modules without an RFC.

### The human role
- Maintainers set **direction** (`evolve/goals.lume`: ranked goals with acceptance tests), review
  the proposals that pass all gates, and **veto**. They do not need to write the code; they need
  to decide what Lume should become.
- The enforced merge gate is **GitHub branch protection** (only the account can merge to `main`; external contributors cannot push). On top of that, `lume evolve approve --by <maintainer>` records an authorized-maintainer approval and a SHA-256 digest over the proposal/candidate (`levolve.src`). **Note:** that digest is **not yet a real signature** — the key is derived from the public maintainer name, so it is forgeable, and the `approved:<name>` path needs no digest at all; real asymmetric (Ed25519) maintainer signatures are tracked in `#194`. The AI proposes; the gates verify; the human
  decides. This is deliberate: a language is a social contract with its users, not an optimisation
  target.

### Why this is safe to attempt
- The core is small and verified, so the blast radius of a wrong change is bounded by the proofs.
- Editions mean existing code never breaks silently.
- The reproducibility root (`lume0` in C) means any state of the language can be rebuilt from
  scratch and audited — a self-evolving compiler cannot hide a backdoor (the triple build plus a
  from-C rebuild detects Thompson's "trusting trust" attack).

### Metrics the evolution optimises (in order)
1. Programs that compile have the meaning the author intended (bug classes eliminated).
2. Time from intent to running program (for humans and AI).
3. Size of the surface language (smaller is better at equal power).
4. Performance of generated code.
5. Breadth of targets.

---

## Milestone calendar (indicative, from 2026-10-24)

| When | Milestone |
|---|---|
| Month 1 | Stage 0 lexer/parser/interpreter run the examples; conformance stage0 green |
| Month 3 | Stage 1 `lumec.lume` parses and type-checks itself under `lume0` |
| Month 5 | Stage 1 C backend; **triple build** green; `lume0` frozen; `lume` CLI + `fmt` + `agent` |
| Month 8 | Stage 2 `comptime`/macros, `table`/`query`, `lume pkg`; inventory app example |
| Month 12 | Stage 3 ownership checker, structured concurrency, effects; HTTP/actor example |
| Month 16 | Stage 4 tensors + autodiff on CPU; MNIST in Lume |
| Month 20 | Stage 4 GPU kernels, `llm fn`, agent example; `lume evolve` running on `std` |
| Month 24 | Stage 5 `lume-os` boots in QEMU; `lume-core` mechanised |

Dates will slip; the order and the exit criteria will not.

---

## Repository layout (target)

```
lume/
├── README.md
├── docs/                 00-catalog, 01-survey, 02-principles, 03-specification, 04-roadmap,
│                         05-vision, diagnostics.md, core/ (formal semantics), rfcs/
├── grammar/lume.ebnf
├── examples/             runnable (stage-tagged) and apps/
├── bootstrap/lume0/      Stage 0 — C, frozen after Stage 1
├── compiler/             Stage 1+ — lumec in Lume
├── std/                  standard library in Lume
├── tests/conformance/    normative tests, stage-tagged
├── evolve/               goals, budgets, proposals, gates (Stage 2+)
└── os/                   lume-os (Stage 5)
```
