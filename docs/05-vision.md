# 05 — Vision: a language for the AI era that can build anything, from a kernel to a mind

*This is the opinionated document. `01-survey.md` says what history taught; this one says what to
build and why, including the bets that could be wrong.*

---

## 0. The thesis in one paragraph

Every mainstream language was designed for a world where **humans write all the code** and **the
computer only executes it**. Both assumptions are now false. Most new code will be written *with*
or *by* AI models; most interesting programs will *contain* models (neural networks, LLM calls,
probabilistic reasoning); and the hardware those programs run on is no longer "a CPU" but a zoo of
CPUs, GPUs, NPUs and FPGAs. A language designed today should take all three as givens. Nobody has
done that yet — Mojo took the hardware part, DSPy/BAML took the LLM part, Rust took safety — and
all of them sit inside or beside Python. Lume's bet is that **one coherent language** can cover the
whole stack: the boot sector, the kernel, the database, the business app, the training loop, the
inference kernel and the agent that orchestrates them — and that being *designed for AI authorship*
is the thing that makes the whole-stack ambition feasible for the first time.

## 1. Four audiences, one language

1. **The human programmer** — needs readability, a small spec, great errors, fast feedback.
2. **The AI programmer** (an LLM writing or editing Lume) — needs *unambiguous* syntax, *local* reasoning (you can understand a function without reading the whole program), machine-checkable intent (types, contracts, effects, tests) so its mistakes are caught by the compiler not the user, and a structured way to receive feedback (diagnostics as data).
3. **The machine** — needs predictable layout, zero-cost abstraction, control over memory and time, and a path to every target (CPU, GPU, bare metal, WASM).
4. **The model inside the program** — needs tensors, gradients, probability, and an integration with the type system so a model is a *typed component*, not a blob.

Every design decision below is scored against all four. That is the innovation: not any single
feature, but refusing to sacrifice one audience for another.

## 2. Ten things no mainstream language does, that Lume does

### 2.1 Designed for AI authorship (the "AI-native" part that is *not* about tensors)

Most "AI languages" mean "languages for writing AI". The first and bigger opportunity is a language
**written by AI, verified by the compiler, reviewed by the human**. Concretely:

- **Zero ambiguity grammar.** No context-sensitive parsing (no C++ `a * b;` dilemmas, no Python
  `print x` legacy, no JS ASI). One canonical formatting. An LLM that emits Lume either parses or
  gets an exact error location.
- **Intent is checkable.** Types, `requires/ensures`, effects, exhaustive `match`, no `null`, no UB:
  the categories of mistakes LLMs make most (off-by-one, wrong null handling, forgetting a case,
  calling I/O where it shouldn't) become compile errors. The compiler is the reviewer that never
  tires.
- **Diagnostics are structured data.** `lume build --json` yields machine-readable errors with
  spans, expected/actual types, candidate fixes. The *agent loop* (write → compile → read errors →
  fix) is a first-class workflow, with a `lume agent` mode exposing the compiler as a service
  (parse, typecheck, query symbols, apply fix-its) over a protocol — LSP for machines.
- **Typed holes.** `?` or `todo "reason"` leave a hole; the program still typechecks and runs
  (the hole panics or asks). An LLM can be asked to fill *exactly* that hole with the compiler
  providing the expected type, in-scope values and the surrounding contracts. Program synthesis
  becomes hole-filling under constraints (Hazel + Idris + Copilot).
- **Local reasoning.** Effects and ownership make a function's signature tell the whole truth:
  what it reads, writes, allocates, may fail with, may call. No hidden globals, no action at a
  distance. This matters doubly for an AI reader with a limited context window.
- **Semantic, not textual, versioning and diffs.** The toolchain diffs ASTs and APIs (Elm/Unison);
  review shows "function `foo` gained effect `net`" rather than 40 changed lines.
- **Specification first.** A `spec` block (contracts + examples + property tests) can exist before
  the body. `lume synth` can ask a model to propose a body that satisfies the spec, and the test
  runner + prover check it. The human writes *what*, the AI proposes *how*, the compiler judges.

### 2.2 Models are typed values

```lume
type Classifier = model
    params:  Params           # a tree of tensors; serialisable; versioned
    forward: fn(x: tensor[f32, batch, 3, 224, 224]) -> tensor[f32, batch, 1000]
```

- A `model` is a first-class type: it has typed parameters, a typed forward function, a schema, an
  ONNX/safetensors import/export, and a *version hash*. You pass it to functions, store it in a
  `table`, test it, ship it in a binary.
- `grad`, `jvp`, `vjp`, `vmap`, `jit`, `shard` are **operators on functions**, implemented as IR
  transformations by the compiler (Jax's design, Enzyme/Zygote's implementation), not a library
  with a tape. They work on *any pure Lume function with tensor types*, including loops and
  branches.
- **Shapes are types.** `tensor[f32, n, 3, h, w]` with symbolic dimensions; mismatches are compile
  errors; `reshape`/`einsum` are typed; broadcasting rules are in the type checker (Dex). The #1
  bug class in ML code disappears.
- Training is ordinary code: `for batch in data: params -= lr * grad(loss)(params, batch)`. No
  framework; the compiler lowers the gradient to CPU/GPU kernels through MLIR.

### 2.3 Accelerators are a target, not a foreign land

```lume
kernel fn saxpy(a: f32, x: tensor[f32, n], y: mut tensor[f32, n])
    for i in par 0..n          # data-parallel; compiler maps to threads/warps/lanes
        y[i] = a * x[i] + y[i]
```

- `kernel fn` compiles to CPU SIMD, CUDA/ROCm/Metal/Vulkan compute, or WASM-SIMD from one source.
- **Algorithm/schedule split** (Halide): `@schedule(tile=(32,32), vectorize=8, unroll=4)` or a
  separate `schedule for saxpy` block. `@autotune` searches schedules at build time.
- Memory spaces are in the type (`tensor[f32, n] @gpu`), movement is explicit (`to(@gpu)`), so the
  compiler proves there are no accidental host↔device copies.
- The same mechanism, through MLIR's HLS dialects, targets FPGA — far future, but the architecture
  allows it.

### 2.4 Probability and knowledge are language constructs

```lume
fn coin(bias_prior: Beta) / {prob}
    let p = sample bias_prior
    observe flips ~ Bernoulli(p)
    return p

let posterior = infer coin(Beta(1, 1)) with mcmc(steps = 10_000)
```

- `sample`, `observe`, `infer` under a `prob` effect: generative models are ordinary functions;
  inference algorithms are *handlers* of the effect (Church/Gen/Pyro, made first-class by the
  effect system).
- `rule` blocks compiled to a Rete/Datalog engine over `table`s: business rules, authorisation,
  program analysis, symbolic reasoning next to neural components (**neuro-symbolic by
  construction**: a rule can call a model; a model's output can be a fact).

### 2.5 LLM calls are typed functions with contracts

```lume
extern llm fn extract_invoice(text: str) -> Invoice / {net, llm}
    """Extract the invoice fields. Dates ISO-8601. Amounts as decimal."""
    ensures result.total == result.items.sum(.amount)
```

- The return type drives **constrained decoding** (the model *cannot* return invalid JSON or a
  wrong shape); the contract is checked; failure retries with the violation fed back; the whole
  thing is a normal `Result`.
- Prompts are versioned, tested (`test llm extract_invoice with examples/*.txt expect …`) and
  *optimised* by the toolchain (DSPy's compile-the-prompt idea) against the eval set.
- Model choice, caching, cost budget and fallbacks are effect-handler configuration, not code.
- An **agent** is then just a program: typed tools (functions), typed memory (`table`s), typed
  planning (`llm fn`s), supervision (actors), and `lume test` evaluating it end to end.

### 2.6 Data is native, persistent and typed

The dBase/Clipper/MUMPS inheritance, modernised:

- `record`, `table[T]`, `index`, `relation`, `query` in the core.
- **Persistence as a language feature**: `persist table[Customer] customers at "data/customers"`
  gives a typed, ACID, crash-safe, concurrent store (an embedded engine in `std.data`; adapters for
  PostgreSQL/SQLite/DuckDB map the *same* `query` syntax to their planners).
- `decimal`, `date`, `time`, `duration`, `money[BRL]` as proper types with units.
- Columnar in-memory layout by default (`@soa`) so `query` over a million records is vectorised —
  the table *is* the DataFrame, typed.
- Time-travel: tables can be `versioned` (append-only log + snapshots) — the model-training data
  lineage problem and the audit problem solved by the same primitive.

### 2.7 Effects: one mechanism for safety, async, capability security and purity

`fn f() -> T / {io, alloc, net, async, prob, gpu, unsafe, interrupt, realtime, llm}` — inferred,
shown at `pub` boundaries, enforced:

- **Capability security**: a package or function without `net` cannot open a socket. The build
  file grants capabilities per dependency (Deno/Austral). Supply-chain attacks become type errors.
- **Purity**: no effects ⇒ memoisable, parallelisable, `comptime`-evaluable, differentiable,
  provable.
- **Async without colouring**: `async` is an effect handled by the runtime; sync code calls async
  code; no `.await` noise, no two copies of every library (Koka/Effekt/OCaml 5).
- **Kernel correctness**: `interrupt` forbids blocking and allocation; `realtime` forbids unbounded
  loops and allocation and lets `lume prove` compute worst-case execution time for the subset.

### 2.8 Memory: safe by default, three honest tiers, no religion

1. **Owned values + inferred borrows** (Rust safety, Hylo ergonomics): the default; zero cost.
2. **Shared**: `rc[T]`/`arc[T]`/`cell[T]` with literal syntax and good diagnostics; cycles detected
   statically where possible, `weak` otherwise.
3. **`@gc` regions**: a tracing collector *for a region/type* (not the whole program) for the
   graph-shaped problems where ownership is the wrong tool (compilers, editors, GUI trees,
   symbolic AI). Kernels and kernels' users never pay for it because it does not exist unless
   declared.

Plus explicit allocators (Zig) everywhere in `std`, so the *same* `std.collections` works in a
kernel with a bump allocator and in a server with a general-purpose one.

### 2.9 From boot sector to browser with one toolchain

- `lume build --target x86_64-freestanding` → a kernel. `--target riscv32-none` → a microcontroller
  firmware. `--target wasm32` → a browser or edge function. `--target cuda` → a kernel for the GPU.
  `--target lumb` → portable bytecode for plugins/REPL. Cross-compiling from any host (Zig).
- **`lume-os`** as the long-term proof and testbed: a capability-based microkernel written in Lume
  where drivers are Lume `platform`s, user programs are Lume `app`s parametrised by the capabilities
  they receive (Roc's split), processes are software-isolated because the compiler proved them safe
  (Singularity's bet, without GC), and the shell is the Lume REPL with tables for the filesystem
  and process list (`query processes where cpu > 50%`). Models run as first-class services
  (`std.os.model` scheduler aware of GPUs).
- **Neural systems** are the second testbed: a training framework (`std.nn`), an inference server,
  and a spiking/neuromorphic simulation library (`std.neuro`: neurons as actors with logical time,
  Lingua Franca style) — demonstrating that the same language expresses a backprop network and a
  biologically-inspired one.

### 2.10 Small verified core, big friendly surface

- **`lume-core`**: ~30 constructs, a formal semantics (mechanised in Lean), the thing the verifier,
  the interpreter, the optimiser and the self-hosted compiler all agree on.
- Everything else — `query`, `model`, `kernel`, `rule`, `llm fn`, string interpolation,
  comprehensions, UFCS — is **sugar defined by macros over the core**, in the standard library,
  readable by users. The language can grow without the core growing (Scheme/Racket's lesson).
- A **size budget** in the spec: the surface language has a maximum keyword count and a maximum
  spec length; adding a feature requires removing or demoting another.

## 3. What would make this "fantastic" rather than merely "complete"

Features are table stakes. These are the experience goals that decide whether people *love* it:

1. **Flow**: edit → result in under 200 ms for the debug tier, always. Turbo Pascal in 1983 did it;
   we have no excuse.
2. **The compiler is a teacher**: every error explains itself, names the concept, shows a fix, and
   links to the one page of the spec that matters. Measured: a new user should fix 90% of errors
   without a search engine.
3. **One tool, zero yak-shaving**: `lume new`, `lume run`, `lume test`, `lume fmt`, `lume doc`,
   `lume prove`, `lume bench`, `lume pkg`, `lume agent`. No Makefiles, no CMake, no virtualenvs, no
   webpack, no Dockerfiles for dev. Reproducible by default (content-addressed store).
4. **Progressive depth**: a 12-year-old writes a script on day one; the same file, with types and
   contracts added, becomes a verified kernel module. No cliffs, no "now rewrite it in C++".
5. **Trust**: no undefined behaviour, no `null`, no data races, no supply-chain surprises, no
   silent float money. A Lume program that compiles has a *meaning*.
6. **Longevity**: editions, a stability promise, and a specification so that Lume code from 2026
   compiles in 2056 — the COBOL/Fortran/Java virtue that fashionable languages forget.

## 4. The bets that could be wrong (and the fallback for each)

| Bet | Risk | Fallback |
|---|---|---|
| Indentation syntax for a systems language | Macro-heavy or generated code may prefer braces; copy-paste hazards | Grammar accepts an explicit-brace form that the formatter normalises; both are the same AST |
| Region inference removes most lifetime annotations | Inference may be slow or produce confusing errors on hard cases | Fall back to Rust-style explicit annotations in those functions only; errors suggest them |
| Effects with inference stay ergonomic | Effect rows can get noisy in higher-order code | Effect aliases (`/ {io}` expands to a named set), inference hides them unless `pub` |
| MLIR as the main backend | Heavy dependency, slow builds | Cranelift tier for debug/REPL; LLVM-only build option; MLIR only for `kernel`/tensor paths at first |
| `@gc` regions won't fragment the ecosystem | Two memory models in one language has hurt D and Nim | Regions are per-type and never cross a `pub` API without being visible in the type; `std` is GC-free |
| LLM calls as language features age badly | Model APIs change monthly | The core only knows "an `extern` function with schema-driven decoding and an effect"; providers are stdlib handlers, replaceable |
| The whole-stack ambition is too big | Scope creep kills languages | Phases (see roadmap): the core language + tooling must be excellent *alone* before `model`/`kernel`/`llm` ship; each is a macro library over the core, so they can lag without blocking |

## 5. Name

"Lume" — light in Portuguese/Galician; short; `.lume` files; `lumec` compiler; `lume` tool. If a
collision appears (there is a small JS library and a LaTeX package with the name), alternatives
with the same feel: **Lúmen**, **Faro** (lighthouse), **Ígnis**, **Aurea**, **Omega** (the author's
brand). Decide before the first public release, not before.

## 6. Where to start (the first 90 days)

1. Freeze **`lume-core`** (the ~30-construct typed IR) and write its interpreter. Everything else is
   sugar, so the core is where correctness is cheap to get right.
2. Parser + formatter for the surface syntax, with the JSON diagnostics protocol from day one —
   the AI-authorship workflow is tested on the compiler itself (an LLM writing Lume, the compiler
   correcting it) before any human ergonomics are polished.
3. Type checker with inference, ADTs, traits, effects (inference only; no handlers yet).
4. Ownership/borrow checker on the core IR.
5. Cranelift backend → native binaries. At this point Lume replaces Go/Python scripts.
6. Then, in order of leverage: `table`/`query` (the data story is Lume's most *visible*
   differentiator), `comptime` + macros (unlocks everything else as libraries), tensors/`grad`
   (the AI story), `freestanding` + a toy kernel (the OS story).

The compiler is written in Rust until self-hosting (phase 5), because Rust's ecosystem (Cranelift,
MLIR bindings, LSP crates) shortens the road, and because a Rust compiler for Lume is itself the
best stress test of "what Rust got wrong that Lume must fix".
