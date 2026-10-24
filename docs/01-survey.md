# 01 — Survey: what every language got right, what it got wrong, what Lume takes

Format for each entry:

- **Good** — the ideas worth keeping, with the reason they worked.
- **Bad** — the mistakes, with the reason they hurt (not just taste).
- **Lume takes** — the concrete decision Lume makes because of this language.

Primary influences (★ in the catalog) get full entries. The rest are grouped, shorter, but every
language in the catalog is covered.

---

## Part I — Primary influences

### Machine code & Assembly ★

**Good**
- Total control: every byte, every cycle, every register is yours. No hidden allocations, no hidden calls.
- Predictability: what you write is what executes. Essential for OS kernels, bootloaders, drivers, crypto, real-time.
- The *only* way to express some things: privileged instructions, atomics with precise ordering, SIMD, interrupts.
- Macro assemblers (MASM, NASM, GAS) showed that even the lowest level wants abstraction.

**Bad**
- Zero portability: every CPU is a new language.
- No types, no safety, no structure. Every bug is a silent memory corruption or a security hole.
- Unreadable at scale; impossible to refactor; no modules.

**Lume takes**
- `asm` blocks with typed register binding, inside `unsafe`. Inline, not a separate file.
- Guaranteed layout: `@repr(c)`, `@packed`, `@align(n)`, explicit field order. What you declare is what's in memory.
- Zero hidden control flow (from Zig): no hidden allocations, no implicit function calls, no exceptions unwinding through your frame.
- A `freestanding` target: no runtime, no stdlib, write a kernel in Lume alone.

### FORTRAN ★

**Good**
- Proved compilers could beat hand-written assembly (Backus's team optimised aggressively from day one).
- Arrays as a primitive with a rich notation (FORTRAN 90: slices, whole-array ops, `where`). Still the fastest numeric code on Earth because the compiler knows arrays don't alias.
- Stability: code from 1970 compiles and runs today.

**Bad**
- Fixed-column punch-card formatting lasted 30 years too long.
- Implicit typing (`I–N` are integers) — a famous source of bugs.
- Global `COMMON` blocks; `GOTO` spaghetti before FORTRAN 77.

**Lume takes**
- Multi-dimensional arrays and tensors as first-class types with shape in the type: `tensor[f32, 3, 224, 224]`.
- Non-aliasing guarantees come free from ownership, so the optimiser gets FORTRAN-level freedom.
- Whole-array expressions and slicing in the core language, not a library.

### LISP ★

**Good**
- Code is data (homoiconicity): macros that rewrite programs with the full language available. Nothing since has matched the power.
- The REPL and interactive development: inspect, redefine, continue. Debugging a *live* system.
- Garbage collection (1959), first-class functions, recursion, dynamic typing, conditionals as expressions — most of modern programming was invented here.
- Common Lisp's condition system: handle an error *where it happened* without unwinding; restarts.

**Bad**
- Parentheses: readable to Lispers, a wall for everyone else. The syntax never won mainstream adoption in 65 years; that is data, not taste.
- Dynamic typing at scale: errors surface at runtime; large codebases drift.
- Fragmentation (dozens of dialects), weak standard libraries, "Lisp curse" (everyone writes their own everything).

**Lume takes**
- Hygienic macros that operate on a typed AST, with a *normal* (non-parenthesised) surface syntax — the Nim/Rust approach, but macros are also available at `comptime` with the full language.
- A real REPL and *live redefinition* for the interpreted tier (see Roadmap phase 2), including hot-reload of functions in a running process.
- Restartable errors in the debugger, not in the language: the error-as-value model stays simple, the tooling adds the Lisp experience.

### COBOL ★

**Good**
- Readable by non-programmers; self-documenting programs. Auditors could read it.
- **Exact decimal arithmetic** (`PIC 9(7)V99`). Floating-point money is a bug; COBOL knew in 1959.
- Record-oriented I/O and copybooks: the data layout *is* the specification.
- Durability: 220 billion lines still in production. Stability is a feature.

**Bad**
- Extreme verbosity (`ADD 1 TO X GIVING Y`); 400+ reserved words.
- No functions/recursion originally; global data everywhere; `ALTER` statement (self-modifying GOTO).
- Divisions and sections are ceremony, not structure.

**Lume takes**
- `decimal(p, s)` as a built-in numeric type with exact arithmetic and declared precision. Money is never a float.
- Records with declared layout that can be read/written directly to files and wire formats (`@layout`).
- Readability as a target: full English keywords, not symbols soup — but terse where repetition would be noise.

### ALGOL 60 ★

**Good**
- Block structure, lexical scoping, `begin…end`, recursion, call-by-name, BNF for the grammar. Defined how languages are *described*.
- A committee that produced something small and elegant (once).

**Bad**
- No I/O in the standard; no strings; every implementation differed.
- Call-by-name was too clever (Jensen's device) and slow.

**Lume takes**
- A formal grammar from day one (`grammar/lume.ebnf`), and a specification that is normative.
- Lexical scoping everywhere; blocks are expressions.

### Simula → Smalltalk → OOP ★

**Good (Simula)**
- Classes, objects, inheritance, virtual methods, coroutines (!). Simulation needs modelling and modelling gave us OOP.

**Good (Smalltalk)**
- Everything is an object; uniform message passing; blocks (closures) as the control-flow primitive (`ifTrue:`, `do:`).
- The live image: the program, the IDE, the debugger and the data are one system you can inspect and change while it runs. Nobody has matched this developer experience since.
- Tiny syntax (fits on a postcard); reflection; the first real IDE, class browser, refactoring tools, unit testing (SUnit → xUnit).

**Bad**
- Implementation inheritance as the main reuse tool proved fragile (the "fragile base class" problem; Java and C++ spent 20 years on `final`, interfaces, composition).
- Dynamic dispatch everywhere costs performance and makes static reasoning hard.
- The image is also a prison: hard to version-control, hard to deploy, isolated from the OS.

**Lume takes**
- **No implementation inheritance.** Types + traits (interfaces with default methods) + composition. Polymorphism is by trait, static when possible, dynamic (`dyn Trait`) when asked.
- Blocks/closures everywhere, cheap and inline-able.
- The *live* experience through tooling: the REPL tier, inspector, time-travel debugger — without the image as the unit of deployment. Source files stay the truth.

### BASIC ★

**Good**
- Instant feedback: type `PRINT 2+2`, see `4`. Millions learned on it. Zero ceremony to start.
- Shipped on every microcomputer; interactive line editing.

**Bad**
- Line numbers and `GOTO` taught a generation unstructured code (Dijkstra's famous complaint).
- Globals only, two-letter variable names, no procedures in early versions.
- Visual Basic later fixed most of it, then Microsoft broke compatibility (VB.NET) and lost the community.

**Lume takes**
- `lume` with no arguments opens a REPL; a single `.lume` file with top-level statements runs as a script. Zero ceremony for beginners, same language for experts.
- Never break compatibility casually: editions (Rust-style) so old code keeps compiling.

### Pascal / Turbo Pascal / Delphi ★

**Good**
- Designed for teaching clear thinking: strong static typing, structured control flow, readable keywords.
- **Range (subrange) types** `1..100`, **enumerations**, **sets** `set of Color`, **records with variants**, nested procedures, `with`. Many of these have never been bettered.
- One-pass compiler: Turbo Pascal compiled a program faster than you could blink, in 1983, on 64 KB. Speed of compilation is a *feature* that keeps programmers in flow.
- Delphi: the best RAD environment ever built; components, properties, events, native code. UCSD Pascal invented the bytecode VM (p-code) 17 years before Java.

**Bad**
- Original Pascal: no modules, no separate compilation, strings of fixed length, I/O was weird. Kernighan's "Why Pascal is not my favorite programming language" (1981) was right about standard Pascal.
- Fragmented dialects (UCSD, Turbo, Object Pascal, Free Pascal) — no single standard moved forward.
- Delphi: proprietary, expensive, Windows-centric; lost the mainstream to Java/C# despite being better in several ways.

**Lume takes**
- Range types: `int in 0..150`, checked at compile time when possible and at runtime (or at boundaries) otherwise. Enums, sets (`set[Color]`, bit-set backed), variant records (as tagged unions = `enum` with payloads).
- Compilation speed as a hard requirement: debug builds must be interactive-fast (sub-second for incremental changes). Architecture chosen for that (see Roadmap).
- Properties on types (Delphi), with getters/setters that look like fields.
- One language, one reference implementation, open source. No dialect wars.

### C ★

**Good**
- Portable assembly: small language, maps to the machine, runs everywhere. The de-facto ABI: every language talks to C.
- Pointers, structs, unions, manual memory — total control with a thin abstraction.
- Unix, Linux, every database, every browser engine, every interpreter is C or C++.
- A spec small enough to know entirely.

**Bad**
- **Undefined behaviour** everywhere: signed overflow, out-of-bounds, use-after-free, data races. 70% of serious security vulnerabilities (Microsoft, Google data) are C/C++ memory bugs.
- Null-terminated strings, `NULL`, arrays decaying to pointers, the preprocessor (text macros, `#include` copying files), no modules, no namespaces, no generics (void* casts), integer promotion rules, `errno`.
- Header files: declaration duplicated from definition, build times, ODR violations.

**Lume takes**
- C ABI compatibility as a first-class target: `extern "C"` both directions, generate headers, consume headers directly (`import c "stdio.h"` à la Zig).
- Everything C can express in memory, Lume can express — but inside `unsafe` when it is actually unsafe.
- No preprocessor. Modules, generics, `comptime` replace every legitimate use of `#define`.
- No undefined behaviour in safe code. Period. Integer overflow is defined (checked in debug, wrapping/saturating on request).

### Prolog ★

**Good**
- Declarative relations instead of procedures: say *what* is true, let the engine search. Unification and backtracking as built-in mechanisms.
- Pattern matching as the core — later borrowed by every ML and Rust.
- Still the best tool for certain problems: parsing, constraint solving, symbolic AI, rules engines (Datalog descendants power Datomic, CodeQL, Soufflé).

**Bad**
- Performance unpredictable; cut (`!`) and side effects break the declarative model; the Byrd box debugging mental model is hard.
- Weak types, weak modules, weak standard library. Never crossed into general-purpose use.

**Lume takes**
- Exhaustive pattern matching on all types, with guards, nested patterns, bindings.
- A Datalog-style **rule/query sub-language** inside `query` (see dBase and SQL below): fixpoint relations over tables, usable for program analysis, authorisation rules, inference. Declared, type-checked, compiled.
- Unification-based constraint solving as a stdlib module, not a core feature.

### ML / OCaml / Standard ML / Haskell ★

**Good**
- **Hindley–Milner type inference**: full static typing without writing types. Rust, Swift, Kotlin, TypeScript all live downstream of this.
- **Algebraic data types + pattern matching**: model the domain exactly; the compiler tells you when you forgot a case.
- Parametric polymorphism (generics) done right, in 1973.
- Haskell: purity and type classes (→ Rust traits), monads as a disciplined way to sequence effects, laziness as a research tool, QuickCheck (property-based testing), STM.
- OCaml: fast native code, a pragmatic blend of functional + imperative + objects, a module system with functors, used for real compilers (Rust's first compiler, Coq, Flow, Hack).

**Bad**
- Haskell: laziness by default makes performance and memory unpredictable (space leaks); monad transformer stacks are an incomprehensible tax for everyday I/O; "avoid success at all costs" kept it academic; string type confusion (`String`, `Text`, `ByteString`…).
- ML family: syntax is alien to the C-descended majority (`let … in`, `;;`, `'a`), weak tooling and library ecosystems for decades (OCaml fixed it late with opam/dune).
- Type errors from inference can be cryptic when the inferred type was not what you meant.

**Lume takes**
- Local type inference everywhere; type annotations required only at function signatures (public API) — the Rust/Swift balance between ML inference and readability.
- ADTs as `enum` with payloads, exhaustive `match`, `Option`/`Result` derived from them.
- Traits (type classes) with associated types and default methods.
- Strict evaluation. Laziness is opt-in (`lazy` values, iterators are lazy by design).
- Property-based testing in the standard test runner (`test property`).
- Effects are tracked, but **not with monads**: see Koka/algebraic effects below — Lume uses an effect system with a lightweight syntax.

### SQL ★

**Good**
- Declarative: describe the result, the engine plans the execution. The best proof that declarative beats imperative for data.
- Relational algebra is a sound mathematical base; set-at-a-time thinking is the right abstraction for data.
- 50 years, every business, every language — the single most successful DSL.

**Bad**
- Syntax order (`SELECT … FROM … WHERE`) is backwards from the logical order (`FROM → WHERE → SELECT`); hard to compose; CTEs and subqueries bolted on.
- NULL three-valued logic; dialect fragmentation; strings in host languages (SQL injection, no type checking at compile time).
- Impedance mismatch: ORMs exist because SQL and host languages have different type systems.

**Lume takes**
- A **`query` expression in the core language**, type-checked against the record type, in logical order (`from/where/group/order/select`), composable as a value, working over in-memory collections, files, and external databases through the same syntax (the LINQ insight, done at the language level and compiled to the backend's native plan).
- No SQL strings. Ever. Parameters are typed; injection is impossible by construction.
- `T?` optionals replace NULL; joins return explicit `Option`.

### Modula-2 / Oberon ★

**Good**
- Modules with *separate* definition (interface) and implementation, explicit `IMPORT`, qualified names. Wirth defined the module system every later language adopted (Go's packages descend from here).
- Oberon: type extension (a simpler inheritance), a whole OS + compiler + UI in ~10k lines. Proof that small is possible.
- Coroutines in Modula-2 (1978).

**Bad**
- Too minimal for industry (no generics, no exceptions, limited strings); Wirth kept *removing* features, the market kept wanting more.
- Case sensitivity + ALL-CAPS keywords; small ecosystem.

**Lume takes**
- A module system with explicit `pub` exports, no cyclic imports, module = file or directory, hierarchical names. Interfaces are what `pub` exposes; a `lume doc` extracts them.
- The Oberon lesson about *co-designing the language with the OS*: Lume's `freestanding` target and `std.os` traits are designed so that an OS can be written in Lume and user programs recompiled against it (see `05-vision.md`).

### dBase / Clipper / FoxPro / Harbour ★

**Good**
- **The data is the program's world.** `USE customers` opens a table; `LIST FOR balance > 1000` queries it; `REPLACE … WITH …` updates it; `INDEX ON` creates an index. No ORM, no connection strings, no SQL strings. Data operations were *statements* in the language.
- Interactive dot prompt: explore your data live, then paste the commands into a program.
- Clipper: compiled xBase to native DOS executables; **code blocks** `{|x| x * 2}` (closures in 1990!), user-defined functions, replaceable database drivers (RDDs), `SET RELATION` (declarative joins), `BROWSE` (a full data grid in one statement), screen `@ SAY/GET` forms.
- FoxPro: Rushmore optimisation (bitmap indexes) made it the fastest desktop database; Visual FoxPro added a data-centric OO model (cursors, views, remote data in the same syntax).
- Reports and screens were part of the language's problem space. A business app was 1/5 the code of the Java equivalent.
- Harbour keeps it alive: cross-platform, still used in 2026 for ERP and point-of-sale.

**Bad**
- Dynamic typing, macro substitution (`&cmd`) with no safety, global memvars by default, `PUBLIC`/`PRIVATE` dynamic scoping — maintenance nightmares.
- Single-user file-based tables (DBF) hit limits with concurrency, size and integrity; locking was manual (`RLOCK()`, `FLOCK()`).
- Proprietary and vendor-bound: Ashton-Tate, Nantucket/CA, Fox/Microsoft each killed or neglected the product. Visual FoxPro was cancelled while beloved. The *paradigm* was never ported to a modern, typed, open language. That is the gap Lume fills.
- Procedural UI (`@ 10,10 SAY`) didn't survive the GUI transition.

**Lume takes**
- **`record` and `table[T]` as core types.** A table is a typed, columnar-or-row collection with optional indexes, that can be in memory, on disk (a modern, concurrent, crash-safe native format — the DBF spirit without the DBF limits), or a view over an external database.
- **`query` statements/expressions** over tables (see SQL). `index`, `relation` (declarative joins), `seek`, `scan` as typed operations.
- Code blocks → closures (`|x| x * 2`) — syntax honours Clipper.
- The dot prompt → the REPL has tables as first-class values: `> use customers; > list where balance > 1000`.
- A `report` block type and a declarative UI/form description in the standard library (not the core language), so business apps are short again.
- The `&macro` operator becomes `comptime` and hygienic macros: the power without the runtime injection.

### Ada ★

**Good**
- Designed for systems where failure kills people: strong typing (no implicit conversions), range and modular types, **design by contract** (`Pre`, `Post`, `Type_Invariant` since Ada 2012), tasks and protected objects (safe concurrency since 1983), packages with private parts, generics, representation clauses (exact bit layout), exceptions with a model for real-time.
- SPARK: a subset that is *formally verified* — proofs of absence of runtime errors, used in avionics and rail.
- Readability chosen over brevity, on purpose, because code is read more than written.

**Bad**
- Verbose to the point of fatigue; slow adoption outside defence/aerospace; expensive compilers for decades (GNAT fixed that late).
- Reputation as "bureaucratic"; the DoD mandate created resentment.
- Exceptions (rather than values) and a large runtime for the full language.

**Lume takes**
- **Contracts in the core**: `requires`, `ensures`, `invariant` on functions and types; checked in debug/test, optionally proven by the `lume prove` tool (SMT-backed, SPARK/Dafny-style) for the subset that admits it.
- Range types and `modular` (wrapping) integer types as in Ada.
- Protected objects → Lume's `sync` types; tasks → structured concurrency.
- Representation clauses → `@layout`, `@bits`.

### C++ ★

**Good**
- **RAII**: resource lifetime tied to scope. Destructors. The single best idea for deterministic resource management; Rust's `Drop` is RAII.
- Zero-overhead principle: you don't pay for what you don't use. Templates generate specialised code; `constexpr` runs at compile time; move semantics avoid copies.
- The only language besides C with 40 years of performance-critical infrastructure (browsers, game engines, databases, LLVM itself).
- `std::` algorithms + iterators + ranges: generic programming (Stepanov) is a real paradigm.

**Bad**
- Complexity beyond any single person's understanding (the standard is ~2,000 pages). Dozens of ways to initialise a variable. Each new standard adds, never removes.
- Undefined behaviour inherited from C plus new kinds; memory unsafety; the compile-time error messages from templates; build times (headers, template instantiation).
- Implicit conversions, implicit copies, hidden control flow (constructors, overloaded operators, exceptions).
- The committee process yields compromises (modules took 15 years and still barely work).

**Lume takes**
- RAII/`Drop` with deterministic destruction order and `defer` for ad-hoc cleanup.
- Generics are monomorphised (zero-cost) by default, with a `dyn` escape hatch; `comptime` instead of template metaprogramming.
- **Explicit is better than implicit**: no implicit copies of non-trivial types (`.clone()`), no implicit conversions (except safe widening of numeric literals), operators overloadable only through named traits.
- A language that **removes** things in new editions. A hard budget on language size (the spec must remain readable in a weekend).

### Erlang / Elixir ★

**Good**
- **Lightweight processes** (millions of them), share-nothing, message passing. **Supervision trees**: processes crash, supervisors restart them — "let it crash" turns errors into a design pattern rather than a catastrophe.
- Hot code loading in production (telecom switches with 99.9999999% uptime).
- Location transparency: a message to a process on another machine looks the same.
- Pattern matching, immutability, the BEAM's preemptive scheduler (no task can starve the others).
- Elixir added Ruby-like syntax, macros, mix (tooling), Phoenix/LiveView — proving the runtime, not the syntax, was Erlang's asset.

**Bad**
- Dynamic typing (Dialyzer's success typing helps but is not a type system; Gleam exists for this reason).
- Numeric performance is poor (BEAM is a message-passing VM, not a number cruncher); not suitable for OS kernels, games, ML.
- Erlang syntax (commas, semicolons, periods) alienated people for 25 years.

**Lume takes**
- An **actor/supervisor runtime** in the standard library (`std.actor`): isolated tasks with mailboxes, supervisors with restart strategies, links/monitors. Typed messages (what Gleam/Pony add).
- Structured concurrency as the base, actors as a pattern on top — not one or the other.
- Hot reload of functions in the interpreted/JIT tier; in the native tier, through a module-replacement protocol for long-running services.
- Lume must be good at the things Erlang is bad at (numbers, kernels) *without* losing the things Erlang is good at.

### Perl / Bash / shell ★

**Good**
- Perl: regex as syntax, text munging unmatched, CPAN was the first great package repository, "make easy things easy and hard things possible".
- Shell: pipes compose programs; the Unix philosophy (small tools, text streams); ubiquitous; interactive.
- Both: pragmatic glue; get it done in one line.

**Bad**
- Perl: write-only syntax (sigils, context sensitivity, implicit `$_`), TIMTOWTDI led to unmaintainable codebases, Perl 6 took 15 years and split the community.
- Shell: word splitting, quoting hell, no data structures, exit codes as the only error model, `set -euo pipefail` needed just to be sane, non-portable between shells. The most dangerous "language" in production (think `rm -rf $DIR/`).

**Lume takes**
- Regex literals with compile-time validation and typed captures (`re"(?<year>\d{4})"` yields a record with `.year`).
- A **shell mode** / `std.sh`: run processes with structured pipes, typed stdout/stderr/exit, and a syntax for pipelines (`run "ls" | lines | filter …`). Lume scripts replace Bash scripts without the quoting hell (the Nushell/Oil insight).
- One obvious way to do things (Python's answer to TIMTOWTDI).

### Python ★

**Good**
- **Readability**: indentation, English keywords, few symbols. "Executable pseudocode." The most-taught and (by many measures) most-used language on Earth.
- Batteries included: a huge standard library; `pip`/PyPI; the ecosystem for science and AI (NumPy, pandas, PyTorch, Jax).
- REPL, notebooks, scripting, duck typing, comprehensions, generators, keyword arguments, slicing, `with` context managers, docstrings, decorators — many small ergonomic wins.
- Gradual typing added later (type hints, mypy/pyright) shows the demand for types.

**Bad**
- Slow (10–100× C) — the whole AI industry writes its real code in C++/CUDA and *drives* it from Python; the "two-language problem".
- The GIL (being removed in 3.13+, but it defined 30 years); no real parallelism in the core.
- Dynamic typing: runtime errors, refactoring fear, type hints bolted on and optional.
- Packaging/environment hell (virtualenv, pip, conda, poetry, uv…), the Python 2→3 split cost a decade.
- Mutable default arguments, `self` everywhere, late binding closures in loops, significant whitespace copy-paste hazards.

**Lume takes**
- **Indentation-based syntax**, Python/Nim-style, with one canonical formatter so the whitespace hazards disappear (the formatter normalises; the compiler rejects ambiguity).
- Comprehensions, slicing, keyword and default arguments, string interpolation, context managers (`with` → `using`), generators (as lazy iterators), decorators (as attributes `@…`).
- Statically typed with inference: the Python feel, with the compiler catching what Python leaves to runtime.
- **Solve the two-language problem**: the same language from the script to the kernel (Julia and Mojo's goal, see below).
- One package manager, one build tool, lockfiles, reproducible builds, editions instead of breaking releases.

### Lua ★

**Good**
- Tiny (≈30k lines of C), embeddable anywhere (games, Redis, nginx, Neovim), fast (LuaJIT was the fastest dynamic language for a decade).
- One data structure (the table) that does arrays, maps, objects, modules. Coroutines done right. Clean C API.
- Metatables: a minimal mechanism for OO/operators, no built-in class system.

**Bad**
- 1-based indexing, `nil` in tables, global by default, `~=`, no integer type until 5.3, small stdlib, no unicode strings, fragmented (5.1 vs 5.4 vs LuaJIT).

**Lume takes**
- Embeddability: the Lume runtime for the interpreted tier is a library with a small C API; a host program can embed Lume as its scripting language (replacing Lua), with the *same* language as the host if the host is Lume.
- Coroutines/generators as a core primitive.
- 0-based indexing (the machine's, C's, Python's, Rust's choice), no implicit globals.

### Ruby ★

**Good**
- Programmer happiness as a design goal; blocks and `yield`; everything is an expression; a beautifully consistent object model; metaprogramming that made Rails possible (DSLs that read like English).
- Rails showed convention-over-configuration and the power of a full-stack framework.

**Bad**
- Slow; monkey-patching core classes made large codebases unpredictable; `method_missing` magic defeats tooling; dynamic typing (Sorbet/RBS bolted on late).

**Lume takes**
- Everything is an expression (`if`, `match`, blocks return values).
- DSL-friendliness through trailing-closure syntax and macros — but checked at compile time, so "magic" is inspectable (`lume expand`).

### Java ★

**Good**
- Garbage collection + a VM made memory safety mainstream (1995). Portability. Huge standard library. The best tooling ecosystem (IntelliJ, Maven/Gradle, profilers, JFR). The JIT (HotSpot) is a masterpiece: adaptive optimisation beats static compilation for many workloads.
- Backward compatibility taken seriously for 30 years; a binary from 1998 runs today.
- Interfaces, generics (2004), lambdas (2014), records, sealed classes, pattern matching, and **virtual threads** (Project Loom, 2023), which make millions of concurrent tasks cheap on the JVM — slow but steady, and the enterprise trusts it.
- **The JVM hosted the first mainstream "many agents at once" platforms**: Akka brought the actor model to the JVM, and JADE (Java Agent DEvelopment Framework) was a standard multi-agent platform. The actor *theory* is Hewitt's (1973) and the first great *implementation* was Erlang, but Java/JVM is where large-scale concurrent-agent systems became an industry.

**Bad**
- Verbosity (`public static void main(String[] args)`, getters/setters, factory-factory patterns); checked exceptions (everyone agrees they failed); `null` (Hoare's "billion-dollar mistake" — Java made it a NullPointerException culture); type erasure in generics; primitives vs boxed types; slow startup and heavy memory for CLI tools; the Oracle licensing scare; XML-era configuration culture.

**Lume takes**
- A *portable bytecode* target (`.lumb`) for the interpreted/JIT tier — Lume compiles to native **or** to bytecode for a small VM — with adaptive optimisation ideas from HotSpot available where they pay (plugins, scripts, REPL).
- No `null`, no checked exceptions, no boxing: generics are reified and monomorphised.
- Serious tooling from day one: `lume fmt/test/doc/bench/prof` and a language server (LSP) shipped with the compiler.
- Long-term stability as a value, with editions to evolve.
- **Cheap massive concurrency** (the Loom lesson): Lume's task runtime multiplexes millions of lightweight tasks, so running many agents at once is normal, not exotic. Combined with the Erlang-style `std.actor` supervisors (see Erlang) and the "an agent is a program" model (see §2.5 of the vision), Lume targets multi-agent orchestration — many agents running, supervised and messaging each other concurrently — as a first-class use case, the thing JVM agent platforms (JADE/Akka) were built for, but with static types, no `null`, and compile-time data-race freedom.

### JavaScript / TypeScript ★

**Good**
- JS: closures, first-class functions, JSON, the event loop + promises + `async/await` made asynchronous programming accessible; runs on every device; V8 proved dynamic languages can be fast via JIT; npm (for better or worse) is the largest ecosystem.
- TypeScript: **gradual, structural typing** retrofitted on a dynamic language with excellent inference and editor tooling; discriminated unions, literal types, mapped/conditional types — a very expressive type system that regular developers actually use; `tsc` errors are good.

**Bad**
- JS: designed in 10 days — `==` coercion, `this` binding, hoisting, `var`, `undefined` *and* `null`, prototype confusion, no integers, callback hell before promises, the npm supply-chain risks (left-pad, event-stream, etc.).
- TS: types are erased, so no runtime guarantees; `any` escape hatch; a build step for everything; the type system is Turing-complete and sometimes abused.

**Lume takes**
- `async`/`await` with structured concurrency underneath (not fire-and-forget promises).
- Structural typing *option* for traits (`impl` is nominal by default, `shape` types are structural — the TypeScript/Go interface insight for interop and duck-typing where it helps).
- Literal types and discriminated unions (natural with ADTs).
- Supply-chain security in the package manager: signed packages, reproducible builds, capability-based permissions for packages (a package that does not declare `net` cannot open sockets — see Deno/Austral).

### Delphi / Object Pascal — see Pascal ★. Additional take: **properties**, **events as first-class**, **components** (a UI library that composes), and *fast native compile of a GUI app* as a benchmark Lume must meet.

### Harbour ★ — see dBase/Clipper. Additional take: Harbour proves the xBase community still exists in 2026; Lume should offer a **migration path** (an `xbase` compatibility library and a translator for Clipper/Harbour code) to inherit that community.

### Cyclone → Rust ★

**Good (Rust)**
- **Ownership and borrowing**: memory safety and data-race freedom at compile time, no GC, no runtime cost. The single most important language advance since GC. Adopted by Linux, Windows, Android, AWS, Cloudflare.
- Traits, generics, enums + exhaustive match, `Option`/`Result` + `?`, iterators that compile to loops, `unsafe` as a clearly delimited contract, macros (declarative + procedural), `Drop`, slices, zero-cost abstractions.
- **Cargo**: the best build/package tool in any language; crates.io; `rustfmt`, `clippy`, `rustdoc`, `rust-analyzer`; the best error messages ever written; editions for evolution.
- A community culture of correctness and documentation.

**Bad**
- **Steep learning curve**: the borrow checker fights beginners for months; lifetimes (`'a`) leak into APIs; self-referential structs, async + lifetimes, `Pin`, trait objects with generics — complexity accretes.
- Slow compile times (monomorphisation + LLVM + heavy macros); large binaries by default.
- Syntax noise: `::`, `&mut`, `<'a, T: Trait + 'a>`, `.unwrap()` everywhere in examples, `Box<dyn Fn(&str) -> Result<(), E> + Send + 'static>`.
- Async is a half-finished story (no standard runtime, function colouring, `Send` bounds). No default allocator abstraction in `std` (Zig does this better). Orphan rules limit trait impls.

**Lume takes**
- **Ownership + borrowing as the memory model**, but with three deliberate simplifications:
  1. **Region inference** (Cyclone's lineage, recent research like Polonius/"Oxide"/Verona): lifetimes are inferred inside modules; annotations only at `pub` boundaries that return borrows, and even then usually elided.
  2. **Parameters borrow by default**; `own` to move in, `mut` to borrow mutably. The common case needs no sigils.
  3. **Shared mutability as a first-class, explicit tier**: `rc[T]`/`arc[T]`/`cell[T]` have short names and literal syntax; the compiler suggests them when the borrow checker fails, instead of a wall of text. A `@gc` region (optional tracing GC for a subgraph, like Nim's ORC or D's) exists for the parts of a program where ownership is the wrong model (compilers, GUIs, graph algorithms) — chosen per type/region, never global.
- Traits, `?`, `Result`/`Option`, exhaustive match, iterators, `Drop`, slices, `unsafe` contracts, editions, cargo-class tooling.
- Compile speed: a fast debug tier (interpreter/baseline JIT, like Cranelift/`-O0`) and incremental compilation designed in, not retrofitted.
- Readable generics: `fn sort[T: Ord](items: mut [T])` — square brackets, no angle-bracket ambiguity.

### Go ★

**Good**
- **Simplicity as a feature**: 25 keywords, a spec you read in an afternoon, one way to format (`gofmt`), fast compilation, a single static binary, cross-compilation by env var.
- Goroutines + channels + `select` (CSP): concurrency accessible to every developer; the runtime multiplexes millions of goroutines.
- Structural interfaces (implicit satisfaction), `defer`, composition over inheritance, excellent standard library (net/http, encoding), tooling (`go test`, `go vet`, pprof, race detector), the fastest GC pause story among GC'd languages.
- Backward-compatibility promise kept for 13+ years. The language of the cloud (Docker, Kubernetes, Terraform).

**Bad**
- `nil` (pointers, maps, interfaces — a nil interface vs interface holding nil is a trap); `if err != nil` repeated 3 times per function; no sum types/enums; generics arrived 12 years late and are limited; zero values can hide bugs; `interface{}`/`any` casts; GC pauses and memory overhead make it unsuitable for kernels/real-time/embedded without effort; goroutine leaks (no structured concurrency); no immutability; GOPATH years; the lack of expressiveness pushes code generation (`go generate`).

**Lume takes**
- Simplicity budget: Lume's spec should be readable in a weekend; features must earn their place.
- `defer`; structural `shape` types alongside nominal traits; a `select` over channels; one formatter; static binaries; cross-compilation built in; a race detector in the test runner (ownership makes most races compile errors already).
- Structured concurrency instead of free goroutines: `scope` blocks own their tasks (no leaks, cancellation propagates).
- Error handling that keeps Go's explicitness (errors are visible values) but with `?` to remove the boilerplate, and typed error sets (Zig) so the compiler knows what can fail.

### Nim ★

**Good**
- Python-like syntax compiled to C → tiny fast binaries everywhere, including microcontrollers.
- **AST macros and templates**: compile-time metaprogramming on a typed AST with the full language; `when` compile-time branches; UFCS (`a.f(b)` ≡ `f(a, b)`), which makes APIs fluent without methods; multiple memory strategies (ARC/ORC/manual) chosen per program; effect tracking (`{.raises: [].}`, `{.tags.}`), `distinct` types, exceptions optional.

**Bad**
- Style insensitivity (`fooBar` == `foo_bar`) is contentious; small team → ecosystem gaps and compiler bugs; the proliferation of pragmas `{.…}`; too many ways to do memory; identity crisis (systems vs scripting); `nimble` tooling weaker than cargo.

**Lume takes**
- Indentation syntax + native compilation (Nim's core bet).
- AST macros at `comptime`, `when` branches, UFCS as the method-call rule (any function whose first parameter is `T` is callable as `value.f()`), `distinct`/newtype wrappers (`type UserId = distinct int`).
- Effects in the type system (what a function can do: alloc, io, panic, async…), inferred, surfaced in the signature when `pub`.
- One memory model with explicit tiers (ownership + `rc`/`arc` + optional `@gc` region), not a per-program switch.

### Clojure ★

**Good**
- **Immutable persistent data structures** as the default — structural sharing makes them cheap; most concurrency bugs vanish.
- Simplicity as a value (Rich Hickey's "Simple Made Easy"): data as plain maps/vectors, functions over data, no classes.
- STM, atoms, agents (sane concurrency primitives); core.async (CSP); the REPL-driven workflow; hosted on the JVM (ecosystem for free); EDN; spec (runtime contracts); transducers.

**Bad**
- Lisp syntax (same adoption wall); JVM startup time; stack traces from hell; dynamic typing at scale (spec is optional); small job market.

**Lume takes**
- **Immutable by default** (`let` is immutable; `var` mutable). Persistent collections (`pvec`, `pmap`) in the standard library with literal syntax; the ownership model makes in-place mutation of uniquely-owned values safe *and* fast, so Lume gets both the Clojure safety and the C performance ("mutable value semantics", see Hylo).
- Plain data (records, maps) over object hierarchies; transducer-like composable iterator pipelines that fuse at compile time.
- Atomic references / STM in `std.sync`.

### Kotlin ★

**Good**
- **Null safety done right** (`T?`, `?.`, `?:`, smart casts) on a mainstream platform; extension functions; data classes; coroutines with structured concurrency (`coroutineScope`) — the first mainstream structured-concurrency implementation; trailing lambdas → DSLs (Gradle KTS, Compose); interop with Java so good that adoption was frictionless; pragmatic (not academic) design; multiplatform (JVM/JS/native).

**Bad**
- Tied to JetBrains/Google; compile times; too many features for the sake of Java interop (platform types); native backend performance lagging.

**Lume takes**
- Optional chaining and coalescing (`?.`, `??`), smart casts after a check, data records with auto-derived `Eq/Hash/Show`, extension functions (via UFCS), trailing closures, structured concurrency scopes.

### Swift ★

**Good**
- Optionals, protocols (traits) with extensions, **value semantics** (structs/enums copy-on-write), ARC (deterministic, no GC pause), pattern matching, generics with associated types, `guard`, `defer`, result builders (declarative DSLs → SwiftUI), ownership features arriving (`borrowing`/`consuming`, `~Copyable`), structured concurrency with actors (2021) — a thoughtful synthesis of Rust and Kotlin ideas for app developers.

**Bad**
- Compile times and type-checker explosions on complex expressions; Apple-centric (Linux/Windows second-class); ARC retain/release overhead and cycles (weak/unowned); the language grew huge (result builders, property wrappers, macros, 5 kinds of concurrency); ABI stability came late and was costly.

**Lume takes**
- Value semantics by default for records; copy-on-write for large values where the owner is shared; `guard` (early-exit with binding); result-builder-style declarative blocks for UI/config DSLs through macros.
- ARC-style `rc`/`arc` as the *shared* tier with cycle detection at `comptime` where possible and a `weak` escape.

### Julia ★

**Good**
- **Solved the two-language problem for science**: Python-like dynamic feel, C speed via LLVM JIT and type specialisation; **multiple dispatch** as the organising principle (methods belong to functions, not classes — composability across packages is unmatched); first-class arrays, broadcasting (`.`), unicode operators, macros (Lisp-style on a normal syntax), metaprogramming, differentiable programming (Zygote), great REPL/notebooks.

**Bad**
- Time-to-first-plot (JIT latency) hurt for years; dynamic typing (type stability is a performance cliff); no static compile story for deployment until recently; package correctness issues; 1-based indexing.

**Lume takes**
- Multiple dispatch *within* trait resolution: trait implementations can be specialised on multiple type parameters; `impl Add[Matrix, Vector]`.
- Broadcasting operators (`.+`, `.*`) for arrays/tensors; unicode operators allowed (ASCII equivalents always exist).
- Static compilation first, JIT for the REPL — the opposite default from Julia, same goal.

### Elm ★

**Good**
- **No runtime exceptions**: pure functional, total; the best compiler error messages ever (friendly, specific, with suggestions); the Elm Architecture (model/update/view) influenced Redux and every UI framework; enforced semantic versioning (the package manager *diffs the API* and refuses a minor bump on a breaking change).

**Bad**
- Deliberately limited (no FFI from user code, no type classes); one person's project; stalled development; web only.

**Lume takes**
- **Error messages as a product**: every diagnostic has a plain-English explanation, the exact span, and a suggested fix. A diagnostics style guide is part of the spec.
- **Enforced semantic versioning** in the package manager: `lume publish` computes the API diff and refuses an incorrect version bump.

### Zig ★

**Good**
- **`comptime`**: ordinary Lume code executed at compile time replaces generics, macros and templates — types are values at comptime. **Explicit allocators** passed as parameters (no hidden heap). Error unions (`!T`) and error sets with `try`; `defer`/`errdefer`; no hidden control flow; **C interop by importing headers directly**, cross-compile to anything out of the box (Zig is also the best C/C++ cross-compiler); small, learnable; `undefined` must be explicit; build system in Zig itself.

**Bad**
- Manual memory management (no ownership checker → use-after-free is still possible); no interfaces/traits (duck typing via comptime); no closures; no destructors/RAII (`defer` only); pre-1.0 churn; no package registry.

**Lume takes**
- `comptime` as a core feature; error unions (`fn f() -> T ! E`) with inferred error sets; `defer`/`errdefer`; explicit allocators *as an option* (the stdlib is allocator-aware; the default allocator is a global you can override, but kernels and embedded pass allocators explicitly); `import c "header.h"`; cross-compilation to every target from any host; `undefined` must be written.
- Combined with ownership and RAII, which Zig refuses — Lume believes safety should be the default and manual control the escape hatch, not the reverse.

### Mojo ★

**Good**
- Python-superset syntax, systems performance (claims 10,000× over Python on specific kernels), **MLIR** as the compiler infrastructure (so the same code targets CPU/GPU/TPU/accelerators), ownership + borrowing (`owned`/`borrowed`/`inout`), `struct` vs `class` tiers, parametric compile-time metaprogramming, SIMD types in the language, autotuning. The first mainstream attempt at a language *designed for the AI hardware era*.

**Bad**
- Proprietary origins (Modular), slow open-sourcing; Python superset compatibility is a huge drag (Python's warts come along); not yet a general-purpose or OS language; small ecosystem.

**Lume takes**
- **MLIR as the backend infrastructure** for the native tier (with Cranelift for the fast debug tier). Heterogeneous targets (CPU, GPU, NPU, FPGA via HLS dialects) from one source.
- SIMD and tensor types with shapes in the type system; `@autotune` for kernel parameters.
- Not a Python superset: compatibility with Python through a *bridge* (call Python libraries, be called from Python) rather than inheriting its semantics.

### Solidity ★ (as a cautionary tale)

**Good**: made programmable money accessible; showed that a language can define an economic platform.
**Bad**: billions of dollars lost to reentrancy, integer overflow, visibility defaults, `delegatecall`, and a JS-like syntax that gave false confidence. Language design mistakes have a dollar value.
**Lume takes**: when the stakes are high, the language must make the wrong thing *impossible* (checked arithmetic by default, explicit visibility, no reentrant default, linear/resource types for assets — see Move).

---

## Part II — Grouped influences (every other language in the catalog)

### Early and historical (Plankalkül, Short Code, Autocode, Speedcoding, FLOW-MATIC, IPL, ALGOL 58, MAD, JOVIAL, CPL, PL/I, ALGOL 68, ALGOL W, BCPL, B, Mesa, CLU, Icon, Vulcan)
- **Good**: each contributed one idea that survived — FLOW-MATIC's English keywords, IPL's lists, CLU's iterators/ADTs/exceptions (Liskov — the parent of Python's generators and Java's exceptions), Mesa's interface/implementation split, Icon's generators and goal-directed evaluation, ALGOL W's records and references, BCPL's braces and portability via O-code (an early portable IR).
- **Bad**: PL/I and ALGOL 68 proved that *putting everything in* kills a language — too big to implement, too big to learn. Typeless BCPL/B gave C its unsafety.
- **Lume takes**: iterators/generators (CLU/Icon), interface vs implementation (Mesa), a portable IR (`.lumb`, like O-code/p-code); and the PL/I warning tattooed on the spec: **size budget**.

### APL / J / K / Q / MATLAB / R / NumPy lineage
- **Good**: thinking in whole arrays; rank polymorphism; incredibly dense expression of numerical ideas; interactive exploration; vectorised performance without loops. K/Q run the world's trading systems with microsecond latency.
- **Bad**: APL's symbols are unreadable to outsiders; R's semantics are inconsistent (three OOP systems, lazy arguments, `<-`); MATLAB is proprietary and 1-based; NumPy's shapes are runtime-only (shape errors at runtime are the #1 bug class in ML code).
- **Lume takes**: rank-polymorphic array operations, broadcasting, and **tensor shapes in the type system** (checked at compile time where static, at the boundary where dynamic) — the thing NumPy/PyTorch can't give.

### SNOBOL / awk / sed / Perl / regex / Tcl / REXX / HyperTalk / AppleScript
- **Good**: text as a first-class domain (SNOBOL patterns, awk's pattern-action, regex); REXX and Tcl showed how a clean scripting language integrates with a host system; HyperTalk/AppleScript explored English-like syntax for non-programmers.
- **Bad**: English-like syntax turns out *harder* to write (what exact phrase does the parser accept?) — AppleScript is famously "read-only"; stringly-typed everything (Tcl) kills performance and safety.
- **Lume takes**: pattern-action style in `match` over text streams; a readable but *formal* syntax (English words, strict grammar); no stringly typing.

### Forth / PostScript / Factor / stack languages
- **Good**: astonishing power per byte; interactive even on bare metal; the entire system (compiler + OS + editor) in a few KB; concatenative composition; Open Firmware boots Macs/Suns with it. The purest "language as OS" example.
- **Bad**: stack shuffling is unreadable (`swap dup rot`); no types; every program is a new language.
- **Lume takes**: the *bootstrap* idea — a minimal Lume core that fits in a boot sector-class footprint for firmware/embedded; interactive REPL over a serial line on bare metal (`freestanding` + `std.repl`).

### MUMPS / Pick / Progress ABL / Informix-4GL / Natural / PowerBuilder / Magic / Clarion / DataFlex / Omnis / 4D / FileMaker / Access VBA / Paradox / RPG / ABAP / PL/SQL
- **Good**: all of them put *persistent data in the language* — MUMPS's globals are a persistent key-value store accessed as arrays (`^PATIENT(123,"name")`), 60 years before Redis; the 4GLs delivered business applications with 5–10× less code than Java/C#; ABAP/PL/SQL run the world's ERPs and databases.
- **Bad**: proprietary, vendor-locked, weakly typed, GUI-era transitions mishandled, considered "legacy" so talent fled; terrible for anything outside their domain.
- **Lume takes**: the 4GL productivity for data applications, in an open, typed, general-purpose language: `table`, `query`, `report`, declarative forms — and *persistence as a language feature* (a typed, transactional, embedded store in `std.data`, accessed like variables: the MUMPS idea with types and ACID).

### Eiffel / Design by Contract / Dafny / SPARK / Whiley / F* / Lean / Rocq / Agda / Idris / ATS / Isabelle / TLA+ / Alloy
- **Good**: contracts (Eiffel) make the specification executable; Dafny/SPARK/F* prove programs correct automatically for large subsets using SMT solvers; dependent types (Idris/Agda/Lean) make the type *be* the specification; Lean 4 is a genuinely usable language *and* a prover with Mathlib; TLA+/Alloy find design bugs before code exists (Amazon uses TLA+ on S3/DynamoDB); seL4 and CompCert are proven-correct kernel and compiler.
- **Bad**: proof burden is enormous for full dependent types; proof assistants have steep learning curves; TLA+ specs are separate from code (drift); verification tools are islands, not integrated.
- **Lume takes**: **gradual verification**: contracts always available (`requires/ensures/invariant`, checked at runtime in debug), *optionally* proven by `lume prove` (SMT-backed, Dafny/SPARK-style) on the pure subset; **refinement types** as a lightweight dependent-type step (`int where self > 0`, `[T; n] where n % 4 == 0`); a path to export to Lean for hard proofs. The goal: a kernel or a neural-network runtime where the critical 5% is *proven*, the rest is tested, in one language.

### Occam / CSP / Linda / Oz / Pict / Join / Cilk / X10 / Chapel / Fortress / Akka / Pony / Lingua Franca / Verse / Bend
- **Good**: CSP (channels, `select`, `par`) is the sanest concurrency model and lives on in Go; Linda's tuple spaces are a beautiful coordination model; Oz's dataflow variables (a variable that blocks until bound) unify futures and logic variables; Cilk's work-stealing and `spawn/sync` are the basis of every modern parallel runtime; Chapel/X10 made distributed-memory parallelism (PGAS) readable; **Pony's reference capabilities** (`iso`, `val`, `ref`, `box`, `tag`, `trn`) prove data-race freedom at compile time with actors; Lingua Franca makes concurrency deterministic (reactors with logical time); Verse's transactional/choice semantics; Bend compiles ordinary recursive functional code to run on thousands of GPU cores via interaction nets.
- **Bad**: most never reached production or stayed in HPC/research; Pony has no ecosystem; Fortress was cancelled; HPC languages ignore the mainstream and vice-versa.
- **Lume takes**: structured concurrency (scopes), channels + `select`, dataflow/futures (`promise[T]`), work-stealing task runtime, `par for`/`par map` for data parallelism (Cilk/Chapel), actors with typed mailboxes and reference-capability-like guarantees via ownership (`arc`, `Send`/`Sync`-style marker traits), a deterministic-time option (`std.time.logical`) for simulations and robotics, and **GPU/accelerator kernels written in Lume itself** (the Bend/Mojo/Futhark/Triton insight), with the compiler handling the data-parallel lowering through MLIR.

### Haskell's children and effect systems: Koka, Eff, Frank, Effekt, Flix, Unison, PureScript, Roc, Gleam, Elm
- **Good**: **algebraic effects and handlers** (Koka, Effekt) generalise exceptions, async, generators, state and dependency injection into one mechanism with a type that says exactly what a function *does*; Koka's Perceus reference counting (reuse analysis makes functional updates in-place); Unison's content-addressed code (no dependency conflicts, no builds, code is identified by hash, distributed computation by sending code); Roc's platform/application split (the platform owns effects, the app is pure — perfect for embedding, plugins, and OS/userland boundaries); Gleam's friendly types on BEAM; Flix embeds Datalog in a typed functional language.
- **Bad**: effects research is still finding ergonomic syntax; most of these have tiny communities.
- **Lume takes**: **an effect system** (`fn read_config() -> Config / {io, alloc}`), inferred, used for: capability security (a function without `net` cannot touch the network), purity (pure functions are provable, memoisable, parallelisable, usable at `comptime`), async (no function colouring — `async` is an effect), and resumable handlers for generators/coroutines. **Content-addressed modules** in the package store (Unison/Nix: no "dependency hell"; two versions coexist). **Platform/application split** as the OS boundary model (Roc).

### Smalltalk's children and live programming: Self, Squeak, Pharo, Io, Dylan, Hazel, Dark, Scratch, Processing, Max/MSP, LabVIEW, Excel
- **Good**: liveness (change the program while it runs and see the result instantly); visual/dataflow programming works for specific audiences (musicians, engineers, kids); **spreadsheets are the most successful programming environment ever** — reactive, visible state, immediate; Self invented the JIT techniques (maps, inline caching) in V8/HotSpot; Hazel shows that programs with holes should still typecheck and run.
- **Bad**: liveness without versioning and reproducibility fails at scale (image rot); visual languages don't scale to large programs, don't diff, don't merge.
- **Lume takes**: **holes** (`?` / `todo` typed holes the compiler reports with the expected type and *lets you run around*); a **reactive/incremental** standard model (`signal[T]`, derived values recompute automatically — the spreadsheet/Elm/Svelte model for UIs *and* for incremental build pipelines); live reload in the dev tier; a structured editor projection (`lume ide` can show a dataflow view of a `query`/`pipeline`) — projections of text, text stays canonical.

### Enterprise and VM languages: C#, F#, Scala, Groovy, Dart, Hack, Vala, Haxe, Boo, Nemerle, Crystal, D, Carbon, Hylo/Val, Vale, Austral, Odin, Jai, Hare, C3, V, Inko, Cppfront, Circle
- **Good**: C# — LINQ (query syntax over any collection — a direct ancestor of Lume's `query`), properties, events, `async/await` (C# invented the syntax), records, pattern matching, spans, a real `decimal` type, excellent tooling; F# — units of measure in types (`float<m/s>`), type providers (types generated from external schemas at compile time), computation expressions, pipelines `|>`; Scala — unification of OO/FP, implicits/givens, Spark; D — compile-time function evaluation, ranges, contracts, scope guards, `@safe` subset; Dart — sound null safety migration at scale, hot reload for UI; Crystal — proves Ruby syntax + static types + global inference works; **Hylo/Val's mutable value semantics** (safety without a borrow checker: no first-class references, projections instead); Vale's generational references; Austral's linear types + capability-based security + no hidden anything; Odin/Jai — explicit context/allocators, SOA layout, the "programmer joy" of game devs; Carbon/Cppfront/Circle — how to migrate off C++ (interop-first, borrow checking added to C++ syntax).
- **Bad**: C#/Java locked to vendors for years; Scala's complexity and binary compatibility breaks; D's split GC/no-GC ecosystem; most of the new systems languages lack adoption; V's over-promising hurt trust.
- **Lume takes**: LINQ-style `query` (C#); **units of measure in types** (`f64[m/s]`) and **type providers** (`comptime` can read a schema — SQL, JSON, Protobuf, a CSV header — and generate types) from F#; pipelines `|>`; CTFE (D/Zig `comptime`); `@safe` by default with `unsafe` islands; SOA/AOS layout control (`@soa`); the Hylo insight that *most* code never needs explicit references (Lume's default: value semantics + borrowing inferred, references as a visible, rarer tool).

### Web and config languages: PHP, ActionScript, CoffeeScript, XSLT, CSS, HTML templates, JSON/YAML/TOML, CUE, Dhall, Jsonnet, Nickel, Pkl, HCL, Nix, Guix, Starlark, Dockerfile, Kubernetes YAML
- **Good**: PHP — deploy by copying a file, immediate feedback, the lowest barrier to a working web page; CoffeeScript — a transpiler can move a whole language forward; CSS — a cascade/constraint system that non-programmers use; **Nix** — a pure lazy functional language that builds whole operating systems reproducibly from a description (the strongest existing link between "language" and "OS"); CUE/Dhall/Pkl — typed, validated configuration; Starlark — a deterministic, hermetic Python subset for builds.
- **Bad**: YAML is the worst language in production (Norway problem, indentation, 9 ways to write a string) and runs all of infrastructure; config "languages" accrete logic until they are bad programming languages (Helm templates, Ansible); PHP's early inconsistency and security defaults.
- **Lume takes**: **Lume is its own configuration language** — a restricted, pure, deterministic subset (`lume.toml` is actually `lume` syntax with effects forbidden: data literals + pure functions, like Starlark/Nix/CUE) for build files, package manifests, infrastructure and OS configuration. One language, one parser, one type checker, from the config file to the kernel. The build system is Lume (Zig's insight).

### Hardware, GPU and domain languages: Verilog, VHDL, Chisel, SpinalHDL, GLSL/HLSL/WGSL/SPIR-V, CUDA, OpenCL, Halide, Futhark, Taichi, Triton, TVM, MLIR, OpenSCAD, G-code, PLC/IEC 61131, Modelica, Simulink, Lustre/Esterel, Q#/OpenQASM
- **Good**: Halide's separation of *algorithm* from *schedule* (what to compute vs how to map it to hardware) is the key idea in modern ML compilers (TVM, Triton, Mojo autotune); Chisel shows hardware generation from a real language (Scala) beats Verilog; MLIR is the Rosetta stone (dialects for every level from tensors to LLVM to hardware); Lustre/Esterel make *time* a type-checked concept (synchronous languages run flight controls); PLC languages run factories with deterministic cycle time; Modelica models physics acausally.
- **Bad**: separate languages for CPU/GPU/FPGA means separate code bases, separate type systems and separate bugs; shader languages are each a dialect of C with no sharing; hardware languages are decades behind software in tooling.
- **Lume takes**: **one language for CPU, GPU and (via HLS dialects) FPGA kernels**, through MLIR; `kernel fn` for data-parallel code with the compiler doing the lowering; **algorithm/schedule split** (`@schedule` attributes and a schedule DSL for `kernel` functions); a **synchronous/real-time subset** (`realtime` effect: no allocation, bounded time, verified WCET where possible) for control systems and kernels' interrupt paths; a Chisel-style hardware generation library as a long-term goal (`std.hw`).

### AI and probabilistic languages: Planner, OPS5, CLIPS, Drools, OWL, SOAR, Church, Stan, Pyro, PyMC, Gen, Lush, Torch, Theano, TensorFlow, PyTorch, Jax, Swift for TensorFlow, Zygote, Dex, Enzyme, Myia, DiffTaichi, ONNX, LMQL, Guidance, DSPy, SGLang, BAML, Nengo, Lava, Brian2, NEST
- **Good**: rules engines (OPS5/CLIPS/Drools) declare knowledge and let a matcher (Rete) fire rules — still the right model for policy/business rules; probabilistic programming (Church/Stan/Pyro/Gen) makes *inference* a language primitive (write the generative model, ask for the posterior); **differentiable programming** (Swift for TensorFlow, Zygote, Enzyme, Dex): `gradient(f)` as a language operator, autodiff on the compiler's IR rather than on a tensor library's tape; Jax's function transformations (`grad`, `vmap`, `jit`, `pmap`) as composable higher-order operators; PyTorch's eager usability; Dex's typed indices (shape errors become type errors); ONNX as a model interchange format; the 2023–2025 "prompt languages" (DSPy, BAML, LMQL, SGLang) treat an LLM call as a *typed function with a schema*, optimise prompts like compilers optimise code, and compile structured output; neuromorphic languages (Nengo, Lava) program spiking networks with time as a dimension.
- **Bad**: every one of them is a DSL inside Python (or a dead standalone language), so: shape errors at runtime, two-language performance cliffs, no static reasoning about models, no way to deploy without a Python runtime, no unified type system across model/data/serving code; Swift for TensorFlow was cancelled because Swift wasn't the AI community's language; probabilistic languages are hard to integrate with regular code; rules engines are stuck in Java-land.
- **Lume takes** (the heart of `05-vision.md`): tensors with typed shapes and typed indices (Dex); **autodiff as a language transformation on Lume's own IR** (`grad f`, `jvp`, `vjp`, `vmap` as first-class operators on any pure function, including control flow, like Enzyme/Zygote); `kernel` functions for accelerators; **`model` as a type** (a parametrised differentiable function with a serialisable state, ONNX export/import built in); **probabilistic primitives** (`sample`, `observe`, `infer`) in a `prob` effect, so a generative model is ordinary code; **rules** (`rule … when … then …`) compiled to a Rete/Datalog engine over `table`s; and **LLM calls as typed functions** (`extern llm fn summarise(text: str) -> Summary` with the schema enforced by constrained decoding, retries and evals in the language's test runner) — the DSPy/BAML idea as a core feature, so an AI agent, its tools, its memory (tables) and its evaluation are one typed program.

### OS and embedded languages: Mesa/Cedar, Lisp machines, Oberon System, Singularity/Sing#, Midori, House, Redox/Theseus/Tock (Rust), seL4, Nix again, Arduino/MicroPython/TinyGo, Forth firmware
- **Good**: Oberon and Lisp machines and Smalltalk show that when the OS and the language are co-designed, the whole system is 10–100× smaller and fully inspectable; Singularity (Microsoft Research) showed a *managed* OS with software-isolated processes (SIPs) and contract-checked channels can be faster than hardware isolation; Theseus (Rust) uses the language's ownership to enforce OS invariants with no runtime cost and live-evolves components; Tock uses Rust for an embedded OS with untrusted apps; seL4 is proven; TinyGo/MicroPython show high-level languages fit in microcontrollers.
- **Bad**: co-designed systems became islands (Lisp machines died with their hardware, Oberon never left ETH); managed OSes needed a GC in the kernel; Rust kernels still need lots of `unsafe` and the language has no notion of "privilege level", "interrupt context" or "page table" — those live in conventions.
- **Lume takes**: `freestanding` target; `realtime` and `interrupt` effects (the type system knows this function may run in interrupt context and forbids allocation/blocking); **capability types** for kernel resources (a page, a port, a device register is a linear capability value — Austral/Move/seL4 style — that cannot be duplicated or forged); typed memory-mapped registers (`@mmio`); the **platform/application split** (Roc) as the kernel/userland boundary: userland code is parametrised by the capabilities the kernel passes it; software-isolated processes become possible because the compiler verifies memory safety (Singularity's bet, without the GC); a reference OS (`lume-os`) as the project's long-term proof.

### Esoteric languages (Brainfuck, Whitespace, INTERCAL, Malbolge, Piet, Befunge, LOLCODE, Rockstar…)
- **Good**: Brainfuck proves eight instructions suffice for Turing completeness — the minimal core matters when bootstrapping a compiler; they teach humility about syntax.
- **Bad**: everything else, intentionally.
- **Lume takes**: a **tiny verified core** (`lume-core`: a small typed IR with a formal semantics, ~30 constructs) that the full language desugars into. Everything fancy is sugar over a core small enough to prove things about and re-implement in a weekend — the key to self-hosting, to the verifier, and to porting Lume to a new machine.

---

## Part III — The scoreboard

What each pillar of Lume owes to whom, in one table:

| Lume feature | Primary source | Secondary sources |
|---|---|---|
| Indentation syntax, readability | Python | Pascal, Nim, COBOL (readability), Ruby |
| Ownership + borrowing, no GC | Rust | Cyclone, Clean (uniqueness), Hylo (value semantics), Pony |
| `rc`/`arc` shared tier, optional `@gc` region | Swift (ARC), Nim (ORC) | Koka (Perceus), D |
| Error unions `! E`, `?`, `defer/errdefer` | Zig, Rust | Go (explicit errors), Swift |
| `T?` optionals, `?.`, `??` | Kotlin, Swift | ML (Option), Haskell (Maybe) |
| ADTs + exhaustive `match` | ML, Rust | Haskell, Prolog (patterns), Scala |
| Traits with associated types/defaults | Rust, Haskell | Swift protocols, Go (structural), Julia (multiple dispatch) |
| Structured concurrency, `scope`, channels, `select` | Kotlin, Go | Trio, Swift, Occam/CSP, Erlang |
| Actors + supervisors | Erlang/Elixir | Pony, Akka, Orleans |
| `comptime` | Zig | D (CTFE), Nim, C++ constexpr, Jai |
| Hygienic AST macros | Nim, Rust | Lisp/Scheme, Julia, Elixir |
| `record`, `table`, `query`, `index`, `relation` | dBase/Clipper/FoxPro, SQL | C# LINQ, MUMPS (persistent globals), 4GLs, Datalog |
| `decimal(p,s)` | COBOL | C#, dBase, PL/SQL |
| Range/refinement types, sets, enums | Pascal, Ada | Idris/Dafny (refinements), F# units |
| Contracts + gradual proof | Eiffel, Ada/SPARK, Dafny | Lean, F*, Whiley |
| Effect system | Koka, Effekt | Nim (effects), Haskell (monads, as the thing to avoid), Roc (platforms) |
| Tensor shapes in types, autodiff, `kernel`, `model` | Dex, Jax, Swift for TensorFlow, Mojo | Futhark, Halide, Triton, Julia/Zygote, Enzyme, FORTRAN (arrays) |
| LLM calls as typed functions, rules | DSPy/BAML/LMQL, OPS5/CLIPS | Prolog, Drools |
| Probabilistic primitives | Church/Gen/Stan/Pyro | |
| Capability types, `freestanding`, `interrupt`/`realtime` effects | seL4, Austral, Move, Rust kernels | Oberon, Singularity, Lustre, Ada |
| C interop by importing headers, cross-compile everywhere | Zig | Rust (bindgen), Nim |
| Unified tooling (`lume build/fmt/test/doc/pkg/repl/prove`) | Cargo/Rust, Go | Elm (semver enforcement), Nix/Unison (content-addressed store), Deno (permissions) |
| Config = restricted Lume | Zig (build.zig), Nix, Starlark, CUE | |
| Fast debug compilation, REPL, live reload | Turbo Pascal, Lisp, Smalltalk | Go, Dart, Erlang |
| Error messages as a product | Elm, Rust | |
| Typed holes, liveness | Hazel, Smalltalk | Idris, spreadsheets |
| Tiny verified core | Scheme, Brainfuck (minimalism), Oberon | Lean, seL4 |
| Size budget, remove features | Go, Oberon | PL/I and C++ as warnings |

Next: `02-principles.md` turns this scoreboard into rules the design must obey, and `05-vision.md`
is the out-of-the-box part — what a language *for the AI era* that can build an operating system
and a neural system should look like.
