# Semantic edits, effect replay and full provenance

Status: **planned**. This document is an execution plan; implementation and validation remain outstanding. Track: [#285](https://github.com/OmegaSoftDLL/lume/issues/285).

## Sources

docs/19-intent-as-source.md sections 3-5; #198; #259

## Scope

Assess and complete canonical typed AST editing and semantic merges, deterministic record/replay across effects, and value/decision lineage. Preserve independent tracking of section 1 in #259.

## Delivery standard

No mocks, placeholders, fixed outputs or simulated results presented as the feature. Deliver real end-to-end functionality, sound independently checked proofs for the applicable guarantees, positive and adversarial regression tests, and green required CI. Keep this item open until the full agreed scope and acceptance criteria are satisfied.

## Execution

1. Inventory current implementation, tests and CI evidence against each requirement; identify verified coverage and remaining gaps.
2. Split outstanding work into reviewable increments with concrete prerequisites and acceptance criteria. Preserve planned long-term items in the issue until delivered or an explicit scope decision is recorded.
3. Implement each increment in Lume, with positive, negative and regression coverage and platform execution as appropriate.
4. Update documentation and CLI claims to match verified behavior; preserve bootstrap fixpoint, zero-C and other required gates.

This draft must not close the tracking issue until its full acceptance criteria are met.
