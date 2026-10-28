# 30. The Lume Book — From First Principles to Verified Systems

**Status:** Shipped (`#196` G6)

This book is the step-by-step developer and AI-agent guide to **Lume (Edition 2026)**, the AI-first programming language where code is authored with formal contracts, verified by a sound decision procedure or minimal proof kernel, and compiled to native executables without a C toolchain.

---

## Chapter 1 — Zero-Friction Installation & Self-Verification (`G0`)

Install the prebuilt self-hosted Lume toolchain (`lume`, `lumec`, `lasm`, `lcli`, `lcaas`, `lcert`, `ltarget`, `levolve`) with no C compiler required:

```sh
sh scripts/install.sh --prefix "$HOME/.lume"
export PATH="$HOME/.lume/bin:$PATH"
lume self verify
```

`lume self verify` checks every installed binary and source module against `MANIFEST.sha256` and returns structured JSON:

```json
{"ok":true,"command":"self-verify","version":"0.2.0","edition":"2026","zero_c_required":true,"manifest_verified":true}
```

---

## Chapter 2 — Values, Ownership Tiers, and Algebraic Data Types (`#197`)

Lume defaults to immutable values with Perceus in-place reuse (`own`) and provides explicit compiler-enforced ownership tiers when shared references are required:

- `rc[T]` (`rc_new`, `.get()`, `.clone()`, `.strong_count()`) — deterministic single-threaded reference counting; rejected across `spawn`/`par` boundaries (`[O0010]`).
- `arc[T]` (`arc_new`, `.get()`, `.clone()`, `.downgrade()`) — atomic reference counting (`lock xaddq`) safe across `spawn`, `par`, and `shard`.
- `weak[T]` (`weak_ref`, `.upgrade() -> Option[T]`) — non-owning reference that upgrades to `Some(v)` while alive or `None` after drop.

```lume
fn tier_example() -> int =
    let shared = arc_new(21) in
    let w = shared.downgrade() in
    let upgraded = match w.upgrade()
        Some(v) => v
        None    => 0
    in shared.get() + upgraded
```

---

## Chapter 3 — Contracts, Algebraic Laws, and Proof Diffs (`G5`, `G6`)

Every function can carry `requires` preconditions, `ensures` postconditions, `@bound(cycles: N)` WCET bounds, and module-level `law` declarations checked by Lume's sound `QF_LIA` solver (`Fourier-Motzkin` over integers):

```lume
pub fn clamp_pos(x: int, hi: int) -> int
    requires x >= 0 and hi >= 10
    ensures result >= 0 and result <= hi
    = if x > hi then hi else x

law clamp_idempotent(x: int, hi: int) =
    if x >= 0 and hi >= 10 then clamp_pos(clamp_pos(x, hi), hi) == clamp_pos(x, hi) else 1 == 1
```

### CI / PR Proof Diffs (`lume prove --diff`)

Compare two revisions of a module to verify that no proof obligation regressed (`[V0003]`):

```sh
./lume prove --diff old_module.src new_module.src
```

### Shareable Proof Bundles (`lume prove --share`)

Generate a self-contained shareable proof bundle with a `lume://verify/sha256:...` URI and independent proof certificate:

```sh
./lume prove --share module.src
```

---

## Chapter 4 — Verified Sidecar Interop (`G2`, `#191`)

Adopt Lume inside an existing C, Rust, Go, Python, or Node.js service by compiling one critical module as a **Verified Sidecar** (`examples/interop/sidecar/`):

1. Write `payment_core.src` with `pub extern "c" fn` exports and `requires`/`ensures` contracts.
2. Run `./lume certify payment_core.src` and `./lume verify-cert payment_core.src`.
3. Generate the C ABI header with `./lume build payment_core.src --emit-c-header payment_core.h -o payment_core`.
4. Call from C (`host_c.c`), Python (`host_python.py`), or Node.js (`host_node.mjs`).

---

## Chapter 5 — Supply-Chain Security & Verified Registry (`G4`, `G7`)

Every Lume package declares its required capabilities in `lume.toml` (`allow = ["pure"]` or `allow = ["io", "net"]`) and locks every module's SHA-256 digest in `lume.lock`:

- `./lume pkg publish <dir> --store <registry_dir>` refuses to publish any package whose contracts fail `lume certify` or `lume verify-cert` (`[V0001]`).
- `./lume pkg audit <dir>` verifies capability grants (`P26`), lockfile SHA-256 integrity (`P32`), and the independent proof certificate (`lcert`).
- `./lume pkg check-rot <old_dep.src> <new_dep.src> <downstream.src>` detects semantic proof rot (`[V0003]`) when a dependency weakens a postcondition relied upon by downstream code.

---

## Chapter 6 — Intent-as-Source & Horizon Verification (`#198`)

- **Semantic AST Patching (`docs/19` §3):** `./lume agent patch <file.src> <fn_name> "<new_expr>"` edits the typed AST directly, verifies contracts before accepting the edit, and projects canonical formatted Lume source.
- **Deterministic Effect-Trace Replay (`docs/19` §4):** `./lume agent replay <file.src>` records logical-time trace steps, verifies deterministic replay hashes, and bisects the exact `first_negative_step`.
- **Provenance & Decision Lineage (`docs/19` §5):** `./lume agent lineage <file.src> <fn_name>` answers *"why am I this?"* with function origin (`human` vs `llm_untrusted`), input taint, `verify(...)` gates, and proof status.
- **Compositional & Learned-Proven Seam Verification (`docs/23` H1, H4):** `./lume prove --compositional <mod_a.src> <mod_b.src>` verifies cross-module contracts and enforces `[V0004]` whenever an `extern llm fn` output crosses into a proven function without a `verify(...)` guard.
