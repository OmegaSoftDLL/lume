# 24 — Phase 4 Plan & Architecture: AI, Tensors, Autodiff, Prob, Rules, `extern llm fn`, and Spiking Networks

Phase 4 (Stage 4 of `docs/04-roadmap.md` and `docs/03-specification.md` §14) brings first-class AI and differentiable/probabilistic/symbolic constructs into the self-hosted Lume compiler (`lumec`) and `lcaas` without compromising Lume's core invariants (`stage2 == stage3` bit-identical bootstrap, Zero-C steady state, and sound `QF_LIA` verification).

---

## 1. Track N1 — Typed Tensors, Symbolic Shapes (`[T0300]`) & `std.tensor`

### 1.1 Syntax & Symbolic Shape Unification
- **Type syntax (`04_parser_decls.src`, `lcaas.src`):**
  - `tensor[T, d1, d2, ...]` parses into `TyTensor(elem_ty, dims)` where `dims` is a list of dimension symbols (`"784"`, `"128"`, `"batch"`, `"n"`).
- **Compile-time shape checking (`06_typecheck.src`, `06b_phase3.src`, `lcaas.src`):**
  - Unifies symbolic dimensions across function parameters and return annotations (e.g. `fn forward(m: Mlp, x: tensor[f32, batch, 4]) -> tensor[f32, batch, 4]`).
  - Verifies 2D matrix multiplication (`matmul(A, B)` / `tensor_matmul(A, B)`): requires `A: tensor[T, m, k]` and `B: tensor[T, k2, n]` with `k == k2`, producing `tensor[T, m, n]`.
  - Verifies elementwise operations (`tensor_add`, `tensor_sub`, `tensor_mul`, `tensor_add_bias`): requires matching shapes `[m, n]` vs `[m, n]` or row-broadcast `[1, n]`.
  - Any concrete or unified symbolic dimension mismatch fails at compile time with `[T0300]: tensor shape mismatch ... (fix-it: ...)` (exit 3, zero assembly emitted, and structured JSON diagnostic in `lcaas` / `lume agent`).

### 1.2 `compiler/stage1/tensor.src` (`use tensor`)
- Deterministic 1D/2D tensor representation `{rows: int, cols: int, scale: int, data: [int]}` (supporting exact integer and fixed-point `f32` milliscale arithmetic):
  - Construction & access: `tensor_zeros`, `tensor_ones`, `tensor_vec`, `tensor_mat`, `tensor_get`, `tensor_set`, `tensor_shape_str`.
  - Linear algebra & broadcasting: `matmul`, `tensor_transpose`, `tensor_add`, `tensor_sub`, `tensor_mul`, `tensor_add_bias`, `tensor_scale`, `einsum_ij_jk`, `einsum_trace`, `einsum_batch_dot`.
  - Activations & reductions: `tensor_relu`, `tensor_sum`, `tensor_mean`, `tensor_argmax_row`.

---

## 2. Track N2 — `model` Types, Autodiff (`grad`/`vjp`/`jvp`/`vmap`/`jit`) & `kernel fn`

### 2.1 `model` Declarations & Autodiff Transform
- **`type Name = model { ... }` (`04_parser_decls.src`, `06b_phase3.src`):**
  - Declares a differentiable parameter record (`OType(Name, "model")`) whose tensor fields form a parameter tree.
- **Compiler Autodiff (`grad`, `vjp`, `jvp`, `vmap`, `jit` in `06b_phase3.src` & `lcaas.src`):**
  - `grad(f)` synthesizes a symbolic derivative function `__ad_grad_<f>` by differentiating pure scalar/piecewise expressions (`Num`, `Var`, `+`, `-`, `*`, and `If(c, t, e)` piecewise branches such as `relu`) with respect to the first parameter, or dispatches to analytical `grad_<f>` for tensor models.
  - `jvp(f, x, dx)` computes forward-mode directional derivative `grad(f)(x) * dx`; `vjp(f, x, dy)` computes reverse-mode pullback `dy * grad(f)(x)`; `vmap(f, xs)` maps `f` across a batch list.
- **Accelerator Kernels (`kernel fn`, `@schedule`, `@autotune`):**
  - `kernel fn` declarations are statically checked for accelerator purity (rejecting `io`, `fs`, `net` side effects inside a kernel with `[E0200]`) and can be lowered to PTX (`kernel_emit_ptx`) and SPIR-V assembly (`kernel_emit_spirv`).

### 2.2 `compiler/stage1/nn.src` (`use nn`)
- 2-layer MLP (`mlp2_new`, `mlp2_forward`, `mlp2_loss`, `mlp2_grad`, `mlp2_accuracy`), optimisers (`sgd_step_mlp2`, `adam_init_mlp2`, `adam_step_mlp2`), and content-addressed serialisation (`model_content_hash`, `model_save_safetensors`, `model_load_safetensors`, `model_export_onnx`).

---

## 3. Track N3 — `prob` Effect, Datalog `rule`, `extern llm fn` & `std.neuro`

### 3.1 Probabilistic Programming (`prob` Effect)
- Builtin `prob` effect (`sample_dist`, `observe_lik`) discharged by inference handlers (`infer_mcmc`, `infer_svi`, `infer_smc`) over `Beta`, `Bernoulli`, and `Normal` distributions.

### 3.2 Datalog `rule` Declarations (`std.rules`)
- Top-level `rule name(params) = expr` (and `rule name(params) when cond = expr`) compiles to `NRuleDecl` + callable `rule_<name>` predicates and semi-naive relational fixpoint evaluation (`datalog_fixpoint`) over tables/relations.

### 3.3 Schema-Constrained `extern llm fn` & Deterministic Replay (`std.llm`)
- `extern llm fn name(params) -> Ret / {llm} requires ... ensures ...` declares an AI-backed function whose signature defines its decoding schema and whose `ensures` clause validates output before discharging `unverified[Ret]` provenance (`[A0003]`).
- Includes BAML-style schema-aligned fallback + contract repair loop (`llm_call_with_repair`), budget enforcement (`llm_local_handler`, `llm_budget_charge`), and deterministic record/replay evaluation (`llm_eval_suite`).

### 3.4 Spiking Neural Networks (`std.neuro`)
- Leaky Integrate-and-Fire (`lif_neuron_new`, `lif_step`, `spiking_network_run`) operating over deterministic logical time (`std.time.logical`).

---

## 4. Stage 4 Exit-Criterion Capstones (`docs/04-roadmap.md` §Stage 4.9)
1. **`examples/apps/mnist/`:** End-to-end MLP digit classifier trained in pure Lume with `grad` + `Adam`, verified contracts (`lume prove`), `safetensors` + `onnx` export, and 100% test suite pass.
2. **`examples/apps/agent/`:** Autonomous invoice & policy reconciliation agent combining typed tools, `table` memory, Datalog `rule` inference, `extern llm fn` planner with contract-driven repair, `supervisor` fault tolerance, and deterministic `test llm` replay evaluation.
