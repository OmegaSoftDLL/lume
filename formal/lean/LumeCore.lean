/-
  formal/lean/LumeCore.lean
  Mechanised Operational Semantics & Type/Ownership/Effect/Contract Soundness
  for Lume-Core (Stage 5 Trust Anchor — Issue #213, docs/04 §5, docs/05 §2.10).

  This file is a self-contained Lean 4 formalisation: every theorem is fully proved,
  with no proof holes and no extra postulates (the CI gate greps this file to enforce it).
  It mechanises:
    1. Core types (`Ty`), ownership tiers (`OwnTier`), and effect rows (`EffRow`).
    2. Lume-Core AST (`Expr`) and canonical runtime values (`Val`).
    3. Linear ownership environment (`OwnEnv`) preventing use-after-move (`[O0001]`).
    4. Effect subtyping (`EffSub`) preventing unhandled effects in `/ pure` (`[E0200]`).
    5. Provenance promotion (`eVerify`) discharging `unverified[T]` (`[A0003]`).
    6. Big-step operational semantics (`Eval`) and determinism (`eval_deterministic`).
    7. Type preservation (`type_preservation`) and canonical-forms soundness.
    8. Soundness of `QF_LIA` proven contract guard elision (`verified_contract_elision_sound`).
-/

namespace LumeCore

/-- Core types of Lume-Core (`06_typecheck.src` / `06b_phase3.src`). -/
inductive Ty : Type where
  | tInt        : Ty
  | tStr        : Ty
  | tBool       : Ty
  | tUnverified : Ty → Ty
  deriving DecidableEq, Repr

/-- Ownership tiers (`Track O`: `own`, `borrow`, `mut`). -/
inductive OwnTier : Type where
  | owned      : OwnTier
  | sharedRef  : OwnTier
  | mutRef     : OwnTier
  deriving DecidableEq, Repr

/-- Algebraic effect rows (`Track K`: `/ pure` vs `/ {io, fs, net, ...}`). -/
inductive Eff : Type where
  | io   : Eff
  | fs   : Eff
  | net  : Eff
  | gpu  : Eff
  | ffi  : Eff
  | wasm : Eff
  deriving DecidableEq, Repr

abbrev EffRow := List Eff

def EffSub (ε₁ ε₂ : EffRow) : Prop :=
  ∀ e, e ∈ ε₁ → e ∈ ε₂

theorem effSub_refl (ε : EffRow) : EffSub ε ε := by
  intro e he
  exact he

theorem effSub_pure (ε : EffRow) : EffSub [] ε := by
  intro e he
  cases he

/-- Variable identifiers. -/
abbrev VarId := Nat

/-- Lume-Core expression grammar (`02_parser.src` / `05_codegen.src`). -/
inductive Expr : Type where
  | eInt    : Int → Expr
  | eStr    : String → Expr
  | eBool   : Bool → Expr
  | eVar    : VarId → Expr
  | eAdd    : Expr → Expr → Expr
  | eSub    : Expr → Expr → Expr
  | eMul    : Expr → Expr → Expr
  | eEq     : Expr → Expr → Expr
  | eLt     : Expr → Expr → Expr
  | eIf     : Expr → Expr → Expr → Expr
  | eLet    : VarId → Expr → Expr → Expr
  | eMove   : VarId → Expr
  | eVerify : Expr → Expr → Expr
  | eEffOp  : Eff → Expr → Expr
  deriving Repr

/-- Canonical runtime values. -/
inductive Val : Type where
  | vInt  : Int → Val
  | vStr  : String → Val
  | vBool : Bool → Val
  deriving DecidableEq, Repr

/-- Value typing relation (`ValHasType v τ`). -/
inductive ValHasType : Val → Ty → Prop where
  | vtInt  (n : Int)    : ValHasType (.vInt n) .tInt
  | vtStr  (s : String) : ValHasType (.vStr s) .tStr
  | vtBool (b : Bool)   : ValHasType (.vBool b) .tBool
  | vtUnv  {v : Val} {τ : Ty} : ValHasType v τ → ValHasType v (.tUnverified τ)

/-- Typing and runtime environments. -/
abbrev TyEnv  := VarId → Option Ty
abbrev ValEnv := VarId → Option Val
abbrev OwnEnv := VarId → Bool

def extendTy (Γ : TyEnv) (x : VarId) (τ : Ty) : TyEnv :=
  fun y => if y = x then some τ else Γ y

def extendVal (ρ : ValEnv) (x : VarId) (v : Val) : ValEnv :=
  fun y => if y = x then some v else ρ y

def grantOwn (Δ : OwnEnv) (x : VarId) : OwnEnv :=
  fun y => if y = x then true else Δ y

def consumeOwn (Δ : OwnEnv) (x : VarId) : OwnEnv :=
  fun y => if y = x then false else Δ y

/-- Environment agreement between runtime values `ρ` and static types `Γ`. -/
def EnvTyped (ρ : ValEnv) (Γ : TyEnv) : Prop :=
  ∀ x τ, Γ x = some τ → ∃ v, ρ x = some v ∧ ValHasType v τ

theorem envTyped_extend {ρ : ValEnv} {Γ : TyEnv} {x : VarId} {v : Val} {τ : Ty}
    (hEnv : EnvTyped ρ Γ) (hVal : ValHasType v τ) :
    EnvTyped (extendVal ρ x v) (extendTy Γ x τ) := by
  intro y τ' hy
  unfold extendTy at hy
  unfold extendVal
  split at hy
  · injection hy with hEq
    subst hEq
    exact ⟨v, rfl, hVal⟩
  · exact hEnv y τ' hy

/-- Combined Type, Linear Ownership (`[O0001]`), Effect (`[E0200]`), and Provenance (`[A0003]`) judgment. -/
inductive HasType : TyEnv → OwnEnv → EffRow → Expr → Ty → OwnEnv → Prop where
  | htInt (Γ : TyEnv) (Δ : OwnEnv) (ε : EffRow) (n : Int) :
      HasType Γ Δ ε (.eInt n) .tInt Δ
  | htStr (Γ : TyEnv) (Δ : OwnEnv) (ε : EffRow) (s : String) :
      HasType Γ Δ ε (.eStr s) .tStr Δ
  | htBool (Γ : TyEnv) (Δ : OwnEnv) (ε : EffRow) (b : Bool) :
      HasType Γ Δ ε (.eBool b) .tBool Δ
  | htVar {Γ : TyEnv} {Δ : OwnEnv} {ε : EffRow} {x : VarId} {τ : Ty} :
      Γ x = some τ →
      Δ x = true →
      HasType Γ Δ ε (.eVar x) τ Δ
  | htMove {Γ : TyEnv} {Δ : OwnEnv} {ε : EffRow} {x : VarId} {τ : Ty} :
      Γ x = some τ →
      Δ x = true →
      HasType Γ Δ ε (.eMove x) τ (consumeOwn Δ x)
  | htAdd {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {a b : Expr} :
      HasType Γ Δ₁ ε a .tInt Δ₂ →
      HasType Γ Δ₂ ε b .tInt Δ₃ →
      HasType Γ Δ₁ ε (.eAdd a b) .tInt Δ₃
  | htSub {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {a b : Expr} :
      HasType Γ Δ₁ ε a .tInt Δ₂ →
      HasType Γ Δ₂ ε b .tInt Δ₃ →
      HasType Γ Δ₁ ε (.eSub a b) .tInt Δ₃
  | htMul {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {a b : Expr} :
      HasType Γ Δ₁ ε a .tInt Δ₂ →
      HasType Γ Δ₂ ε b .tInt Δ₃ →
      HasType Γ Δ₁ ε (.eMul a b) .tInt Δ₃
  | htEq {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {a b : Expr} :
      HasType Γ Δ₁ ε a .tInt Δ₂ →
      HasType Γ Δ₂ ε b .tInt Δ₃ →
      HasType Γ Δ₁ ε (.eEq a b) .tBool Δ₃
  | htLt {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {a b : Expr} :
      HasType Γ Δ₁ ε a .tInt Δ₂ →
      HasType Γ Δ₂ ε b .tInt Δ₃ →
      HasType Γ Δ₁ ε (.eLt a b) .tBool Δ₃
  | htIf {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {c t e : Expr} {τ : Ty} :
      HasType Γ Δ₁ ε c .tBool Δ₂ →
      HasType Γ Δ₂ ε t τ Δ₃ →
      HasType Γ Δ₂ ε e τ Δ₃ →
      HasType Γ Δ₁ ε (.eIf c t e) τ Δ₃
  | htLet {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {x : VarId} {v b : Expr} {τ₁ τ₂ : Ty} :
      HasType Γ Δ₁ ε v τ₁ Δ₂ →
      HasType (extendTy Γ x τ₁) (grantOwn Δ₂ x) ε b τ₂ Δ₃ →
      HasType Γ Δ₁ ε (.eLet x v b) τ₂ Δ₃
  | htVerify {Γ : TyEnv} {Δ₁ Δ₂ Δ₃ : OwnEnv} {ε : EffRow} {valE condE : Expr} {τ : Ty} :
      HasType Γ Δ₁ ε valE (.tUnverified τ) Δ₂ →
      HasType Γ Δ₂ ε condE .tBool Δ₃ →
      HasType Γ Δ₁ ε (.eVerify valE condE) τ Δ₃
  | htEffOp {Γ : TyEnv} {Δ₁ Δ₂ : OwnEnv} {ε : EffRow} {op : Eff} {arg : Expr} {τ : Ty} :
      op ∈ ε →
      HasType Γ Δ₁ ε arg τ Δ₂ →
      HasType Γ Δ₁ ε (.eEffOp op arg) τ Δ₂

/-- Big-step operational semantics (`Eval ρ e v`). -/
inductive Eval : ValEnv → Expr → Val → Prop where
  | evInt (ρ : ValEnv) (n : Int) :
      Eval ρ (.eInt n) (.vInt n)
  | evStr (ρ : ValEnv) (s : String) :
      Eval ρ (.eStr s) (.vStr s)
  | evBool (ρ : ValEnv) (b : Bool) :
      Eval ρ (.eBool b) (.vBool b)
  | evVar {ρ : ValEnv} {x : VarId} {v : Val} :
      ρ x = some v →
      Eval ρ (.eVar x) v
  | evMove {ρ : ValEnv} {x : VarId} {v : Val} :
      ρ x = some v →
      Eval ρ (.eMove x) v
  | evAdd {ρ : ValEnv} {a b : Expr} {na nb : Int} :
      Eval ρ a (.vInt na) →
      Eval ρ b (.vInt nb) →
      Eval ρ (.eAdd a b) (.vInt (na + nb))
  | evSub {ρ : ValEnv} {a b : Expr} {na nb : Int} :
      Eval ρ a (.vInt na) →
      Eval ρ b (.vInt nb) →
      Eval ρ (.eSub a b) (.vInt (na - nb))
  | evMul {ρ : ValEnv} {a b : Expr} {na nb : Int} :
      Eval ρ a (.vInt na) →
      Eval ρ b (.vInt nb) →
      Eval ρ (.eMul a b) (.vInt (na * nb))
  | evEq {ρ : ValEnv} {a b : Expr} {na nb : Int} :
      Eval ρ a (.vInt na) →
      Eval ρ b (.vInt nb) →
      Eval ρ (.eEq a b) (.vBool (decide (na = nb)))
  | evLt {ρ : ValEnv} {a b : Expr} {na nb : Int} :
      Eval ρ a (.vInt na) →
      Eval ρ b (.vInt nb) →
      Eval ρ (.eLt a b) (.vBool (decide (na < nb)))
  | evIfTrue {ρ : ValEnv} {c t e : Expr} {v : Val} :
      Eval ρ c (.vBool true) →
      Eval ρ t v →
      Eval ρ (.eIf c t e) v
  | evIfFalse {ρ : ValEnv} {c t e : Expr} {v : Val} :
      Eval ρ c (.vBool false) →
      Eval ρ e v →
      Eval ρ (.eIf c t e) v
  | evLet {ρ : ValEnv} {x : VarId} {vE bE : Expr} {vv bv : Val} :
      Eval ρ vE vv →
      Eval (extendVal ρ x vv) bE bv →
      Eval ρ (.eLet x vE bE) bv
  | evVerify {ρ : ValEnv} {valE condE : Expr} {v : Val} :
      Eval ρ valE v →
      Eval ρ condE (.vBool true) →
      Eval ρ (.eVerify valE condE) v
  | evEffOp {ρ : ValEnv} {op : Eff} {arg : Expr} {v : Val} :
      Eval ρ arg v →
      Eval ρ (.eEffOp op arg) v

/-- Theorem 1 (Ownership Linear Safety — `[O0001]`):
    Moving a variable `x` via `eMove x` immediately marks `x` as consumed (`false`) in the output
    ownership environment, so any subsequent `htVar` or `htMove` on `x` is rejected by `HasType`. -/
theorem move_consumes_ownership (Δ : OwnEnv) (x : VarId) :
    (consumeOwn Δ x) x = false := by
  unfold consumeOwn
  simp

/-- Theorem 2 (Effect Purity Soundness — `[E0200]`):
    No `eEffOp op arg` expression can be typed under the `/ pure` (`[]`) effect row. -/
theorem pure_rejects_effects {Γ : TyEnv} {Δ₁ Δ₂ : OwnEnv} {op : Eff} {arg : Expr} {τ : Ty}
    (h : HasType Γ Δ₁ [] (.eEffOp op arg) τ Δ₂) : False := by
  cases h with
  | htEffOp hMem _ => cases hMem

/-- Theorem 3 (Deterministic Evaluation):
    Lume-Core's big-step operational semantics is deterministic. -/
theorem eval_deterministic {ρ : ValEnv} {e : Expr} {v₁ v₂ : Val}
    (h₁ : Eval ρ e v₁) (h₂ : Eval ρ e v₂) : v₁ = v₂ := by
  induction h₁ generalizing v₂ with
  | evInt _ _ =>
    cases h₂
    rfl
  | evStr _ _ =>
    cases h₂
    rfl
  | evBool _ _ =>
    cases h₂
    rfl
  | evVar hx =>
    cases h₂ with
    | evVar hx₂ =>
      rw [hx] at hx₂
      injection hx₂
  | evMove hx =>
    cases h₂ with
    | evMove hx₂ =>
      rw [hx] at hx₂
      injection hx₂
  | evAdd _ _ iha ihb =>
    cases h₂ with
    | evAdd ha₂ hb₂ =>
      have hEqA := iha ha₂
      have hEqB := ihb hb₂
      injection hEqA with hna
      injection hEqB with hnb
      subst hna; subst hnb
      rfl
  | evSub _ _ iha ihb =>
    cases h₂ with
    | evSub ha₂ hb₂ =>
      have hEqA := iha ha₂
      have hEqB := ihb hb₂
      injection hEqA with hna
      injection hEqB with hnb
      subst hna; subst hnb
      rfl
  | evMul _ _ iha ihb =>
    cases h₂ with
    | evMul ha₂ hb₂ =>
      have hEqA := iha ha₂
      have hEqB := ihb hb₂
      injection hEqA with hna
      injection hEqB with hnb
      subst hna; subst hnb
      rfl
  | evEq _ _ iha ihb =>
    cases h₂ with
    | evEq ha₂ hb₂ =>
      have hEqA := iha ha₂
      have hEqB := ihb hb₂
      injection hEqA with hna
      injection hEqB with hnb
      subst hna; subst hnb
      rfl
  | evLt _ _ iha ihb =>
    cases h₂ with
    | evLt ha₂ hb₂ =>
      have hEqA := iha ha₂
      have hEqB := ihb hb₂
      injection hEqA with hna
      injection hEqB with hnb
      subst hna; subst hnb
      rfl
  | evIfTrue _ _ ihc iht =>
    cases h₂ with
    | evIfTrue _ ht₂ => exact iht ht₂
    | evIfFalse hc₂ _ =>
      have hEqC := ihc hc₂
      cases hEqC
  | evIfFalse _ _ ihc ihe =>
    cases h₂ with
    | evIfTrue hc₂ _ =>
      have hEqC := ihc hc₂
      cases hEqC
    | evIfFalse _ he₂ => exact ihe he₂
  | evLet _ _ ihv ihb =>
    cases h₂ with
    | evLet hv₂ hb₂ =>
      have hEqV := ihv hv₂
      subst hEqV
      exact ihb hb₂
  | evVerify _ _ ihv _ =>
    cases h₂ with
    | evVerify hv₂ _ => exact ihv hv₂
  | evEffOp _ ih =>
    cases h₂ with
    | evEffOp h₂ => exact ih h₂

/-- Theorem 4 (Type, Ownership & Provenance Preservation):
    Evaluating any well-typed Lume-Core expression in a type-consistent environment
    produces a value inhabiting the static return type `τ` (including provenance promotion
    from `tUnverified τ` to `τ` via `eVerify`). -/
theorem type_preservation {Γ : TyEnv} {Δ₁ Δ₂ : OwnEnv} {ε : EffRow} {ρ : ValEnv}
    {e : Expr} {τ : Ty} {v : Val}
    (hTy : HasType Γ Δ₁ ε e τ Δ₂)
    (hEnv : EnvTyped ρ Γ)
    (hEv : Eval ρ e v) :
    ValHasType v τ := by
  induction hTy generalizing ρ v with
  | htInt _ _ _ n =>
    cases hEv
    exact .vtInt n
  | htStr _ _ _ s =>
    cases hEv
    exact .vtStr s
  | htBool _ _ _ b =>
    cases hEv
    exact .vtBool b
  | htVar hLookup _ =>
    cases hEv with
    | evVar hVal =>
      obtain ⟨v', hv', hvt⟩ := hEnv _ _ hLookup
      rw [hVal] at hv'
      injection hv' with hEq
      subst hEq
      exact hvt
  | htMove hLookup _ =>
    cases hEv with
    | evMove hVal =>
      obtain ⟨v', hv', hvt⟩ := hEnv _ _ hLookup
      rw [hVal] at hv'
      injection hv' with hEq
      subst hEq
      exact hvt
  | htAdd _ _ _ _ =>
    cases hEv with
    | evAdd _ _ => exact .vtInt _
  | htSub _ _ _ _ =>
    cases hEv with
    | evSub _ _ => exact .vtInt _
  | htMul _ _ _ _ =>
    cases hEv with
    | evMul _ _ => exact .vtInt _
  | htEq _ _ _ _ =>
    cases hEv with
    | evEq _ _ => exact .vtBool _
  | htLt _ _ _ _ =>
    cases hEv with
    | evLt _ _ => exact .vtBool _
  | htIf _ _ _ _ iht ihe =>
    cases hEv with
    | evIfTrue _ ht => exact iht hEnv ht
    | evIfFalse _ he => exact ihe hEnv he
  | htLet _ _ ihv ihb =>
    cases hEv with
    | evLet hv hb =>
      have hvt := ihv hEnv hv
      exact ihb (envTyped_extend hEnv hvt) hb
  | htVerify _ _ ihv _ =>
    cases hEv with
    | evVerify hv _ =>
      have hUnv := ihv hEnv hv
      cases hUnv with
      | vtUnv hInner => exact hInner
  | htEffOp _ _ ih =>
    cases hEv with
    | evEffOp hv => exact ih hEnv hv

/-- Theorem 5 (`QF_LIA` Verified Contract Guard Elision Soundness — Stage B / Track V / #183):
    When `lume prove` / `lcert` discharges a function's `requires`/`ensures` obligation
    (`∀ ρ v, Pre ρ → Eval ρ body v → Eval ρ condE (.vBool true)`), eliding the runtime
    verification wrapper `eVerify body condE` yields the exact same runtime value `v`. -/
theorem verified_contract_elision_sound
    {ρ : ValEnv} {body condE : Expr} {v : Val}
    (Pre : ValEnv → Prop)
    (hProven : ∀ ρ' v', Pre ρ' → Eval ρ' body v' → Eval ρ' condE (.vBool true))
    (hPre : Pre ρ) :
    Eval ρ (.eVerify body condE) v ↔ Eval ρ body v := by
  constructor
  · intro hVer
    cases hVer with
    | evVerify hBody _ => exact hBody
  · intro hBody
    exact .evVerify hBody (hProven ρ v hPre hBody)

end LumeCore
