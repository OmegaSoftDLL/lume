# 06 — Software protection and anti-reverse-engineering (REA)

Status: **design requirement**, implemented across stages (hooks in Stage 1, full passes in
Stage 5). This document specifies protection of the *programs a user compiles with Lume*. It is a
first-class goal of the toolchain, not an afterthought, because Lume targets commercial and
proprietary software (ERP, trading, games, licensed products) where the shipped artifact is
intellectual property its author is entitled to protect.

> Scope. These features protect **the developer's own compiled program** against inspection,
> tampering and cloning by third parties who receive the binary. They are the same class of
> capability offered by LLVM's obfuscation forks, ProGuard/R8 (Java/Android), VMProtect/Themida,
> and the hardening flags of every production toolchain. They are controlled entirely by the author
> of the code being compiled, apply only to that author's own artifact, and never hide anything
> from that author. Lume does not ship malware tooling: there is no capability here for attacking
> other people's systems, defeating someone else's protection, or evading security software.

## P-REA — the governing principle (added to `02-principles.md`)

**P33 — The author owns the artifact.** The Lume toolchain gives the author of a program practical
control over how much its shipped binary reveals, down to a configurable level, without ever
weakening the safety guarantees of the source (P9) and without obstructing the author's own
debugging, auditing or reproducibility. Protection is opt-in, per build profile, and composable.

## 1. Threat model

The protected party is the **author/vendor** shipping a binary to an end user or deploying it to a
host they do not fully control. The adversary is anyone with the binary who wants to:

1. **Read the logic** — recover algorithms, formulas, business rules, prompts, model weights.
2. **Extract secrets** — license keys, API endpoints, embedded credentials-by-mistake (Lume also
   warns against embedding these; see §6).
3. **Tamper** — patch out a license check, a payment step, an integrity test.
4. **Clone / lift** — copy a proprietary component into another program.

Non-goals: Lume does not claim perfect protection (no software protection is unbreakable against a
determined analyst with the hardware). The goal is to raise the cost of each attack by orders of
magnitude, which is what commercial protection does in practice, and to make tampering *detectable*.

## 2. Build profiles

Protection is selected per profile in `lume.toml`:

```lume
profile.release
    protect = .standard          # none | standard | hardened | max

profile.ship
    protect = .max
    protect.vm       = [sensitive]        # run marked functions on an embedded bytecode VM
    protect.seal     = true               # tamper-evident integrity seal
    protect.license  = "std.license.ed25519"
```

`protect` never changes program *meaning* (P9/P25); it changes only the shape of the artifact. A
`debug` build always has `protect = .none` so the author can always inspect their own code.

## 3. The protection passes (what each level turns on)

### `.standard` (default for `release`)
- **Full symbol stripping**: no function/variable/type names, no module paths, no debug info in the
  shipped binary (DWARF/line tables emitted to a *separate* `.lsym` file the author keeps for
  crash-symbolication — the symbols leave the binary, not the author's hands).
- **Dead-code and unreachable-metadata elimination**; no reflection tables unless used.
- **String table minimisation**: no incidental format strings, assert messages or panic locations
  in release (replaced by stable numeric codes resolvable from the kept `.lsym`).
- **Deterministic, path-independent builds** (no absolute source paths leak into the binary).

### `.hardened`
Adds, as opt-in MLIR/`lume-core` passes:
- **Control-flow flattening** and **opaque predicates** on functions marked `@protect` (or all, if
  configured): the static control-flow graph no longer mirrors the source structure.
- **Constant/string encryption**: literal data (keys the author chose to embed, prompt templates,
  tables) is stored encrypted and decrypted at use; keys are derived at runtime, not stored plainly.
- **Instruction substitution / MBA** (mixed boolean-arithmetic) on arithmetic the author marks
  sensitive (e.g. licensing math), so trivial pattern-matching on the disassembly fails.
- **Anti-debug / anti-instrumentation hooks** (`std.protect.guard`): optional, cooperative checks
  the program can run on itself (ptrace/debugger presence, timing) that the author decides how to
  react to. These are *self-checks of the author's own process*, documented as best-effort.
- **Import/call obfuscation**: indirect resolution of external calls so the import table reveals
  less about behaviour.

### `.max`
Adds:
- **Selective virtualisation (`protect.vm`)**: functions listed are lowered not to native code but
  to a **randomised bytecode** executed by a per-build-specialised interpreter embedded in the
  binary. Each build randomises the bytecode encoding (handler order, operand coding), so lifting
  the logic requires first reversing that build's VM. This is the VMProtect/Themida technique,
  expressed as a `lume-core` → bytecode lowering we own.
- **Tamper seals (`protect.seal`)**: the binary carries signed hashes of its own code pages; a
  bootstrap check verifies them and refuses to run if patched. Combined with `std.license`, a
  defeated license patch also breaks the seal.
- **White-box transforms** for embedded cryptographic keys where a key must exist in the binary
  (the hard case; Lume integrates a white-box AES/ECC library rather than inventing crypto).

## 4. Protecting models and prompts (the AI-era case)

Lume ships models and LLM prompts *inside* programs (§14 of the spec), which are themselves IP:
- `model` parameters can be stored encrypted (`@protect model`) and decrypted into locked memory
  pages at load, never written to disk in the clear by the runtime.
- `extern llm fn` prompt templates obey `.hardened` string encryption, so the shipped binary does
  not expose the author's prompt engineering as plain text.
- `std.protect.enclave` (Stage 5): run a model or a licensing routine inside a hardware enclave
  (SGX/SEV/TrustZone) where the platform offers one, through a capability type.

## 5. What protection must never do (hard constraints)

1. **Never change observable behaviour** of a correct program (P9, P25). Protection is a shape
   transform; a `max`-protected build and a `debug` build compute the same results.
2. **Never hide code from its own author.** The `.lsym` sidecar, the source, and a `debug` build
   always let the author see everything. Lume keeps no author-opaque blob.
3. **Never obstruct safety.** Memory safety, contracts and effect checks run at *compile time* on
   the unobfuscated IR; obfuscation happens after checking, so a protected binary is exactly as
   safe as its source.
4. **Never bundle attack tooling.** Lume has no feature to deobfuscate, unpack, or defeat the
   protection of a binary someone else produced, and no feature to evade antivirus/EDR. `std.protect`
   hardens *your* program; it is not an offensive toolkit.
5. **Reproducibility is preserved**: given the same source, profile and a recorded per-build
   randomisation seed, the artifact rebuilds bit-for-bit (needed for the triple self-build, P27,
   and for audits). The seed is an input the author controls.

## 6. Secrets hygiene (the complement to obfuscation)

Obfuscation is the last resort, not the first. Lume's effect and capability system (§8) does the
primary work:
- The compiler **warns** (`W-SEC-001`) when a string literal looks like a credential, private key
  or token and is reachable in a shipped profile, and suggests `std.secret` (load at runtime from
  the environment, a keystore, or an enclave).
- `std.secret[T]` is a type whose value is never logged, never serialised by `Show`, zeroised on
  drop, and kept out of core dumps — so the *right* fix (don't embed it) is easier than embedding it.
- Capability grants (P26) mean a cloned component cannot silently gain `net`/`fs` access in another
  program without that program's manifest declaring it.

## 7. Roadmap placement

| Stage | Protection deliverable |
|---|---|
| 1 | Symbol stripping, `.lsym` sidecar, deterministic/path-independent builds, string minimisation, `std.secret`, `W-SEC-001` |
| 2 | `profile.*.protect` in `lume.toml`; constant/string encryption; `@protect` attribute plumbing |
| 3 | `std.protect.guard` self-checks; import/call obfuscation |
| 4 | `@protect model`, prompt encryption, encrypted-weights loader |
| 5 | Control-flow flattening, MBA, selective VM (`protect.vm`), tamper seals, `std.protect.enclave`, white-box crypto |

Every pass is a `lume-core`/MLIR transformation we own, so protection travels to every target
(native, WASM, freestanding) and composes with the rest of the pipeline.
