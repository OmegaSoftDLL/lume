# 12 — Phase 2 plan (self-evolving Lume: comptime, metaprogramming, data, tooling)

Phase 1 is complete: the compiler (`compiler/stage1/lumec.src`) and the assembler + ELF
writer (`compiler/stage1/lasm.src`) are written in Lume-Core and self-host — `make bootstrap`
runs the staged build (GCC/Rust/Go model) and asserts **stage 2 == stage 3, bit for bit**, with
no C in the loop (the C seed `lume0` is only the frozen bootstrap root). See `docs/10` and `docs/11`.

Phase 2 grows Lume into the language the pillars (`docs/02-principles.md`) promise. The flagship is
**comptime**. Every brick obeys the project rules: **AI-first**, **superior to the source language,
never a shallow clone** (consult the reference implementation when a brick is hard, rather than
getting stuck), one green PR with a regression test, and the **self-host fixpoint stays bit-identical**
(now gated in CI by the `make bootstrap` job).

## Tracks and bricks

### F — Type foundation (everything else depends on it)
- **F1** named `type`/`enum`/`record` declarations with typed fields in the self-hosted parser. *Ref: ML/Haskell ADTs, Rust `enum`/`struct`.*
- **F2** type annotations on params/returns + a consistency checker (catches the model's mistakes — AI-first). *Ref: TypeScript gradual typing, Rust.*
- **F3** Hindley–Milner inference in the core: annotations optional but checked. *Ref: OCaml/Haskell.*
- **F4** unify the two front-ends (surface Lume vs ML-core) into one grammar/AST.

### C — comptime (FLAGSHIP)
- **C1 ✅ done (#116)** `comptime <expr>` of the pure core folds to a constant in the binary; non-evaluable comptime fails the compiler loudly. *Ref: Zig `comptime`. Superior to C++ `constexpr`: deterministic, exact integers, legible errors.*
- **C2** compile-time calls to **pure user functions** (recursion, `match`), with a purity/termination guard. *Ref: Zig comptime fn, C++ `constexpr` fn, Lisp `eval`.*
- **C3** comptime **types** → generics via comptime (`fn(comptime T: type)`). Depends on F. *Ref: Zig generics-via-comptime.*
- **C4** `comptime assert`, precomputed tables, build-time data embedding.

### M — macros (metaprogramming)
- **M1** the AST as a first-class comptime value (the compiler exposes the tree). *Ref: Lisp homoiconicity, Nim.*
- **M2** hygienic macros / quasiquote. *Ref: Scheme `syntax-case`, Rust, Nim.*

### D — data (pillar P7)
- **D1** `decimal(p, s)` exact decimal type + literals + arithmetic. *Ref: COBOL/dBase, SQL NUMERIC.*
- **D2** `table[T]` typed in-memory relation + `read_csv`. *Ref: dBase/Clipper, pandas, Arrow.*
- **D3** integrated `query` (where/order/join/group), type-checked. *Ref: LINQ (C#), SQL, Datalog.*

### T — one tool
- **T1** `lume` single CLI (build/run/fmt/test/doc). *Ref: cargo, go.*
- **T2** one canonical formatter. *Ref: gofmt, rustfmt.*
- **T3** modules + package manager. *Ref: cargo, go modules.*

### E — self-evolution (`lume evolve`) — closes "Lume evolves Lume on its own"
- **E2 ✅ done** automated evolution gate: every PR runs `make bootstrap` in CI, asserting the triple
  self-build stays bit-identical, alongside `make test`/`asan`. Any change that breaks self-hosting is
  rejected automatically — the prerequisite for safe multi-contributor work. *Ref: our staged bootstrap + CI; `docs/04-roadmap.md`.*
- **E1** compiler-as-a-service: expose `lumec` as a callable API/library (source in → diagnostics/AST/asm/result out), not only a CLI. Depends on F. *Ref: Roslyn (C#), rustc query API, clangd.*
- **E3** the governance loop: an AI proposes a change via E1, the E2 gates run automatically, a human approves. *Ref: `docs/04-roadmap.md`.*

## Recommended order
C1✅ → C2 (deliver the flagship early) alongside F1 → F2 (foundation); E2✅ already protects `main`;
then C3 (needs F), M1/M2, D1 → D3, T1 → T3, E1 → E3.

Each brick: a green PR, a regression test, the fixpoint intact, verified by running it — and
**superior to the language it borrows from**.
