# 36 — Verified content-addressed component registry

Refs #337.

## Status and scope

`lume registry` is a **shipped local P0 increment** for publishing, discovering, and resolving
Lume components with freshly verified evidence. It is separate from the rejected `lmesh` registry
model. It does not implement a network service, a global component network, the economic benchmark,
or the SaaS/kernel capstones.

This increment executes components only on `x86_64-linux-elf`. Publication and resolution reject
other targets instead of substituting interpreted results or claiming cross-target evidence.

## Commands

```sh
lume registry publish \
  --adapter adapter.src \
  --source component.src \
  --tests component_tests.src \
  [--proof proof.cert.json] \
  [--store .lume/registry] [--target x86_64-linux-elf]

lume registry resolve \
  --cid sha256:<64-lowercase-hex> \
  [--store .lume/registry] [--target x86_64-linux-elf]

lume registry discover \
  [--capability <name>] [--contract-signature sha256:<64-lowercase-hex>] \
  [--store .lume/registry] [--target x86_64-linux-elf]
```

The default store is `${LUME_REGISTRY_DIR}` when set, otherwise
`${TMPDIR:-/tmp}/lume_verified_registry`. The adapter, source, and test files are library-style
Lume-Core artifacts. The registry concatenates adapter and source in that order, then appends the
test artifact for test execution. At least one public contract and one executed passing `test_*`
function are required.

If `--proof` is omitted, publication generates the proof artifact. If supplied, the certificate
must pass the independent `lcert` kernel and be byte-identical to a fresh certificate for the actual
adapter/source artifact. A caller cannot substitute a verdict, evidence label, component name,
capability list, adapter digest, or contract signature.

## Publication and resolution checks

Publication performs all of the following before moving an entry into the content-addressed store:

1. Construct the component from the real adapter and source bytes.
2. Certify its parsed contracts with the real compiler/prover.
3. Independently re-verify the certificate and every proof term with `lcert`.
4. Compile and execute the real test artifact, requiring at least one pass and zero failures.
5. Parse the public AST interface and derive capabilities and the contract signature from declared
   grants/effects, parameter and return types, effects, `requires`, and `ensures` clauses.
6. Hash the actual adapter, source, certificate, tests, and test-output artifacts with SHA-256.
7. Bind those hashes, target, toolchain identity, capabilities, contract signature, public contract
   count, and proof-kernel identity into the CID and deterministic manifest.

Resolution repeats proof certification, independent proof verification, native test execution,
interface derivation, artifact hashing, toolchain comparison, CID derivation, and canonical manifest
generation over the stored bytes. A stored `PASS` is not trusted. Discovery returns an entry only
after the same resolution checks succeed and its derived metadata matches the query.

The store layout is:

```text
STORE/components/sha256:<cid>/
  manifest.json
  adapter.src
  source.src
  component.src
  proof.cert.json
  tests.src
  test-component.src
  test-output.txt
```

`component.src` and `test-component.src` are deterministic derived artifacts. `manifest.json` uses
schema `lume-verified-component/0.1`; there are no timestamps or caller-selected names, so identical
artifacts and toolchain inputs produce identical manifests and CIDs.

## Failure behavior

Errors are structured JSON with `ok:false` and a stable code:

- `R0001`: malformed request or missing artifact.
- `R0002`: forged, stale, mismatched, or independently rejected proof/interface evidence.
- `R0003`: incomplete entry, artifact-hash mismatch, CID mismatch, or non-canonical manifest.
- `R0004`: incompatible target or stale toolchain evidence.
- `R0005`: failing, missing, stale, or tampered executable test evidence.

Existing entries are never overwritten by publication. An existing CID must be resolved rather than
replaced. Recomputing a CID after changing an artifact or verdict does not bypass fresh proof and
test verification.

## Validation and limits

`scripts/check-verified-registry.sh` exercises real publication/resolution and uses Python's
`hashlib` as an independent CID oracle. Adversarial coverage includes a false proof with a
maliciously recomputed certificate hash, mismatched adapter hash, source/proof staleness, test-output
tampering with a recomputed component CID, stale toolchain identity, failing tests, capability and
contract-signature discovery, and target incompatibility. The Stage-0 suite invokes this gate on
Linux x86-64; other hosts do not claim target execution coverage.

The increment authenticates content and evidence freshness, not publisher identity or repository
permissions. SHA-256 collision resistance is assumed. Proof guarantees are limited to the existing
`lcert`-supported theories; test evidence covers only the executed tests. Dependency graphs beyond
the supplied adapter/source pair, remote replication, publisher signatures, garbage collection,
additional execution targets, economic results, and capstone acceptance remain future work.
