# Lume — the evolution of programming languages

> *Lume* (Portuguese/Galician: light, glow). Working title — easy to rename, see `docs/02-principles.md`.

Lume is a **general-purpose programming language** designed around one question:

> If we took what every language got right in 70 years — from machine code to Rust — and left behind what each one got wrong, what would come out?

The one-sentence answer:

**Readable like Python and Pascal, safe like Rust, simple like Go, fast like C, data-oriented like dBase/Clipper, metaprogrammable like Nim and Lisp, dependable like Ada — with a single command-line tool that builds, formats, tests, documents and packages.**

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
├── docs/
│   ├── 00-catalog.md          ← ~260 languages, by era and family
│   ├── 01-survey.md           ← what each got right / wrong, and what Lume takes
│   ├── 02-principles.md       ← P1–P32: the rules the design obeys
│   ├── 03-specification.md    ← the language specification v0.1 (normative)
│   ├── 04-roadmap.md          ← Stages 0–5 and the self-evolution governance
│   └── 05-vision.md           ← why: a language for the AI era, from kernel to mind
├── grammar/lume.ebnf          ← formal grammar
├── examples/                  ← .lume programs per stage (future/ for later stages)
├── bootstrap/lume0/           ← Stage 0: disposable C interpreter (the seed) — lexer done
├── compiler/                  ← Stage 1+: lumec, written in Lume
├── std/                       ← standard library, in Lume
├── tests/conformance/         ← normative tests, stage-tagged
└── .github/workflows/ci.yml   ← CI: build + test lume0 on Linux/macOS, docs checks
```

## How Lume is built (no third language in the loop)

1. **Stage 0 — `lume0`** (C, disposable): a small interpreter for the core subset. Its only job is
   to run Stage 1 once. Then it is frozen as the reproducibility root.
2. **Stage 1 — `lumec`** (Lume): the real compiler, written in Lume, compiled by `lume0` the first
   time, then by itself (triple build: A → B → C, B ≡ C).
3. **Stage 2+ — `lume evolve`**: the language evolves through its own tooling. AI proposes changes
   via the compiler-as-a-service API; automated gates (conformance, triple self-build, proofs,
   benchmarks, size budget) verify; humans decide. See `docs/04-roadmap.md`.

## Status

- [x] Phase 0 — Catalog, survey, principles, specification v0.1, grammar, roadmap, vision, examples
- [x] Stage 0 — Lexer (`bootstrap/lume0`): layout tokens, all literals, diagnostics, tests, ASan clean
- [ ] Stage 0 — Parser → AST, checker, interpreter; `lume0 run` on all Stage-0 examples
- [ ] Stage 1 — `lumec` in Lume; triple self-build; `lume0` frozen
- [ ] Stage 2 — `comptime`, macros, `table`/`query`, package manager
- [ ] Stage 3 — Ownership checker, structured concurrency, effects, `@gc` regions
- [ ] Stage 4 — Tensors, autodiff, GPU kernels, `prob`, `rule`, `llm fn`
- [ ] Stage 5 — Bare metal, `lume-os`, verified core

## Contributing

Read `docs/02-principles.md` first; every change cites the principles it serves. Language changes
go through an RFC in `docs/rfcs/`. CI must be green (`make -C bootstrap/lume0 test`).

## License

To be decided (suggestion: MIT or Apache-2.0, like Rust/Go).
