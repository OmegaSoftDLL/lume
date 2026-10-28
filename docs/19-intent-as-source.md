# 19 — AI-native north star: intent as source, the compiler as an oracle

Lume was created so an **AI can author correct software faster, easier, and more cheaply**. The
phased roadmap (ownership, effects, concurrency, verification) makes Lume an excellent language; this
note records the **north star beyond that** — the five moves that make Lume not "one more language an
AI uses" but **the first language where the AI programs at the level of intent and the machine
guarantees correctness**. Each is tied to a concrete foothold already in the plan, so this is a
direction, not a wish list.

## 1. (Flagship) Intent is the source; code is a *proven cache*

You author the **spec** — examples + properties + contracts + types + effects — and the
implementation is **synthesised and maintained by the machine and proven to satisfy the spec**. Change
the requirement and the code regenerates, **correct by construction**. The author works on *what*; the
compiler guarantees the *how*. It is "vibe coding" with a proof instead of a hope.

- **Foothold:** `evolve/proposals/*` already carry a `spec` block with examples + property tests;
  **A1 typed holes** + **V2 `lume prove`/SMT** are the synthesis-and-proof pair; `lume evolve` (E3) is
  the governance loop. The step is to make the **spec the primary artifact** and the `.src` a
  regenerated, proof-carrying cache keyed by the spec's content hash (reuse P32 content-addressing).
- *No source language does this.* Dafny/F\* prove hand-written code; here the code is **derived** from
  intent and re-proven on every spec change.

## 2. The compiler answers questions (an oracle with witnesses)

Beyond diagnostics, a model can **ask** and get a structured (JSON) answer:
- "Give me an input that breaks this contract" → an SMT **counterexample** (V2 already synthesises these).
- "Fill this hole" → ranked candidates (A1).
- "What is the smallest change to make this `total` / pass this contract?" → a repair.
- "What breaks if I change this type / signature?" → the impact set.

The compiler does the formal heavy lifting; the AI does the creative part. Programming becomes a
**dialogue**. *Foothold:* `lcaas` + `lume agent` (E1b) is the query surface; extend it from
"check/compile/fill" to "witness/repair/impact".

## 3. Semantic edits only — the AST patch is the unit of change

A model never edits text; every change is a **typed AST transform** the compiler validates. Two
consequences: **syntax errors become impossible**, and **merges are semantic** (meaning-level, not
text-level, so there are no spurious conflicts). *Foothold:* `lume agent` already transforms source
trees "never raw text edits" (P-evolve); elevate AST-patches to the canonical, only edit channel, and
make `lume fmt` + content-addressing give every edit a stable semantic identity.

## 4. Deterministic time-travel by default

Every run is reproducible with built-in record-replay, so a failure is a **replayable, bisectable
artifact**, not a guess. The AI debugs by **querying history** ("which call first made this value
negative?") rather than re-running blind. *Foothold:* Phase 3 Track S already targets
**deterministic logical-time replay** for concurrency; generalise it to all effects (tie to P14
effects + the `prob`/`io` boundaries) so any execution can be recorded and replayed.

## 5. Provenance of everything

Any value can answer **"why am I this?"** — a first-class query over its derivation (which inputs,
which decisions, which AI/`prob` source). For an AI maintaining a system, explainability is built in,
not bolted on. *Foothold:* this is **A3 (provenance/uncertainty in the type)** generalised from
"AI-origin" to full data/decision lineage, composed with the effect and contract systems.

## The bet

If Lume has **#1 (intent as proven source)** + **#2 (the compiler as an oracle)** + **A3
(provenance)**, it is the first language where **the AI operates at the level of intent and the machine
guarantees the code matches** — correctness by construction instead of by testing. That is the leap no
existing language offers, and it is the reason Lume is worth building AI-first from the ground up
rather than retrofitting AI onto a human-first language.

**Status:** All five Intent-as-Source pillars (§1–§5) are **shipped and tested in CI** (`#178`, `#195`, `#198`). What is implemented lives in `compiler/stage1/lume/06b_phase3.src`, `compiler/stage1/lcaas.src`, `compiler/stage1/levolve.src`, `compiler/stage1/intent_oracle_ok.src`, and `evolve/proposals/0002_intent_oracle/proposal.bundle`:
- **§1 Hole-Guided `QF_LIA` & Law Synthesis (`abest_hole_expr` / `cs_best_hole_expr`):** Typed holes (`_`, `_hole*`) generate linear arithmetic (`v + 1`, `v - 1`, `v + w`, `v - w`, `w - v`) and branching (`if v >= w then v else w`, `if v <= w then v else w`) AST candidates alongside variables and helper calls, score each candidate against `requires`/`ensures` contracts and `law` (`P34`) declarations using the Fourier–Motzkin `QF_LIA` decision procedure (`vqflia_prove_fn`/`vqflia_prove_law`) and counterexample search, prune refuted candidates, emit `proven_candidates` in `lume agent fill`, automatically fill private draft holes (`afill_prog_holes`/`cs_fill_prog_holes`) with zero runtime guards when proven, and repair `pub fn` `A0001` holes inside the 6-gate `lume evolve` loop.
- **§2 Oracle `witness`, `repair`, and `impact` Queries (`caas_witness_json`, `caas_repair_json`, `caas_impact_json`, #195):** `lume agent witness <file.src>` returns concrete counterexample witnesses for any refuted contract/law/invariant; `lume agent repair <file.src>` synthesizes both deterministic diagnostic fix-its and `QF_LIA`-proven contract-body repairs (`"kind":"contract_body_repair"`, `"proven":true`) when a function's `ensures` contract is refuted (`V0001`); and `lume agent impact <file.src>` computes `"direct_callers"` and `"affected_proofs"` (both caller contracts and `law` declarations) for every symbol in a module. All three are also exposed in `lume mcp` (`lume_witness`, `lume_repair`, `lume_impact`).
- **§3 Semantic AST-Patch Editing (`caas_patch_json`, `lume agent patch`, `lume_patch`, #198):** `lume agent patch <file.src> <target_fn> <patch_expr>` parses and validates the replacement expression as a typed AST node (`A0004` on syntax/parse failure or missing target function), splices it into the target function AST (`cs_replace_fn_body`), runs static `QF_LIA` contract and law verification (`V0001` if the AST patch refutes any `ensures` postcondition or `law`), computes `before_ast_hash` and `after_ast_hash`, and projects canonical surface code (`projected_source`).
- **§4 Deterministic Logical-Time Effect-Trace Replay (`caas_replay_json`, `lume agent replay`, `lume_replay`, #198):** `lume agent replay <file.src>` executes top-level expressions and function calls in deterministic logical-time order, records per-step inputs, outputs, and cumulative state hashes (`trace_hash`), verifies replay determinism (`"replay_verified":true`), and bisects the exact `first_negative_step` whenever a step produces a negative value.
- **§5 First-Class Provenance & Decision Lineage (`caas_lineage_json`, `lume agent lineage`, `lume_lineage`, #198):** `lume agent lineage <file.src> [<fn_name>]` answers *"why am I this?"* for any function or module by returning structured JSON lineage (`origin`: `human_verified` vs `llm_untrusted`, `effect`, `inputs` with `untrusted_llm` taint tracking and `verify(...)` discharge status, `callees`, `decisions` branch conditions, `contracts`, `proof_status`, and `ast_hash`).
