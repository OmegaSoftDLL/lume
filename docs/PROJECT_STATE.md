# Lume — Project State & Rules (handoff for any agent)

This document is the single source of truth for **how Lume is built, what exists, and
the rules every contributor (human or AI agent) must follow**. Read it fully before
changing anything. If you add a capability, update this file in the same change.

Repo: public, `github.com/OmegaSoftDLL/lume`. Primary dir: `~/Projetos/lume`.

---

## 1. Vision (what Lume is)

Lume is a programming language **designed to be written by AI and reviewed by humans**
("AI-first"). It takes the best ideas from every major language and must be **superior to
the piece it borrows, never a shallow clone**. End state: Lume generates Lume and evolves
itself; the toolchain depends on **no C and no external assembler/linker** — the AI
ultimately programs down to the bits (0s and 1s), with anti-reverse-engineering (REA)
protection. License: dual **MIT OR Apache-2.0** (like Rust).

## 2. Laws — the hard rules (do / do NOT)

**DO**
- **Converse with the user in Portuguese (pt-BR).** Write all project artifacts — docs,
  code, comments, filenames, commit messages, PRs — in **English**.
- **Golden rule: ZERO bugs in Lume's own toolchain.** It is inadmissible. Every fix ships
  with a regression test. If backends disagree, that is a bug — hunt it (a real `%`
  interpreter bug was found exactly this way).
- **Every advance is its own green PR, then merge.** Keep CI green. Use `gh`. Merge after
  checks pass.
- Run `make -C bootstrap/lume0 test` AND `make -C bootstrap/lume0 asan` before committing;
  both must be `0 failed`.
- Treat **C as the disposable seed only.** The seed interpreter `lume0` is C and will be
  deleted once Lume self-hosts. New language capability should move toward native (no C),
  and ultimately no `as`/`ld` either.
- For each borrowed feature, check our implementation is **genuinely superior** to the
  source language's version, not a shallow clone. If it is only a doc promise, say so.
- **Edit the self-hosted compiler in `compiler/stage1/lume/NN_*.src`, NOT in `lumec.src`.**
  `compiler/stage1/lumec.src` is **generated** (git-ignored) by amalgamating the
  `compiler/stage1/lume/*.src` modules in numeric order (`scripts/amalgamate.sh`, run as
  `make amalgamate`). The concatenation is byte-identical to the old monolith, so the
  fixpoint is preserved. See `docs/13-module-split-plan.md` for the module map.
- Keep this file and `docs/10-stage1-status.md` honest and current.

**DO NOT**
- Do **not** use Rust to implement Lume.
- Do **not** edit the generated `compiler/stage1/lumec.src` by hand — your change would be
  overwritten by the next amalgamation. Edit the module under `compiler/stage1/lume/`.
- Do **not** fake completion or claim a milestone that is not real. (The triple
  self-build / fixpoint **is reached** — `stage 2 == stage 3` in CI — so never regress it;
  state every other feature at its real level: shipped, a model/stub, or planned.)
- Do **not** let `check` regress; all 9 `examples/0*.lume` must `check` clean (name
  resolution has zero false positives by design — keep it that way).
- Do **not** add a predictive lottery-number feature (unrelated; a standing user rule).

## 3. Build, test, run

```
make -C bootstrap/lume0            # build the seed (strict: -Werror -Wconversion -Wpedantic -Wshadow)
make -C bootstrap/lume0 test       # full conformance suite
make -C bootstrap/lume0 asan       # same under ASan/UBSan
```
Seed CLI (`bootstrap/lume0/lume0`):
- `lume0 check <f.lume> [--json]` — lex + parse + **name resolution + basic type + duplicate checks**.
- `lume0 parse <f.lume> [--json]` — AST (parse only, no semantics).
- `lume0 run <f.lume|f.lbe> [args]` — interpret. NOTE: the interpreter does **not** print
  top-level expression values; only the asm backend does.
- `lume0 emit <f.lume> <out.lbe>` / `run <out.lbe>` — Lume Binary Encoding (AST as bytes).
- `lume0 emit-asm <f.lume> <out.s>` — direct x86-64 for the integer/fn/if subset (seed, C).
- `lume0 asm <in.s> <out>` — **our own assembler + ELF64 writer**: x86-64 asm -> static ELF,
  no `as`/`ld`/`cc`.
- `lume0 describe [--json]` — machine-readable self-description a model loads first; it now
  lists `check_catches`.

## 4. Architecture (three layers — do not confuse them)

- **Seed `lume0`** — `bootstrap/lume0/*.c` (~4.8k lines C11). lexer, parser, interp (dynamic),
  `lbe.c` (binary AST), `asmgen.c` (x86-64 for the integer subset), `resolve.c` (semantic
  checker), `elf.c` (assembler + ELF writer). Disposable.
- **Surface Lume (Layer A)** — the full language in `docs/03-specification.md` and
  `examples/`. Runs on the seed interpreter. This is what `lumec` is written in.
- **ML-core (Layer B)** — `compiler/stage1/data_*.lume`: a smaller ADT language
  (constructors, match, records, lists, strings, ints, builtins). It is the real
  self-hosting vehicle and **already lowers to code**:
  - `data_codegen.lume` -> C (transitional scaffold / correctness oracle).
  - `data_asmgen.lume` -> x86-64 (a codegen **written in Lume**, boxed-value heap via `brk`).
  - driver: `data_cc.lume` (.src -> .c), `data_asm_cc.lume` (.src -> .s).
- **Block language** — `compiler/stage1/block_*.lume`: an imperative mini-language
  (var/while/for/if/arrays) compiled to C by `block_cc.lume`.

Historical note (`docs/10`): an earlier gap had Layer A and the ML-core not unified and `lumec`
compiling only a subset. The triple self-build **has since been reached** (`stage 2 == stage 3`
in CI); sections 6 and "Next bricks" below predate Phases 1–5 and are kept only as history —
see the Phase 1–5 entries in §5 for the current state.

## 5. What is DONE (verified, with tests)

- Full toolchain-free native pipeline: `lume0 run data_asm_cc.lume prog.src prog.s` (codegen
  written in Lume) then `lume0 asm prog.s prog` (our ELF writer) -> native binary, with **no
  C, no as/ld, no libc**. Proven for nfib/ev/strtest/modtest/reclist/list AND the full
  core-written compiler `calc.src`, output identical to the as/ld oracle.
- ML-core -> native covers: ints, strings (==, ordering, concat, print via write), enum
  constructors + match, records + field access, lists + indexing, builtins
  count/at/slice/toint/str, let-in, if, recursion.
- **Top-level expression statements** (`ExprS` in `lumec.src`, `DExprS` in the `data_asmgen`
  oracle / `data_parser`): a bare top-level expression is evaluated for its effect and its value
  discarded (`popq %rax`, no `printval`) — as in C/Go/Rust. This let `lasm.src`'s entry point become
  `assemble(readall())` then `flush()` (no `print`), so `lasm`'s ELF is now **byte-for-byte identical
  to the `lume0 asm` oracle** (0 diff; previously 1 trailing `\n`). The fixpoint stays bit-identical
  (`lumec.src` itself uses no expression statements, so its parse is unchanged). The transitional C
  backend (`data_codegen`) and the tree-walking interpreter (`data_eval`) do not lower expression
  statements — nothing routes them there (native/asm only).
- `lume0 asm` encoder: mov/lea, ALU group, imul, neg/not/mul/div/idiv/inc/dec, test, cqto,
  movzbq/l, setcc, push/pop, jmp/call/jcc, syscall, ret; ModRM/SIB/REX; rip-relative;
  labels incl. GAS numeric (`1:`/`1f`/`2b`); .rodata/.ascii/.asciz; .lcomm bss.
- **Semantic `check`** (`resolve.c`): `R0200` undefined name, `T0001` literal type mismatch
  vs annotation, `D0001` duplicate parameter, `D0002` duplicate top-level function.
- **P35 test coverage warning** (`compiler/stage1/lcaas.src`): `lume agent check` reports
  structured warning `[P35]` for a `pub fn` without its matching `fn test_<name>()`. This is
  advisory and does not fail compilation; example discovery and strict enforcement remain future work.
- LBE (binary AST), `lume0 describe`, JSON diagnostics.

## 6. What is NOT done (promises, not code — be honest about these)

- No lexical scoping in the checker yet (resolution is flat per module: use-before-def and
  out-of-scope are not caught).
- No real type system: inference, call arity, match exhaustiveness, operator types. **(Phase 2 F1 ✅)** the
  self-hosted `lumec.src` now parses named `type`/`enum`/`record` declarations (inline, layout-free) and runs a
  **gradual** checker: a program with no `type` is unchecked (constructors free — backward-compatible, fixpoint
  intact); once it declares a `type`, every constructor use/pattern must match a declared enum variant + arity and
  every nominal `T{...}` field must belong to record type `T`, else a clear compile-time error (exit 3). Decls emit
  no code. Not yet: `fn` param/return annotations + arg-type checks (F2), HM inference (F3). See `docs/12`.
- **Phase 3 Track O (Ownership, Borrow Checker, `from` Regions & Memory Tiers: O1–O4 ✅)** is implemented in `compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/lume/04_parser_decls.src`, `compiler/stage1/lcaas.src`, `compiler/stage1/ownership.src`, and `compiler/stage1/own_ok.src`: enforces single-owner move semantics (`O0001` use-after-move, `O0002` move from borrowed parameter/field), deterministic LIFO `defer` / `errdefer` lowering, parameter borrow modes (`shared` default, `mut` exclusive mutable borrow with `O0010` immutable-to-mut and `O0011` overlapping-borrow checks, `own` consuming transfer with `O0012` call-site `own` enforcement), intra-module region inference and `pub fn ... from <param>` return-origin clauses (`O0020` escaping local borrow, `O0021` missing/mismatched `from` origin), and explicit shared/region tiers (`rc[T]`, `arc[T]`, `weak[T]`, `cell[T]`, `arena_new`/`arena_alloc`, `@gc` region boundary check, and `@freestanding` + `with alloc` `O0030` check), with structured JSON diagnostics and fix-its in `lcaas` / `lume agent`.
- **Phase 3 Track A (AI-Native Authoring: A1–A3 ✅)** is implemented in `compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/lume/04_parser_decls.src`, `compiler/stage1/lcaas.src`, `./lume`, and `compiler/stage1/ai_authoring_ok.src`: A1 typed holes (`_`, `_hole*`) with expected-type & scope inference, `lume agent fill <file.src>` (`caas_fill`) JSON candidate synthesis, gradual lowering in private drafts, and `[A0001]` rejection in `pub fn`; A2 `total fn` / `pub total fn` termination checking proving structural recursion on ADT constructor sub-patterns or guarded decreasing arithmetic measures (`n - k`, `n / k`) and rejecting non-terminating recursion with `[A0002]`; and A3 first-class provenance & uncertainty in the type (`unverified[T]`, `llm fn`, `prob_sample`), rejecting unverified flow into `pub fn` returns or trusted/effectful sinks (`io_write`, `db_exec`, `net_send`, `fs_write`) with `[A0003]` until discharged via `verify(val, contract)`, a contract branch guard, `law_check(val, law)`, or `evolve_approve(val)`.
- **Phase 3 Track K (Effect System, Algebraic Handlers & Perceus Reuse: K1–K3 ✅)** is implemented in `compiler/stage1/lume/01_lexer.src`, `04_parser_decls.src`, `05_codegen.src`, `06b_phase3.src`, `compiler/stage1/lcaas.src`, `compiler/stage1/effects_ok.src`, and `compiler/stage1/perceus_ok.src`: K1 effect rows (`/ {pure, io, fs, net, alloc, async, fail, panic, rand, time}` and user-declared effects), automatic intra-module effect inference for private functions, required `/ {...}` effect annotations on impure `pub fn` (`[E0200]` with inferred effect-row fix-it), and `@capability` module capability checking (`[E0200]`); K2 user-declared algebraic `effect Name { fn op(...) -> R }` and lexical `with handler Name { fn op(...) = ... } in <body>` discharging the handled effect from the enclosing function's row and lowering tail-resumptively across both direct operations and effect-polymorphic function calls (`olowercall_fn` / `kuses_hop` / `ksubst_e`) with zero closure/continuation allocation; and K3 Perceus in-place constructor reuse (`ktryp_perceus_arm` + `lreuse1`/`lreuse2` mutating `0(24(%rax))` and `8(24(%rax))` of an `own` matched constructor cell in place with zero `call alloc` / `call mkctor`).
- **Phase 3 Track S (Structured Concurrency, Channels & Supervised Actors: S1–S3 ✅, #193)** is implemented in `compiler/stage1/lume/01_lexer.src`, `02_parser.src`, `05_codegen.src`, `06b_phase3.src`, `compiler/stage1/lcaas.src`, `compiler/stage1/concurrency.src`, `compiler/stage1/actor.src`, and `compiler/stage1/concurrency_ok.src`: S1 lexical `scope` / `spawn` (`ScopeE`, `SpawnE`) and static `Send`/`Sync` checking (`[S0001]` rejecting unstructured `spawn` outside `scope`, capturing non-`Send` `rc[T]`/`cell[T]`/`Ref[T]`, or capturing unmoved `var`/`mut` borrows across `spawn`); S2 bounded/unbounded typed channels (`chan[T]`: `chan_new`, `chan_send`, `chan_recv`, `chan_close`), deterministic seeded `select` (`chan_select2`, `chan_select_timeout`), data-parallel combinators (`par_for_sum_range`, `par_map_scale`, `par_map_add`, `par_sum`, `par_count_above`), real OS multi-process parallelism (`chan_os_new`, `par_fork_scale2`, `par_map_scale_os` via `sys_fork` + `sys_pipe` + `sys_wait4` running concurrent workers in separate OS processes with distinct kernel PIDs), and hardware atomics (`atomic_xadd64` `lock xaddq`, `spinlock_acquire`/`spinlock_release` `lock cmpxchgq`) in `compiler/stage1/concurrency.src`; and S3 `std.actor` mailbox state machines, Erlang/OTP-style `supervisor` (`one_for_one`, `one_for_all`, bounded `max_restarts` escalation), and `std.time.logical` deterministic virtual clock & PRNG schedule seed (`lclock_new`, `lclock_advance`, `lclock_next_slot`) in `compiler/stage1/actor.src`.
- **Phase 3 Track V (Contract Verification, `lume prove` v0 & `LAWS` `P34`: V1–V3 ✅) and Stage 3 Exit-Criterion Capstone (`examples/apps/job_server/` ✅)** are implemented in `compiler/stage1/lume/04_parser_decls.src`, `06b_phase3.src`, `compiler/stage1/lcaas.src`, `./lume`, `compiler/stage1/contracts_ok.src`, and `examples/apps/job_server/`: V1 first-class `requires` / `ensures` / `invariant` and `old(...)` AST nodes (`VContractSig`, `VTypeInvariant`) lowered to sound `runtime_checked` assertions (`vwrap_fn_contract`, never eliding guards on sampled checks); V2 compile-time bounded counterexample search (`lume prove`, `lume agent prove`, `caas_prove`) classifying obligations as `refuted` (`[V0001]` with concrete counterexample bindings) or `runtime_checked` (full `QF_LIA` decision procedure for zero-overhead `proven` elision is tracked as future work); V3 top-level `law` declarations (`VLawDecl` + callable `law_<Name>`) checked against counterexample search and wired into `lume evolve`; and the Stage 3 capstone `examples/apps/job_server/` exercising Tracks O, A, K, S, and V with zero `unsafe`. All 5 tracks of Phase 3 (**O, A, K, S, V**) and the Stage 3 Capstone are 100% complete.
- **Big-Tech Engineering, Security & Release Gates (Lote 1, Lote 2 & Lote 3 ✅):** `.github/CODEOWNERS`, `.github/pull_request_template.md`, `.github/dependabot.yml` (weekly GitHub Actions security updates), `.github/workflows/codeql.yml` (CodeQL static analysis on `c-cpp` for the disposable Stage-0 C seed `bootstrap/lume0` and `actions` for workflows; the self-hosted `.src` compiler is analyzed by Lume's own `lcaas`/`lume prove` gates as CodeQL has no Lume extractor), `.github/workflows/ci.yml` enforcing `make bootstrap` (`stage2 == stage3`), `make zero-c` (both as a required status check and inside `bootstrap`), and dynamic `find`-based `./lume fmt --check` as required CI gates, plus `CHANGELOG.md` (Keep a Changelog format covering Stages 1–3 and Inflection Stages A–D) and `.github/workflows/release.yml` packaging self-hosted release binaries (`lumec`, `lasm`, `lume`, `lume0`) with `SHA256SUMS` on semver tags.
- **Inflection Plan (`docs/21-inflection-plan.md`) — P0 (#167 ✅), Stage A (#172 ✅), Stage B (#174 ✅), Stage C (#175 ✅) & Stage D (✅):**
  - **P0 (#167, PR #171 ✅):** Root-caused and fixed inter-procedural effect handler lowering (`olowercall_fn` / `kuses_hop` / `ksubst_e` / `kbind_hop_args` / `klower_eff_ops`), restored `execute_job_step` in `examples/apps/job_server/engine.src`, and added cross-platform conformance tests (`step(c) + step(c) == 210`).
  - **Stage A (PR #172 ✅):** Dogfooded V1 `requires` / `ensures` contracts directly into existing compiler passes with zero logic change (`numend`, `decimalend`, `decscan` in `01_lexer.src`; `findcloseb` in `02_parser.src`; `parsevariantend`, `parsetypedecimal2` in `04_parser_decls.src`; `tifn3`, `henv0`, `hbind` in `06_typecheck.src`), mirrored in `compiler/stage1/data_parser.lume` (`dwrap_contract`), verified as `runtime_checked` (never falsely `proven`) by `lume prove`, and covered by 9 per-contract regression tests (`compiler/stage1/compiler_contracts_ok.src`).
  - **Stage B (PR #174, Closes #173 ✅):** Graduated the verifier from v0 sampling to a sound `QF_LIA` decision procedure over $\mathbb{Z}$ via Fourier–Motzkin variable elimination (`vlin_*`, `vfm_*`, `vqflia_prove_fn`, `vqflia_prove_law` in `06b_phase3.src` and `cs_vlin_*`, `cs_vfm_*`, `cs_vqflia_prove_fn`, `cs_vqflia_prove_law` in `lcaas.src`) plus constraint-driven boundary counterexample extraction (`vpts_from_hyps` / `vfm_trace_hyps`). Obligations decided by `QF_LIA` are marked `proven` and elide their runtime guards (`vwrap_fn_c1`, including `imax` dogfooded in `05_codegen.src`); off-sample linear violations (`10 - x >= 0` at `x = 11`, `x - 1000000 != 0` at `x = 1000000`) are refuted at compile time with `[V0001]`; and out-of-fragment nonlinear obligations (`sq(x) = x * x`) remain `runtime_checked` with active runtime guards (`compiler/stage1/qflia_ok.src`). Also optimized `data_eval.lume` function lookup from O(N) linear scan to O(1) map lookup.
  - **Stage C (PR #175 ✅):** Dogfooded algebraic effects at the compiler's I/O boundary (`compiler/stage1/lume/07_driver.src` and mirrored in `compiler/stage1/data_parser.lume`): declared `effect CompilerIO { fn read_source() -> str }`, marked the five pure compiler pipeline passes (`lumec_lex`, `lumec_parse`, `lumec_check`, `lumec_expand`, `lumec_compile`) explicitly `/ pure` (enforced by `[E0200]`), defined `fn lumec_drive() / {CompilerIO} = lumec_compile(read_source())`, and discharged `CompilerIO` at the CLI entry point via `print with handler CompilerIO { fn read_source() -> str = readall() } in lumec_drive()`, covered by `compiler/stage1/compiler_effects_ok.src`.
  - **Stage D (PR #176 ✅):** Dogfooded ownership (`own`) and Perceus in-place constructor reuse (`__perceus_reuse_1` / `__perceus_reuse_2`) across hot compiler data structures (`joinall`/`pairs`/`pairs2` in `05_codegen.src`, called from `01_lexer.src`, `02_parser.src`, and `05_codegen.src`; and `gerase`/`gestmts`/`gestmt`/`geexpr`/`geargs`/`gerec`/`gearms` plus `cfold`/`cfoldstmts`/`cfoldstmt` in `03_comptime.src`, wired in `07_driver.src` and mirrored in `compiler/stage1/data_parser.lume`), eliminating heap allocation (`call alloc` / `call mkctor`) in `gestmts`, `geargs`, and `cfoldstmts` and reusing 1- and 2-field AST cells in place across `pairs2`, `gerec`, `gearms`, `gestmt`, `cfoldstmt`, and `geexpr` (`compiler/stage1/compiler_perceus_ok.src`). All stages (**P0, A, B, C, D**) of `docs/21-inflection-plan.md` are 100% complete.
- **Compiler-as-Oracle & Intent-as-Source Loop (`docs/19-intent-as-source.md` ✅):** Implemented in `compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/lcaas.src`, `compiler/stage1/levolve.src`, `compiler/stage1/intent_oracle_ok.src`, and `evolve/proposals/0002_intent_oracle/proposal.bundle`: combines typed holes (`_`, `_hole*`) with the sound `QF_LIA` decision procedure (`vqflia_prove_fn`/`vqflia_prove_law`), `requires`/`ensures` contracts, and `law` (`P34`) declarations to synthesize, prune, and prove AST candidates (`v + 1`, `v - 1`, `v + w`, `v - w`, `w - v`, `if v >= w then v else w`, `if v <= w then v else w`, variables, and helper calls), emit `proven_candidates` in `lume agent fill`, elide runtime guards (`call ctfail` == 0) on `QF_LIA`-proven synthesized implementations, and automatically repair `A0001` holes in `lume evolve`.
- **Track M (first-class comptime AST + hygienic quasiquote macros) is done (M1 #129, M2)** in `compiler/stage1/lume/03_comptime.src`: `quote(<expr>)` reifies expressions into Lume-Core's own AST constructor vocabulary (`Num`, `Str`, `Var`, `Bin`, `If`, `LetIn`, `Call` over `Cons`/`Nil`), `ceval` evaluates constructors, structural `==`/`!=`, pure string/int builtins, and `match` patterns over AST trees, `unquote(<expr>)` interpolates comptime AST values (and auto-lifts `int`/`str` scalars) inside `quote(...)` with automatic lexical hygiene (`let` bindings inside `quote(...)` are alpha-renamed per expansion so caller variables passed into macros are never captured), and `splice(<expr>)` validates and splices the resulting AST into native code with zero runtime overhead. See `compiler/stage1/{ast_meta,macro_hygiene}.src`.
- `comptime`: a **seed** exists (Phase 2, first brick). `lumec` folds `comptime(<expr>)` at compile time
  over the pure core (`Num`/`Str`/`Bin` with `+ - * / % == != < > <= >= and or`/`If`/`LetIn`/`Var` bound by a
  comptime `let`), emitting the result as a constant (`comptime(2 + 3 * 4)` emits the same asm as `14`); any
  non-foldable expression (a function call, `match`, record/field, free `Var`, `read()`) is a hard compile-time
  error (`ctfail` → clear stderr message, exit 3, no output). **(C2 ✅)** comptime now also evaluates calls to pure
  user functions (recursion + `if` + arithmetic) with step/depth guards; **(C3 ✅)** comptime **types** give generics
  (`fn id(comptime T: type, x: T)`): a type name is a comptime value, the comptime parameter is erased at runtime
  (recorded in a `GSig` statement, dropped from `Fn` and from every call by the `gerase` pass, so the fixpoint is
  byte-identical), and the call site binds the type parameter and checks the remaining annotated arguments via the
  F2 checker (`id(int, "hi")` → clear type error, exit 3); **(C4 ✅)** `comptime assert(cond, "msg")`, precomputed
  comptime lookup tables (`comptime(squares(6))`), and `comptime T: type` generics over user-declared `record`/`enum`
  and parameterized container types (`Box[T]`, `Option[T]`, `[T]`, `table[T]`, #212). See
  `compiler/stage1/{ct,ct2,generics_ok,generics_container_ok,comptime_assert,comptime_table}.src` and the `phase-2` tests.
- Domain features D1–D3 (`decimal`, `table[T]`, and typed table queries) are implemented;
  tensor/`grad`/`llm fn` remain future work, and `examples/future/*` pass the lexer only.
  Track D1 (`#120`) parses exact decimal literals as signed integer
  coefficient + scale values in both the ML-core parser and self-hosted `lumec`.
  `compiler/stage1/decimal.src` provides parsing, scale-preserving formatting, checked
  add/subtract/multiply, comparison, `decimal(p, s, value)` casts, exact division at a requested
  scale, and shortest exact division for finite decimal results. Decimal records carry a precision
  field after explicit casting; add/subtract preserve integral capacity, multiplication combines
  input precision, and division derives a result precision. Operators are lowered for literals,
  helper-call results, local variables, and annotated function parameters/returns; function
  signature context inserts checked casts for exactly representable literals. Coefficient digits
  and scale are capped at 18, and inexact rescaling, overflow, division by zero, or repeating
  decimal division fail instead of rounding. Interpreter, C oracle, and native assembly agree on
  the regression suite.
  Track D2 (`#127`) is implemented in `compiler/stage1/table.src`: `read_csv(text, schema)` parses
  CSV fields (including quoted delimiters, doubled quotes, CRLF, and multiline quoted fields),
  requires an exact header/schema match, converts `str`, `int`, and `decimal` cells without type
  guessing, and returns a relation with typed cell tags and explicit errors. `table_row_count`,
  `table_column_count`, and checked `table_cell` access the relation. `lumec` accepts `table[T]`
  function annotations and substitutes the row type through C3 generic return types, rejecting
  known mismatches. The CSV runtime schema remains explicit and caller-supplied; reflecting a
  nominal row record into a schema automatically remains future work. Track D3 (#131) adds
  compiler/stage1/query.src: query_where, stable query_order, equijoin query_join, and
  query_group with count and sum. Calls carry compile-time row type names, erased by
  compiler/stage1/lume/03_comptime.src; compiler/stage1/lume/06_typecheck.src checks declared
  row fields, comparison value types, join key compatibility, and group/join output record shape.
  Runtime schema lookup still checks columns and returns table errors; join output columns use
  <prefix>_<field>. Regression coverage compares the ML-core interpreter and native assembly,
  and checks accepted and rejected typed queries. PR #131 merged as bd2a4a7; its final four
  required CI checks passed, including Self-host bootstrap (stage2 == stage3) and Stage 0 on
  Ubuntu/macOS. An initial Ubuntu test failure exposed a missing local list-append helper and a
  missing expected group-count line; both were fixed in f44b12b, then all checks passed. D1,
  D2, and D3 are complete. Future data-track scope is open. See
  docs/14-data-track-handoff.md for the detailed D1–D3 handoff.
- **Track T (`lume` single CLI, `lcli.src` self-hosted CLI control plane, `lfmt.src` canonical formatter, `lmod.src` + `lume pkg` modules & package manager, live stdio `lume lsp`/`lume dap` servers & interactive `lume repl`) is done (T1 #118, T2 #122, T3 #124, #204, #205, #210, #211)**: `./lume` provides `build`, `run`, `test`, `doc` (`--json`), `fmt` (`--check` / `--stdout`), `pkg` (`init`/`check`/`lock`/`verify`/`tree`), `compile` (`--json`/`--run`), `agent` (`check`/`compile`/`ast`/`expand`/`symbols`/`eval`/`witness`/`repair`/`impact`/`hashes`/`grammar`/`project`/`lsp`/`dap`/`repl`/`c-export`/`kernel`), `lsp` (`--stdio` JSON-RPC 2.0 server loop over stdio supporting `Content-Length:` and line-delimited framing, `initialize`, `textDocument/didOpen`, `textDocument/didChange` pushing `textDocument/publishDiagnostics`, `textDocument/hover`, `textDocument/codeAction`, `textDocument/documentSymbol`, `shutdown`/`exit`, plus `--json <file.src>` one-shot mode), `dap` (`--stdio` DAP 1.65 server loop over stdio supporting `initialize`, `launch`, `next`, `stepIn`, deterministic `stepBack` record-replay, `stackTrace`, `scopes`, `evaluate`, `disconnect`, plus `--json <file.src>` one-shot mode), `repl` (interactive line-by-line read-eval-print loop accumulating definitions across turns and rolling back failed turns, plus `--kernel` multi-message Jupyter protocol loop with `execution_count` and first-class `application/vnd.lume.table+json` table MIME rendering), `check` (`--json`), and `describe` (`--json`), backed by native Lume-Core tools (`compiler/stage1/lcli.src`, `compiler/stage1/ldoc.src`, `compiler/stage1/ltest.src`, `compiler/stage1/lfmt.src`, `compiler/stage1/lmod.src`, and `compiler/stage1/lcaas.src` compiled to ELF by `lumec` + `lasm`, with `lcli2 == lcli3` verified in the bootstrap fixpoint and `bootstrap/lume0/SEED_FREEZE.sha256` freezing the Stage-0 C seed). `lfmt.src` proves idempotence and byte-for-byte codegen invariance; `lmod.src` + `lume pkg` provide topological `use` bundling, cycle detection, SHA-256 `lume.lock` verification (P32), and capability-scoped dependency auditing (P26). Surface `.lume` files use the seed lexer/parser/resolver for `check` and token-based zero-argument `test_*` discovery with real interpreter execution; textual prefix checks are not used.
- **Track E1 (Compiler-as-a-Service, AI-Author Substrate & LLM-Generatability CI Gate: E1a #138 + E1b + #209 + #230) is done** in `lume`, `compiler/stage1/lcaas.src`, `compiler/stage1/lume/07_driver.src`, and `scripts/check-llm-gen-eval.sh`: exposes the compiler pipeline both over the process boundary (`lume compile --json --run`) and in-process via `use lcaas` (`caas_compile(src)` returning `{ok, diagnostics, symbols, ast_sexpr, expanded_sexpr, asm, result}`) + `lume agent <cmd> <file.src>` (structured JSON), with non-aborting multi-error accumulation across F1/F2/C1/C2/C3/M1/M2, deterministic fix-it suggestions, per-definition content-addressed AST hashes + proof-status cache keys (`lume agent hashes`), constrained-decoding grammar & AST constructor schemas (`lume agent grammar`), canonical AST/surface projectional round-trip verification (`lume agent project`), and the `scripts/check-llm-gen-eval.sh` CI gate (`tests/llm_gen_eval/`, measuring `pass@1`, `pass@2`, and `total_tokens_to_green` to produce Lume source that both compiles AND discharges `QF_LIA` proofs, explicitly labelled `"eval_mode":"recorded_transcript_fixture"` when run offline).
- **Stages 1–5, Inflection Stages A–D & Intent-as-Source Oracle are 100% done**:
  - **Stage 1 & 2:** Self-hosted `lumec` + `lasm` + `lcli` (`stage2 == stage3` fixpoint, Zero-C gate + `SEED_FREEZE.sha256` #204/#205), Comptime (`C1–C4`, including `C4c` generics over declared containers #212), Hygienic Macros (`M1–M2`), Data/Queries (`D1–D4` #214: `decimal(p,s)`, `table[T]`, typed `query`, unit-typed `money[CCY]` `[T2001]`, durable on-disk WAL `persist table` written via Linux `open` + `flock(LOCK_EX)` + `write` + `fsync` + `close` in `fs_write_sync`/`fs_append_sync`/`fs_read_file` surviving process kill & concurrent multi-process access via `persist_recover_path` / `persist_tx_insert_row`, plus in-memory `versioned` time-travel & `@soa` columnar layout), Single-Binary CLI + live stdio `lume lsp`/`lume dap` servers & interactive `lume repl` / Jupyter `--kernel` (`T1–T3`, #210, #211), `lume evolve` + AI-Author Substrate + LLM-gen eval gate (`E1–E3`, #209, #230), Hindley–Milner + F3c `let`-polymorphism (`Forall`/`hgeneralize`/`hinstantiate`, `[T0500]`) + F4 Unification (`F1–F4`), Core Features (`#197` + `#215` shipped: `trait`/`impl` with static method dispatch `[T0400]`/`[T0401]`, first-class closures with lexical environment capture & `Send` checking `[S0001]`, refinement types `T where <pred>` with `QF_LIA` `proven` guard elision / `[V0001]` refutation / exit-3 runtime guards, `async fn`/`.await`/`block_on` `[E0200]`, compiler-level `rc[T]`/`arc[T]`/`weak[T]` with literal `rc(v)`/`arc(v)` constructors, automatic deref on field access & UFCS trait calls, native heap control blocks (`__lume_rc_*`/`__lume_arc_*`/`__lume_weak_*` with `mfence` + `lock incq`/`lock xaddq`), `[O0001]` use-after-drop & `[S0001]` non-`Send` `rc[T]` capture rejection, `shard` `[S0001]`/`[E0200]`, and compiler-level `promise[T]`/`signal[T]` with `promise(v)`/`signal(v)`, `.resolve(v)`/`.await`/`.get()`/`.set(v)`/`.version()`, `[T2001]` type-mismatch enforcement, and native `__lume_promise_*`/`__lume_signal_*` heap cells), and `examples/apps/inventory/`.
  - **Stage 3 & Inflection Plan (`docs/21-inflection-plan.md`, `docs/19-intent-as-source.md`):** Ownership & Borrows (`O1–O3`), AI Authoring & Provenance (`A1–A3`), Algebraic Effects & Perceus Reuse (`K1–K3`), Concurrency & Supervised Actors (`S1–S3`), Sound `QF_LIA` Contract & `law` Verification (`V1–V3`), Hole-Guided Synthesis + Oracle `witness`/`repair`/`impact` Queries (`intent_oracle_ok.src`, `lume agent witness|repair|impact`, `lume mcp`, #195), `lume evolve` Governance Gates + RFC 8032 Ed25519 Maintainer Signatures (`evolve/maintainers.pub`) + End-to-End `lume evolve apply` (`levolve.src`, `evolve/proposals/0003_decimal_clamp/proposal.bundle` evolving `compiler/stage1/decimal.src`, #194, #203), and `examples/apps/job_server/`.
  - **Stage 4 (`docs/24-phase4-plan.md`, #192, #250):** Shape-typed `tensor[DType, D1, ...]` (`[T0300]`, `tensor.src`), differentiable `model` + `grad`/`value_and_grad`/`jvp`/`vjp` over the polynomial fragment (`+`, `-`, `*`, `if`, `let`, `relu`, `sq`, and division/modulo by constants) with `[D0001]` / `[N0400]` diagnostics on non-differentiable operators/calls and `QF_LIA` gradient verification, compile-time `jit(fn, args...)` call-site specialisation & constant folding (`n4_lower_jit_call`), AST-lowered `kernel fn` to SSA CUDA PTX 8.0 (`ld.param.s64`, `add.s64`, `sub.s64`, `mul.lo.s64`, `div.s64`, `rem.s64`, `setp.<cmp>.s64`, `selp.s64`, `st.global.s64`) and Vulkan SPIR-V 1.6 (`OpConstant`, `OpIAdd`, `OpISub`, `OpIMul`, `OpSDiv`, `OpSMod`, `OpSGreaterThan`/`OpSLessThan`/`OpIEqual`, `OpSelect`, `OpStore`) with instruction-level PTX (`ptx_vm_exec` / `cs_ptx_vm_exec`) and SPIR-V (`spirv_vm_exec` / `cs_spirv_vm_exec`) virtual machine execution & cross-backend equivalence verification (`"gpu_emu_verified": true`, `[E0200]` purity check + `[K0400]` rejection on non-scalar constructs; physical GPU driver launch remains planned), `nn.src` with cryptographic FIPS 180-4 SHA-256 `model_content_hash` and weight-bearing `safetensors` (`dtype`, `shape`, `data_offsets`, `data`) / `onnx` (`initializers`) serialization, `std.prob` (`sample_bernoulli`, `observe_bernoulli`, and distinct step/particle-sensitive `infer_mcmc`, `infer_svi`, `infer_smc` over $(3,2)$ Padé natural-log Beta-Bernoulli likelihoods) + genuine semi-naive Datalog (`dl_seminaive_loop` joining `delta` facts + named relations `dl_fact`/`dl_query_rel`) + `extern llm fn` + LIF `std.neuro` (`ai_runtime.src`), and capstones `examples/apps/mnist/` & `examples/apps/agent/`.
  - **Stage 5 (`docs/25-phase5-plan.md`, `docs/26-targets.md`, `docs/27-systems-and-kernels.md`, `docs/06-software-protection.md`, `docs/20-interop.md`, `docs/22-adoption.md`, `docs/23-horizon.md`):** Machine-level layout & MMIO (`@repr(c)`, `@packed`, `@bits`, `@mmio` in `interop.src`), C/WASM Interop (`#191` shipped: `extern "c" fn` with `"ffi"` effect `[E0200]`, foreign provenance taint `[A0003]`, and System V AMD64 unboxing/boxing trampolines in `05_codegen.src`; dynamic ELF64 `PT_INTERP`/`PT_DYNAMIC`/`DT_NEEDED`/`.dynsym`/`.got.plt`/`.rela.dyn`/PLT emission in `lasm.src`; `pub extern "c" fn` + `@export("C")` System V AMD64 C-ABI entrypoints with lazy Lume heap init compiled to `.a`/`.so`/`.h`/`.wat`/`.wasm` via `lume build` and `lume agent c-export` with `[I0002]` WASM fragment diagnostics; `[ffi] libs` manifest parsing + `[E0200]` capability enforcement in `lmod.src`; N-parameter `lume agent bind <header.h>`; `std.wasm` is an in-language capability/fuel/bounds reference sandbox model paired with compile-time `[E0200]` capability enforcement), Systems & Kernels K0–K9 (`#227` shipped on x86-64 QEMU: `@freestanding` zero-syscall codegen & physical frame allocator at `0x200000`, system ISA `cli`/`sti`/`hlt`/`iretq`/`lgdt`/`lidt`/`invlpg`/`movq %crN`/`outb`/`inb`/`int $imm8`/`lock xaddq`/`lock cmpxchgq` and `.kernel` MBR + 4-level x86-64 page-table Long Mode image writer `asmkernel` in `lasm.src`, volatile MMIO, hardware 64-bit IDT + `@interrupt(irq: 32)` ISR lowering + `iretq`, COM1 `0x3F8` UART driver, `@bound(cycles: N)` structural cycle-budget analyzer emitting `"status":"budget_checked"` or `[V0002]`, linear `cap[Page]`/`cap[Port]`/`cap[Irq]` `[O0001]`, and `examples/apps/lume_os/kernel_qemu.src` booting in `qemu-system-x86_64`), Universal Targets T0–T5 (`#206` shipped in `compiler/stage1/ltarget.src`: orthogonal `(ISA encoder) ⊥ (container writer) ⊥ (OS/ABI layer)` selector `lume targets [--json]` and `lume build --target <triple> <file.src> -o <out>` supporting `x86_64-linux-elf`, `aarch64-linux-elf` verified in `qemu-aarch64`, `aarch64-macos-macho` 16 KB page-aligned Mach-O 64, `x86_64-windows-pe` PE32+/COFF `.exe`, `riscv64-linux-elf` RV64IM verified in `qemu-riscv64`, `<isa>-none-ihex` Intel HEX firmware with verified checksums, `x86_64-none-raw` bootable MBR+Long Mode kernel, `wasm32-wasi`, and `lumb` `LUMB\x01` portable bytecode + VM `lume run <file.lumb>`, preserving `QF_LIA` `proven` guard elision and native target-ISA exit-3 runtime contract traps), Standalone Independent Proof Certificate Kernel & MCP (`lcert.src` with zero compiler imports, FIPS 180-4 SHA-256 tamper verification, and standalone `QF_LIA` solver; `lume certify`, `lume verify-cert`, `lume mcp`), Mechanised Lean 4 Lume-Core Formalisation (`formal/lean/LumeCore.lean`, machine-checked in CI with zero `sorry`/`axiom` proving `[O0001]`, `[E0200]`, `[A0003]`, determinism, type preservation, and `QF_LIA` contract guard elision soundness, #213), P33 Software Protection across Bricks W1–W5 (`compiler/stage1/protect.src`, `compiler/stage1/lprotect.src`, `[W0001]`, `lume build --protect <standard|hardened|max> [--lsym <file.lsym>]`), and Stage 5 capstone `examples/apps/lume_os/`.
  - **Adoption & Distribution (`docs/22-adoption.md` G0–G9, `#196` shipped) & Horizon / Intent-as-Source §3–§5 (`docs/19-intent-as-source.md`, `docs/23-horizon.md` H1–H5, `#198` shipped):** Zero-C installer `scripts/install.sh` + `lume self status|verify|update` with `MANIFEST.sha256` (G0); 15-tool `lume mcp` server + `scripts/check-llm-gen-eval.sh --leaderboard` (G1); Verified Sidecar interop `examples/interop/sidecar/` (`payment_core.src`, `host_c.c`, `host_python.py`, `host_node.mjs`, G2); proof-gated registry publishing & auditing `lume pkg publish` / `lume pkg audit` rejecting unproven packages with `[V0001]` (G4); semantic proof-diff review `lume prove --diff` (`@mode proof-diff`, `[V0003]`, G5); shareable proof bundles `lume prove --share` (`@mode share`, `lume://verify/sha256:...`) + `docs/30-lume-book.md` (G6); fintech supply-chain security demo `examples/apps/supply_chain_demo/` (G7); RFC & governance `docs/31-rfc-and-governance.md` (G9); semantic AST-patch editing `lume agent patch` (`@mode patch`, `[A0004]` / `[V0001]`, `docs/19` §3); deterministic logical-time effect-trace record & replay with `first_negative_step` bisection `lume agent replay` (`@mode replay`, `docs/19` §4); first-class provenance & decision lineage queries `lume agent lineage` (`@mode lineage`, *"why am I this?"*, `docs/19` §5); compositional multi-module & relational law verification + linear temporal logic (`temporal law`, `always` $\Box$, `eventually` $\Diamond$, `until` $\mathcal{U}$, `leads_to` $\leadsto$ proved by `QF_LIA` inductive invariants & ranking functions, with `[V0005]` multi-step trace & Büchi lasso counterexamples and standalone `lcert.src` certificate verification) + learned⊕proven seam verification `lume prove --compositional` (`@mode compose`, `[V0004]` / `[V0005]`, H1 & H4); and anti-rot dependency proof-delta checking `lume pkg check-rot` (`@mode check-rot`, `[V0003]`, H2).

## 7. Genuine differentiators (real, with evidence)

- **LBE** — `lbe.c`: AST as bytes, executed without re-parse. "A syntactically invalid
  program cannot exist."
- **No-C / no-as-ld bootstrap** — codegen in Lume (`data_asmgen.lume`) + our own assembler/ELF
  (`elf.c`). Rare at this stage; supports the trusting-trust / reproducibility thesis.
- **AI-first loop** — `lume0 describe` + stable JSON diagnostics + a `check` that now does
  real semantics (edit -> check -> fix).

## 8. Conventions & PR workflow

- One change = one branch = one PR. Commit with `git -c user.name="Ricardo Bueno"
  -c user.email="rmixirica@gmail.com" commit -F <file>` (use `-F` or a quoted heredoc to
  avoid backtick/`$` issues in bash).
- Commit messages end with: `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`
  (follow the current session's attribution reminder if different).
- PR descriptions end with: `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
- Conformance tests live in `tests/conformance/stage1/*.expected` and
  `bootstrap/lume0/tests/`. A generic loop runs any `*.expected` whose basename matches a
  `compiler/stage1/<base>.lume` through `lume0 run`; so **native/asm test expected files must
  NOT share a basename with a `.lume`** — use a distinct suffix (e.g. `_asm`). `.src` files
  are ignored by that loop.

## 9. Known pitfalls (hard-won — do not rediscover)

- **Reserved keywords cannot be identifiers** (`loop`, `scope`, `fail`, `in`, `is`, `mut`,
  `own`, ...). Using one as a var/param gives `P0153 expected a pattern` or `P0001 expected
  an identifier` / `R0001 undefined name`. Rename (`loop`->`lp`, `fail`->`flbl`).
- In `.lume` **string literals, `{` and `}` trigger interpolation** — double them (`{{`/`}}`)
  when emitting C/asm that contains braces.
- **Do NOT edit `.lume` string literals with Python heredocs** — it corrupts literal `\n`.
  Use the Edit tool for those.
- `\n` in a `.lume` string becomes a newline; `\\n` stays the two chars `\n`; `\"` is a quote
  (matters when emitting C/asm text).
- **AST shapes** (from `lume0 parse`): `if/elif/else` is a FLAT `N_IF` with kids
  `[cond0, body0, cond1, body1, ..., elseBody]` (else optional = odd kid count), NOT nested.
  `for i in a..b` is `N_FOR[PBIND, N_RANGE[lo,hi], BLOCK]`, inclusive via the `F_INCLUSIVE`
  flag (not the text). `FIELD_GET` stores the member in `.text` (not a child); `OPTCHAIN`
  stores the member as `kids[1]` (skip it in name resolution). `use mod (a,b)` currently
  parses the import group as a trailing `TUPLE`.
- The ML-core / seed interpreter does not print top-level values; the asm/native backends do.
- Lume methods supported by the interp: chars/concat/contains/entries/enumerate/join/keys/
  len/map/push/repeat/sorted/split/str (no `.slice` method — use the `slice(s,i,j)` builtin
  in the ML-core, or `.chars()` to iterate).
- Native runtime builtins include `slicen(s, len, i, j)` — a length-aware `slice` that clamps
  against the passed `len` instead of calling `strlen` (O(1), not O(n)). Use it on a hot path
  that already knows the string's length (e.g. the assembler's line splitting over a large
  source); it is in both `lumec.src` and `data_asmgen.lume` runtimes, byte-identical.
- **Memory is the scaling wall — build strings in O(n), and index strings in O(1).** The seed
  interpreter and the native runtime (`alloc` in `data_asmgen`) have **no GC**: nothing is freed,
  so peak memory is the TOTAL allocated. Two consequences, both already hit and fixed: (1) never
  accumulate output with `s = s + piece` in a loop (O(n^2) copies that all stay live) — gather
  chunks in a `[str]` and `join("")` once, as `data_asmgen` now does for funcs/main/rodata and
  inside `aexpr`; (2) indexing a string `s[i]` / `s[i..j]` must NOT rebuild a per-codepoint list
  each time — the seed now walks UTF-8 bytes (`str_cp_byteoff` in `interp.c`), O(1) alloc per
  access. Both were the reason a ~100-line core file needed gigabytes to compile. **When the
  lexer/compiler are rewritten native (self-hosting), index source by byte offset, not by
  materialising `chars()`, and plan real memory management for the native runtime** — a bump
  allocator that never frees will not survive compiling the whole compiler.
- `elf.c` and everything else must build under `-Werror -Wconversion -Wpedantic -Wshadow`:
  no `strdup` (C11; provide your own), one statement per line after an `if`/`for`
  (misleading-indentation), explicit casts for byte/size math.

## 10. Next bricks (priority order)

1. Lexical scoping in the checker (use-before-def, out-of-scope).
2. Fuller type checking: call arity, match exhaustiveness, operator/return types.
3. Unify the surface language and the ML-core front-ends.
4. Grow the native/ELF path to the full surface language; then self-host `lumec`; then the
   triple self-build (Phase 1 exit).
5. Turn the showcased safety features into real enforcement (effects, contracts, ownership).

Keep each brick a green, tested PR, and keep it **superior to the borrowed idea**, not a clone.

## 11. Publicizing the language (GitHub recognition)

**Preparation status:** issue #217 stages a TextMate grammar, a draft languages.yml entry, and four
non-trivial project samples under contrib/. The .lume extension has no current Linguist entry;
.src is intentionally excluded because it is generic. The existing public
lume-lang/tree-sitter-lume grammar uses syntax different from this project's specification, so it
is not used as the grammar source.

**Validation:** .github/workflows/linguist-grammar.yml runs Linguist's script/add-grammar against
the current Lume revision in a temporary Linguist checkout. The workflow uses the public Lume
repository as the grammar source and does not publish changes to github/linguist.

**Submission gate:** adding the language definition and samples to github/linguist remains subject
to the existing maturity gate below.

GitHub's repository language bar is computed by **Linguist**, which only lists languages already
in its catalogue; a brand-new language like Lume cannot appear there via `.gitattributes`
(the `linguist-language=Lume` override is ignored for an unknown name). Today the C seed and
scaffolding are vendored so the bar is not dominated by C, and the "100% Lume" signal lives in the
README badge, the repo description and topics.

**Agreed future task:** once Lume is mature — a stable grammar/spec and the `.lume` extension in
use across several public repositories — submit Lume to `github/linguist` (a PR to that repo with
a language definition, color, sample files and usage evidence) so GitHub recognizes and displays it
officially. Until then, do not fake it by mapping `.lume` to another language.
