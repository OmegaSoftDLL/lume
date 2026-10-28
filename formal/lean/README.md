# Lume-Core Mechanised Semantics & Soundness in Lean 4 (`#213`)

This directory contains the machine-checked formalisation of **Lume-Core** in Lean 4 (`formal/lean/LumeCore.lean`), serving as the formal trust anchor described in [`docs/04-roadmap.md`](../../docs/04-roadmap.md) Stage 5 §5 and [`docs/05-vision.md`](../../docs/05-vision.md) §2.10.

## Contents (`LumeCore.lean`)

1. **Core Syntax & Types (`Ty`, `OwnTier`, `Eff`, `Expr`, `Val`)**:
   - Models the core expression calculus (`eInt`, `eStr`, `eBool`, `eVar`, `eAdd`, `eSub`, `eMul`, `eEq`, `eLt`, `eIf`, `eLet`, `eMove`, `eVerify`, `eEffOp`) lowered by `compiler/stage1/lume/06b_phase3.src` and compiled by `05_codegen.src`.
2. **Combined Type, Ownership, Effect & Provenance Judgment (`HasType`)**:
   - **Linear Ownership (`[O0001]`)**: `htMove` consumes `x` in `OwnEnv` (`consumeOwn Δ x`), and `move_consumes_ownership` proves that any subsequent read or move of `x` is statically rejected.
   - **Effect Purity (`[E0200]`)**: `htEffOp` requires `op ∈ ε`, and `pure_rejects_effects` proves that `/ pure` (`[]`) functions cannot perform unhandled effect operations.
   - **Provenance Verification (`[A0003]`)**: `htVerify` promotes `unverified[T]` (`Ty.tUnverified τ`) to trusted `τ` only when accompanied by a boolean verification condition.
3. **Big-Step Operational Semantics (`Eval`) & Soundness Theorems (Zero `sorry`)**:
   - `eval_deterministic`: Big-step evaluation of Lume-Core is deterministic.
   - `type_preservation`: Well-typed expressions evaluate to values of the static type `τ`.
   - `verified_contract_elision_sound`: When the `QF_LIA` solver (`06b_phase3.src` / `lcert.src`) proves `∀ ρ v, Pre ρ → Eval ρ body v → Eval ρ condE (.vBool true)`, eliding the runtime contract guard (`call ctfail == 0`) is observationally equivalent to the guarded program.
