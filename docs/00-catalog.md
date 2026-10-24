# 00 — Catalog of programming languages

A census of the languages that matter to the design of Lume: every language that introduced an idea,
dominated an era, or still runs production code today. Thousands of languages exist (the Wikipedia
timeline alone lists over 700, HOPL over 8,000 names); this catalog keeps the ~260 that left a mark.

Grouped by era and family. Year = first public release or paper. ★ = a primary influence on Lume,
analysed in depth in `01-survey.md`.

---

## 1. Machine level and the first abstractions (1940s–1950s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ Machine code | 1940s | — | imperative, binary | The ground truth every compiler targets |
| ★ Assembly (x86, ARM, RISC-V, 6502, Z80, 68k…) | 1949 | Wheeler (EDSAC) | imperative, symbolic | One mnemonic per instruction; macros came later |
| Plankalkül | 1945 | Konrad Zuse | imperative | First high-level design; never ran in its era |
| Short Code | 1949 | Mauchly | interpreted | First interpreted "high-level" language |
| Autocode | 1952 | Alick Glennie | imperative | First compiled language (Manchester Mark 1) |
| Speedcoding | 1953 | John Backus | interpreted | IBM 701; led to FORTRAN |
| ★ FORTRAN | 1957 | Backus / IBM | imperative, procedural | First widely used compiled language; still rules HPC |
| FLOW-MATIC | 1955 | Grace Hopper | business | English-like syntax; parent of COBOL |
| IPL | 1956 | Newell, Shaw, Simon | list processing | Linked lists, recursion; first AI language |
| ★ LISP | 1958 | John McCarthy | functional, symbolic | Code as data, GC, REPL, macros, recursion |
| ALGOL 58 | 1958 | IFIP committee | procedural | Block structure; precursor to ALGOL 60 |
| ★ COBOL | 1959 | CODASYL / Hopper | business, imperative | Records, decimal arithmetic, readable; still runs banks |
| MAD | 1959 | U. Michigan | procedural | Fast compiler for the IBM 704 |
| JOVIAL | 1959 | Jules Schwartz | procedural | Embedded/military; ALGOL dialect |

## 2. The ALGOL lineage and the structured revolution (1960s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ ALGOL 60 | 1960 | Naur, Backus, et al. | procedural, structured | BNF, lexical scope, recursion; the "Latin" of languages |
| APL | 1962 | Kenneth Iverson | array | One-symbol operators, whole-array thinking |
| SNOBOL | 1962 | Griswold et al. | string processing | Pattern matching as control flow |
| CPL | 1963 | Strachey, Cambridge | procedural | Parent of BCPL → B → C |
| ★ Simula | 1962/67 | Dahl & Nygaard | object-oriented | Classes, objects, inheritance, coroutines — OOP is born |
| ★ BASIC | 1964 | Kemeny & Kurtz | imperative | Teaching language; brought programming to microcomputers |
| PL/I | 1964 | IBM | multi-paradigm | "Everything from FORTRAN and COBOL"; too big |
| ★ Smalltalk | 1972 (1980 public) | Kay, Ingalls, Goldberg (Xerox PARC) | pure OO, live | Everything is an object, message passing, live image, IDE |
| ALGOL 68 | 1968 | van Wijngaarden et al. | procedural | Orthogonal design; famously hard to implement |
| ALGOL W | 1966 | Wirth & Hoare | procedural | Records, references; Wirth's step toward Pascal |
| BCPL | 1967 | Martin Richards | procedural | Typeless; `{ }` braces; bootstrap language of C |
| LOGO | 1967 | Papert, Feurzeig | educational, Lisp dialect | Turtle graphics; learning by doing |
| MUMPS (M) | 1966 | Mass. General Hospital | database, imperative | Integrated persistent key-value store; still runs hospitals |
| RPG | 1959/1969 | IBM | report generation | Fixed-column business reports on IBM midrange |
| ★ Forth | 1970 | Chuck Moore | stack-based, concatenative | Tiny, extensible, interactive; runs on bare metal |
| B | 1969 | Ken Thompson | procedural | BCPL stripped down for the PDP-7 |

## 3. Systems, Pascal, and the Unix era (1970s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ Pascal | 1970 | Niklaus Wirth | procedural, structured | Strong typing, ranges, sets, records; teaching + Turbo Pascal/Delphi industry |
| ★ C | 1972 | Dennis Ritchie | procedural, systems | Portable assembly; Unix; the ABI of everything |
| ★ Prolog | 1972 | Colmerauer, Roussel | logic | Unification, backtracking, declarative relations |
| ★ ML | 1973 | Robin Milner | functional | Hindley–Milner type inference, algebraic types, pattern matching |
| Scheme | 1975 | Sussman & Steele | functional, Lisp | Minimal Lisp, lexical scope, continuations, hygienic macros |
| ★ SQL | 1974 | Chamberlin & Boyce (IBM) | declarative, relational | The one declarative language everyone uses |
| ★ Modula / Modula-2 | 1975/78 | Niklaus Wirth | procedural, modular | Modules with explicit interfaces; coroutines |
| CLU | 1975 | Barbara Liskov | procedural, abstract data types | Iterators, exceptions, ADTs, parametric types |
| Mesa | 1976 | Xerox PARC | modular | Interfaces/implementations split; influenced Modula, Java |
| awk | 1977 | Aho, Weinberger, Kernighan | text processing | Pattern-action, associative arrays |
| ★ Bourne shell / sh | 1977 | Stephen Bourne | scripting | Pipes and composition as programming |
| Icon | 1977 | Griswold | goal-directed | Generators, success/failure instead of booleans |
| MATLAB | 1978 (1984 commercial) | Cleve Moler | array, numeric | Matrices as the primitive; engineering lingua franca |
| ★ dBase II | 1979 | Wayne Ratliff (Ashton-Tate) | database, 4GL | Data tables as the program's world; interactive dot prompt |
| REXX | 1979 | Mike Cowlishaw (IBM) | scripting | Readable scripting on mainframes/OS/2/Amiga |
| ★ Ada | 1980 | Jean Ichbiah (DoD) | procedural, concurrent, safety-critical | Packages, tasks, strong typing, generics; later contracts (SPARK) |
| Vulcan | 1978 | Wayne Ratliff | database | The JPL prototype that became dBase |

## 4. Objects, scripting, and the PC boom (1980s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ C++ | 1983/85 | Bjarne Stroustrup | multi-paradigm, systems | Classes + C, RAII, templates, zero-overhead principle |
| Objective-C | 1984 | Cox & Love | OO (Smalltalk messaging on C) | Dynamic dispatch, categories; Apple's language until Swift |
| ★ Common Lisp | 1984 | ANSI X3J13 | multi-paradigm Lisp | CLOS, condition system, the big unified Lisp |
| Standard ML | 1983/90 | Milner et al. | functional | Formal semantics, module system (functors) |
| ★ Clipper | 1985 | Nantucket | database, xBase compiler | Compiled dBase; code blocks, user-defined functions, DOS business apps |
| FoxBASE / FoxPro | 1984/89 | Fox Software | database, xBase | Fastest xBase; Rushmore indexing; later Visual FoxPro |
| dBase III / IV | 1984/88 | Ashton-Tate | database, 4GL | The xBase standard everyone cloned |
| Paradox PAL | 1985 | Borland | database | Query-by-example desktop DB |
| ★ Turbo Pascal | 1983 | Anders Hejlsberg (Borland) | procedural | One-pass compiler, IDE, units; made Pascal industrial |
| Miranda | 1985 | David Turner | lazy functional | Parent of Haskell |
| Eiffel | 1986 | Bertrand Meyer | OO, design by contract | `require` / `ensure` / `invariant` built in |
| ★ Erlang | 1986 | Armstrong, Virding, Williams (Ericsson) | functional, actor, concurrent | Lightweight processes, supervisors, hot code reload, "let it crash" |
| Self | 1987 | Ungar & Smith | prototype-based OO | Prototypes, JIT techniques that fed into V8/HotSpot |
| ★ Perl | 1987 | Larry Wall | scripting, text | Regex as a first-class citizen; CPAN; "there's more than one way" |
| Oberon | 1987 | Wirth & Gutknecht | procedural, modular | Language + OS in one small system; type extension |
| Tcl | 1988 | John Ousterhout | scripting, embeddable | Everything is a string; Tk GUI |
| Mathematica / Wolfram | 1988 | Stephen Wolfram | symbolic, term rewriting | Notebooks, symbolic computation |
| HyperTalk | 1987 | Apple | English-like scripting | HyperCard; natural-language flavor |
| Clean | 1987 | Plasmeijer et al. | lazy functional | Uniqueness types (precursor of ownership) |
| Oz | 1991 | Smolka et al. | multi-paradigm, constraint | Dataflow variables, constraints (Mozart) |
| ABC | 1987 | CWI (Meertens, Pemberton) | teaching | Indentation blocks; direct parent of Python |
| Occam | 1983 | INMOS (May) | concurrent (CSP) | Transputer; channels and parallel blocks |
| ★ Bash | 1989 | Brian Fox (GNU) | scripting | The shell that runs the world's glue |
| PostScript | 1982 | Adobe | stack-based | Printers run a Forth-like language |
| Verilog | 1984 | Gateway Design | hardware description | Describes circuits, not programs |
| VHDL | 1983/87 | DoD / IEEE | hardware description | Ada-flavored HDL |
| Lotus 1-2-3 macros / spreadsheets | 1983 | Lotus | reactive, dataflow | Spreadsheets are the most used functional reactive language |

## 5. The web, the VM, and the "scripting golden age" (1990s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ Haskell | 1990 | committee (Hudak, Peyton Jones, Wadler…) | pure lazy functional | Type classes, monads, purity; the research vehicle |
| ★ Python | 1991 | Guido van Rossum | multi-paradigm, dynamic | Readability, batteries included, REPL, indentation |
| Visual Basic | 1991 | Microsoft (Cooper) | event-driven, RAD | Drag-and-drop GUIs; most-used language of the 90s |
| ★ Lua | 1993 | Ierusalimschy et al. (PUC-Rio) | scripting, embeddable | Tables, coroutines, tiny C API; games |
| R | 1993 | Ihaka & Gentleman | statistical, array | Statistics and plotting; CRAN |
| ★ Ruby | 1995 | Yukihiro Matsumoto | OO, dynamic | Programmer happiness; blocks; Rails |
| ★ Java | 1995 | James Gosling (Sun) | OO, VM | Write once run anywhere, GC, huge ecosystem, enterprise |
| ★ JavaScript | 1995 | Brendan Eich (Netscape) | multi-paradigm, prototype OO | The web's language; closures, event loop; accidental universality |
| PHP | 1995 | Rasmus Lerdorf | web scripting | Templating that grew into a language; runs most of the web |
| ★ Delphi (Object Pascal) | 1995 | Borland (Hejlsberg) | OO Pascal, RAD | Visual Pascal; fast native Windows apps |
| ★ OCaml | 1996 | Leroy et al. (INRIA) | functional + OO | Fast native ML; pragmatic purity; modules |
| ★ Harbour / xHarbour | 1999 | open source | xBase compiler | Free Clipper successor, still maintained |
| Visual FoxPro | 1995 | Microsoft | database, OO xBase | Data-centric OO; killed in 2007, still loved |
| Squeak | 1996 | Kay, Ingalls (Apple/Disney) | Smalltalk | Open Smalltalk; Etoys, Scratch ancestry |
| ActionScript | 1998 | Macromedia | ECMAScript | Flash |
| XSLT | 1999 | W3C | declarative, functional | Pure functional transforms over XML |
| AppleScript | 1993 | Apple | English-like scripting | Inter-application automation |
| Limbo | 1995 | Bell Labs (Pike, Dorward) | concurrent (CSP) | Inferno OS; direct ancestor of Go |
| Dylan | 1992 | Apple | dynamic OO, Lisp-derived | Infix Lisp with macros |
| Pike | 1994 | Hübinette | C-like interpreted | Roxen web server |
| Curl | 1998 | MIT | web content | Document + program in one |
| Alice ML | 2000 | Saarland | functional, distributed | Futures, pickling |
| ★ Cyclone | 2001 | Cornell / AT&T | safe C | Region-based memory — the direct seed of Rust lifetimes |
| D | 2001 | Walter Bright | multi-paradigm, systems | "C++ done right": GC optional, CTFE, ranges, contracts |
| C# | 2000 | Hejlsberg (Microsoft) | OO, VM | Java refined: properties, LINQ, async/await, generics done right |
| Scratch | 2003 | MIT Media Lab | visual blocks | Programming for children; millions of users |
| Groovy | 2003 | James Strachan | dynamic JVM | Scripting for Java; Gradle |
| ★ Scala | 2004 | Martin Odersky | OO + functional, JVM | Unifies OO and FP; implicits; Spark |
| F# | 2005 | Don Syme (Microsoft) | functional, .NET | OCaml for .NET; units of measure; type providers |
| ★ Erlang/OTP → Elixir | 2011 | José Valim | functional, actor (BEAM) | Ruby-flavored syntax on Erlang's VM; Phoenix |
| Io | 2002 | Steve Dekorte | prototype OO | Minimal message-passing language |
| Factor | 2003 | Slava Pestov | concatenative | Modern Forth with types and a large library |
| Boo | 2003 | Rodrigo B. de Oliveira (Brazil) | Python-like for .NET | Static typing with inference, macros |
| Nemerle | 2003 | Wrocław | .NET, macros | Hygienic macros on .NET |
| Fortress | 2006 | Sun (Guy Steele) | HPC | Math-notation source; cancelled |
| Chapel | 2009 | Cray | parallel HPC | Locales, domains, PGAS |
| X10 | 2004 | IBM | parallel (APGAS) | Places, async/finish |
| Processing | 2001 | Reas & Fry | creative coding (Java) | Sketches; p5.js descendant |
| ★ Clojure | 2007 | Rich Hickey | functional Lisp, JVM | Immutable persistent data structures, STM, simplicity |
| ★ Go | 2009 | Pike, Thompson, Griesemer (Google) | procedural, concurrent (CSP) | Goroutines, channels, gofmt, fast builds, static binaries |
| CoffeeScript | 2009 | Jeremy Ashkenas | compiles to JS | Pushed ES6 syntax (arrows, classes) |
| Vala | 2006 | Jürg Billeter (GNOME) | OO compiled to C | C# syntax, GObject, no VM |
| Haxe | 2005 | Nicolas Cannasse | multi-target | One source, many targets (JS, C++, JVM…) |
| ★ Nim | 2008 | Andreas Rumpf | multi-paradigm, compiled to C | Python syntax, AST macros, optional GC/ARC, UFCS |
| OpenCL C / CUDA C | 2007/2009 | Khronos / NVIDIA | GPU kernels | Data-parallel programming on GPUs |
| Agda | 2007 | Norell (Chalmers) | dependently typed | Proofs as programs |
| Coq / Gallina (now Rocq) | 1989 | INRIA | proof assistant | CompCert, verified software |
| Idris | 2007/2011 | Edwin Brady | dependently typed, practical | Type-driven development |
| ATS | 2004 | Hongwei Xi | dependently typed systems | Linear types + proofs at C speed |
| Ur/Web | 2008 | Adam Chlipala | functional web | Whole-stack web type safety |
| Opa | 2011 | MLstate | web | Full-stack in one language |

## 6. The modern wave: safety, concurrency, data (2010s)

| Language | Year | Author / origin | Paradigm | Note |
|---|---|---|---|---|
| ★ Rust | 2010/2015 | Graydon Hoare → Mozilla | systems, ownership | Memory safety without GC, traits, fearless concurrency, Cargo |
| ★ Kotlin | 2011 | JetBrains | OO + functional, JVM/native | Null safety, extension functions, coroutines; Android |
| Dart | 2011 | Google (Bak, Lund) | OO, VM/AOT | Flutter; sound null safety |
| ★ TypeScript | 2012 | Hejlsberg (Microsoft) | gradual typing on JS | Structural types, inference, the most adopted gradual type system |
| ★ Julia | 2012 | Bezanson, Karpinski, Shah, Edelman | multiple dispatch, numeric | Dynamic feel, C speed, LLVM JIT; scientific computing |
| ★ Elm | 2012 | Evan Czaplicki | pure functional web | No runtime exceptions; friendly compiler errors; TEA architecture |
| Hack | 2014 | Facebook | gradual PHP | Gradual types at scale |
| ★ Swift | 2014 | Chris Lattner (Apple) | multi-paradigm, ARC | Optionals, protocols, value semantics, ARC; Apple platforms |
| Crystal | 2014 | Manas (Argentina) | Ruby syntax, compiled | Ruby feel, static types by global inference |
| ★ Zig | 2016 | Andrew Kelley | systems, no hidden control flow | `comptime`, explicit allocators, error unions, C interop/cross-compile |
| Pony | 2015 | Sylvan Clebsch | actor, capabilities | Reference capabilities prove data-race freedom |
| Reason / ReScript | 2016 | Facebook (Walke) | OCaml for JS | OCaml with JS syntax |
| PureScript | 2013 | Phil Freeman | pure functional → JS | Haskell for the browser; row types |
| ★ Elixir | 2011 | José Valim | functional, actor | See §5; the "modern Erlang" |
| Gleam | 2016/2024 | Louis Pilfold | typed functional on BEAM | Static types for Erlang's VM |
| Red | 2011 | Nenad Rakocevic | Rebol-like, full-stack | From GUI to bare metal in one language |
| Wolfram Language | 2014 (named) | Wolfram | symbolic | Knowledge-based |
| Ballerina | 2017 | WSO2 | cloud/network | Network types, sequence diagrams from code |
| ★ Solidity | 2014 | Ethereum (Wood) | contract-oriented | Smart contracts; shows how costly language mistakes get |
| Move | 2019 | Facebook/Diem → Aptos/Sui | resource-oriented | Linear "resources" for assets |
| Vyper | 2017 | Ethereum | contract, Python-like | Security over expressiveness |
| Q# | 2017 | Microsoft | quantum | Qubits as a type |
| OpenQASM / Qiskit / Cirq | 2017 | IBM / Google | quantum circuits | Circuit-level quantum |
| Lean 4 | 2021 | Leonardo de Moura (Microsoft) | dependently typed + proof assistant | Mathlib; a real language and a prover; metaprogramming in itself |
| F* | 2011 | Microsoft Research / INRIA | dependently typed, effects | Verified crypto (HACL*) |
| Dafny | 2009 | Rustan Leino | verification-aware imperative | Specs + code, automated proofs (Z3) |
| Whiley | 2010 | David Pearce | verification | Extended static checking |
| Koka | 2012 | Daan Leijen (Microsoft) | effect types | Algebraic effects + Perceus reference counting |
| Eff / Frank / Effekt | 2012–2020 | research | algebraic effects | Effect handlers as the control-flow primitive |
| Unison | 2019 | Chiusano & Bjarnason | content-addressed functional | Code identified by hash; no builds, no dependency conflicts |
| Dark | 2019 | Ellen Chisa, Paul Biggar | deployless | Live editing of running backends |
| Hazel | 2017 | Omar et al. | live, hole-aware | Programs with holes always have meaning |
| Futhark | 2014 | DIKU (Henriksen) | data-parallel functional | GPU arrays |
| Halide | 2012 | MIT (Ragan-Kelley) | image-processing DSL | Separates algorithm from schedule |
| ★ Mojo | 2023 | Chris Lattner (Modular) | Python superset, MLIR | Python syntax + systems performance + hardware portability for AI |
| Carbon | 2022 | Google (Carruth) | C++ successor | Interop-first migration path from C++ |
| Val / Hylo | 2022 | Abrahams, Racordon | mutable value semantics | Safety without borrow checker complexity |
| Vale | 2020 | Evan Ovadia | generational references | Memory safety with a new mechanism |
| Austral | 2022 | Fernando Borretti | linear types, capabilities | Minimal, no hidden anything |
| Odin | 2016 | Ginger Bill | systems, C-like | Explicit allocators, SOA, "joy of programming" |
| Jai | 2014 (private) | Jonathan Blow | systems, compile-time | Compile-time execution, SOA, game dev |
| Hare | 2022 | Drew DeVault | systems, C-like | Small, no macros, qbe backend |
| C3 | 2019 | Christoffer Lernö | C evolution | C with modules, generics, contracts |
| V | 2019 | Alexander Medvednikov | Go-like | Fast compile, autofree (promises vs reality) |
| Inko | 2015 | Yorick Peterse | actor, ownership | Single ownership + actors without a GC |
| Grain | 2017 | Oscar Spencer | functional → WASM | WASM-first |
| AssemblyScript | 2017 | — | TypeScript → WASM | TS subset for WebAssembly |
| WebAssembly (WAT) | 2017 | W3C | stack-machine bytecode | The portable target of the 2020s |
| Roc | 2019 | Richard Feldman | pure functional, platforms | Elm for everything; platform/application split |
| Bosque | 2019 | Microsoft (Marron) | regularized programming | No loops, no mutation, no `undefined` behavior classes |
| Verse | 2023 | Epic (Peyton Jones, Sweeney) | functional logic | Choice, failure, transactions for games |
| Bend / HVM | 2024 | Higher Order Co (Taelin) | massively parallel functional | Interaction nets on GPU |
| Jax / Triton / TVM / Taichi | 2018–2021 | Google / OpenAI / Apache / MIT | tensor DSLs embedded in Python | How ML actually programs accelerators today |

## 7. Domain-specific and notable niche languages

| Language | Year | Domain | Note |
|---|---|---|---|
| ★ SQL (and dialects PL/SQL, T-SQL, PL/pgSQL) | 1974 | relational data | The universal declarative data language |
| Datalog | 1977 | deductive databases | Prolog subset; back in fashion (Datomic, Soufflé, CozoDB) |
| ★ Regex | 1951/1968 | text | Kleene → Thompson → Perl; a language inside every language |
| make | 1976 | builds | Dependency graph language |
| sed | 1974 | text | Stream editing |
| TeX / LaTeX | 1978 | typesetting | Macro language that typesets the world's papers |
| Emacs Lisp | 1985 | editor scripting | A Lisp that is an operating environment |
| VimScript / Vim9 | 1991 | editor scripting | Replaced by Lua in Neovim |
| ★ GLSL / HLSL / MSL / WGSL / SPIR-V | 1992–2021 | shaders | Programs that run per-pixel on GPUs |
| Erlang's Mnesia / Dialyzer | 1990s | DB / types | Success typing |
| Max/MSP, Pure Data | 1988 | visual dataflow (audio) | Patch cables as programs |
| LabVIEW (G) | 1986 | visual dataflow (instruments) | Engineering graphical programming |
| Simulink | 1984 | block diagrams | Model-based design |
| Modelica | 1997 | equation-based modeling | Acausal physical systems |
| Mathematica, Maple, Maxima, Sage | 1980s–2005 | computer algebra | Symbolic math |
| Stan, BUGS, PyMC, Pyro, Church, Anglican | 1989–2017 | probabilistic programming | Programs that define distributions; inference as a built-in |
| OpenSCAD | 2010 | 3D CAD | Functional geometry |
| G-code | 1963 | CNC machines | Still drives every 3D printer |
| PLC: Ladder, Structured Text, IEC 61131-3 | 1968/1993 | industrial control | Factories run on these |
| Ladder / Grafcet / SFC | 1970s | automation | State machines for machines |
| ABAP | 1983 | SAP | COBOL-ish enterprise 4GL |
| PL/SQL | 1988 | Oracle | Ada-flavored stored procedures |
| Informix 4GL, Progress ABL (OpenEdge), Natural (ADABAS), PowerBuilder PowerScript, Magic | 1980s | business 4GLs | The xBase cousins in corporate IT |
| Salesforce Apex, Google Apps Script | 2007 | cloud platform scripting | Vendor-locked but ubiquitous |
| GDScript | 2014 | Godot game engine | Python-like, engine-integrated |
| UnrealScript → Blueprints | 1998/2014 | games | Visual scripting at scale |
| Lingo (Director), Torque Script | 1990s | multimedia | |
| Inform 7 | 2006 | interactive fiction | Literate, rule-based natural English |
| Twine / Ink / Yarn | 2009–2016 | narrative scripting | |
| JSON / YAML / TOML / XML / Protobuf / CUE / Dhall / Jsonnet / Nickel / KCL / Pkl | 2001–2023 | configuration | The "configuration languages" that keep growing until they are programs |
| Terraform HCL, Bicep, Pulumi, CDK, Ansible YAML, Nix | 2003–2020 | infrastructure as code | Declarative infrastructure |
| ★ Nix | 2003 | package/system config | Pure lazy functional language for building whole operating systems reproducibly |
| Guix (Scheme) | 2012 | package/system config | Nix ideas in Scheme |
| Puppet / Chef (Ruby) / Salt | 2005–2011 | config management | |
| Dockerfile / Compose / Kubernetes YAML / Helm | 2013+ | deployment | Accidental languages |
| Gradle (Groovy/Kotlin), Bazel/Starlark, Buck, Meson, CMake | 2000–2015 | build systems | Starlark = deterministic Python subset |
| BPMN, DMN, Camunda FEEL | 2004+ | business process | Executable process diagrams |
| MDX / DAX / M (Power Query) | 1997–2010 | BI | Spreadsheet-adjacent query languages |
| Excel formulas + LAMBDA | 1985 / 2020 | spreadsheets | Turing-complete since LAMBDA; the most used language on Earth |
| Cypher, Gremlin, SPARQL, GraphQL | 2008–2015 | graph query | |
| XPath / XQuery | 1999 | XML query | |
| PromQL, LogQL, KQL, Splunk SPL, jq | 2012+ | observability/data query | |
| CSS (+ Sass/Less) | 1996 | style | A constraint/cascade language used as a programming language |
| HTML + HTMX, Svelte/Vue/JSX templates | 1993+ | UI | Templates evolve into languages |
| Markdown / reStructuredText / AsciiDoc / Org-mode | 2004 / 2002 / 2002 / 2003 | documents | |
| PlantUML, Mermaid, Graphviz DOT | 1991–2014 | diagrams as code | |
| Alloy, TLA+, PlusCal, Z, B-method, Event-B, Promela/SPIN | 1977–2003 | formal specification | Specifying systems before coding them |
| Isabelle/HOL, HOL4, ACL2, PVS, Mizar, Metamath | 1970s–1990s | theorem provers | |
| Lustre / SCADE, Esterel, SIGNAL | 1980s | synchronous reactive | Avionics; time as a first-class concept |
| Céu, Lingua Franca, Reactors | 2010s | reactive embedded | Deterministic concurrency |
| Arduino (Wiring, C++ subset), MicroPython, CircuitPython, TinyGo, Espruino | 2003–2015 | embedded scripting | |
| Brainfuck, Whitespace, INTERCAL, Malbolge, Piet, Shakespeare, LOLCODE, Befunge | 1972–2003 | esoteric | Teach us what *not* to do, and what minimal computation needs |
| Rockstar, ArnoldC, Chef, Velato | 2010s | esoteric / joke | |

## 8. Lisp family (detail)

Lisp 1.5 (1962) · MacLisp (1966) · Interlisp (1967) · Scheme (1975) · Franz Lisp (1979) · Zetalisp (1980) · Common Lisp (1984) · EuLisp (1990) · ISLISP (1997) · Emacs Lisp (1985) · AutoLISP (1986) · Guile (1993) · Racket/PLT Scheme (1995) · newLISP (1991) · Arc (2008) · Clojure (2007) · ClojureScript (2011) · Hy (2013, Lisp on Python) · Fennel (2016, Lisp on Lua) · Janet (2017) · Carp (2016, Lisp without GC) · LFE (Lisp Flavored Erlang) · Shen · Picolisp · Chicken, Chez, Gambit, Gerbil, Chibi (Scheme implementations).

## 9. xBase / data-centric family (detail)

Vulcan (1978) · dBase II (1979) · dBase III (1984) · dBase III+ (1986) · dBase IV (1988) · Clipper Summer '87 · Clipper 5.x (1990–1997) · FoxBASE (1984) · FoxPro 1.0/2.x (1989–1993) · Visual FoxPro 3–9 (1995–2007) · Quicksilver · Force · dBXL · Arago · FlagShip (xBase on Unix) · Harbour (1999–) · xHarbour (2001) · CA-Visual Objects (1995) · Vulcan.NET · X# (XSharp, 2017, xBase on .NET) · Alaska Xbase++ (1997–) · Recital · Lianja · dBase Plus (dBASE LLC, still sold in 2026).
Related 4GLs: Informix-4GL, Progress ABL, Natural, Magic/uniPaaS, PowerBuilder, Oracle Forms, Clarion, DataFlex, Omnis, 4D, FileMaker scripting, Paradox PAL/ObjectPAL, Access VBA.

## 10. Pascal / Wirth family (detail)

ALGOL W (1966) · Pascal (1970) · UCSD Pascal (1978, p-code VM — the "JVM" of 1978) · Modula (1975) · Modula-2 (1978) · Modula-3 (1988, DEC/Olivetti: GC, exceptions, threads, modules — fed into Java, Python, Go) · Concurrent Pascal (1975, Brinch Hansen) · Turbo Pascal (1983) · Object Pascal (Apple, 1986) · Delphi (1995–) · Oberon (1987) · Oberon-2 (1991) · Component Pascal (1997) · Oberon-07 (2007) · Active Oberon · Zonnon · Free Pascal (1993–) / Lazarus (1999–) · GNU Pascal · PascalABC.NET · Smart Pascal · Oxygene (RemObjects, Pascal on .NET/JVM/Cocoa).

## 11. C family (detail)

BCPL (1967) · B (1969) · C (K&R 1978, ANSI C89, C99, C11, C17, C23) · Concurrent C · Objective-C (1984) · C++ (1985; C++98/03/11/14/17/20/23/26) · C-- (1997, compiler IR) · Cyclone (2001) · Handel-C (hardware) · Cilk (1994, parallel C) · OpenMP C · UPC (Unified Parallel C) · Split-C · C* · CUDA C/C++ (2007) · OpenCL C (2009) · Checked C (Microsoft, 2016) · Ch · Alef (Plan 9) · Limbo (Inferno) · Go (2009) · D (2001) · Vala (2006) · Genie · C2 · C3 · Zig · Hare · Odin · Jai · Carbon · Cppfront (Herb Sutter's C++ syntax 2) · Circle (Sean Baxter's C++ with borrow checking).

## 12. ML / typed functional family (detail)

ISWIM (1966, Landin — "the next 700 programming languages") · ML (1973) · Hope (1980) · Standard ML (1983/1990/1997) · Caml (1985) · Lazy ML · Miranda (1985) · Haskell (1990, 98, 2010) · Caml Light · OCaml (1996) · Clean (1987) · Mercury (1995, logic + functional, typed) · Curry (1995, functional logic) · F# (2005) · Scala (2004) · Idris (2007/Idris 2 2020) · Agda · Coq/Rocq · Lean · Elm (2012) · PureScript (2013) · Reason/ReScript (2016) · Gleam (2016) · Roc (2019) · Koka · Unison · Grain · Flix (effects + Datalog) · Links (web) · Hazel · Dhall (config) · 1ML · MLton, MLkit, Poly/ML, SML/NJ (SML implementations) · GHC, Hugs, UHC (Haskell implementations).

## 13. Concurrent / distributed family (detail)

Concurrent Pascal (1975) · CSP (Hoare, 1978 — the theory) · Occam (1983) · Ada tasks (1980) · Erlang (1986) · Linda (1986, tuple spaces) · Oz (1991) · Alef, Newsqueak, Limbo (Pike, 1980s–1995) · Pict (1995, π-calculus) · JoCaml / Join Java (join calculus) · Cilk (1994) · X10 (2004) · Chapel (2009) · Go (2009) · Akka (Scala actors, 2009) · Orleans (virtual actors) · Elixir (2011) · Pony (2015) · Rust async / Tokio (2019) · Kotlin coroutines (2018) · Swift structured concurrency (2021) · Java virtual threads (Loom, 2023) · Lingua Franca · Verse · Bend/HVM (2024).

## 14. Dynamic / scripting family (detail)

JOSS (1963) · BASIC (1964) · APL (1962) · Logo (1967) · SNOBOL4 (1967) · MUMPS (1966) · Smalltalk (1972) · sh (1977) · awk (1977) · REXX (1979) · Perl (1987) · Tcl (1988) · Python (1991) · Visual Basic (1991) · Lua (1993) · R (1993) · Ruby (1995) · JavaScript (1995) · PHP (1995) · Bash (1989) · Zsh (1990) · fish (2005) · PowerShell (2006) · Nushell (2019) · Groovy (2003) · Io (2002) · Boo (2003) · Falcon · Squirrel (2003) · AngelScript · ChaiScript · Wren (2014) · Janet (2017) · Raku/Perl 6 (2015) · Julia (2012) · Elixir (2011) · Hy, Coconut (Python supersets) · MoonScript, Teal, Luau (Roblox, typed Lua) · CoffeeScript (2009) · LiveScript · TypeScript (2012) · Flow · Dart (2011) · Nim (2008) · Crystal (2014) · Mojo (2023).

## 15. Languages for AI and machine learning (detail)

IPL (1956) · LISP (1958) · Prolog (1972) · Planner (1969) · Conniver · OPS5 (1977, production rules) · CLIPS (1985, NASA expert systems) · Jess · Drools (DRL) · KL-ONE / description logics → OWL (2004) · SOAR · ACT-R · Mercury · Datalog revival · Church (2008), Anglican, Venture, WebPPL, Stan (2012), Pyro (2017), PyMC, Turing.jl, Gen.jl (probabilistic programming) · Lush (LeCun, 1987/2002, Lisp for neural nets) · Torch7 (Lua, 2011) · Theano (Python, 2007) · TensorFlow graph mode (2015) · PyTorch (2016, eager) · Jax (2018) · Swift for TensorFlow (2018–2021, differentiable programming built into the language — cancelled, but the ideas live) · Julia Flux/Zygote (source-to-source autodiff) · Dex (Google, typed array language) · Futhark · Halide · TVM/Relay/TensorIR · MLIR (compiler infrastructure, 2019) · Triton (OpenAI, 2021) · Mojo (2023) · Bend (2024) · ONNX (model interchange, 2017) · Tensor Comprehensions · Taichi · Myia · DiffTaichi · Enzyme (LLVM autodiff) · Tangent · Probabilistic Soft Logic · Markov Logic Networks (Alchemy) · ProbLog · LMQL, Guidance, DSPy, SGLang, BAML (2023–2025: "prompt programming" languages that treat an LLM call as a typed function) · Nengo (neuromorphic/spiking) · Lava (Intel Loihi) · PyNN, Brian2, NEST (spiking neural simulation) · Chisel/SpinalHDL (hardware generators used for AI accelerators).

## 16. Operating-system construction languages (detail)

Assembly (every early OS) · BCPL (early Unix tools) · C (Unix, Linux, BSD, Windows NT kernel) · C++ (Windows userland, Fuchsia parts, BeOS/Haiku, Genode) · Mesa (Xerox Alto/Pilot OS) · Cedar · Lisp (Lisp machines: Genera, Interlisp-D) · Smalltalk (the Smalltalk-80 image *is* an OS) · Oberon (Oberon System — language + OS co-designed) · Modula-2 (Lilith) · Modula-3 (SPIN OS) · Ada (military/avionics kernels, seL4-adjacent) · Forth (Open Firmware, boot ROMs) · Erlang (telecom switches as OS-like systems) · Limbo (Inferno) · Alef (Plan 9 early) · Sing# / Spec# (Singularity OS, Microsoft, 2003 — managed OS) · C# (Cosmos, Midori) · Java (JavaOS, JNode) · Oberon-07 (Project Oberon 2013) · Rust (Redox OS, Theseus, Tock, Hubris; Linux kernel modules since 6.1; Windows kernel parts; Android, Fuchsia) · Zig (experimental kernels) · Nim (Nimkernel) · D (PowerNex, XOmB) · Go (Biscuit, gVisor userspace kernel, TamaGo bare-metal Go) · Haskell (House OS, seL4 prototype) · ATS · Dafny/Rocq-verified components (seL4 is C verified in Isabelle/HOL; CertiKOS in Rocq) · Lisp again: Mezzano (2015, Common Lisp OS) · Verve (verified) · Unison/Nix as "OS as a program" ideas.

---

### Reading the catalog

Three patterns jump out, and they shape Lume:

1. **Every decade re-invents the same five things**: a readable syntax, a safe memory model, a concurrency story, a way to talk to data, and a way to talk to the machine. Languages that got one of them right became immortal (C for the machine, SQL for data, Erlang for concurrency, Python for syntax, Rust for memory). None got all five.
2. **The "data" languages were abandoned, not defeated.** dBase/Clipper/FoxPro made tables, indexes and reports a *language* feature, not a library. Nothing modern replaced that; we glue SQL strings into general-purpose languages instead.
3. **The AI-era languages are all DSLs embedded in Python** (Jax, Triton, PyTorch, DSPy). That is a sign the host language is missing something: differentiable programming, tensors with shapes in the type system, probabilistic semantics and LLM calls as typed functions are not first-class anywhere mainstream.

`01-survey.md` goes language by language: what each got right, what it got wrong, and what Lume takes.
