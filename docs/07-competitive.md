# 07 — Competitive scan and the improvements Lume adopts

Date of scan: 2026-10-24. This distils a survey of comparable/newer projects on GitHub and records
the concrete ideas Lume adopts, with sources. "CONFIRMED/UNVERIFIED" tags carry over from the scan.

## Closest and most instructive projects

| Project | What it is | The idea Lume takes |
|---|---|---|
| **Bend 2** — github.com/HigherOrderCO/Bend (24.1k★, CONFIRMED) | GPU-parallel language that "blocks AI mistakes via proof", dependent types, targets C/CUDA/Metal/JS | A project-level **`LAWS` file** of compiler-enforced invariants, proof-obligated on every change |
| **NanoLang** — github.com/jordanhubbard/nanolang (630★, CONFIRMED) | Language "designed to be targeted by coding LLMs"; self-hosting; transpiles to C; verified bytecode (NanoISA); Coq soundness; mandatory `shadow` test block per function; LSP+DAP+playground | **Mandatory test block per `pub` function**, enforced by the compiler; a **verifier-checked bytecode** |
| **BAML** — github.com/boundaryml/baml (9.4k★, CONFIRMED) | DSL for typed LLM functions; built-in tests + eval framework; schema-aligned parsing; colourless concurrency | **Eval/mocking framework for `extern llm fn`** + **schema-aligned parsing fallback** for providers without constrained decoding |
| **Mojo** — github.com/modular/modular (~30k★, CONFIRMED; compiler open since Aug 2026) | Python-like, CPU+GPU from one source, ownership, in-tree kernel library `max/kernels` | **Ship a real kernel library with the language** as the `kernel` benchmark |
| **Koka** — github.com/koka-lang/koka (4.1k★, v3.2.9 2026-09-17, CONFIRMED) | Typed effect handlers (async/exceptions/probabilistic as libraries); Perceus reference counting, GC-free C backend | Implement **`sample`/`observe`/`infer` and `async` as effect handlers**, not special cases; **Perceus-style reuse analysis** |
| **Unison** — github.com/unisonweb/unison (6.8k★, CONFIRMED) | Definitions identified by AST hash; cached tests, rename-safe edits, abilities (effects) | **Content-address definitions (per-AST-hash), not only packages** → cached tests and proofs, rename-safe AI edits |
| **Austral** — github.com/austral/austral (1.6k★, CONFIRMED) | Linear types + capability security; dependencies restricted in which resources they touch; spec-first | **Capabilities in package metadata**, checked by the package store; keep a small written spec |
| **Futhark** — github.com/diku-dk/futhark (2.8k★, CONFIRMED) | Pure data-parallel language, guaranteed fusion, GPU+CPU | A **parallel-by-construction, fusion-guaranteed array core** as the `kernel` base (tractable under MLIR) |
| **Taichi** — github.com/taichi-dev/taichi (28.4k★, CONFIRMED) | One source → CUDA/Vulkan/OpenGL/Metal/WASM | **Target Vulkan+Metal (not just CUDA) from the start**, with a WASM fallback |
| **Carbon** — github.com/carbon-language/carbon-lang (33.9k★, CONFIRMED, "experimental") | C++ successor, interop-first migration | **C ABI + C-header importer on day one**; also a cautionary tale (high stars, no usable release) |
| **Zig** — github.com/ziglang/zig (43.3k★, CONFIRMED; moved to Codeberg) | Language-native scriptable build system; `zig cc` cross-compiling C toolchain | Build system as a first-class language-native tool; compiler as a **cross-compiling C toolchain** |
| **Hylo** — github.com/hylo-lang/hylo (1.6k★, CONFIRMED; rewriting as hylo-new) | Mutable value semantics as a simpler alternative to borrow-checker lifetimes | Keep evaluating **mutable value semantics** to lower the ownership learning curve |
| **Redox OS** — github.com/redox-os/redox (16.7k★, CONFIRMED, active) | Microkernel OS in Rust | Reference point for `lume-os`'s capability model (not yet analysed) |

Also noted: **MoonBit** (human-AI collaboration language; only docs repo found, 2.5k★), **SPELL** (AI-native
dataflow language with JSON source, Zenodo 2025), **DSPy** (38.8k★, not a language but proves demand for
"program, don't prompt" with prompt optimisers).

## Gaps the scan found in Lume's plan — now addressed

1. **Project-level laws** the compiler enforces on every change (Bend). → principle **P34**, roadmap Stage 2.
2. **Mandatory tests per `pub` function** (NanoLang). → principle **P35**, roadmap Stage 1.
3. **LLM eval/mocking + schema-aligned parsing fallback** (BAML). → spec §14 note, roadmap Stage 4.
4. **LSP + DAP + C-header import** shipped with the first compiler (NanoLang, Carbon, Zig, Mojo). → roadmap Stage 1 (`lume lsp`, `lume dap`, `import c`).
5. **Effect handlers for `sample`/`observe`/async** (Koka) and **content-addressed definitions** (Unison). → roadmap Stage 3 (handlers) and Stage 2 (content-addressed store already planned; extend to per-definition hashing).
6. **A machine-checked core** — already planned (Lean, Stage 5); the scan reinforces doing a small one early.
7. **Cross-platform incl. Windows** — add to the target matrix explicitly (Stage 1/5).
8. **LLM-generation eval as a CI gate** (rjreeves/lume): measure tokens/attempts for a model to
   emit Lume that compiles *and* proves, as a regression gate on surface/syntax changes —
   extends the warning below (a clean grammar is not enough). → tracked in #230.

## Validation from the scan

- AI-native language design is now a crowded, active category (NanoLang, Bend 2, SPELL, NERD, MoonBit, all Dec 2025–Oct 2026). Bend 2's pitch "blocks AI mistakes via proof" is Lume's pillar 1. Lume's bet is timely, not unique — differentiation must be the *whole-stack* coherence (data + AI + OS in one self-evolving language), not any single feature.
- Typed LLM functions validated by BAML (9.4k★). Effects + GC-free C backend proven by Koka. Tiny bootstrap + self-hosting proven by NanoLang. Content-addressed code + effects shipping for years (Unison). GPU-from-one-source has a real market (Mojo/Taichi/Bend).

## Warnings the scan surfaced

- **Scope kills languages.** Carbon (33.9k★) and Roc (6.2k★) are still not usable after years; Hylo restarted its compiler. Lume's scope is larger than all of them — hence the staged roadmap and the rule that `model`/`kernel`/`llm` are macro libraries over a small core (P23), able to lag without blocking the core language.
- **"Designed for LLMs" is not enough.** Simon Willison's one-shot NanoLang test produced code that did not compile (simonwillison.net/2026/Jan/19/nanolang/). A model has not seen Lume, so the AI-author loop needs docs-in-context, an example corpus, and the JSON-diagnostic repair loop — not just a clean grammar. → reinforces the `lume agent` priority in Stage 1.
- **Stars precede openness/usability.** Mojo was closed for years; many high-star languages have no release. Lume should keep shipping runnable increments (as it is doing) rather than marketing ahead of substance.
- **Scope your security claims.** NanoLang is candid that its sandbox is not a security boundary. Lume's anti-reverse-engineering (P33) and capability-OS claims must stay precisely scoped.

## Name-homonym scan on GitHub (2026-10-27)

A scan of GitHub projects that share our name ("Lume"/"Lumi"/"Lumen"/"Luma") found **no feature Lume
lacks**: on the language-design axis Lume is ahead of every homonym (none has a sound decision
procedure, self-hosting with native-ELF emission, or provenance tracking). Kept here as naming and
competitive intelligence.

| Project | What it is | Verdict for Lume |
|---|---|---|
| **lumeland/lume** — github.com/lumeland/lume (2.3k★, active) | Static-site generator for Deno; executable `_config.js`; ordered `process`/`preprocess` passes by extension | Not a language. **Dominates search for "lume"/"lume lang"** — the main discoverability risk. Passes/config-as-code are generic; nothing to take. |
| **lume/lume** — github.com/lume/lume (1.5k★) | 3D/AR/VR toolkit; reactive web-component "behaviors" | Different domain (web UI). Nothing transfers. |
| **Lumen** — github.com/alliecatowo/lumen (0★, active, Rust) | Self-styled "AI-native language" for LLM agents; effects in signatures, `@deterministic`, `.lm.md` markdown source | Same slogan as us. Effects-in-signature is a **subset of our effect rows (`[E0200]`)**; `@deterministic` is **subsumed** (our `/ pure` already forbids clock/rand/uuid). Uses `.lm`. |
| **rjreeves/lume** — github.com/rjreeves/lume (0★) | Small statically-checked script language "optimised for AI generation"; `ai-eval.ps1`, `lume-api.json`, versioned system prompt | Exact homonym, same niche. **One genuinely new idea** → the LLM-generation eval gate (gap §8 above). |
| **Lumi** — github.com/meircif/lumi-lang (16★) | Compiles via C; tiered references; per-type integer-overflow policy; built-in tests/mocking; `.lm` | Overflow-per-type **already covered** by our `+%`/`+|`/`@overflow`; tiered refs weaker than our linear ownership (`[O0001]`). |
| **Luma** — github.com/Luma-Programming-Language/Luma (89★, self-hosted→C) | `#takes_ownership`/`#returns_ownership` annotations instead of lifetimes | Our linear ownership is stronger; annotation-ownership is a lighter subset. Nothing to take. |
| **Lumina** — github.com/luminalang/lumina (126★, Rust) | Functional; reflection via const-folding after monomorphisation (a macro alternative) | Same idea as our **comptime**; already have it. |
| Others — Lumen/Firefly (BEAM→native, **archived** 3.6k★), laravel/lumen (PHP, **archived**), rxi/lume (Lua lib), AAsteria/Lumi (λ-calculus) | — | Domain-irrelevant or dead; only naming noise. |

No Spanish-language "Lumi/Lumen" project was found. Lume uses the **`.lume`** source extension, so there
is **no extension collision** with the `.lm` homonyms.

### Positioning (decision 2026-10-27)
"AI-first" alone is contested: two 0★ homonyms (`alliecatowo/lumen`, `rjreeves/lume`) use the same
slogan, and `lumeland/lume` owns the search term. Lume therefore leads with what is **unique and
verifiable** — a **sound `QF_LIA` proof layer**, **self-hosting with zero C and native-ELF emission**,
and **provenance tracking** — not the slogan. README, GitHub topics and taglines state these first.

Sources: the GitHub repos above, modular.com/blog/mojo-open-source, simonwillison.net/2026/Jan/19/nanolang/
and /2026/Aug/18/mojo-is-now-open-source/, zenodo.org/records/17826541.
