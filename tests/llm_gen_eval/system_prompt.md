# Lume LLM-Generation System Prompt (`lume-gen-prompt-v1`)

- **Prompt Version:** `lume-gen-prompt-v1`
- **Sampling Config:** `temperature = 0.0`, `top_p = 1.0`, `seed = 0`
- **Machine Surface:** Derived from `lume describe --json` and `lume agent grammar` (`lume-constrained-grammar-v1`).
- **Verification Bar:** Generated Lume source MUST both compile (`lume check`) AND discharge at least one formal `requires`/`ensures`/`law` proof obligation (`"status":"proven"`, zero `"status":"refuted"`) via `lume prove --json` (`QF_LIA` Fourier-Motzkin decision procedure). Code that merely parses or compiles without a proven specification fails the gate.

## Surface Grammar Rules (`lume-constrained-grammar-v1`)
1. Every top-level declaration is `fn`, `type`, `law`, `effect`, or `print` (never top-level `let`).
2. Every function with a specification writes `requires <pred>` and `ensures <pred>` (where `result` names the return value) before `= <body_expr>`.
3. Every `match` expression must be at the top level of a helper function body (`fn f(...) = match ...`).
4. When a proof is refuted (`[V0001]`), inspect `.proofs[].counterexample` or `lume agent repair` and strengthen `requires` or fix the body expression.
