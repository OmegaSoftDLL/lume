# 09 — LBE: the AI writes structure, the machine writes the bits

The project's steer: "after Lume is ready, the AI will program at the level of 0s and 1s — that is
the innovation; make the language the easiest possible for an AI to write." This document takes that
seriously, separates the genius in it from the trap, and records what Lume actually builds.

## The trap, stated honestly

If an AI emitted **literal machine code** (raw 0s and 1s), it would throw away everything that makes
Lume good for an AI author: the types, the effects, the contracts, the proofs, the exhaustiveness. A
model's mistakes would become invisible until runtime, on a representation that is unportable,
unverifiable and unsafe. Raw bits are the *hardest* target for a fallible author, not the easiest —
they remove the compiler, which is the one reviewer that never tires (P1, `08-ai-first.md`).

## The genius, extracted

The easiest thing for an AI to emit is **neither human text nor machine bits**. Human text must be
re-parsed and can be syntactically invalid ("it didn't compile because of a stray token"). Machine
bits are unsafe and unverifiable. The sweet spot is for the AI to emit **the program's structure
directly, in a dense binary encoding, where every symbol it produces is a valid move in the grammar**
— so a syntactically-invalid program cannot exist — and for the *compiler* to lower that structure to
the actual machine bits that run. The AI thinks in structure; the machine produces the bits.

This is the natural end of the AI-first bet (`08-ai-first.md`): the canonical program is the typed
AST, not the `.lume` text. The text is a human-review projection; the structure is the native
substrate the model authors in.

## What Lume builds: LBE (Lume Binary Encoding)

**LBE is a dense binary serialization of the AST.** Stage 0 implements it end to end:

```
lume0 emit  program.lume  program.lbe     # text -> structure (binary AST)
lume0 run   program.lbe                   # load the structure and run it, with NO text re-parse
```

- `emit` lexes and parses the text once (the human on-ramp) and writes the AST as LBE.
- `run` on a `.lbe` file reads the structure directly with `lbe_read` and executes it. The lexer and
  parser are never invoked, so there is no text, no tokens, and no way for a stray character to make
  the program fail to compile.
- **Round-trip proof (conformance):** for every Stage-0 example, `run program.lume` and
  `run program.lbe` produce byte-identical output. The structure *is* the program.

Format (v1): a 4-byte magic `LBE1`, then each node as LEB128 varints `kind, flags, line, col,
text?(len+bytes), nkid`, children in preorder. It is deliberately simple and self-describing, not yet
minimal: it keeps positions and flags so diagnostics and ranges survive. A production LBE would intern
strings, drop positions it can regenerate, and bit-pack node kinds — but the property that matters is
already here: **reparse-free, structurally valid by construction.**

## Why this is "the easiest possible for an AI to program"

1. **Invalid syntax is impossible.** The AI emits nodes, not characters. There is no "forgot a
   bracket" failure mode. (The next step, Stage 2+, is constrained decoding over the grammar as a
   state machine, so the model literally cannot emit an ill-formed node sequence.)
2. **Edits are structural and local.** Combined with content-addressing (`08-ai-first.md`), the AI
   changes one node; the rest is untouched, and its cached tests/proofs stay valid.
3. **The bits are the compiler's job, not the AI's.** The machine code (the real 0s and 1s) is
   produced by lowering the verified structure — native backend in Stage 1/4 — so the AI gets the
   performance of bits without carrying the risk of writing them.
4. **One structure, many projections.** The same LBE renders as indented Lume for a human reviewer,
   as a graph for a tool, or as a token stream for another model.

## Roadmap placement

- Stage 0 (done): `emit`/`run .lbe`, the round-trip proof.
- Stage 1: the native backend makes "lower structure to machine bits" literal. **Done two ways in
  miniature:** (a) `lume0 emit-asm` is a **direct x86-64 backend in the bootstrap** that lowers the
  AST to assembly — no C in the pipeline — assembled with `as`/`ld` into a native binary that prints
  via raw Linux syscalls (conformance `x86-64 asm -> native (no C)`, let+arithmetic subset); and (b)
  `compiler/stage1/codegen.lume` is a code generator *written in Lume* that lowers the AST to C; the
  Lume-written lexer → parser → codegen pipeline emits C that the system compiler turns into a native
  binary with correct output (conformance: `codegen -> C -> native binary`). Structure in, bits out,
  with the compiler itself written in Lume.
- Stage 2: content-addressed definitions (per-AST-hash) + constrained-decoding grammar so an AI
  emits only valid node sequences.
- Stage 2+: projectional editing — the AI edits LBE, the human sees the text projection.

So: the AI does program at the level of structure that becomes 0s and 1s — just not by typing the
bits by hand, which would be the one way to lose everything the language is for.
