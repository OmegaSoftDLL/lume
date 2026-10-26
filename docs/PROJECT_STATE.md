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
- Keep this file and `docs/10-stage1-status.md` honest and current.

**DO NOT**
- Do **not** use Rust to implement Lume.
- Do **not** fake completion or claim a milestone that is not real (e.g. the triple
  self-build / fixpoint is NOT done — never say it is).
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

The honest gap (`docs/10`): Layer A and the ML-core are not unified; `lumec` compiles only a
subset; the triple self-build is not reached.

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
- Ownership/borrow, effects (parsed then discarded), contracts (runtime only, no `prove`/SMT)
  — all documented, none enforced.
- Concurrency (`spawn`/`select`/`async`), `macro`, C interop, cross-compile — keywords only.
- `comptime`: a **seed** exists (Phase 2, first brick). `lumec` folds `comptime(<expr>)` at compile time
  over the pure core (`Num`/`Str`/`Bin` with `+ - * / % == != < > <= >= and or`/`If`/`LetIn`/`Var` bound by a
  comptime `let`), emitting the result as a constant (`comptime(2 + 3 * 4)` emits the same asm as `14`); any
  non-foldable expression (a function call, `match`, record/field, free `Var`, `read()`) is a hard compile-time
  error (`ctfail` → clear stderr message, exit 3, no output). NOT yet: compile-time calls to pure user functions,
  comptime over types, `comptime` blocks/params. See `compiler/stage1/ct.src` and the `phase-2 comptime` tests.
- Domain features (`table`/`query`, tensor/`grad`/`llm fn`) — docs only; `examples/future/*`
  pass the lexer only. Track D's D1 implementation parses exact decimal literals as signed
  integer coefficient + scale values in both the ML-core parser and self-hosted `lumec`.
  `compiler/stage1/decimal.src` provides parsing, scale-preserving formatting, checked
  add/subtract/multiply, comparison, `decimal(p, s, value)` casts, exact division at a requested
  scale, and shortest exact division for finite decimal results. Decimal records carry a
  precision field after explicit casting; add/subtract preserve integral capacity,
  multiplication combines input precision, and division derives a result precision. Decimal
  arithmetic operators are lowered for literals, helper-call results, and variables initialized
  from decimals (including local `let` bindings) in the ML-core parser and self-hosted compiler;
  interpreter, C oracle, and native assembly agree on the regression suite. Coefficient digits
  and scale are capped at 18, and inexact rescaling, overflow, division by zero, or repeating
  decimal division fail instead of rounding. `decimal(p, s)` is available on function
  parameters and returns; the gradual checker verifies explicit decimal constructor arguments,
  and the self-hosted compiler lowers arithmetic over annotated decimal parameters to the exact
  helpers. D1 remains incomplete: decimal function return values and unannotated parameters are
  not inferred for operator dispatch, and automatic literal-context inference is outstanding.
- **Track T (`lume` single CLI, `lfmt.src` canonical formatter, `lmod.src` + `lume pkg` modules & package manager) is done (T1 #118, T2 #122, T3)**: `./lume` provides `build`, `run`, `test`, `doc` (`--json`), `fmt` (`--check` / `--stdout`), `pkg` (`init`/`check`/`lock`/`verify`/`tree`), `check` (`--json`), and `describe` (`--json`), backed by native Lume-Core tools (`compiler/stage1/ldoc.src`, `compiler/stage1/ltest.src`, `compiler/stage1/lfmt.src`, and `compiler/stage1/lmod.src` compiled to ELF by `lumec` + `lasm`). `lfmt.src` proves idempotence and byte-for-byte codegen invariance; `lmod.src` + `lume pkg` provide topological `use` bundling, cycle detection, SHA-256 `lume.lock` verification (P32), and capability-scoped dependency auditing (P26). `lsp`, P34 LAWS, P35 mandatory tests — not built yet.
- **Self-hosting `lumec` and the triple self-build: NOT done.** This is Phase 1's exit.

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

GitHub's repository language bar is computed by **Linguist**, which only lists languages already
in its catalogue; a brand-new language like Lume cannot appear there via `.gitattributes`
(the `linguist-language=Lume` override is ignored for an unknown name). Today the C seed and
scaffolding are vendored so the bar is not dominated by C, and the "100% Lume" signal lives in the
README badge, the repo description and topics.

**Agreed future task:** once Lume is mature — a stable grammar/spec and the `.lume` extension in
use across several public repositories — submit Lume to `github/linguist` (a PR to that repo with
a language definition, color, sample files and usage evidence) so GitHub recognizes and displays it
officially. Until then, do not fake it by mapping `.lume` to another language.
