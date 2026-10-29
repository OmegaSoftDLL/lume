# Reconcile documentation status and normative core

Status: **planned**. This document is an execution plan; implementation and validation remain outstanding. Track: [#301](https://github.com/OmegaSoftDLL/lume/issues/301).

## Sources

docs/03-specification.md; docs/14-data-track-handoff.md; docs/15-phase2-status.md; docs/19-intent-as-source.md; docs/24-phase4-plan.md; docs/29-worklist-order.md; docs/32-ai-authoring.md; docs/index.md

## Scope

Resolve conflicting completion claims, missing docs/core reference, edition boundaries and worklist status using source and CI evidence; clearly link every unfinished or uncertain commitment to an open issue.

## Delivery standard

No mocks, placeholders, fixed outputs or simulated results presented as the feature. Deliver real end-to-end functionality, sound independently checked proofs for the applicable guarantees, positive and adversarial regression tests, and green required CI. Keep this item open until the full agreed scope and acceptance criteria are satisfied.

## Execution

1. Inventory current implementation, tests and CI evidence against each requirement; identify verified coverage and remaining gaps.
2. Split outstanding work into reviewable increments with concrete prerequisites and acceptance criteria. Preserve planned long-term items in the issue until delivered or an explicit scope decision is recorded.
3. Implement each increment in Lume, with positive, negative and regression coverage and platform execution as appropriate.
4. Update documentation and CLI claims to match verified behavior; preserve bootstrap fixpoint, zero-C and other required gates.

This draft must not close the tracking issue until its full acceptance criteria are met.
