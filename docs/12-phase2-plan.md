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
- **F3a ✅ done** local gradual inference of a function's **return type** from its annotated parameters, its `let`
  bindings, and operator/`if` result types (`tknownl`), checked against the `-> R` annotation. `fn half(n: int) -> int = n / 2`
  type-checks; `fn bad(n: int) -> str = n + 1` fails at compile time (exit 3, no asm) with `function 'bad' returns int
  but is declared to return str`. It returns a concrete type only when certain (TyAny otherwise), so it adds real
  checks with **zero false positives**, and `tknown` is preserved as `tknownl(·, Nil)` so every existing checker path
  is byte-identical; a program that declares no types is still returned unchanged, so the self-host fixpoint is
  untouched. NOT yet *(historical; done in F3b/F3c below)* (**F3b**): full Hindley–Milner — unification, type variables, let-polymorphism, and inferring
  argument types through a function body so anonymous records/field access are checked without annotation. *Ref: OCaml/Haskell, TypeScript gradual typing.*
- **F3b ✅ done (argument inference)** an argument that is a variable / operator / `let` / `if` whose type infers to
  `int` or `str` is now checked against the parameter's type — but **only when the parameter is also int or str**, so
  a decimal/record/list/table/generic parameter is never inspected and the existing decimal and D3/query code can
  raise no new error. `fn bad(n: int) -> str = greet(n)` (greet expects str) now fails at compile time. It is purely
  additive: `tchkarg`/`tcompat` and every existing checker path are byte-for-byte unchanged (a new `tchkinferargs`
  reads the function's parameter env via `tt.lenv`), so the self-host fixpoint is untouched. *Ref: TypeScript/Rust
  gradual argument typing.* NOT yet *(historical; done in F3c below)* (**F3c**): full Hindley–Milner — unification, type variables, let-polymorphism,
  and inference over decimal/record/list arguments.
- **F3c ✅ done (#154)** Hindley–Milner inference engine: unification (`hunify`), type variables (`TyVar`), substitution application (`happly`), and body inference (`tcollecti`/`tinfer`) for unannotated functions, plus anonymous-record structural field checking. *Ref: OCaml/Haskell.* (Generalising polymorphic `let`-bound functions across distinct instantiations is implemented in #239 — `hgeneralize`/`hinstantiate` with `[T0500]` on unification mismatch.)
- **F4 ✅ done** unify the two front-ends (surface Lume vs ML-core) into one grammar/AST (`docs/05-vision.md:224`, `docs/17-frontend-unification.md`).
  - **Slice 1 ✅ done (#143):** named enum payloads (`Leaf(value: int)`) normalise to `{tag, ar}` (byte-identical codegen).
  - **Slice 2 ✅ done:** string interpolation (`"hi {who()}, the answer is {answer()}!"` + `{{`/`}}` escaping) desugars into a left-associative `+` chain with `str(...)`, emitting byte-identical assembly to explicit concatenation.
  - **Slice 3 ✅ done (#152):** `elif` chains desugar to nested `If` AST nodes.
  - **Slice 4 ✅ done:** indented layout form (`f4_layout.src`), explicit-brace form (`f4_brace.src`), and inline Lume-Core form (`f4_core.src`) lower to the same AST, emit byte-identical assembly, and `lume fmt` normalises both surface forms to the canonical Lume-Core representation.

### C — comptime (FLAGSHIP)
- **C1 ✅ done (#116)** `comptime <expr>` of the pure core folds to a constant in the binary; non-evaluable comptime fails the compiler loudly. *Ref: Zig `comptime`. Superior to C++ `constexpr`: deterministic, exact integers, legible errors.*
- **C2 ✅ done** compile-time calls to **pure user functions** (recursion + `if` + arithmetic), with a lazy purity guard (anything `ceval` cannot reduce — `read()`/builtins/undefined names — is a hard error) and an explicit termination guard (a step budget + a call-depth budget, so runaway recursion gives a legible error, never a hang or crash). `comptime(fib(10))` folds to `movq $55` with no `call fib`. The fold moved to a post-parse pass (all `fn`s known). `match`-over-constructors in comptime is deferred to C3 (fib/fact use `if`); a comptime that reaches `match` still fails with a clear error. *Ref: Zig comptime fn, C++ `constexpr` fn, Lisp `eval`. Superior to C++ constexpr: deterministic, exact 64-bit integers, legible errors, an explicit terminating guard.*
- **C3 ✅ done** comptime **types** → generics via comptime (`fn(comptime T: type, x: T)`). A type name (`int`, `str`, a
  declared type) is a comptime value; a parameter marked `comptime` receives one at compile time. The parameter is
  **erased at runtime** (type erasure, the effect that PROVES the comptime happened): the parser keeps only the runtime
  parameters in `Fn(name, params, body)` (so a comptime-free function's AST — and the self-host fixpoint — is byte-identical),
  records the comptime parameters in a separate compile-time-only `GSig(name, plist, rettype)` statement (as F2 did with
  `Sig`), a post-check `gerase` pass drops the comptime arguments from every call site (`id(int, 42)` emits the call
  `id(42)` — the type argument is consumed at build time and never pushed), and codegen is untouched. This is
  monomorphisation-lite: the runtime is dynamic/boxed so one erased body serves every instantiation; the win is the
  checking plus the erasure, not specialised code. **Generic checking (reuses F2):** at a call the comptime type
  argument binds the type parameter (`T := int`) in a type environment, and each remaining annotated argument has its
  declared type substituted through that environment and checked with F2's inequivocal-only rule, so `id(int, "hi")`
  is `type error: argument 2 of 'id' has type str but parameter 'x' expects int` (exit 3, no asm) while an argument of
  unknown type is never flagged (zero false positives). Depends on F. *Ref: Zig generics-via-comptime. Superior to Zig:
  gradual (annotations optional), AI-first (one clear message), zero false positives, and the fixpoint stays bit-identical.*
  **(C4c ✅ done #212)** Generics over user-declared `type` (`record`/`enum`) and parameterized container types (`type Box[T] = record value: T`, `type Option[T] = enum None | Some(T)`, `[T]`, `table[T]`) as the bound comptime type argument (`id(Point, ...)`, `id(Box[int], ...)`, `id([int], ...)`, `unbox(int, Box{value: 42})`, `unwrap_or(str, Some("ok"), "")`) with gradual type checking and full runtime type erasure (`compiler/stage1/generics_container_ok.src`).
- **C4a ✅ done** `comptime assert(cond)` / `comptime assert(cond, "msg")`: the condition is evaluated entirely
  at compile time by the comptime evaluator and the build is aborted (exit 3, no asm) with a legible message if it
  is false; a passing assert folds to a constant, so the condition itself leaves **no check in the binary**. It is
  recognised only as the head of a `comptime` node and only when no user `fn assert` shadows it, so a comptime-free
  program — `lumec.src` included — is rebuilt unchanged and the self-host fixpoint stays bit-identical. *Ref: Zig
  `comptime` assertions, C `static_assert`. Superior: the condition may call pure user functions (C2), runs fully
  at build time with exact integers, and emits an AI-legible error.*
- **C4b ✅ done (precomputed tables)** a `comptime` whose body is a pure function returning a data structure
  (e.g. a list) folds the **whole table** at compile time: every element is computed during the build and the
  generating function is never called at runtime — only the constant table survives (proven by `grep -c 'call
  squares' == 0` on the emitted asm). *Ref: Zig comptime tables, D's CTFE.* NOT yet, deferred by design: **build-time
  data embedding** (`embed(file)`, à la Zig `@embedFile`) reads a file during compilation, which is an I/O
  **capability** surface — it belongs behind the P26 capability model (like `lume pkg`'s `allow = [...]`), not a
  casual builtin, so it is left for a dedicated capability-gated design rather than shipped unguarded.

### M — macros (metaprogramming)
- **M1 ✅ done (#129)** the AST as a first-class comptime value (`compiler/stage1/lume/03_comptime.src`): `quote(<expr>)` reifies any Lume-Core expression into first-class algebraic constructor values (`Num(n)`, `Str(s)`, `Var(nm)`, `Bin(op, l, r)`, `If(c, t, e2)`, `LetIn(nm, v, b)`, `Call(nm, args)` with `Cons`/`Nil` argument lists) — the **exact same constructor vocabulary** `lumec.src` uses internally. `ceval` now evaluates ADT constructors (`cv_ctor`), structural equality `==`/`!=` over AST/constructor trees, pure string/int builtins (`str`, `count`, `at`, `slice`, `toint`, `ord`), and **`match` pattern-matching at compile time** (`PWild`, `PInt`, `PStr`, `PBind`, `PCons`), and `splice(<expr>)` reflects a comptime-computed AST value back into a live compiler AST node (validated for arity and operator legality via `ctfail`) that is subsequently compiled to native x86-64 with zero runtime overhead. *Ref: Lisp homoiconicity, Nim. Superior to Lisp/Nim: typed algebraic AST constructors with full `match` pattern matching (including string/int sub-patterns like `Bin("+", l, Num(0)) => l`) and compile-time structural validation on `splice`.*
- **M2 ✅ done** hygienic macros / quasiquote (`compiler/stage1/lume/03_comptime.src`): inside `quote(...)`, `unquote(<expr>)` evaluates `<expr>` in the enclosing comptime environment and interpolates the resulting AST value (or auto-lifts scalar `int` → `Num(n)` and `str` → `Str(s)`) into the quoted template; using `unquote` outside `quote(...)` or with wrong arity is a clear compile-time error (exit 3, no asm). Every `let` binding introduced inside a `quote(...)` template is automatically alpha-renamed with a step-stamped hygiene environment (`qenv`) while `unquote(...)` arguments are evaluated in the outer environment (`env`), so a macro's local `let tmp = ...` can never capture a caller variable named `tmp` passed via `unquote(...)` or confuse nested/sibling expansions of the same macro (`chygiene` canonicalises `_hyg_` bindings per statement into distinct stack slots while leaving macro-free code and the self-host fixpoint byte-identical). *Ref: Scheme `syntax-case`, Rust `macro_rules!`, Nim. Superior to Lisp `gensym` / Nim `genSym`: automatic lexical hygiene by default (zero manual `gensym` boilerplate) combined with direct algebraic AST pattern-matching on the same quasiquoted trees.*

### D — data (pillar P7)
- **D1 ✅ done (#120)** `decimal(p, s)` exact decimal type + literals + arithmetic. Exact values carry a signed coefficient and scale; casts, operators, function signatures, and literal-context coercion reject inexact rescaling and overflow instead of rounding. *Ref: COBOL/dBase, SQL NUMERIC.*
- **D2 ✅ done (#127)** `table[T]` row-type annotations and schema-checked in-memory relations via `read_csv(text, schema)`. CSV validates headers, quoting, row widths, and explicitly typed `str`/`int`/`decimal` cells; `lumec` propagates `T` through generic returns and rejects known row-type mismatches. Runtime schema reflection from nominal records is still future work. *Ref: dBase/Clipper, pandas, Arrow.*
- **D3 ✅ done (#131)** `compiler/stage1/query.src` adds typed `where`, stable ordering, equijoin, and grouped count/sum over D2 tables. Query row types are compile-time arguments erased before runtime; hooks in `compiler/stage1/lume/06_typecheck.src` validate fields, comparison types, join keys, and declared output records. Interpreter/native regression coverage and positive/negative type-check cases pass. The PR merged as `bd2a4a7` with all four required CI checks green. *Ref: LINQ (C#), SQL, Datalog.*

### T — one tool
- **T1 ✅ done (#118)** `lume` single CLI (`./lume`: `build`/`run`/`fmt`/`test`/`doc`/`check`/`describe`), backed by native Lume-Core tools (`compiler/stage1/ldoc.src` for Markdown + AI-first `--json` doc/signature/contract extraction, and `compiler/stage1/ltest.src` for zero-config `fn test_*()` discovery and native execution). *Ref: cargo, go.*
- **T2 ✅ done (#122)** one canonical formatter (`compiler/stage1/lfmt.src`, compiled to native ELF by `lumec` + `lasm` and wired into `lume fmt` / `lume fmt --check`). Parses Lume-Core declarations and statements into an AST while preserving `##` doc comments and section blank lines, aligns `match` arm `=>` arrows vertically, and performs precedence-aware minimal parenthesisation (stripping redundant parens while preserving required ones), proving both idempotence (`lfmt(lfmt(x)) == lfmt(x)`, including `lfmt(lfmt.src) == lfmt.src`) and byte-for-byte codegen invariance (`lumec(lfmt(x)) == lumec(x)`). *Ref: gofmt, rustfmt.*
- **T3 ✅ done** modules + package manager (`compiler/stage1/lmod.src` compiled to native ELF by `lumec` + `lasm`, wired into `lume build`/`run`/`test`/`check` and `lume pkg init|check|lock|verify|tree`). Resolves multi-module `use <mod>` graphs in topological order with diamond deduplication and circular-dependency/missing-module detection, validates `lume.toml`, writes and verifies deterministic SHA-256 content-addressed `lume.lock` lockfiles (P32), and enforces capability-scoped dependencies (P26: rejecting imported modules that invoke I/O builtins `read`/`readall`/`emit`/`flush` when `lume.toml` restricts capabilities to `allow = ["pure"]`). *Ref: cargo, go modules. Superior to cargo/go mod: capability-audited dependencies + content-addressed module verification.*

### E — self-evolution (`lume evolve`) — closes "Lume evolves Lume on its own"
- **E2 ✅ done** automated evolution gate: every PR runs `make bootstrap` in CI, asserting the triple
  self-build stays bit-identical, alongside `make test`/`asan`. Any change that breaks self-hosting is
  rejected automatically — the prerequisite for safe multi-contributor work. *Ref: our staged bootstrap + CI; `docs/04-roadmap.md`.*
- **E1a ✅ done (#138)** compiler-as-a-service: `lume compile [--json] [--run] <file.src>` runs the self-hosted compiler
  and returns a machine-readable result — `{ok, exit, asm_lines, diagnostics, ran, run_exit, stdout}` — so an agent
  (or the E3 loop) can submit source and read back structured diagnostics, the emitted assembly size, and, with
  `--run`, the program's native stdout, without scraping the CLI. The compiler itself is unchanged (source in →
  diagnostics / asm / result out). *Ref: Roslyn (C#), rustc query API, clangd.*
- **E1b ✅ done (#140)** in-process compiler library API (`compiler/stage1/lcaas.src` + `compiler/stage1/lume/07_driver.src`, wired into `use lcaas` and `lume agent`): exposes the Lume-Core compiler pipeline both as an in-process callable Lume-Core library (`use lcaas`: `caas_compile`, `caas_check`, `caas_ast`, `caas_expand`, `caas_symbols`, `caas_asm`, `caas_eval`, `caas_json`) and as a structured JSON query service (`lume agent [check|compile|ast|expand|symbols|eval] <file.src>`). Given source text in memory, `caas_compile(src)` returns `{ok, diagnostics, symbols, ast_sexpr, expanded_sexpr, asm, result}` without ever aborting the host process on invalid input, accumulating multiple F1 (`T1001`/`T1002`/`T1003`), F2 (`T2001`/`T2002`), C3 (`T3001`), and C1/C2/M1/M2 (`C1001`) errors in a single pass with deterministic AI-first fix-it suggestions (suggesting closest declared constructors or record fields) alongside the pre- and post-comptime/macro-expanded ASTs, native x86-64 assembly, and evaluated execution result. *Ref: Roslyn, rustc query API. Superior to Roslyn/clangd: single unified in-memory request returns multi-error diagnostics + deterministic fix-its + macro-expanded AST + native x86-64 asm + evaluated execution result, callable directly inside Lume-Core programs (`use lcaas`) for self-evolution (E3).*
- **E3 ✅ done** the governance loop (`compiler/stage1/levolve.src` importing `use lcaas`, wired into `use levolve` and `lume evolve verify|propose|approve|goals|budgets`, plus `evolve/goals.src`, `evolve/budgets.src`, and `evolve/proposals/0001_math_clamp`): an AI or human proposes a change against a baseline module with property/acceptance tests; `levolve` runs the P1/P30/P31 self-repair loop (`write -> compile -> read fixit -> repair -> re-verify`) in memory via `lcaas` deterministic fix-its (`T1001` constructor and `T1003`/`T2002` record field repairs), executes all 6 automated evolution gates (`compile`, `conformance`, `api_compat` typed signature preservation, `capability` P26 audit, `budget` P24 assembly line budget, and `fixpoint` deterministic expansion), and enforces the human governance rule (*"The AI proposes; the gates verify; the human decides"*): passing all 6 gates yields `verified_awaiting_human_approval` (`can_merge: false`) until an explicit maintainer approval (`lume evolve approve --by <maintainer>`) transitions it to `approved_ready_to_apply` (`can_merge: true`), while any failed gate non-bypassably yields `rejected_by_gates` (`can_merge: false`) even if human approval is supplied. *Ref: `docs/04-roadmap.md`. Superior to ad-hoc CI bots: compiler-native in-process AST/type/capability/API-diff verification + deterministic fix-it self-repair + non-bypassable dual gate (6 automated gates ∧ human approval).*

## Recommended order
C1✅ → C2 (deliver the flagship early) alongside F1 → F2 (foundation); E2✅ already protects `main`;
then C3✅ (needs F), M1/M2, D1 → D2 → D3, T1 → T3, E1 → E3.

Each brick: a green PR, a regression test, the fixpoint intact, verified by running it — and
**superior to the language it borrows from**.
