# 37 — Portable renewable evidence protocol 0.1

Refs #330 #335 #337.

## Status and claim boundary

`compiler/stage1/levidence.src` is an implementation candidate for portable evidence issuance and
freshness verification. It is not accepted as shipped until its native Lume regressions and the full
project gates pass. The implementation, CLI, and regression harness are written in Lume.

Freshness does not establish correctness, execution assurance, publisher identity, or independent
evaluation. A `PASS` describes the current proof obligations and compatible retained evidence only.

## Material identity

Every issued bundle binds the actual source and proof certificate to:

- intent and source content;
- sorted compiler and proof-kernel artifacts;
- sorted dependency artifacts;
- target triple and target description;
- execution environment;
- acceptance policy;
- the declared claim dependency graph.

Missing required frames, incomplete graphs, unknown references, duplicate claims, cycles, and malformed
canonical input fail closed to `UNKNOWN`. There is no caller-supplied verdict or certificate override.

## Issue and verify

The native `levidence` tool accepts length-prefixed frames on standard input. `@mode issue` compiles
the source with `caas_compile`, derives the real proof counts and certificate, and emits a canonical
`lume-portable-evidence/0.1` bundle with its semantic identity and SHA-256 bundle hash.

`@mode verify` receives the old bundle plus the current material frames. It independently checks the
bundle hash, semantic identity, proof counters, verdict consistency, and current compiler/certificate
results before deciding freshness. Tampered or malformed records are rejected.

The response distinguishes:

- `retained`: unchanged claims with compatible evidence;
- `invalidated`: claims that require revalidation;
- `stale`: the old evidence for those invalidated claims;
- `missing`: required evidence that is absent;
- `newly_required`: claims requiring fresh evidence.

Source, intent, compiler/kernel, target, environment, policy, or claim-graph changes conservatively
invalidate all current claims. Dependency-only changes invalidate exactly the transitive affected
closure, allowing compatible unchanged claims to be retained with canonical identity, proof, and
certificate equivalence. A completely fresh response retains the previous verdict; incomplete or stale
freshness returns `UNKNOWN`, never a retained `PASS`.

## Native Lume regressions

`levidence.src` contains seven `test_evidence_*` functions. The `ltest` tool synthesizes a native Lume
test harness and strips the CLI entrypoint. The cases cover fresh retention, direct dependency impact,
transitive impact, environment invalidation, tamper rejection, missing environment identity, and a
newly required dependency absent from the previous bundle.
