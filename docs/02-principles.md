# 02 — Design principles

Rules the language must obey. Each has a number so that spec sections, RFCs and compiler diagnostics
can cite them (`P7`). A feature that violates a principle needs an RFC explaining why, and the RFC
must name the principle it breaks.

## A. Who the language is for

- **P1 — AI author, human reviewer (primary).** Lume is authored by an AI model and reviewed and
  directed by a human (see `08-ai-first.md`). Constructs are judged for four readers — the AI author
  (primary), the human reviewer, the machine, and the model inside the program — but when machine
  checkability conflicts with human writing-convenience, the machine wins, because the AI writes the
  code and the human only needs to understand and approve the result. A feature that eases human
  typing but weakens the compiler's ability to catch the AI's mistakes is dropped.
- **P2 — Progressive depth, no cliffs.** A beginner's script and a verified kernel module are the
  same language. Adding types, contracts, effects or ownership annotations refines a program; it
  never requires rewriting it in another language or dialect.
- **P3 — Read more than write.** Prefer the form that is clearer to read. English keywords over
  symbols; one meaning per keyword; no sigils except `?` (optional/propagate), `!` (error union)
  and `@` (attributes).

## B. Syntax

- **P4 — One grammar, zero ambiguity.** The grammar is LL(k) or equivalent, context-free, with no
  feedback from the type checker to the parser. Every byte sequence is either one AST or one error
  with one location. Significant indentation is lexed into explicit `INDENT`/`DEDENT` tokens; tabs
  are an error.
- **P5 — One canonical formatting.** `lume fmt` is normative; the spec defines its output. Code
  review never discusses style. The grammar also accepts an explicit-brace form that formats to the
  canonical indented form.
- **P6 — Everything is an expression.** `if`, `match`, blocks, `loop` with `break value` yield
  values. Statements are expressions whose value is `()`.
- **P7 — Explicit over implicit.** No implicit conversions (except integer/float *literals* adopting
  the context type), no implicit copies of non-trivial values, no hidden allocation, no hidden
  control flow (no exceptions, no constructors running behind an `=`, no overloaded operators
  outside named traits).
- **P8 — Uniform call syntax.** `f(a, b)` ≡ `a.f(b)` for any function whose first parameter accepts
  `a` (UFCS). Methods are functions; there is no separate method namespace.

## C. Types and semantics

- **P9 — No null, no undefined behaviour in safe code.** Absence is `T?`. Failure is `T ! E`. A
  safe Lume program that compiles has a defined meaning on every input. `unsafe` blocks are the
  only place where the programmer, not the compiler, carries that proof, and they are auditable
  (`lume audit unsafe`).
- **P10 — Make illegal states unrepresentable.** Sum types with payloads, exhaustive `match`,
  range and refinement types, `distinct` newtypes, units of measure, linear capability types. The
  type system is the first line of testing.
- **P11 — Local reasoning.** A function's signature tells the truth about it: parameters,
  ownership, return, errors, effects. No global mutable state without an effect; no action at a
  distance. You can understand a function without reading the rest of the program.
- **P12 — Values by default.** `let` is immutable; records have value semantics; mutation requires
  `var`/`mut` and is visible at the call site. Shared mutability is a named, visible type
  (`rc`, `arc`, `cell`, `@gc`), never the default.
- **P13 — Errors are values, and the compiler knows which.** Error sets are inferred and checked;
  `?` propagates; `defer`/`errdefer` clean up; panics are for bugs, not for expected failure, and
  are not catchable in ordinary code.
- **P14 — Effects are part of the type.** What a function can do (`io`, `alloc`, `net`, `async`,
  `prob`, `gpu`, `unsafe`, `interrupt`, `realtime`, `llm`…) is inferred, enforced, and shown at
  `pub` boundaries. Purity is the absence of effects, and the compiler exploits it.
- **P15 — Contracts are code.** `requires`, `ensures`, `invariant` are part of the function, checked
  in debug/test builds, usable as documentation, and provable by `lume prove` on the pure subset.
  A spec that is not executable is a comment; Lume prefers executable specs.

## D. Memory and machine

- **P16 — Safe by default, control on request.** Ownership and inferred borrows cost nothing and
  are the default. `rc`/`arc` and `@gc` regions are explicit opt-ins with visible types. `unsafe`,
  `asm`, `@layout`, explicit allocators give total control when asked.
- **P17 — Zero cost means zero cost.** Abstractions compile away: generics monomorphise, iterators
  fuse, traits dispatch statically unless `dyn`, `comptime` runs at compile time, `defer` is a jump.
  If a feature cannot be made zero-cost, it lives in the standard library, not the core.
- **P18 — Layout is a contract.** `@repr(c)`, `@packed`, `@align`, `@soa`, `@mmio`, `@bits` give
  the programmer exact control over bytes, and the compiler never reorders what is declared
  explicit. C interop is by importing headers, both directions.
- **P19 — Every target, one source.** CPU, GPU (`kernel`), bare metal (`freestanding`), WASM,
  portable bytecode (`lumb`). Cross-compilation from any host is the default, not a feature.

## E. Data and AI

- **P20 — Data is native.** `record`, `table`, `index`, `relation`, `query`, `decimal`, `date` are
  core types/expressions, type-checked, with the same syntax over memory, files and databases.
  SQL strings are not a thing.
- **P21 — Models are values.** `model`, `tensor[T, shape…]`, `grad`/`vmap`/`jit` operators,
  `kernel` functions, `prob` effect, `rule` blocks and typed `llm fn`s are language constructs
  defined as sugar over the core, so the AI stack is in the language, not in a Python library.
- **P22 — Shapes, units and schemas are types.** Tensor shapes, physical units, decimal precision,
  database schemas and LLM output schemas are checked at compile time where static and at the
  boundary where dynamic.

## F. Growth and evolution

- **P23 — Small verified core.** All of Lume desugars to `lume-core` (~30 constructs with a
  mechanised semantics). Features are sugar via hygienic macros where possible. The core changes
  rarely and only with proofs updated.
- **P24 — Size budget.** The surface language has a budget (keywords ≤ 60, spec ≤ 200 pages).
  Adding costs; removing earns. A feature must pay for itself with concrete programs that improve.
- **P25 — Stability with editions.** Code that compiled in edition N compiles forever with
  `edition = N`. Breaking changes happen only in a new edition with an automatic migrator
  (`lume fix --edition`).
- **P26 — Semantic versioning is enforced.** The package tool diffs APIs and refuses a wrong
  version bump. Dependencies declare the capabilities (effects) they need; the build grants them.
- **P27 — Self-hosted, self-evolving, human-steered.** `lumec` is written in Lume. Changes to the
  language, compiler and standard library — including those proposed by AI through `lume evolve` —
  pass the same gates: conformance suite, triple self-build, verified-core proofs, benchmarks, and a
  human maintainer's decision. The bootstrap root is a small C interpreter kept for reproducibility.

## G. Tooling and experience

- **P28 — One tool.** `lume` does new/build/run/test/fmt/doc/prove/bench/pkg/repl/agent/evolve.
  No external build system is needed for a Lume project, ever.
- **P29 — Fast feedback.** Debug-tier edit→run under 200 ms for a typical module; incremental
  compilation and the REPL are architecture requirements, not optimisations added later.
- **P30 — Errors are a product.** Every diagnostic has a stable code, a plain-English message, the
  span, the expected vs actual, a suggested fix where one exists, and is available as JSON. The
  diagnostics style guide is normative.
- **P31 — Holes are first-class.** `todo` and `?` holes typecheck, run, and report the expected
  type and context — for humans and for AI fill-in.
- **P32 — Batteries included, capabilities declared.** A rich standard library ships with the
  compiler; packages declare effects; the store is content-addressed so two versions coexist and
  builds are reproducible.


## H. Software protection

- **P33 — The author owns the artifact.** *(design rule, not yet enforced — nothing is implemented)* The toolchain lets the author of a program control how
  much its shipped binary reveals (symbol stripping, obfuscation, constant encryption, selective
  virtualisation, tamper seals), per build profile, opt-in and composable, *without* changing the
  program's meaning (P9/P25), weakening compile-time safety, or hiding anything from the author
  themselves. Protection applies only to the author's own artifact; the toolchain ships no feature
  for attacking, deobfuscating or evading anyone else's software. See `06-software-protection.md`.

## I. AI-authorship guardrails (from the competitive scan, `07-competitive.md`)

- **P34 — Project-level laws.** A project may declare invariants in a `LAWS` block that the compiler
  turns into a proof/test obligation on *every* build, so an edit (human or AI) cannot silently break
  a cross-cutting rule. Per-function contracts (P15) guard a function; laws guard the whole program.
  (Modelled on Bend 2.)
- **P35 — No untested public surface.** *(design rule, not yet enforced — the compiler does not emit a missing-test diagnostic)* Every `pub` function carries at least one test or example
  the test runner executes; the compiler's JSON diagnostics report a missing one. AI-written code is
  never merged as untested code. (Modelled on NanoLang's mandatory `shadow` block.)