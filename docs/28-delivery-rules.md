# 28 — Delivery hardening: the no-theater checklist for every PR

Lume's recurring failure mode is not a lack of capability — it is **shipping impressive machinery
whose trust/soundness primitive is theater**, labelled as if it were real. This document is the
self-review every contributor (human or AI) runs **before opening a PR**, so the author catches what
a reviewer would. The short version is in `AGENTS.md`; this is the full rationale and checklist.

## Why this exists — the pattern, with real examples

Each of these shipped looking done, passed a green CI, and was caught only on code-level review:

- **Prover that samples, not proves.** `lume prove` v0 evaluated a contract on a fixed set of 2–3
  integers (`{0,1,-1}`) and returned `proven`, eliding the runtime guard. A contract false on an
  unsampled input shipped "proven". (#173 → sound QF_LIA / Fourier–Motzkin.)
- **Certificate that rubber-stamps.** The "minimal proof kernel" only re-read the compiler's own
  `status == "proven"` and used a toy hash mislabelled `sha256:`. It verified nothing independently.
  (#183 → an independent S-expression parser + standalone FM solver + real FIPS-180-4 SHA-256.)
- **Silent wrong results.** Symbolic autodiff returned `0` for any non-polynomial operator — wrong
  math, no error. (#184 → `[N0400]` diagnostic; never a silent default.)
- **Overclaimed labels.** WCET was a heuristic per-node estimate labelled `proven`; the WASM "sandbox"
  was an in-language record model presented as isolation. (#185/#186 → `budget_checked`; "reference
  model".)
- **A hash pretending to be a signature.** `lume evolve` "cryptographic maintainer signature" was
  `sha256(public_name ‖ …)` — forgeable by anyone, plus an unsigned `approved:<name>` bypass. (#194 →
  real RFC 8032 Ed25519, public key in repo / private off-repo, no bypass, validated against the
  official §7.1 vectors.)
- **One-shot labelled a server; a model labelled durable.** `lume lsp/dap/repl` emitted a single
  protocol-shaped JSON but were called "serve LSP/DAP" / "interactive Jupyter kernel"; `persist` was a
  35-line in-language model called "crash-safe ACID … done". (#210/#211/#214 + #225 honesty.)

The content was often real; the **label exceeded the substance**. That is the thing to stop.

## The checklist (run before opening any PR)

### 1. State the real level — shipped / model / planned
- Label every feature honestly in the docs, the CLI help, and `PROJECT_STATE.md`, in the **same PR**.
- Forbidden unless it is literally true of what executes: "serve", "server", "interactive",
  "kernel", "ACID", "durable", "crash-safe", "proven", "sound", "verified", "✅ done".
- A reference model or one-shot or stub is labelled exactly that. No exceptions "to be fixed later".

### 2. "Done" means it does the real thing
- Not that it emits the right JSON shape, protocol label, or plausible bytes.
- A compile target or kernel brick is **done only when a real, non-trivial program runs on real
  hardware or a faithful emulator (QEMU)** with its guarantees intact — see `docs/26`, `docs/27`.

### 3. Trust & soundness primitives must be genuine
- **`proven`** comes only from a sound decision procedure (QF_LIA / Fourier–Motzkin); never from
  sampling or enumeration. If undecidable → `runtime_checked`. A false `proven` is the worst bug here.
- **Never silently return a wrong or default result.** Outside a supported fragment, **diagnose**
  (an error code), never return `0`/empty/identity silently.
- **Signatures are asymmetric** (Ed25519 or equivalent): the public key lives in the repo, the private
  key strictly off-repo; a SHA-256 over public data is a checksum, not a signature; no unsigned path.
- **Verifiers/certificates re-derive independently** — they must not trust the producer's own verdict;
  they re-run the check. Hashes are real cryptographic hashes.
- **Validate crypto and decision procedures against official test vectors** (e.g. RFC 8032 §7.1),
  never only against self-generated ones (a shared signer/verifier bug would pass).
- An analysis that cannot be made sound is **labelled honestly** (e.g. `budget_checked`, "heuristic"),
  never "proven" / "sound".

### 4. Write tests that can FAIL if it's theater
A green CI does **not** prove soundness. Alongside the happy path, include:
- the **negative case**: a forged signature is rejected; a false contract is `refuted` **with a
  concrete counterexample**; an out-of-fragment obligation is `runtime_checked`, not `proven`;
- **mechanism asserts**: e.g. the generated asm contains **no** residual `call …` when a guard is
  claimed elided; output is **byte-identical** across protection/optimisation levels;
- **standard conformance**: official vectors (crypto), published behaviour;
- **it actually runs**: on QEMU / real hardware for systems bricks.

### 5. GOLDEN RULE preserved
`stage 2 == stage 3`, `make zero-c`, and the oracle byte-equivalence stay green. New features and
backends are written in **Lume** (`.src`), not new C in the frozen seed (`#205`). Every fix ships a
regression test. If a shortcut is genuinely unavoidable, it ships **labelled `model`/`planned`** and
is tracked as an issue — it never ships as a guarantee. **Zero theater inside Lume.**

### 6. Deliver the real construct, not a mock of it
A feature is done only when the **real language construct** exists in the compiler — not when a
hand-written function or library imitates its behaviour well enough to pass one test. This is the most
seductive form of theater: it runs, it prints the right numbers, and it is still fake.
- **A typed feature is the type, not a library that imitates it.** `rc[T]` means the compiler has the
  `rc[T]` ownership tier (literal `rc(value)`, automatic deref, ownership analysis) — **not** a set of
  user functions `rc_new`/`rc_drop` poking memory and returning `{tier:"rc", …}` records. If the real
  construct is not there, the feature is `planned`; its issue **stays open**.
- **Diagnostics come from the real analysis, never faked.** A `[CODE]` error must be emitted by the
  compiler's own check. Printing it yourself — `ctfail("…[O0001]…")` with a hardcoded string, or
  hand-returning the JSON a diagnostic would produce — is theater: it fires whether or not the analysis
  exists, so it proves nothing.
- **The test exercises the real feature, not a stand-in.** A negative test for `rc[T]` must use `rc[T]`;
  `fn rc_drop(x: int) -> int = x` standing in for it, passing via generic move semantics, tests nothing
  about the feature. If the only program that exercises the "feature" is its own conformance test, and
  no real program or agent would ever call it, it is a mock with no purpose — it must not ship, and it
  must **not** close the feature's issue.
- **Regenerate the expected from the real output — never tune the program to the expected.** A
  conformance number must be the *natural* output of the real code, regenerated and then checked
  against a hand trace so each value is known-correct. Changing demo inputs (e.g. a `shard 0..2` →
  `0..1` range) so a rewritten program's total matches a **stale/old** expected is fitting code to a
  dead target — the opposite of correct, and it can mask a real bug.
- **Assert each behaviour independently; don't hide many behaviours in one opaque sum.** A test that
  collapses N features into a single integer (`a + b + c + …`) lets a bug in one term be cancelled by
  tuning another. Print/assert each observable on its own (e.g. each ref-count, each upgrade result,
  each promise/signal value), so no term can be fitted to rescue the total.
- **Build exactly what the issue asks.** Delivering an adjacent, easier thing (a model, a stub, a
  look-alike library) and closing the issue as done is a false completion (§1). Deliver the real thing,
  or label it honestly and leave the issue open.

## In one line
Build the real thing or label it honestly; prove it with a test that would fail if it were fake; and
never let a word in the docs, the CLI, or a status line promise more than the code does.
