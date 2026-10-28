# 32 — Build real software in Lume: an onboarding guide for any AI

**Purpose.** A self-contained guide you can hand to any AI model so it can install Lume and build
**real, verified** software in it. Lume is AI-first: a model is the primary author. This complements
the full [`docs/30-lume-book.md`](30-lume-book.md) (The Lume Book) and the AI-first rationale in
[`docs/08-ai-first.md`](08-ai-first.md). Everything below is **run-verified** per `docs/28` §7 — if a
capability isn't listed as working, it isn't ready yet (see the honest boundary at the end).

## 1. Install (no C compiler needed)

**From a release / local checkout — the self-hosted toolchain:**
```sh
sh scripts/install.sh --prefix "$HOME/.lume"      # writes MANIFEST.sha256 over every binary + module
export PATH="$HOME/.lume/bin:$PATH"
lume self verify                                  # {"ok":true,...,"manifest_verified":true}
```

**From source (staged self-host, the GCC/Rust model):**
```sh
make bootstrap    # C seed mints lumec+lasm once, then Lume rebuilds the toolchain; asserts stage2 == stage3
make test         # conformance suite   (make asan for the sanitized build)
```
The C seed under `bootstrap/lume0/` is frozen and only mints the first tools once; the steady-state
build uses **no C compiler and no external `as`/`ld`** (proved by `make zero-c`).

## 2. The toolchain (one CLI)
```
lume build <f.src> -o <out>   # compile to a native ELF (x86-64; --target aarch64-linux-elf | riscv64-linux-elf | lumb | …)
lume run   <f.src>            # compile + run
lume check <f.src>            # lex + parse + name/type/ownership/effect checks (fast edit→check loop)
lume prove --json <f.src>     # discharge requires/ensures/law obligations via the sound QF_LIA solver
lume test  <f.src>            # run fn test_*()        lume fmt / lume doc
lume describe --json          # machine-readable language surface — feed this to the model first
lume agent grammar <f.src>    # constrained-decoding grammar (lume-constrained-grammar-v1)
lume agent check  <f.src>     # structured JSON diagnostics with fix-its (the edit→check→fix loop)
lume agent repair|witness|impact <f.src>   # oracle: counterexample-guided repair, witnesses, blast radius
```

## 3. How an AI authors Lume — the loop

Lume optimises for an unambiguous **edit → check → prove → fix** loop, not human typing comfort.

1. **Teach the model the surface:** run `lume describe --json` and `lume agent grammar <file>` and put
   their output in context. The vetted system prompt is `tests/llm_gen_eval/system_prompt.md`
   (`lume-gen-prompt-v1`).
2. **Write the spec, then the body.** Every non-trivial function states its contract first:
   ```lume
   fn clamp_non_neg(x: int) -> int requires x >= 0 ensures result >= 0 = if x > 0 then x else 0
   ```
3. **Check:** `lume agent check f.src` → JSON diagnostics with error codes (`[O0001]` moved value,
   `[E0200]` unhandled effect, `[V0001]` refuted contract, `[T….]` type, `[A0003]` unverified
   provenance) and concrete fix-its. Apply the fix; repeat.
4. **Prove:** `lume prove --json f.src`. A `"status":"proven"` obligation elides its runtime guard; a
   `"status":"refuted"` returns a **counterexample** — inspect `.proofs[].counterexample` or call
   `lume agent repair` and strengthen `requires` or fix the body. "Compiles" is not the bar; **compiles
   AND proves** is.
5. **Build/run/test** on a real target and keep it green.

**Core rules (from `lume-gen-prompt-v1`):** top-level declarations are `fn`/`type`/`law`/`effect`/`print`
(never top-level `let`); specified functions write `requires`/`ensures` (with `result`) before `= body`;
`match` sits at the top of a function body; on `[V0001]`, use the counterexample to repair.

## 4. What you can build **today** (real, run-verified)
- Native apps on **x86-64, AArch64, RISC-V** (ARM64/RISC-V under QEMU) and a portable **LUMB** bytecode VM.
- The full language: records/enums/`match`, `comptime`+macros, exact `decimal`/`table`/`query`, errors-as-values,
  `T?` optionals, **traits/impl, closures, refinement types, `async`/`.await`, `shard`**, generics, HM inference.
- **Memory & safety:** linear ownership + `rc[T]`/`arc[T]`/`weak[T]` tiers, effect rows + handlers,
  `requires`/`ensures`/`invariant`/`law` with **sound QF_LIA proofs**.
- **Interop:** link and call C (`.so`/`.a`), export C-ABI/WASM; capability-scoped effects.
- **Systems:** a `@freestanding` x86-64 microkernel that **boots on QEMU** (paging, IDT, timer IRQ, serial, SMP).
- **Data/tooling:** durable on-disk `persist` (WAL), LSP/DAP, REPL/Jupyter; `lume evolve` self-evolution.

## 5. Honest boundary (what is NOT ready — `docs/28` §7)
Don't instruct a model to rely on these yet; they're open issues, not shipped features:
- **GPU `kernel fn`** — codegen is a placeholder (no real body lowering / GPU execution): `#250`.
- **Temporal-logic proof** (LTL/CTL) — `@mode compose` does cross-module *relational* QF_LIA laws, not temporal: `#198`.
- **A general-purpose OS** (filesystems, network, userland) on top of the microkernel — delivered incrementally: `#241`.
- Native execution on **Apple Silicon / Windows / bare-metal hardware** is run-verified in CI via emulators;
  physical-hardware bring-up beyond that is not claimed.

If you need one of these, treat it as a feature to build (and prove), not one to call.
