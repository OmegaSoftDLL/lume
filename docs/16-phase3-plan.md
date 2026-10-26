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

### O — Ownership, borrowing, regions, and memory tiers (`§6`)

- **O1 — Move semantics, `Copy` classification, and deterministic `Drop` (`§6.1`)**
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

- **O2 — Parameter borrow modes: shared, `mut`, and `own` (`§6.2`)**
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

- **O3 — Intra-module region inference and `from` return-origin clauses (`§6.2`)**
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

- **O4 — Shared ownership tiers, `@gc` regions, and `with alloc` contexts (`§6.3–6.4`)**
  - Explicit shared tiers: `rc[T]` (single-threaded), `arc[T]` (atomic, `Send + Sync`), `weak[T]`,
    and `cell[T]`.
  - Scoped `@gc` type attribute for cyclic graph structures confined to a region, checked at `pub`
    API boundaries.
  - Implicit allocator context (`with alloc = arena ...`) and `O0030` freestanding enforcement
    (rejecting ambient heap allocation when compiling under `@freestanding`).
  - *Ref: Zig allocator passing, Odin `context.allocator`, Nim/D scoped GC.*

---

### K — Effect system, algebraic handlers, and Perceus reuse (`§8.2`)

- **K1 — Effect rows, inference, and capability checking (`§8.2`, `P26`)**
  - Functions carry an effect row `/ {pure, io, fs, net, alloc, async, fail, panic, rand, time}`.
  - Private functions infer their minimal effect row from their body and callees; `pub` functions
    require an explicit `/ {...}` annotation when impure (with a compiler fix-it emitting the exact
    inferred row).
  - Connects directly to `lume.toml`'s `[capabilities] allow = [...]`: calling a function whose
    effect row exceeds the package's granted capabilities fails with `E0200`.
  - *Ref: Koka effect rows, Eff, Austral capabilities.*

- **K2 — User-declared `effect` and `with handler` (`§8.2`)**
  - Algebraic effect declarations (`effect Clock { fn now() -> int }`, `effect Log { fn emit(msg: str) -> int }`)
    and lexical handlers (`with handler Clock { fn now() = 1700000000 } in ...`).
  - Unifies dependency injection, deterministic test mocking, `yield` generators, and `async`
    execution without function coloring.
  - Tail-resumptive handlers compile to direct calls with zero closure/continuation allocation.
  - *Ref: Koka, OCaml 5 effect handlers, Unison abilities.*

- **K3 — Perceus reference-count & in-place reuse analysis**
  - Static drop-guided reuse pass on Lume-Core: when a pattern match consumes the last reference to
    a constructor or record (`Cons(h, t)` or `Rec(...)`) and immediately constructs a node of the
    same size, the allocation slot is mutated in place rather than allocating from the heap.
  - Proven in conformance tests by asserting zero additional heap growth across long functional
    list/table transformations.
  - *Ref: Koka Perceus (Reinking et al.), Lean 4 FBIP (Functional But In-Place).*

---

### S — Structured concurrency, channels, and supervised actors (`§10`)

- **S1 — Structured `scope` / `spawn` and static `Send`/`Sync` checking (`§10`)**
  - Tasks are spawned exclusively inside a lexical `scope s`: the scope joins all child tasks before
    exiting, propagates task panics/errors, and cancels siblings on failure (zero leaked tasks).
  - Compiler derives `Send` and `Sync` from ownership tiers (`O1`–`O4`) and rejects capturing a
    non-`Send` value (`rc[T]`, raw `mut` borrow outliving the scope) across `spawn` with diagnostic
    `S0001`.
  - *Ref: Trio / Kotlin structured concurrency, Rust `std::thread::scope`.*

- **S2 — Typed channels (`chan[T]`), `select`, and `par for` / `par map`**
  - Bounded and unbounded `chan[T]` channels transferring ownership (`own`) of messages across tasks
    with zero copying.
  - Deterministic `select` over channel receives/sends and timeouts.
  - Data-parallel `par for` and `par map` over slices and `table[T]` partitions backed by a
    work-stealing task runner.
  - *Ref: Go channels/`select`, Rayon data parallelism.*

- **S3 — `std.actor`, Erlang-style `supervisor`, and `std.time.logical`**
  - Typed `actor` definitions with message-protocol checking and `supervisor` restart policies
    (`one_for_one`, `one_for_all`, bounded restart intensity).
  - `std.time.logical`: deterministic virtual clock and schedule seed so concurrent actor systems
    and timeouts replay bit-identically in `lume test`.
  - *Ref: Erlang/OTP supervisors, TigerBeetle/FoundationDB deterministic simulation testing.*

---

### V — Contract verification (`lume prove` v0 and `LAWS` `P34`, `§9`)

- **V1 — First-class `requires` / `ensures` / `invariant` and `old(...)` in Lume-Core (`§9`)**
  - Promotes contract clauses from doc/test annotations into checked Lume-Core AST nodes on
    functions and `type ... = record` declarations.
  - Runtime instrumentation in debug/`lume test` builds plus static verification condition
    generation (VCGen) over the pure subset.
  - *Ref: Eiffel Design by Contract, Ada SPARK, Dafny.*

- **V2 — `lume prove` v0: static discharge + counterexample synthesis**
  - Built-in quantifier-free linear integer and exact-decimal arithmetic solver (`QF_LIA` / scale-aligned
    `decimal(p, s)`) in Lume-Core for zero-dependency offline proofs, plus optional SMT-LIB2 bridge
    to external `z3`/`cvc5`.
  - Every contract is classified as `proven` (runtime check elided), `refuted` (compile-time error
    `V0001` with a concrete counterexample binding, e.g. `counterexample: amount = -1`), or
    `runtime_checked` (kept as a runtime assertion when outside the decidable fragment).
  - Exposed in `lcaas` and `lume agent prove <file.src>` as structured JSON.
  - *Ref: Dafny, Why3, Lean/Z3.*

- **V3 — `LAWS` (`P34`) project-level invariants and `lume evolve` integration**
  - Project-wide `law` declarations (e.g. conservation of accounting balance across invoice
    transformations, monotonicity of stock levels) compiled to proof and property-test obligations.
  - Wired into `lume evolve` (`compiler/stage1/levolve.src`) as a verification gate so self-evolved
    patches must preserve all declared `LAWS`.

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
   └──► Track V (V1 ──► V2 lume prove v0 ──► V3 LAWS + evolve gate)
                                │
                                ▼
             Stage 3 Capstone (examples/apps/job_server/)
```

## Stage 3 Exit Criterion (`docs/04-roadmap.md` line 122)

A complete concurrent service + worker pool + supervised actor job system in
`examples/apps/job_server/` with **zero `unsafe`**, exercising:
- Ownership, borrowing, `from` regions, and Perceus reuse (`Track O` + `Track K`),
- Effect-checked handlers and capability-audited modules (`Track K`),
- Scoped tasks, typed channels, actor supervision, and deterministic logical-time replay (`Track S`),
- Statically proven accounting/queue invariants via `lume prove` and `LAWS` (`Track V`),
passing a deterministic concurrency stress suite in CI with zero data races by construction.
