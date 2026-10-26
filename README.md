# Lume — the AI-first programming language

<p align="center">
  <img alt="Language: Lume" src="https://img.shields.io/badge/language-Lume-8A2BE2">
  <img alt="Built with 100% Lume" src="https://img.shields.io/badge/built%20with-100%25%20Lume-8A2BE2">
  <a href="https://github.com/OmegaSoftDLL/lume/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/OmegaSoftDLL/lume/actions/workflows/ci.yml/badge.svg"></a>
  <img alt="License: MIT OR Apache-2.0" src="https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-blue">
  <img alt="Status: Phase 2 complete, Phase 3 in progress" src="https://img.shields.io/badge/status-phase%202%20complete%20%C2%B7%20phase%203%20in%20progress-8A2BE2">
  <img alt="PRs welcome" src="https://img.shields.io/badge/PRs-welcome-brightgreen">
</p>

> **100% Lume.** The compiler that turns Lume into native machine code is itself written in
> Lume; a program goes from Lume source to a runnable ELF executable with **no C compiler and no
> external assembler or linker** in the loop. The C seed under `bootstrap/` is a disposable
> bootstrap that is removed once the language self-hosts. (GitHub's language bar is computed by
> Linguist, which only lists languages already in its catalogue, so a new language like Lume does
> not appear there yet.)

> *Lume* (Portuguese/Galician: light, glow). Working title — easy to rename, see `docs/02-principles.md`.

Lume is a **general-purpose programming language** designed around one question:

> If we took what every language got right in 70 years — from machine code to Rust — and left behind what each one got wrong, what would come out?

The one-sentence answer:

**Readable like Python and Pascal, safe like Rust, simple like Go, fast like C, data-oriented like dBase/Clipper, metaprogrammable like Nim and Lisp, dependable like Ada — with a single command-line tool that builds, formats, tests, documents and packages.**

> **Lume is authored by AI, reviewed by humans.** Its primary user is a model, not a person: the
> design optimises for machine checkability and an unambiguous edit→check→fix loop over human typing
> convenience. `lume describe` even teaches the language to a model that has never seen it. See
> `docs/08-ai-first.md`.

## The ten pillars

| # | Pillar | Inherited from |
|---|--------|----------------|
| 1 | Readability over cleverness; one canonical formatting | Python, Pascal, Go |
| 2 | Memory safety **without** a garbage collector, with region inference | Rust, Cyclone |
| 3 | Errors are values; `!` and `?` instead of exceptions | Rust, Zig, Go |
| 4 | No `null`: `T?` is an explicit optional type | Kotlin, Swift, Rust, Haskell |
| 5 | Structured concurrency: tasks live inside scopes | Go, Kotlin, Trio (Python) |
| 6 | `comptime` and hygienic macros: the compiler is programmable | Zig, Nim, Lisp, C++ constexpr |
| 7 | Data as a first-class citizen: `record`, `table`, `query`, `decimal`, dates | dBase, Clipper, SQL, COBOL |
| 8 | Contracts in the code: `requires`, `ensures`, `invariant`, range types | Ada, Eiffel, Pascal |
| 9 | Full control when needed: `unsafe`, `asm`, explicit memory layout | C, Assembly, Zig |
| 10 | One tool, one static binary, fast compilation | Go, Cargo (Rust) |
| 11 | Ship-time protection of your own artifact (opt-in) | ProGuard/R8, LLVM-obfuscator, VMProtect |

## A taste

```lume
mod payroll

use std.io
use std.data (table, query)

type Employee = record
    name:   str
    age:    int in 16..120           # range type, from Pascal
    salary: decimal(12, 2)           # exact decimal, from COBOL/dBase
    dept:   Dept

type Dept = enum Sales | IT | Finance

fn bonus(e: Employee) -> decimal(12, 2)
    requires e.salary > 0
    match e.dept
        Sales   => e.salary * 0.10
        IT      => e.salary * 0.08
        Finance => e.salary * 0.05

fn main() ! Error
    let team = table[Employee].read_csv("team.csv")?

    let senior_it = query team
        where dept == IT and age >= 30
        order by salary desc

    for e in senior_it
        io.println("{e.name}: bonus {bonus(e)}")

    io.println("Total payroll: {team.sum(.salary)}")
```

## Project layout

```
lume/
├── README.md
├── AGENTS.md                  ← read first: rules for any contributor/agent
├── CONTRIBUTING.md            ← how to contribute
├── docs/
│   ├── PROJECT_STATE.md       ← the full source of truth: state, rules, pitfalls
│   ├── 00-catalog.md          ← ~260 languages, by era and family
│   ├── 01-survey.md           ← what each got right / wrong, and what Lume takes
│   ├── 02-principles.md       ← P1–P32: the rules the design obeys
│   ├── 03-specification.md    ← the language specification v0.1 (normative)
│   ├── 04-roadmap.md          ← Stages 0–5 and the self-evolution governance
│   ├── 05-vision.md           ← why: a language for the AI era, from kernel to mind
│   ├── 06-software-protection.md ← anti-reverse-engineering as a toolchain goal (P33)
│   ├── 07-competitive.md         ← scan of comparable projects and the ideas Lume adopts
│   ├── 08-ai-first.md            ← Lume is authored by AI, reviewed by humans (the center of gravity)
│   ├── 09-lbe.md                 ← LBE: the AI emits structure (binary AST), the machine emits the bits
│   ├── 10-stage1-status.md       ← honest inventory of what the Lume-written compiler covers
│   ├── 11-self-hosting-plan.md   ← how the compiler was rewritten in Lume and reached the fixpoint
│   └── 12-phase2-plan.md         ← Phase 2 roadmap: comptime, macros, data, tooling, self-evolution
├── grammar/lume.ebnf          ← formal grammar
├── examples/                  ← .lume programs per stage (future/ for later stages)
├── bootstrap/lume0/           ← Stage 0: disposable C interpreter (the seed) — lexer done
├── compiler/                  ← Stage 1+: lumec, written in Lume
├── std/                       ← standard library, in Lume
├── tests/conformance/         ← normative tests, stage-tagged
└── .github/workflows/ci.yml   ← CI: build + test lume0 on Linux/macOS, docs checks
```

## How Lume is built (no third language in the loop)

This is a **staged, self-verifying bootstrap** — the model GCC, Rust and Go use — run by
[`scripts/bootstrap.sh`](scripts/bootstrap.sh) (`make bootstrap`):

1. **Stage 0 — `lume0`** (C): the frozen bootstrap seed. It mints the first `lumec` (compiler)
   and `lasm` (assembler + ELF writer) once — exactly as GCC needs a system C compiler and Rust
   needs a prior `rustc`. It is kept as the reproducibility root and is **never on the
   steady-state path**.
2. **Stages 1–3 — Lume builds Lume, no C**: the seed-built tools rebuild the whole toolchain
   (`lumec.src` + `lasm.src`) from Lume source; stage 2 and stage 3 then build it again, and the
   bootstrap asserts **stage 2 == stage 3, bit for bit**. `lumec` emits x86-64 and `lasm` writes
   the ELF — no C compiler, assembler or linker in the loop. This fixpoint holds today.
3. **Stage 4+ — `lume evolve`** (future): the language evolves through its own tooling. AI proposes
   changes via the compiler-as-a-service API; automated gates (conformance, the staged self-build,
   proofs, benchmarks, size budget) verify; humans decide. See `docs/04-roadmap.md`.

## Status

- [x] **Phase 0 — design.** ~260-language survey, principles (P1–P35), specification v0.1, grammar, roadmap, vision, and worked examples.
- [x] **Stage 0 — seed `lume0`** (C, disposable; now frozen as the reproducibility root). Lexer, parser, tree-walking interpreter, a semantic `check` (undefined names, literal/annotation types, call arity, duplicate and immutable-assignment errors — the AI-first edit→check→fix loop), and the seed's own x86-64 assembler + ELF64 writer (`lume0 asm`), so no external `as`/`ld`/`cc` is ever needed.
- [x] **Stage 1 — self-hosting, no C in the loop. ✅ Done.** Both the compiler (`compiler/stage1/lumec.src`) and the assembler + ELF writer (`compiler/stage1/lasm.src`) are written in Lume-Core. `lumec` lexes, parses (functions, `print`, expressions, `match`, records, constructors) and emits x86-64; `lasm` writes the ELF byte-by-byte. `make bootstrap` runs the staged build — the GCC/Rust/Go model: the frozen C seed mints the first tools once, then stages 1–3 rebuild the whole toolchain from Lume source with **no C**, asserting **stage 2 == stage 3, bit for bit**. `lasm`'s ELF is byte-identical to the seed assembler and strictly more correct (nearest GAS `1f`/`1b` resolution, which the seed gets wrong); the 300 KB compiler assembles in ~2.8 s. `make test` 218/0, `make asan` 177/0. See `docs/10-stage1-status.md` and `docs/11-self-hosting-plan.md`.
- [x] **Stage 2 — comptime, macros, data, tooling, self-evolution, types. ✅ Done.** `comptime` (fold, pure-fn calls, generics via `comptime T: type`, `assert`, precomputed tables), hygienic macros + quasiquote, exact `decimal(p, s)` / `table[T]` / type-checked `query`, the single `lume` CLI (`build`/`run`/`test`/`fmt`/`doc`/`pkg`/`agent`/`evolve`) with capability-audited deps + SHA-256 lockfiles, the `lume evolve` self-evolution loop (compiler-as-a-service + 6 gates + human approval), and the gradual type system (F1 declarations, F2 annotations, F3 Hindley–Milner inference, F4 unified front-ends). Exit criterion met: the `examples/apps/inventory/` business app, built only with `lume`. All keeping `stage 2 == stage 3` and the Zero-C gate green in CI. See `docs/15-phase2-status.md`.
- [ ] **Stage 3 — ownership, effects, concurrency, verification.** *In progress:* Track O (move/borrow checker, `from` regions, memory tiers) ✅ and Track A (AI-native authoring: typed holes, `total`, provenance/uncertainty in the type) ✅ merged; effect handlers (K), structured concurrency + actors (S), and `lume prove`/SMT contract verification (V) remaining, plus the `job_server` capstone. See `docs/16-phase3-plan.md` and the AI-first north star `docs/19-intent-as-source.md`.
- [ ] **Stage 4** — tensors, autodiff, GPU kernels, `prob`, `rule`, `llm fn`.
- [ ] **Stage 5** — bare metal, `lume-os`, verified core.

## What works today (self-hosted, no C in the loop)

Everything below compiles and runs through the Lume-written toolchain (`lumec` + `lasm`) — the C seed only mints the first binaries once.

**comptime — the compiler runs Lume at build time (superior to C++ `constexpr`: deterministic, exact 64-bit ints, legible errors, guaranteed termination):**
```lume
fn fib(n) = if n < 2 then n else fib(n - 1) + fib(n - 2)
print comptime(fib(10))     # folded to the constant 55 at compile time — no fib call in the binary
```

**Generics via comptime types — the type parameter is erased at runtime (Zig-style, but gradual + AI-first checked):**
```lume
fn id(comptime T: type, x: T) = x
print id(int, 42)           # emitted call passes only 42; `id(int, "x")` is a clear compile-time type error
```

**Named types + gradual, AI-first checking (declare to opt in; typos and wrong arity are caught, zero false positives):**
```lume
type Expr = enum Lit(int) | Add(Expr, Expr)
fn eval(e) = match e
    Lit(n)    => n
    Add(a, b) => eval(a) + eval(b)
```

**One tool, all in Lume:** `lume build|run|fmt|test|doc|check|pkg` — a canonical formatter (`lume fmt`), zero-config test discovery (`lume test` finds `fn test_*()`), doc/contract extraction (`lume doc`), and a module system + package manager with **capability-audited dependencies and content-addressed lockfiles** (superior to cargo/go mod). Plus **exact `decimal` arithmetic** for money/data.

## Quickstart

```sh
make bootstrap          # staged self-host: C seed -> lumec+lasm -> rebuild in Lume; assert stage2==stage3
make test               # run the conformance suite (and `make asan` for the sanitized build)

# or by hand:
make -C bootstrap/lume0                               # build the seed (strict warnings)
bootstrap/lume0/lume0 check  examples/01_hello.lume   # lex + parse + name/type/arity checks
bootstrap/lume0/lume0 run    examples/01_hello.lume   # interpret
# source -> native with NO C in the loop (compiler AND assembler are Lume):
make amalgamate                                       # generate compiler/stage1/lumec.src from compiler/stage1/lume/*.src
bootstrap/lume0/lume0 run compiler/stage1/data_asm_cc.lume compiler/stage1/lumec.src /tmp/l.s
bootstrap/lume0/lume0 asm /tmp/l.s /tmp/lumec         # one-time seed build of lumec
bootstrap/lume0/lume0 run compiler/stage1/data_asm_cc.lume compiler/stage1/lasm.src /tmp/la.s
bootstrap/lume0/lume0 asm /tmp/la.s /tmp/lasm         # one-time seed build of lasm
cat compiler/stage1/nfib.src | /tmp/lumec | /tmp/lasm > /tmp/nfib && chmod +x /tmp/nfib && /tmp/nfib
```

## Contributing

Read **[`AGENTS.md`](AGENTS.md)** and **[`docs/PROJECT_STATE.md`](docs/PROJECT_STATE.md)** first —
they hold the rules, architecture and pitfalls — then **[`CONTRIBUTING.md`](CONTRIBUTING.md)** for
the workflow. In short: English only in the repo; one focused change per PR; `make -C
bootstrap/lume0 test` **and** `asan` must pass (0 failed); every fix ships with a regression test
(golden rule: no bugs in the toolchain); `main` is protected and requires a green PR. `docs/02-principles.md`
lists the principles (P1–P35) each change serves.

## License

Dual-licensed under **MIT** (`LICENSE-MIT`) **or Apache-2.0** (`LICENSE-APACHE`), at your option —
the same scheme as the Rust project (Go uses BSD-3-Clause; both are permissive). See `COPYRIGHT`.
The license covers the Lume toolchain, standard library and docs; it does not restrict what you do
with programs you write in Lume or the binaries it produces for you.

## Software protection (anti-reverse-engineering)

Protecting the programs you ship is a first-class toolchain goal (`docs/06-software-protection.md`,
principle P33): per-profile symbol stripping, constant/string encryption, control-flow obfuscation,
selective bytecode virtualisation and tamper seals — opt-in, composable, and applied only to your
own artifact without changing its behaviour or weakening compile-time safety. Models and LLM
prompts embedded in a program are protected the same way. Lume ships no tooling for attacking,
deobfuscating or evading anyone else's software.
