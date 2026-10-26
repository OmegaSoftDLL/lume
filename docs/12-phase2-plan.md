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
- **F1 ✅ done** named `type`/`enum`/`record` declarations with typed fields in the self-hosted parser (`lumec.src`),
  plus a **gradual, AI-first checker**. Inline, layout-free syntax (the lexer is indentation-blind, so the README's
  indented form is adapted): `type Dept = enum Sales | IT | Finance`, `type Node = enum Nil | Cons(int, Node)`
  (typed positional payloads), `type Point = record x: int, y: int`; a type `T` is `int`, `str`, a declared name,
  or `[T]`. Parts are delimited by lookahead (variants by `|`, fields by `,`), like `match`. **Gradual rule**
  (TypeScript/Zig spirit): a program that declares no `type` is unchecked — constructors stay free, so every existing
  `.src` and `lumec.src` itself compile byte-identically and the self-host fixpoint is untouched. The moment a program
  declares any `type` it opts in: every constructor used (expression or pattern) must resolve to a declared enum variant
  with the right **arity**, and every field in a **nominal** record literal `T{...}` must belong to record type `T`
  (anonymous `{...}` records are never checked — no false positives, and ad-hoc helper records stay legal). The checker
  catches the model's typos: undeclared constructor, wrong constructor arity, unknown record field — each a hard error
  with a legible message (exit 3, no asm), the same failure channel comptime uses. Type declarations emit **no code**
  (compile-time only). NOT yet (deferred): ~~type annotations on `fn` params/returns and arg-type checking (**F2**)~~ (done in F2, below),
  Hindley–Milner inference so anonymous records/field access are checked without annotation (**F3**). *Ref: ML/Haskell ADTs, Rust `enum`/`struct`, TypeScript gradual typing. Superior: opt-in gradualism with zero false positives + AI-legible errors.*
- **F2 ✅ done** optional type annotations on `fn` params and return: `fn greet(name: str, n: int) -> str = ...` (`T` is `int`, `str`, a declared
  name or `[T]`, as in F1; every annotation is optional, so `fn f(x) = ...` is unchanged). The parser keeps `Fn(name, params, body)` byte-for-byte
  and emits the annotations as a separate compile-time-only `Sig(name, paramnames, paramtypes, rettype)` statement just before an annotated `fn`
  (none for an unannotated one, so the AST — and the self-host fixpoint — of annotation-free code is untouched; codegen ignores `Sig`). **Gradual
  argument checking, zero false positives:** at a call `f(args)` only arguments whose type is INEQUIVOCAL without inference are checked — an integer
  literal is `int`, a string literal `str`, a constructor of a declared enum (its unique owner) that enum, a nominal record literal that record;
  a variable, a call result or an operator expression is unknown and never flagged. A function whose body is such a literal/constructor is also
  checked against `-> R`. Errors share the F1 channel (exit 3, no asm), e.g. `type error: argument 1 of 'greet' has type int but parameter 'name'
  expects str`. Annotations alone (no `type` declaration) enable this checking but not F1's constructor/record declaration checks. Deferred to **F3**:
  checking arguments/returns whose type needs inference (variables, calls, operators), arity of plain calls, and validating annotation names.
  *Ref: TypeScript gradual typing, Rust param types. Superior: opt-in, AI-legible errors, zero false positives.*
- **F3** Hindley–Milner inference in the core: annotations optional but checked. *Ref: OCaml/Haskell.*
- **F4** unify the two front-ends (surface Lume vs ML-core) into one grammar/AST.

### C — comptime (FLAGSHIP)
- **C1 ✅ done (#116)** `comptime <expr>` of the pure core folds to a constant in the binary; non-evaluable comptime fails the compiler loudly. *Ref: Zig `comptime`. Superior to C++ `constexpr`: deterministic, exact integers, legible errors.*
- **C2 ✅ done** compile-time calls to **pure user functions** (recursion + `if` + arithmetic), with a lazy purity guard (anything `ceval` cannot reduce — `read()`/builtins/undefined names — is a hard error) and an explicit termination guard (a step budget + a call-depth budget, so runaway recursion gives a legible error, never a hang or crash). `comptime(fib(10))` folds to `movq $55` with no `call fib`. The fold moved to a post-parse pass (all `fn`s known). `match`-over-constructors in comptime is deferred to C3 (fib/fact use `if`); a comptime that reaches `match` still fails with a clear error. *Ref: Zig comptime fn, C++ `constexpr` fn, Lisp `eval`. Superior to C++ constexpr: deterministic, exact 64-bit integers, legible errors, an explicit terminating guard.*
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
- **T1 ✅ done (#118)** `lume` single CLI (`./lume`: `build`/`run`/`fmt`/`test`/`doc`/`check`/`describe`), backed by native Lume-Core tools (`compiler/stage1/ldoc.src` for Markdown + AI-first `--json` doc/signature/contract extraction, and `compiler/stage1/ltest.src` for zero-config `fn test_*()` discovery and native execution). *Ref: cargo, go.*
- **T2 ✅ done** one canonical formatter (`compiler/stage1/lfmt.src`, compiled to native ELF by `lumec` + `lasm` and wired into `lume fmt` / `lume fmt --check`). Parses Lume-Core declarations and statements into an AST while preserving `##` doc comments and section blank lines, aligns `match` arm `=>` arrows vertically, and performs precedence-aware minimal parenthesisation (stripping redundant parens while preserving required ones), proving both idempotence (`lfmt(lfmt(x)) == lfmt(x)`, including `lfmt(lfmt.src) == lfmt.src`) and byte-for-byte codegen invariance (`lumec(lfmt(x)) == lumec(x)`). *Ref: gofmt, rustfmt.*
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
