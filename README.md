# Lume — the AI-first programming language

<p align="center">
  <img alt="Language: Lume" src="https://img.shields.io/badge/language-Lume-8A2BE2">
  <img alt="Built with 100% Lume" src="https://img.shields.io/badge/built%20with-100%25%20Lume-8A2BE2">
  <a href="https://github.com/OmegaSoftDLL/lume/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/OmegaSoftDLL/lume/actions/workflows/ci.yml/badge.svg"></a>
  <img alt="License: MIT OR Apache-2.0" src="https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-blue">
  <img alt="Status: Phases 0–5 complete · verification hardening in progress" src="https://img.shields.io/badge/status-phases%200--5%20complete%20%C2%B7%20verification%20hardening-8A2BE2">
  <img alt="PRs welcome" src="https://img.shields.io/badge/PRs-welcome-brightgreen">
</p>

> **100% Lume.** The compiler that turns Lume into native machine code is itself written in
> Lume; a program goes from Lume source to a runnable ELF executable with **no C compiler and no
> external assembler or linker** in the loop. The C seed under `bootstrap/` is kept frozen
> as the reproducibility root and is never on the steady-state path. (GitHub's language bar is computed by
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

## The eleven pillars

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
| 10 | One tool, fast compilation (the `lume` driver is a POSIX `sh` script; the native tools are static ELF binaries, but some commands still shell out to the C seed — see Status) | Go, Cargo (Rust) |
| 11 | Ship-time protection of your own artifact (`lume build --protect <standard\|hardened\|max>`, `.lsym`, `std.secret`, `[W0001]`) | ProGuard/R8, LLVM-obfuscator, VMProtect |

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
│   ├── 02-principles.md       ← P1–P35: the rules the design obeys
│   ├── 03-specification.md    ← the language specification v0.1 (normative)
│   ├── 04-roadmap.md          ← Stages 0–5 and the self-evolution governance
│   ├── 05-vision.md           ← why: a language for the AI era, from kernel to mind
│   ├── 06-software-protection.md ← anti-reverse-engineering as a toolchain goal (P33)
│   ├── 07-competitive.md         ← scan of comparable projects and the ideas Lume adopts
│   ├── 08-ai-first.md            ← Lume is authored by AI, reviewed by humans (the center of gravity)
│   ├── 09-lbe.md                 ← LBE: the AI emits structure (binary AST), the machine emits the bits
│   ├── 10-stage1-status.md       ← honest inventory of what the Lume-written compiler covers
│   ├── 11-self-hosting-plan.md   ← how the compiler was rewritten in Lume and reached the fixpoint
│   ├── 16-phase3-plan.md         ← Phase 3: ownership, effects, concurrency, verification (lume prove)
│   ├── 19-intent-as-source.md    ← north star: intent as source, the compiler as an oracle
│   ├── 21-inflection-plan.md     ← the compiler dogfoods its own features (contracts/effects/Perceus)
│   ├── 22-adoption.md            ← AI-first go-to-market: oracle/MCP, verified certificates, flywheel
│   └── 23-horizon.md             ← the horizon: proving whole systems across time, outliving Lume
├── grammar/lume.ebnf          ← formal grammar
├── examples/                  ← .lume programs per stage (future/ for later stages)
├── bootstrap/lume0/           ← Stage 0: frozen C seed (lexer, parser, interpreter, resolver, x86-64 asm + ELF writer)
├── compiler/                  ← Stage 1+: lumec, written in Lume; the std.* modules are .src files in compiler/stage1/ (there is no top-level std/ directory)
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
3. **Self-evolution — `lume evolve`** (implemented): the language evolves through its own tooling.
   AI proposes changes via the compiler-as-a-service API; automated gates (conformance, the staged
   self-build, proofs, benchmarks, size budget) verify; humans decide. The compiler now also
   **dogfoods its own features** — its passes carry `requires`/`ensures` contracts, its I/O boundary
   is an algebraic effect, and hot paths use `own`/Perceus reuse. See `docs/04-roadmap.md` and
   `docs/21-inflection-plan.md`.

## Status

- [x] **Phase 0 — design.** ~260-language survey, principles (P1–P35), specification v0.1, grammar, roadmap, vision, and worked examples.
- [x] **Stage 0 — seed `lume0`** (C, disposable; now frozen as the reproducibility root). Lexer, parser, tree-walking interpreter, a semantic `check` (undefined names, literal/annotation types, call arity, duplicate and immutable-assignment errors — the AI-first edit→check→fix loop), and the seed's own x86-64 assembler + ELF64 writer (`lume0 asm`), so no external `as`/`ld`/`cc` is ever needed.
- [x] **Stage 1 — self-hosting, no C in the loop. ✅ Done.** Both the compiler (`compiler/stage1/lumec.src`) and the assembler + ELF writer (`compiler/stage1/lasm.src`) are written in Lume-Core. `lumec` lexes, parses (functions, `print`, expressions, `match`, records, constructors) and emits x86-64; `lasm` writes the ELF byte-by-byte. `make bootstrap` runs the staged build — the GCC/Rust/Go model: the frozen C seed mints the first tools once, then stages 1–3 rebuild the whole toolchain from Lume source with **no C**, asserting **stage 2 == stage 3, bit for bit**. `lasm`'s ELF is byte-identical to the seed assembler and strictly more correct (nearest GAS `1f`/`1b` resolution, which the seed gets wrong); the 300 KB compiler assembles in ~2.8 s. `make test` 218/0, `make asan` 177/0. See `docs/10-stage1-status.md` and `docs/11-self-hosting-plan.md`.
- [x] **Stage 2 — comptime, macros, data, tooling, self-evolution, types. ✅ Done.** `comptime` (fold, pure-fn calls, generics via `comptime T: type`, `assert`, precomputed tables), hygienic macros + quasiquote, exact `decimal(p, s)` / `table[T]` / type-checked `query`, the single `lume` CLI (`build`/`run`/`test`/`fmt`/`doc`/`pkg`/`agent`/`evolve`) with capability-audited deps + SHA-256 lockfiles, the `lume evolve` self-evolution loop (compiler-as-a-service + 6 gates + human approval), and the gradual type system (F1 declarations, F2 annotations, F3 Hindley–Milner inference, F4 unified front-ends). Exit criterion met: the `examples/apps/inventory/` business app, built only with `lume`. All keeping `stage 2 == stage 3` and the Zero-C gate green in CI. See `docs/15-phase2-status.md`.
- [x] **Stage 3 — ownership, effects, concurrency, verification. ✅ Done.** Track O (move/borrow checker, `from` regions, memory tiers), Track A (AI-native authoring: typed holes, `total`, provenance/uncertainty in the type), Track K (effect rows + algebraic handlers + `@capability` + Perceus reuse), Track S (structured concurrency, channels, supervised actors), and Track V (`requires`/`ensures`/`invariant`/`old`, `lume prove`, `LAWS`), plus the `job_server` capstone. The contract verifier is a **sound `QF_LIA` decision procedure** (Fourier–Motzkin / Farkas): `proven` obligations elide their runtime guard, others stay `runtime_checked`, never falsely proven. See `docs/16-phase3-plan.md`.
- [x] **Stage 4 — tensors, autodiff, GPU kernels, `prob`, `rule`, `llm fn`. ✅ Done.** Shape-typed tensors with compile-time shape unification (`[T0300]`), differentiable `model` + symbolic autodiff (`grad`/`jvp`/`vjp`/`jit`), accelerator `kernel fn` (PTX/SPIR-V), probabilistic programming (`std.prob`), Datalog `rule`, typed `extern llm fn` with contract-repair + provenance, and spiking `std.neuro`; `mnist` and neuro-symbolic `agent` capstones. **Caveats (see the hardening item below):** symbolic autodiff covers only the `+ - * if` fragment and rejects other operators at compile time with `[N0400]` (#184, fixed); `jit` currently lowers to a plain call (identity wrapper, #10); `kernel fn` emits placeholder PTX/SPIR-V text with no body lowering or GPU execution (#8); `std.prob` inference is a closed-form Beta-Bernoulli posterior mean (`infer_svi`/`infer_smc` are aliases of `infer_mcmc`), not sampling (#6); safetensors/ONNX "export" produce descriptor strings, not valid files (#9). See `docs/24-phase4-plan.md`.
- [x] **Stage 5 — systems, interop, realtime, proof certificates, `lume-os`. ✅ Done.** C-ABI + capability-scoped WASM interop (Tracks I1–I5), realtime `@interrupt`/`@bound` WCET, a minimal proof-certificate kernel (`lume certify`/`verify-cert`) + MCP server (`lume mcp`), and the capability-based `lume-os` microkernel capstone. **Caveats:** interop is parse plus effect/provenance checks only — no native linking, real C-ABI call/export or WASM export yet (#11); the WASM "sandbox" (`std.wasm`) is a reference model with compile-time `{wasm}`/`{fs}` capability enforcement (`[E0200]`), not a real WASM runtime with module isolation (#186, fixed); `lume-os` is a deterministic user-space *model* of microkernel services (no boot, QEMU, hardware or isolation, #12); `spawn`/`par`/channels are static Send/Sync/scope checking plus a sequential simulation, not OS threads or a parallel runtime (#13). See `docs/25-phase5-plan.md`.
- [x] **Inflection — the compiler dogfoods its own features. ✅ Done.** Contracts in the lexer/parser/typecheck, effects at the I/O boundary, `own`/Perceus in hot paths, and the **intent-as-source oracle** (hole-guided `QF_LIA` synthesis + `lume prove` + `lume agent witness|repair|impact` + `lume mcp` + `lume evolve` repair, RFC 8032 Ed25519 asymmetric maintainer signature verification against `evolve/maintainers.pub`, and end-to-end `lume evolve apply` landing `0003_decimal_clamp` into `compiler/stage1/decimal.src`, #194/#195/#203) — the compiler-as-oracle of `docs/19-intent-as-source.md`. See `docs/21-inflection-plan.md`.
- [x] **Verification trust-layer hardening (first round + P33 software protection + oracle, Ed25519 governance, `lcli.src` & C seed freeze). ✅** The proof-certificate kernel now re-derives each proof **independently** (standalone S-expression parser + Fourier–Motzkin `QF_LIA` solver) with FIPS-180-4 SHA-256, rejecting any term its own solver can't confirm (#183); autodiff rejects non-differentiable operators with `[N0400]` / `[D0001]` instead of silently zeroing (#184); WCET reports `budget_checked`, not a sound-WCET claim (#185); `std.wasm` is a labelled reference model with compile-time capability enforcement (#186); P33 software protection (`lume build --protect <standard|hardened|max> [--lsym]`, `.rodata` string encryption, `.lsym` sidecars, `std.secret` + `[W0001]`, `std.protect.guard`, `@protect model`, MBA/CFF/VM/tamper seals/white-box PRF) is implemented across Bricks W1–W5 (#188); `lume evolve` enforces `proofs_ok`, `benchmark_ok`, `stage2 == stage3` fixpoint gates, asymmetric RFC 8032 Ed25519 maintainer signature verification (`evolve/maintainers.pub`, with no unsigned bypass), and `lume evolve apply` (#194, #203); `lume agent witness|repair|impact` + `lume mcp` implement `docs/19` §2 oracle queries (#195); the CLI control plane (`compiler/stage1/lcli.src`) is compiled to native ELF (`lcli`) and verified in `stage2 == stage3` (#204); and the Stage-0 C seed (`bootstrap/lume0/*.c`) is cryptographically frozen via `bootstrap/lume0/SEED_FREEZE.sha256` (#205).
- [ ] **Remaining hardening & build-out — in progress.** Interop is parse/check only, no real linking/export (#191); the Phase-4 numeric ops (`jit`, GPU `kernel`, `prob`, Datalog, safetensors) are stubs (#192); concurrency runs sequentially (#193); multi-architecture / multi-OS targets are tracked in #206. GOLDEN RULE scales from the compiler to the ecosystem: no overclaim ships as a guarantee.
- **Beyond the roadmap — the horizon.** Vision docs for what comes next: `docs/19` (intent-as-source / compiler-as-oracle), `docs/20` (interop), `docs/22` (AI-first adoption), and `docs/23` (the horizon: proving whole systems across time, in a form that outlives Lume).

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

**One tool:** `lume build|run|fmt|test|doc|check|pkg|describe|evolve` — the compiler (`lumec`), assembler (`lasm`), CLI control plane (`lcli.src`: `--version`, `--help`, `describe`, `compile --json`, `pkg init|lock|tree`, `evolve propose`, and surface `check`/`test`), and all subcommand tools (`ldoc`, `ltest`, `lfmt`, `lmod`, `lcaas`, `levolve`, `lcert`, `lprotect`) are written in Lume, compiled to native ELF binaries, and verified in the `stage2 == stage3` bootstrap fixpoint (#204). A thin POSIX `sh` launcher dispatches process execution across hosts. It includes a canonical formatter (`lume fmt`), zero-config test discovery (`lume test` finds `fn test_*()`), doc/contract extraction (`lume doc`), and a module system + package manager with **capability-audited dependencies and content-addressed lockfiles** (superior to cargo/go mod). Plus **exact `decimal` arithmetic** for money/data.

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

Protecting the programs you ship is a first-class toolchain feature (`docs/06-software-protection.md`,
principle P33): `lume build --protect <standard|hardened|max> [--lsym <file.lsym>]` (`compiler/stage1/lprotect.src`)
and `use protect` (`compiler/stage1/protect.src`) provide per-profile symbol stripping with a `.lsym`
crash-symbolication sidecar, secret-hygiene diagnostics (`[W0001]` + `std.secret`), `.rodata` string/constant
stream encryption with in-memory decrypt-on-use (`mkstr_enc`), self-check guards (`std.protect.guard`),
encrypted `@protect model` weights and prompt templates, Mixed Boolean-Arithmetic (MBA), control-flow
flattening, randomised bytecode virtualisation (`protect.vm`), FIPS 180-4 SHA-256 tamper seals, and
white-box key tables — opt-in, composable, and applied only to your own artifact without changing its
behaviour or weakening compile-time safety. Lume ships no tooling for attacking, deobfuscating or
evading anyone else's software.
