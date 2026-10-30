#!/usr/bin/env sh
set -u

ulimit -s unlimited 2>/dev/null || ulimit -s 524288 2>/dev/null || ulimit -s 65520 2>/dev/null || true
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)
FIX="$ROOT/tests/conformance/stage1/semantic_mesh"
OUT=$(mktemp -d)
if [ -z "${LUME_BIN_DIR:-}" ]; then
    LUME_BIN_DIR="$OUT/bin"
    export LUME_BIN_DIR
fi
pass=0
fail=0
ok() { pass=$((pass + 1)); printf '  ok   %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf '  FAIL %s\n' "$1"; }

"$ROOT/lume" build "$FIX/assertions.src" -o "$OUT/lmesh_assert" >"$OUT/assertions-build.out" 2>"$OUT/assertions-build.err" || {
    cat "$OUT/assertions-build.out" "$OUT/assertions-build.err"
    exit 1
}

_mesh_invoke() {
    _mesh_name=$1
    _mesh_rc=0
    "$ROOT/lume" mesh "$FIX/$_mesh_name.mesh" >"$OUT/$_mesh_name.json" 2>"$OUT/$_mesh_name.err" || _mesh_rc=$?
}
_mesh_ok() {
    _mesh_invoke "$1"
    [ "$_mesh_rc" -eq 0 ] && printf '@mode %s\n@path %s\n' "$2" "$OUT/$1.json" | "$OUT/lmesh_assert"
}
_mesh_err() {
    _mesh_invoke "$1"
    [ "$_mesh_rc" -ne 0 ] && printf '@mode error\n@path %s\n@other \n@code %s\n' "$OUT/$1.json" "$2" | "$OUT/lmesh_assert"
}

if _mesh_ok negotiate negotiate; then ok "P0 mesh (#337): negotiate goal/contracts/effects/resources/target with real compiler, target, and prover"; else bad "P0 mesh negotiation"; cat "$OUT/negotiate.json" "$OUT/negotiate.err" 2>/dev/null; fi
if [ -z "${LUME0_ASAN:-}" ]; then
    if _mesh_ok transact transact; then ok "P0 mesh (#337): AST add/replace contract, set target, rollback dependents, and branch/replay parent lineage"; else bad "P0 mesh transaction and lineage"; cat "$OUT/transact.json" "$OUT/transact.err" 2>/dev/null; fi
fi
if _mesh_err invalid_mode M1003; then ok "P0 mesh negative: unknown mode is structured and fail-closed"; else bad "P0 mesh unknown mode"; cat "$OUT/invalid_mode.json" 2>/dev/null; fi
if _mesh_err ambiguous_payload M1002; then ok "P0 mesh negative: duplicate negotiation fields are ambiguous and rejected"; else bad "P0 mesh ambiguous payload"; cat "$OUT/ambiguous_payload.json" 2>/dev/null; fi
if _mesh_err unknown_operation M1003; then ok "P0 mesh negative: unknown transaction operation cannot fall through to success"; else bad "P0 mesh unknown operation"; cat "$OUT/unknown_operation.json" 2>/dev/null; fi
if [ -z "${LUME0_ASAN:-}" ]; then
    if _mesh_err duplicate_symbols M2002; then ok "P0 mesh negative: duplicate function names are rejected before AST edits"; else bad "P0 mesh duplicate symbols"; cat "$OUT/duplicate_symbols.json" 2>/dev/null; fi
    if _mesh_err capability_violation M2004; then ok "P0 mesh negative: effects and module grants cannot exceed negotiated capabilities"; else bad "P0 mesh capability violation"; cat "$OUT/capability_violation.json" 2>/dev/null; fi
    if _mesh_err top_level_capability M2004; then ok "P0 mesh negative: top-level expression effects cannot bypass capability negotiation"; else bad "P0 mesh top-level capability violation"; cat "$OUT/top_level_capability.json" 2>/dev/null; fi
    if _mesh_err top_level_expression M1004; then ok "P0 mesh negative: backend-dropped top-level expressions are rejected, never reported as supported"; else bad "P0 mesh top-level expression rejection"; cat "$OUT/top_level_expression.json" 2>/dev/null; fi
    if _mesh_err invalid_target M2003; then ok "P0 mesh negative: invalid target is rejected by the bounded protocol target validator"; else bad "P0 mesh invalid target"; cat "$OUT/invalid_target.json" 2>/dev/null; fi
    if _mesh_err false_contract V0001; then ok "P0 mesh negative: actual QF_LIA prover refutes a false contract"; else bad "P0 mesh false contract"; cat "$OUT/false_contract.json" 2>/dev/null; fi
    if _mesh_ok out_of_fragment out_of_fragment; then ok "P0 mesh proof boundary: nonlinear obligation stays runtime_checked, never promoted to proven"; else bad "P0 mesh out-of-fragment proof status"; cat "$OUT/out_of_fragment.json" 2>/dev/null; fi
fi
if _mesh_err ambiguous_contract M1002; then ok "P0 mesh negative: trailing contract tokens are rejected as ambiguous payload"; else bad "P0 mesh ambiguous contract"; cat "$OUT/ambiguous_contract.json" 2>/dev/null; fi
if [ -z "${LUME0_ASAN:-}" ]; then
    _mesh_invoke canonical_a
    _mesh_ca_rc=$_mesh_rc
    _mesh_invoke canonical_b
    _mesh_cb_rc=$_mesh_rc
    if [ "$_mesh_ca_rc" -eq 0 ] && [ "$_mesh_cb_rc" -eq 0 ] && printf '@mode canonical\n@path %s\n@other %s\n' "$OUT/canonical_a.json" "$OUT/canonical_b.json" | "$OUT/lmesh_assert"; then ok "P0 mesh canonicalization: independent length-prefixed state and parent/node hashes resist delimiter adversaries"; else bad "P0 mesh canonicalization adversaries"; cat "$OUT/canonical_a.json" "$OUT/canonical_b.json" 2>/dev/null; fi
fi
_mesh_cli_rc=0
"$ROOT/lume" mesh --fixed-success >"$OUT/cli_bad.out" 2>"$OUT/cli_bad.err" || _mesh_cli_rc=$?
if [ "$_mesh_cli_rc" -ne 0 ]; then ok "P0 mesh CLI negative: unknown arguments are not discarded"; else bad "P0 mesh CLI unknown arguments"; fi

printf 'semantic mesh: %d passed, %d failed\n' "$pass" "$fail"
rm -rf "$OUT"
[ "$fail" -eq 0 ]
