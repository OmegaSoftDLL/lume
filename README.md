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
├── README.md                  ← you are here
├── docs/
│   ├── 01-survey.md           ← what every language taught us (the good and the bad)
│   ├── 02-principles.md       ← the design principles derived from the survey
│   ├── 03-specification.md    ← the language specification (syntax, types, memory, concurrency…)
│   └── 04-roadmap.md          ← how to build the compiler, phase by phase
├── grammar/
│   └── lume.ebnf              ← formal grammar
├── examples/                  ← .lume programs that illustrate the language
└── compiler/                  ← `lumec`, the compiler (Rust). Phase 1: lexer done and tested
```

## Status

- [x] Phase 0 — Survey, principles, specification v0.1, grammar, examples
- [x] Phase 1 (started) — Lexer in Rust with tests (`cd compiler && cargo test`)
- [ ] Phase 1 — Parser → AST
- [ ] Phase 2 — Type checker + interpreter/REPL
- [ ] Phase 3 — Ownership/borrow checker
- [ ] Phase 4 — Native backend (Cranelift → LLVM) and WASM
- [ ] Phase 5 — Self-hosting (a Lume compiler written in Lume)

See `docs/04-roadmap.md` for each phase in detail.

## License

To be decided (suggestion: MIT or Apache-2.0, like Rust/Go).
