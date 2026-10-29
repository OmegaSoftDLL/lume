# GPU hardware dispatch and CPU/GPU capstone

Status: **planned**. This document is an execution plan; implementation and validation remain outstanding. Track: [#279](https://github.com/OmegaSoftDLL/lume/issues/279).

## Sources

docs/04-roadmap.md Stage 4; docs/24-phase4-plan.md; #250; #256; #257

## Scope

Integrate actual CUDA/Vulkan dispatch where supported and run equivalent real workloads and the Stage 4 CPU/GPU capstone on hardware or an independently faithful environment.

## Delivery standard

No mocks, placeholders, fixed outputs or simulated results presented as the feature. Deliver real end-to-end functionality, sound independently checked proofs for the applicable guarantees, positive and adversarial regression tests, and green required CI. Keep this item open until the full agreed scope and acceptance criteria are satisfied.

## Execution

1. Inventory current implementation, tests and CI evidence against each requirement; identify verified coverage and remaining gaps.
2. Split outstanding work into reviewable increments with concrete prerequisites and acceptance criteria. Preserve planned long-term items in the issue until delivered or an explicit scope decision is recorded.
3. Implement each increment in Lume, with positive, negative and regression coverage and platform execution as appropriate.
4. Update documentation and CLI claims to match verified behavior; preserve bootstrap fixpoint, zero-C and other required gates.

This draft must not close the tracking issue until its full acceptance criteria are met.
