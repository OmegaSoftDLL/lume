# 17 — F4: unifying the two front-ends into one grammar / AST

This is the plan for **F4** (the last piece of the type/front-end track — **now complete, all 4 slices shipped**). It is written as a
handoff reference: the goal, what already shipped, the one hard constraint that makes the core
risky, and the safe route (surface-to-core desugaring).

## Goal

One **grammar and one AST**. The two surface forms — the layout (indentation) form and the
explicit/inline form — normalise to the **same** Lume-Core AST, so `lume fmt` can rewrite one into
the other and both compile to identical code (`docs/05-vision.md` §"small verified core, big
friendly surface": *"Grammar accepts an explicit-brace form that the formatter normalises; both are
the same AST"*). Lume-Core (`compiler/stage1/lume/*.src`) is the canonical AST; the richer surface
(`.lume`) lowers to it.

## What already shipped (all 4 slices complete)

- **Slice 1 — named enum payloads (#143):** the core parser accepts `Leaf(value: int) | Node(left:
  Tree, right: Tree)` (surface named fields) and normalises the names away to the same `{tag, ar}`
  AST as the positional `Leaf(int) | Node(Tree, Tree)` — byte-identical codegen.
- **Slice 2 — string interpolation (`compiler/stage1/interp.src`):** `"hi {who()}, the answer is
  {answer()}!"` (with `{{`/`}}` escaping) desugars into a left-associative `+` chain with `str(...)`
  on valid expression holes `{<expr>}` (containing no `:`, `"`, `\n`, `{`, `}`), producing the exact
  same AST and byte-identical assembly as explicit concatenation while leaving record/JSON strings
  in `lumec.src` untouched.
- **Slice 3 — `elif` chains (#152):** `if c then t elif c2 then t2 ... else e` desugars to the core's
  nested `If` AST (`elif X` == `else if X`); the plain `else` path is byte-identical.
- **Slice 4 — unified layout form, explicit-brace form (`docs/05-vision.md:224`), and `lume fmt`
  normalisation (`f4_layout.src`, `f4_brace.src`, `f4_core.src`):** both `lumec` and `lfmt.src`
  accept indented layout declarations/blocks (`type enum`, `type record`, `fn`, newline `let`,
  `do`, `if`/`elif`/`else`) and explicit-brace declarations/blocks (`{ ... }`, `;`-separated `let`,
  `match { ... }`), lowering all three forms to the exact same AST, emitting byte-identical x86-64
  assembly, and normalising both surface forms into `f4_core.src` via `lume fmt`.

Both worked because they add surface syntax that **`lumec.src` itself does not use**, so when the
compiler compiles its own source, neither the self-hosted parser nor the frozen oracle parser ever
meets the new syntax — no divergence.

## The hard constraint (read before touching the core parser/lexer)

The self-host suite has a gate — **`self-host asm: lasm ELF == lume0 asm oracle byte-for-byte`**
(`bootstrap/lume0/tests/run.sh`, ~lines 237–264) — that compiles `lumec.src` through BOTH paths:

1. the self-hosted compiler (`compiler/stage1/lume/*.src`), and
2. the frozen ML-core oracle (`compiler/stage1/data_parser.lume` + `data_asmgen.lume`, run by `lume0`),

and asserts the two outputs are byte-identical. Therefore **any change to the core lexer/parser that
the oracle does not mirror diverges by construction** and breaks the build. String interpolation was
attempted directly in the core parser (PR #145) and was correctly rejected by this gate, because
`lumec.src`'s own string literals `"{"` / `"}"` would be reinterpreted.

Rule of thumb:

- A surface syntax `lumec.src` **never uses** → safe to add to the core parser additively (slices 1, 3).
- A feature that changes how `lumec.src` **itself** is parsed (its layout, its string literals, …)
  → must be mirrored in `data_parser.lume` in the **same PR**, or it must not touch the core at all.

## The safe route: surface-to-core desugaring

Do the heavy unification as a **desugaring layer** that converts the surface to the core's inline
form **before** the core parser, leaving the fixpoint-critical core parser and the oracle untouched:

1. **Layout / indentation.** The surface uses indentation for blocks; Lume-Core is layout-free and
   expression-based. Desugar the indented surface into the core's inline form (or emit
   INDENT/DEDENT in a **surface** layer, never in the core lexer). The core and the oracle do not
   change, so the fixpoint holds.
2. **String interpolation `"{v}"`.** A surface feature — desugar `"a{e}b"` to `"a" + str(e) + "b"`
   in the surface layer (the surface already has `{{` / `}}` escaping; see `AGENTS.md` pitfalls).
   Do **not** put it in the core parser (which relies on literal `"{"` / `"}"`).
3. **One shared parser.** End state: a single parser that produces the core AST for both forms. If a
   step genuinely needs a core change, mirror it in `data_parser.lume` in the same PR so the oracle
   byte-equivalence gate stays green.

## Acceptance

Each slice is one green PR with a regression test, and all three gates stay green: `make bootstrap`
(stage 2 == stage 3), `make zero-c`, and the oracle byte-for-byte test. `lume fmt` should normalise
the two surface forms to one, which is the operational proof of "one AST". F4 is complete when the
layout form and the inline form of a non-trivial program produce byte-identical codegen and `lume
fmt` converts between them.

## Note (historical)

At the time this plan was written, F4's remaining heavy work (layout, one shared parser) and the F3c Hindley–Milner engine
(`feat/f3c-hm-core`, see the PR #151 close note) are the compiler-surgery pieces that need a machine
able to run the self-hosted toolchain for debugging (the ~117 KB compiler; a 4 GB host OOMs).
