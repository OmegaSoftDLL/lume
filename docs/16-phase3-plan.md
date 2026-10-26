# 16 — Phase 3 plan (Stage 3: ownership, effects with handlers, structured concurrency, verification)

Phase 2 delivered Lume's compile-time metaprogramming (`comptime`, hygienic quasiquote macros),
exact data engine (`decimal(p, s)`, `table[T]`, compile-time checked `query`), single-binary
tooling (`lume build|run|test|fmt|doc|pkg|agent|evolve`), and the self-evolution governance loop
(`use lcaas`, `use levolve`), all keeping the **stage 2 == stage 3** self-host fixpoint and the
**Zero-C steady state** bit-identical in CI.

Phase 3 (**Stage 3** in `docs/04-roadmap.md`) brings **systems-grade memory safety without a
garbage collector**, **effect handlers without function coloring**, **fearless structured
concurrency**, and **machine-checked contract verification (`lume prove` v0)** to the self-hosted
compiler.

> **Entry gate:** Phase 3 implementation begins only after the Stage 2 exit-criterion application
> (`examples/apps/inventory/`) and the remaining Track F items (`F3c` Hindley–Milner core inference
> and `F4` front-end unification) are merged and green on `main`.

Every Phase 3 brick obeys the project's non-negotiable rules:
1. **AI-first (`P1`–`P6`):** Every new check emits a stable diagnostic code, structured JSON in
   `lcaas`/`lume agent`, and a deterministic fix-it suggestion.
2. **Superior to the reference language:** Rust-grade safety with `from`-clause region ergonomics
   (no `'a` lifetime noise in 95% of signatures), Koka-style Perceus in-place reuse + effect
   handlers, Erlang-style supervision with static `Send`/`Sync` guarantees, and Dafny/SPARK-style
   counterexamples on failed contracts.
3. **Self-host & Zero-C invariant:** Every brick lands as a green PR with positive and negative
   conformance tests while keeping `make bootstrap` (`stage2 == stage3`) and `make zero-c` green.

---

## Tracks and bricks

### O — Ownership, borrowing, regions, and memory tiers (`§6`) — **Complete**

- **O1 — Move semantics, `Copy` classification, and deterministic `Drop` (`§6.1`) — Done (`compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/own_ok.src`)**
  - Every binding has a single owner. Plain-data types (`int`, `bool`, `decimal(p, s)`, enums and
    records composed solely of `Copy` fields) are `Copy`; heap-backed and resource types move on
    `own` transfer or assignment.
  - Deterministic LIFO scope-exit destruction (`Drop`) and `defer` / `errdefer` lowering in
    Lume-Core.
  - Diagnostics:
    - `O0001`: use of moved value (identifies move site and use site; fix-it suggests borrowing
      or explicit `.clone()`).
    - `O0002`: moving out of a borrowed parameter or record field.
  - *Ref: Rust MIR borrowck, Austral linear capabilities. Superior: gradual opt-in for untyped
    scripts while enforcing strict ownership on resource/annotated types, with concrete fix-its.*

- **O2 — Parameter borrow modes: shared, `mut`, and `own` (`§6.2`) — Done (`compiler/stage1/lume/04_parser_decls.src`, `compiler/stage1/lume/06b_phase3.src`)**
  - Parameters default to **shared borrow** (read-only, non-escaping); `mut` grants an **exclusive
    mutable borrow**; `own` transfers ownership into the callee (`consume(own file)` at call site
    per `P7`).
  - Aliasing rule: any number of shared borrows **or** a single `mut` borrow active at a call or
    statement boundary, never both.
  - Diagnostics:
    - `O0010`: passing an immutable `let` binding to a `mut` parameter (fix-it suggests changing
      `let` to `var`).
    - `O0011`: overlapping `mut` and shared borrows in the same call or scope.
    - `O0012`: missing `own` keyword at call site for an `own` parameter.
  - *Ref: Rust borrow checker, Hylo (Val) value-semantics parameters. Superior: zero `&`/`&mut` sigil
    clutter in parameter lists and call sites.*

- **O3 — Intra-module region inference and `from` return-origin clauses (`§6.2`) — Done (`compiler/stage1/lume/06b_phase3.src`)**
  - Within a module, borrow lifetimes (regions) are inferred automatically across local bindings and
    helper calls.
  - A `pub` function returning a borrowed view declares its origin via `from`:
    `fn first(items: [T]) -> T from items`.
  - Diagnostics:
    - `O0020`: borrowed value escapes its owner's scope.
    - `O0021`: function returns a borrow without a `from <param>` origin clause (fix-it synthesizes
      the exact `from` clause).
  - *Ref: Rust elided lifetimes, Carbon/Austral region tracking. Superior: readable `from param`
    clauses instead of viral `'a` type parameters.*

- **O4 — Shared ownership tiers, `@gc` regions, and `with alloc` contexts (`§6.3–6.4`) — Done (`compiler/stage1/ownership.src`, `compiler/stage1/own_ok.src`)**
  - Explicit shared tiers: `rc[T]` (single-threaded), `arc[T]` (atomic, `Send + Sync`), `weak[T]`,
    and `cell[T]`.
  - Scoped `@gc` type attribute for cyclic graph structures confined to a region, checked at `pub`
    API boundaries.
  - Implicit allocator context (`with alloc = arena ...`) and `O0030` freestanding enforcement
    (rejecting ambient heap allocation when compiling under `@freestanding`).
  - *Ref: Zig allocator passing, Odin `context.allocator`, Nim/D scoped GC.*

---

### K — Effect system, algebraic handlers, and Perceus reuse (`§8.2`, **DONE**)

- **K1 — Effect rows, inference, and capability checking (`§8.2`, `P26`) ✅**
  - Functions carry an effect row `/ {pure, io, fs, net, alloc, async, fail, panic, rand, time}` (and user-declared effects).
  - Private functions infer their minimal effect row from their body and callees; `pub` functions
    require an explicit `/ {...}` annotation when impure (failing with `[E0200]` and a compiler fix-it
    emitting the exact inferred row).
  - Connects directly to module capabilities (`@capability {io, ...}` / `lume.toml`'s `[capabilities] allow = [...]`):
    calling a function whose effect row exceeds the module's granted capabilities fails with `[E0200]`.
  - *Ref: Koka effect rows, Eff, Austral capabilities.*

- **K2 — User-declared `effect` and `with handler` (`§8.2`) ✅**
  - Algebraic effect declarations (`effect Clock { fn now() -> int }`, `effect Log { fn info(code: int) -> int }`)
    and lexical handlers (`with handler Clock { fn now() -> int = 1000 } in ...`).
  - Unifies dependency injection, deterministic test mocking, `yield` generators, and `async`
    execution without function coloring, discharging the handled effect from the enclosing function's row.
  - Tail-resumptive handlers lower directly in `06b_phase3.src` with zero closure/continuation allocation.
  - *Ref: Koka, OCaml 5 effect handlers, Unison abilities.*

- **K3 — Perceus reference-count & in-place reuse analysis ✅**
  - Static drop-guided reuse pass on Lume-Core (`ktryp_perceus_arm` in `06b_phase3.src` + `lreuse1`/`lreuse2`
    in `05_codegen.src`): when a pattern match on an `own` parameter consumes a constructor cell
    (`Boxed(v)` or `Cons(h, t)`) and constructs a node of the same tag and arity in tail position,
    the matched cell's payload array (`0(24(%rax))`, `8(24(%rax))`) is mutated in place with zero
    `call alloc` and zero `call mkctor` instructions.
  - Proven in `compiler/stage1/perceus_ok.src` and `bootstrap/lume0/tests/run.sh` by inspecting the
    emitted x86-64 assembly to confirm zero heap allocations on the reused path.
  - *Ref: Koka Perceus (Reinking et al.), Lean 4 FBIP (Functional But In-Place).*

---

### S — Structured concurrency, channels, and supervised actors (`§10`, **DONE**)

- **S1 — Structured `scope` / `spawn` and static `Send`/`Sync` checking (`§10`) ✅**
  - Tasks are spawned exclusively inside a lexical `scope s in ...`: the scope joins all child tasks
    before exiting (`ScopeE` / `SpawnE` in `01_lexer.src`, `02_parser.src`, `06b_phase3.src`, `lcaas.src`),
    and unstructured `spawn` outside a `scope` is rejected at compile time with `[S0001]`.
  - Compiler derives `Send` and `Sync` from ownership tiers (`O1`–`O4`) and rejects capturing a
    non-`Send` value (`rc[T]`, `cell[T]`, `Ref[T]`) or an unmoved `var`/`mut` borrow across `spawn`
    with diagnostic `[S0001]` and a fix-it (`use 'arc_new' instead of 'rc_new'` / `move ownership with 'own'`).
  - *Ref: Trio / Kotlin structured concurrency, Rust `std::thread::scope`.*

- **S2 — Typed channels (`chan[T]`), `select`, and `par for` / `par map` ✅**
  - Bounded and unbounded `chan[T]` channels (`chan_new`, `chan_unbounded`, `chan_send`, `chan_recv`,
    `chan_close` in `compiler/stage1/concurrency.src`) transferring messages FIFO with capacity and
    close-state enforcement.
  - Deterministic `select` (`chan_select2`, `chan_select_timeout`) over channel receives and deadlines
    driven by a logical-time schedule seed.
  - Data-parallel `par_for_sum_range`, `par_map_scale`, `par_map_add`, `par_sum`, and `par_count_above`
    combinators in `compiler/stage1/concurrency.src`.
  - *Ref: Go channels/`select`, Rayon data parallelism.*

- **S3 — `std.actor`, Erlang-style `supervisor`, and `std.time.logical` ✅**
  - Typed `actor` state machines (`actor_new`, `actor_send`, `actor_step`, `actor_restart`, `actor_stop`
    in `compiler/stage1/actor.src`) with Erlang/OTP-style `supervisor` restart policies (`one_for_one`,
    `one_for_all`, and bounded `max_restarts` escalation via `supervisor_heal2`).
  - `std.time.logical` (`lclock_new`, `lclock_advance`, `lclock_next_slot`): deterministic virtual
    clock and PRNG schedule seed so concurrent actor systems and timeouts replay bit-identically in
    `lume test`.
  - *Ref: Erlang/OTP supervisors, TigerBeetle/FoundationDB deterministic simulation testing.*

---

### V — Contract verification (`lume prove` v0 and `LAWS` `P34`, `§9`, **DONE**)

- **V1 — First-class `requires` / `ensures` / `invariant` and `old(...)` in Lume-Core (`§9`) ✅**
  - Promotes contract clauses from doc/test annotations into checked Lume-Core AST nodes (`VContractSig`
    on functions and `VTypeInvariant` on `type ... = record` declarations in `04_parser_decls.src`,
    `06b_phase3.src`, and `lcaas.src`), with `result` / `ret` and `old(...)` entry snapshots.
  - Contracts that pass compile-time counterexample search are classified as `runtime_checked` and
    lowered into runtime assertions (`vwrap_fn_contract`) so off-sample violations always trap safely
    at runtime — never silently assumed or elided without a full decision proof.
  - *Ref: Eiffel Design by Contract, Ada SPARK, Dafny.*

- **V2 — `lume prove` v0: compile-time counterexample search + sound runtime guards ✅**
  - Built-in compile-time bounded counterexample evaluator in `06b_phase3.src` and
    `compiler/stage1/lcaas.src` for zero-dependency offline refutation over integer and scale-aligned
    `decimal(p, s)` expressions (full `QF_LIA` decision procedure for zero-overhead `proven` elision
    is planned as the next verification upgrade).
  - Every contract, call-site `requires`, and record `invariant` is classified as `refuted`
    (compile-time error `[V0001]` with a concrete counterexample binding, e.g.
    `counterexample: bal = 0, amt = 1 (result = -1)`) or `runtime_checked` (runtime contract guard
    preserved in emitted code).
  - Exposed via `./lume prove [--json] <file.src>`, `lume agent prove <file.src>`, and `caas_prove(src)`
    emitting structured `"proofs":[...]` JSON objects.
  - *Ref: Dafny, Why3, Lean/Z3.*

- **V3 — `LAWS` (`P34`) project-level invariants and `lume evolve` integration ✅**
  - Top-level `law <Name>(...) = <expr>` (`VLawDecl`) declarations are checked against bounded
    counterexample search at compile time (rejecting refutable laws with `[V0001]` and a concrete
    counterexample) and lowered to callable `law_<Name>(...)` functions for property tests (`lume test`).
  - Wired into `lume evolve` (`compiler/stage1/levolve.src` via `use lcaas`) so any candidate patch
    violating a declared `law` or `requires`/`ensures` contract is automatically rejected by the
    evolution gates.

### A — AI-native authoring: holes, totality, and provenance (the AI-first multiplier, **DONE**)

> **Priority: not deferred.** Lume exists so an AI can author *correct* code faster; A1–A3 are the
> features that most directly serve that mission, so they run **early and in parallel** with O/K/S/V,
> not after. Each is driven by the compiler-as-a-service (`lcaas`) and `lume agent`, machine-first.

- **A1 — Typed holes + hole-driven synthesis (`_`) ✅**
  - A `_` (hole) is a well-typed term: the checker assigns it an expected type and the in-scope
    environment, and `lume agent fill <file.src>` (`caas_fill` in `compiler/stage1/lcaas.src`) returns,
    per hole, its type, local context, and ranked candidate completions (in-scope bindings, functions
    whose result unifies with the hole, and canonical default values), plus diagnostic `A0001` with
    the top synthesized fix-it.
  - A model writes a program incrementally and **never ships an ill-typed draft** — private draft
    functions with holes coexist with runnable code, while unfilled holes in `pub fn` fail at compile
    time with `[A0001]`. *Ref: Agda/Idris interactive holes, GHC typed holes, Hazel. Superior:
    machine-first (JSON candidates for an AI), gradual (holes coexist with runnable code).*

- **A2 — `total` totality / termination checking ✅**
  - An opt-in `total fn` (`total fn` / `pub total fn`) is rejected with `[A0002]` unless the checker
    proves termination (structural recursion on ADT constructor sub-patterns or a guarded decreasing
    integer measure `n - k` / `n / k`), and rejects calling non-total recursive functions from a
    `total fn`.
  - For AI-generated code this guarantees **no shipped infinite loop**. *Ref: Idris `total`, Dafny
    `decreases`, Agda termination checker.*

- **A3 — Provenance & uncertainty in the type (the one feature no source language has) ✅**
  - A value produced by an `llm fn`, `prob_sample`, or `unverified[T]` parameter carries an
    `unverified` **provenance tag in its type**. The checker FORBIDS an `unverified` value from
    flowing into trusted code (a `pub` boundary or an effectful/trusted sink such as `io_write`,
    `db_exec`, `net_send`, `fs_write`) with `[A0003]` until a contract (`verify(val, contract)` or
    comparison branch guard), a `LAWS` gate (`law_check(val, law)`), or human governance
    (`evolve_approve(val)`) discharges it.
  - Makes **AI-origin first-class**, exactly as effects (`P14`) make capabilities first-class: an AI
    hallucination cannot silently become a production bug. *Ref: none — Lume's own, required by its
    AI-first mission (`P1`–`P6`). Composes with effects, contracts, and `lume evolve`.*

---

## Execution order & dependencies

```
Phase 2 Exit (F3c + F4 + examples/apps/inventory/)
   │
   ├──► Track O (O1 ──► O2 ──► O3 ──► O4)
   │       │
   │       ├──► Track K (K1 ──► K2 ──► K3 Perceus reuse)
   │       │
   │       └──► Track S (S1 ──► S2 ──► S3 actors + logical time)
   │
   ├──► Track V (V1 ──► V2 lume prove v0 ──► V3 LAWS + evolve gate)
   │                            │
   └──► Track A (A1 holes ║ A2 total ║ A3 provenance)  ← AI-first, runs EARLY in parallel, not deferred
                                │
                                ▼
             Stage 3 Capstone (examples/apps/job_server/)
```

## Stage 3 Exit Criterion (`docs/04-roadmap.md` line 122, **DONE ✅**)

Implemented in `examples/apps/job_server/` (`lume.toml`, `lume.lock`, `protocol.src`, `engine.src`,
`workers.src`, `main.src`, `expected_output.txt`) — a complete concurrent service + worker pool +
supervised actor job system with **zero `unsafe`**, exercising:
- Ownership, borrowing, `from` regions, `defer` LIFO cleanup, and Perceus reuse (`Track O` + `Track K`),
- Effect-checked handlers (`Clock`, `Audit`), AI provenance verification (`llm fn` + `verify`), and capability-audited modules (`Track A` + `Track K`),
- Scoped tasks (`scope`/`spawn`), `arc[T]`, typed channels (`chan[T]`), deterministic `select`, actor supervision (`one_for_one`), and deterministic logical-time replay (`Track S`),
- Checked accounting/queue contracts (`requires`/`ensures`/`old`/`invariant`) and project-level `law` declarations via `lume prove` and `LAWS` (`Track V`),
passing `lume pkg verify`, `lume prove`, `lume test` (7 test suites), and native `lume run` in CI with zero data races by construction.
