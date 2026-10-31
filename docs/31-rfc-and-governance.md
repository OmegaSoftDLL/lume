# 31. Lume RFC Process, Foundation Governance & Long-Term Stability (`G9`)

**Status:** Shipped (`#196` G9)

This document defines the open governance model, the **Proof-Carrying RFC Process**, and the **Edition Stability Guarantee** for the Lume programming language (`docs/22-adoption.md` §G9).

---

## 1. Governance Principles

Lume's governance is anchored in three non-negotiable technical invariants enforced by CI and the self-evolution engine (`lume evolve`):

1. **Proof-Carrying Evolution (`P30`, `P31`):** No compiler pass, standard library module, or language surface change merges without passing the 6 automated `lume evolve` gates (`compile`, `conformance`, `api_compat`, `capability`, `budget`, `fixpoint`) and Ed25519 cryptographic maintainer sign-off (`evolve/maintainers.pub`).
2. **Zero-C Steady State (`scripts/check-zero-c.sh`):** The self-hosted compiler (`lumec`), assembler (`lasm`), target emitter (`ltarget`), proof kernel (`lcert`), and CLI (`lcli`) build and reach a byte-identical `stage2 == stage3` binary fixpoint without invoking `cc`, `gcc`, `clang`, `as`, or `ld`.
3. **Anti-Theater Delivery Rule (`docs/28-delivery-rules.md`):** Every shipped feature must be a genuine compiler/runtime mechanism with both positive end-to-end execution proofs and negative compile-time or runtime rejection tests.

---

## 2. The Proof-Carrying RFC Lifecycle

Every language or compiler proposal follows a four-stage lifecycle:

| Stage | Artifact | Gate |
|---|---|---|
| **1. Draft** | `evolve/proposals/<NNNN>_<name>/proposal.bundle` or `docs/<NN>-<topic>.md` | Specifies principles (`@principles`), capability ceiling (`@capability`), and assembly/WCET budget (`@max_asm_lines`). |
| **2. Oracle Verification** | `./lume evolve verify <proposal.bundle>` | Runs static `QF_LIA` contract proofs, `lume prove --diff` (`[V0003]` zero proof regressions), and conformance tests. |
| **3. Independent Core Check** | `./lume verify-cert --json <certificate.json>` | Core typing, linearity (`O0001`), capability containment (`E0200`), or soundness changes must be accepted by the independent Lume certificate kernel in `lcert.src`. |
| **4. Signed Approval & Apply** | `./lume evolve approve --by <id> --sig <ed25519_hex>` | Verifies the RFC 8032 Ed25519 signature against `evolve/maintainers.pub` and applies the change only when `"can_merge": true`. |

---

## 3. Edition Stability & Anti-Rot Guarantee (`edition = "2026"`)

- **Canonical AST Pinning:** Every module in `edition = "2026"` has a deterministic structural AST hash (`./lume agent hashes <file.src>`) that is invariant under layout vs. brace surface syntax (`./lume agent project`).
- **Semantic Proof-Diff Gate (`[V0003]`):** Package upgrades are checked by `./lume pkg check-rot <old_dep.src> <new_dep.src> <downstream.src>`. A minor or patch release cannot weaken any `ensures` postcondition or `law` invariant relied upon by downstream dependents.
- **Security & Capability Audit (`P26`, `P32`):** `./lume pkg audit` and `./lume verify-cert` allow any consumer or CI pipeline to audit capability boundaries and re-verify proof certificates in milliseconds via the standalone `lcert` kernel.
