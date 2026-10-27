# 08 — Lume is authored by AI, reviewed by humans

This document is the project's center of gravity, stated plainly so no later decision drifts from it:

> **Lume's primary author is an AI model. Its primary human role is reviewer and director, not
> typist.** Every design choice is made for the machine that writes the code first, and for the
> human who reads, audits and steers it second.

This is not a slogan; it inverts defaults that every prior language took for granted. Those languages
assumed a human types the code and the machine only runs it. Both assumptions are now false for Lume.

## What changes when the author is a model, not a person

Humans optimise a language for *writing*: terseness, few keystrokes, implicit behaviour, clever
shortcuts. A model does not tire, does not mind redundancy, and fails in different ways: it invents
plausible-but-wrong details, forgets a case, mishandles absence, calls an effect it shouldn't. So
Lume optimises for the opposite of human convenience:

1. **Checkability over brevity.** Redundancy a human would resent — explicit effects, types at every
   public boundary, exhaustive `match`, mandatory contracts and tests — is a *feature*, because it
   gives the compiler more of the author's intent to verify. The compiler is the reviewer that never
   gets tired; the more it can check, the fewer of the model's mistakes reach the human.
2. **Unambiguity over expressiveness.** One canonical form, one formatting, no two ways to do a thing.
   A model's output must be predictable and its diffs semantic. Stylistic freedom is a bug here.
3. **The interface is the diagnostic loop, not the text editor.** The model proposes a change; the
   compiler answers with typed diagnostics, counterexamples and proof obligations as structured data;
   the model fixes; repeat. `lume <tool> --json` and `lume agent` are the real surface of the
   language. Text is a *projection* for the human reviewer, not the primary medium.
4. **Programs carry their own evidence.** Each function ships its intent (a natural-language docstring
   the model wrote), its contract, its tests and — where possible — its proof. The compiler
   cross-checks the implementation against the stated intent (via property tests and, later, SMT),
   so "the code does what the author said" is machine-verified, not trusted.
5. **The language teaches itself to the model.** A model has not seen Lume in training (the field's
   cautionary tale: "designed for LLMs" languages still fail one-shot because the model lacks the
   language in context). So Lume ships a compact, complete, machine-ingestible self-description the
   model loads before writing: `lume describe --json` emits the keywords, operators, token kinds, AST
   node kinds, the diagnostic protocol and the edit→check→fix workflow. Grounding is a built-in, not
   a blog post. (Stage 0 already implements `lume0 describe`.)

## Outside the box: the canonical program is a structure, not a file

The deepest consequence, and Lume's long-term bet: **the source of truth is the typed AST graph, not
the `.lume` text.** The model edits structure; the human sees a generated, perfectly-formatted
projection. Combined with content-addressing (each definition identified by the hash of its AST, as
in Unison), this gives properties a text-first language cannot:

- **Localised edits.** The model changes one definition; nothing else is touched, renames are free,
  and the diff is exact.
- **Cached verification.** A definition's tests and proofs are cached by its hash, so re-checking
  after an edit only re-runs what actually changed — essential when a model makes thousands of small
  edits and re-verifies each time.
- **No parse ambiguity, ever.** The model can emit the structure directly (not text to be reparsed),
  so "it didn't compile because of a stray token" stops being a failure mode.
- **Multiple projections.** The same program renders as indented Lume for a human, as a graph for a
  tool, or as a compact token stream for another model — all from one canonical structure.

The text syntax (`docs/03-specification.md`) remains the human-review projection and the on-ramp; it
is not abandoned. But the roadmap treats the structured, content-addressed representation as the
native substrate the AI author works in (*planned, not delivered:* content-addressed definitions and the
projectional editing surface; today only a per-file SHA-256 lockfile exists).

## Humans are not removed — they are promoted

Human readability still matters, because the human is the reviewer with veto power
(`docs/04-roadmap.md`, `lume evolve` governance). The point is the *order of priority*: when machine
checkability and human writing-convenience conflict, the machine wins, because the machine writes and
the human only needs to *understand* the result, not produce it. The human sets direction, reads the
projection, and approves or rejects — the highest-leverage thing a person can do when a tireless,
fast, but fallible author is doing the typing.

## How this reorders the principles

- **P1 is amended**: the four readers remain, but the AI author is primary and the human is the
  reviewer/director. A feature that helps the human write but hurts the compiler's ability to check
  the AI is rejected, not balanced.
- **P30 (errors are a product)** is aimed first at the *machine* consumer: every diagnostic is
  structured data with a stable code, spans, expected/actual and a suggested fix, so the model's
  repair loop is mechanical.
- **P35 (no untested public surface)** and **P34 (project-level laws)** exist because the author is a
  model. P35 currently warns when a public function lacks its paired test; strict merge enforcement
  and example-based coverage are future work.

Everything else in the project serves this: the unambiguous grammar, the effect system, the contracts,
the `lume agent` compiler-as-a-service, the self-description, and the self-evolution loop in which AI
proposes language changes and automated gates plus a human decide. Lume is the language a machine
writes and a person governs.
