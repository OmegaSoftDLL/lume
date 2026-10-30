# Semantic intent protocol 0.1 (partial P0, #337)

`lume mesh` is a machine-native negotiation and transactional-authoring interface for a
bounded Lume-Core fragment. It parses requests, edits the parsed Lume AST, reprojects a
canonical source form, and validates every resulting state with the existing `lcaas`
compiler/prover. Targets are a bounded semantic constraint, not an emitted artifact.
The tool does not synthesize implementations, produce a persistent server, or close the
umbrella P0 issue.

## Invocation

```sh
lume mesh request.mesh
lume mesh --json request.mesh
cat request.mesh | lume mesh
```

`--json` is explicit but optional because JSON is the only output format. A successful
request prints `"ok":true` and exits zero. A rejected request prints structured
`diagnostics` and exits nonzero. Unknown CLI arguments are errors; they are not ignored.

## Request grammar

Fields are case-sensitive, start in column zero, and occur exactly once in this order:

```text
@protocol lume-semantic-intent/0.1
@mode negotiate
@goal prove increment composition
@target x86_64-linux-elf
@capabilities io
@resources max_source_bytes=4096,max_transactions=4
@acceptance prove
@source
fn inc(x: int) -> int requires x >= 0 ensures result == x + 1 = x + 1
print inc(41)
@end
```

- `mode` is `negotiate` or `transact`. `negotiate` forbids operations; `transact`
  requires at least one.
- `goal` is a nonempty, single-line semantic goal. Delimiter-like text is data, not a
  field boundary.
- `target` is `x86_64-linux-elf` or `lumb`; their existing aliases are canonicalized to
  those names. Other targets fail with `M2003`. Version 0.1 negotiates this constraint
  but does not claim to emit or execute a target artifact.
- `capabilities` is `pure` or a comma-separated set drawn from `io`, `fs`, `net`,
  `alloc`, `async`, `fail`, `panic`, `rand`, `time`, `prob`, `llm`, `wasm`, and `ffi`.
  Duplicates and unknown capabilities are rejected. The implementation sorts the set
  canonically and checks existing `lcaas` effect inference, declared effect rows,
  top-level output, and module capability grants against it.
- `resources` uses exactly `max_source_bytes=N,max_transactions=N`, with positive
  decimal integers in that order. The source bound applies to the normalized ASCII
  source block; the transaction bound applies to this request journal.
- `acceptance` is `compile` or `prove`. `compile` preserves and reports each real proof
  status. `prove` additionally requires a nonempty set of obligations all classified
  `proven` by the existing compiler. `runtime_checked`, `mixed`, `absent`, and `refuted`
  never satisfy `prove`.
- `source` is a nonempty Lume-Core program. Version 0.1 accepts printable ASCII plus
  tab and LF/CRLF. Other control characters and non-ASCII input fail closed rather than
  relying on an ambiguous byte mapping.

Unknown, duplicated, reordered, missing, and trailing fields produce `M1002` or `M1003`
diagnostics. A request never falls back to a demonstration or fixed response.

## AST fragment and validation

The canonical projector supports function declarations (including typed/public
signatures), module capability grants, `requires`/`ensures`, effect rows, numeric and
string literals, variables, calls, arithmetic/comparison/boolean operators, conditionals,
`let`/`var` expressions, and `print`. Top-level expression statements and other AST
constructors are rejected with `M1004`; they are never silently omitted or replaced.

Every state transition performs all of the following:

1. Reject duplicate function names, duplicate associated signatures, and orphaned
   signatures with `M2002`.
2. Check negotiated capabilities with the existing effect inference (`M2004`).
3. Project the complete supported AST to canonical source.
4. Run `caas_compile` and preserve its diagnostics and proof records.
5. Reparse and require both the canonical AST S-expression and the full canonical
   source projection (including contracts, effects, and signatures) to reach an exact
   fixed point.
6. Canonicalize and enforce the bounded target constraint (`M2003` on rejection).
7. Enforce the selected acceptance criterion.

A failed transition returns an error response and does not append a lineage node.
False contracts therefore return the real `V0001` compiler diagnostic. Nonlinear or
otherwise undischarged obligations remain `runtime_checked`; they are not relabelled.

## Transaction operations

Operations follow the source block in `@op` records.

Add a contract (an identical parsed obligation is rejected):

```text
@op add_contract
@symbol inc
@kind ensures
@expr result > x
@end
```

Replace exactly one existing contract of the selected kind (zero or multiple matches
are ambiguous and rejected):

```text
@op replace_contract
@symbol inc
@kind ensures
@expr result == x + 1
@end
```

Change target:

```text
@op set_target
@target lumb
@end
```

Rollback to a previous snapshot by visible lineage index (the negotiation root is `0`):

```text
@op rollback
@to 1
@end
```

Replay ordinary add/replace/set-target records after rolling back to their exact source
snapshot:

```text
@op replay
@from 1
@to 3
@end
```

`from` identifies the current snapshot; records `from + 1` through `to` are validated
and applied again. Nested rollback/replay records are not replayable. A state mismatch,
missing index, or invalid range fails with `M3004`.

Contract operations construct and validate a parsed expression AST and update
`VContractSig` nodes. They do not search and replace raw function text. Canonical source
is only the projection used to invoke the existing compilers.

## Canonical state and lineage

ASCII length-prefix encoding is `N:value`, where `N` is the byte length of `value`.
The state digest is SHA-256 over the concatenation of these encoded fields, prefixed by
`sha256:`:

```text
lume-semantic-intent-state/0.1
goal
canonical target
canonical capabilities (`pure` for the empty set)
canonical resources
acceptance
canonical AST S-expression
canonical source
```

Each node digest is SHA-256 over length-prefixed fields:

```text
lume-semantic-intent-node/0.1
parent node ID (empty only for the root)
canonical operation
state digest
```

The root operation is `negotiate`. Rollback and replay append fresh nodes; they never
rewrite history or reuse the old node identity. Their `parent_index` and `parent_id`
identify the node from which the transition occurred. `active_branch` follows these
parents, so abandoned futures remain visible without becoming ambiguous. Replay node
operations include the original node ID and operation.

Each node reports:

- `affected_symbols`: functions directly edited, or all functions for target changes;
- `invalidated_symbols`: the changed functions plus transitive callers from the actual
  pre-transition AST (all functions when the target changes).

Rollback computes declaration changes between the two canonical AST snapshots and
invalidates their dependents. Replay recomputes impact for each freshly applied
transition. The conformance helper independently recomputes every state and node ID.

## Response and current limits

Success responses contain the canonical `state`, compiler/target/proof `validation`
(including the actual in-memory compiler `result`), full `lineage`, `active_branch`, and
compiler diagnostics. `session` is the root node
ID and `current` is the current node ID.

Version 0.1 journals all steps in one request. It has no durable cross-process session
store, transport server, adapter generation, component registry, economic benchmark,
or capstone implementation. It does not claim independent evidence certification,
execution assurance, or completion of #337. Those remain separate P0 work.
