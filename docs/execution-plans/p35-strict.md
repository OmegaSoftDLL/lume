# P35 example coverage and strict enforcement

Status: **planned**. This document is an execution plan; implementation and validation remain outstanding. Track: [#269](https://github.com/OmegaSoftDLL/lume/issues/269).

## Sources

docs/02-principles.md; docs/04-roadmap.md; #208; #218

## Scope

Discover examples that exercise public functions; implement configurable strict enforcement and structured diagnostics. Keep existing advisory behavior available.

## Execution

1. Inventory current implementation, tests and CI evidence against each requirement; identify verified coverage and remaining gaps.
2. Split outstanding work into reviewable increments with concrete prerequisites and acceptance criteria. Preserve planned long-term items in the issue until delivered or an explicit scope decision is recorded.
3. Implement each increment in Lume, with positive, negative and regression coverage and platform execution as appropriate.
4. Update documentation and CLI claims to match verified behavior; preserve bootstrap fixpoint, zero-C and other required gates.

This draft must not close the tracking issue until its full acceptance criteria are met.
