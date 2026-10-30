#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
LUME=${LUME_CLI:-$ROOT/lume}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
STORE="$TMP/store"
export LUME_REGISTRY_DIR="$STORE"

mkdir -p "$TMP/input"
cat > "$TMP/input/adapter.src" <<'EOF'
pub fn adapter_twice(x: int) -> int requires x >= 0 ensures result >= x = x + x
EOF
cat > "$TMP/input/source.src" <<'EOF'
pub fn component_clamp(x: int, lo: int, hi: int) -> int requires lo <= hi ensures result >= lo and result <= hi = if x < lo then lo else if x > hi then hi else x
EOF
cat > "$TMP/input/tests.src" <<'EOF'
fn test_adapter_twice() = adapter_twice(3) == 6
fn test_component_clamp_low() = component_clamp(0 - 4, 0, 10) == 0
fn test_component_clamp_high() = component_clamp(40, 0, 10) == 10
fn test_component_clamp_inner() = component_clamp(4, 0, 10) == 4
EOF

"$LUME" registry publish --adapter "$TMP/input/adapter.src" --source "$TMP/input/source.src" --tests "$TMP/input/tests.src" > "$TMP/publish.json"
python3 - "$TMP/publish.json" "$TMP/input" "$STORE" <<'PY'
import hashlib
import json
import pathlib
import sys

publish_path = pathlib.Path(sys.argv[1])
input_dir = pathlib.Path(sys.argv[2])
store = pathlib.Path(sys.argv[3])
manifest = json.loads(publish_path.read_text(encoding="utf-8"))
assert manifest["schema"] == "lume-verified-component/0.1"
assert manifest["evidence"]["status"] == "PASS"
assert manifest["public_contracts"] == 2
entry = store / "components" / manifest["cid"]
assert entry.is_dir()
for artifact, filename in [
    ("adapter_sha256", "adapter.src"),
    ("source_sha256", "source.src"),
    ("tests_sha256", "tests.src"),
]:
    actual = "sha256:" + hashlib.sha256((input_dir / filename).read_bytes()).hexdigest()
    assert manifest["artifacts"][artifact] == actual
for artifact, filename in [
    ("proof_sha256", "proof.cert.json"),
    ("test_output_sha256", "test-output.txt"),
]:
    actual = "sha256:" + hashlib.sha256((entry / filename).read_bytes()).hexdigest()
    assert manifest["artifacts"][artifact] == actual
material = (
    "lume-verified-component/0.1\n"
    f"adapter_sha256={manifest['artifacts']['adapter_sha256']}\n"
    f"source_sha256={manifest['artifacts']['source_sha256']}\n"
    f"proof_sha256={manifest['artifacts']['proof_sha256']}\n"
    f"tests_sha256={manifest['artifacts']['tests_sha256']}\n"
    f"test_output_sha256={manifest['artifacts']['test_output_sha256']}\n"
    f"target={manifest['target']}\n"
    f"toolchain_sha256={manifest['toolchain_sha256']}\n"
    f"capabilities={','.join(manifest['capabilities'])}\n"
    f"contract_signature={manifest['contract_signature']}\n"
    f"public_contracts={manifest['public_contracts']}\n"
    f"proof_kernel={manifest['evidence']['proof_kernel']}\n"
)
expected_cid = "sha256:" + hashlib.sha256(material.encode("ascii")).hexdigest()
assert manifest["cid"] == expected_cid
pathlib.Path(sys.argv[1] + ".cid").write_text(expected_cid, encoding="ascii")
PY
CID=$(cat "$TMP/publish.json.cid")
ENTRY="$STORE/components/$CID"
SIGNATURE=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1], encoding="utf-8"))["contract_signature"])' "$TMP/publish.json")

"$LUME" registry resolve --cid "$CID" > "$TMP/resolve.json"
cmp -s "$TMP/publish.json" "$TMP/resolve.json"
"$LUME" registry discover --capability pure --contract-signature "$SIGNATURE" > "$TMP/discover.json"
"$LUME" registry discover --capability ffi > "$TMP/discover-other.json"
python3 - "$TMP/discover.json" "$TMP/discover-other.json" "$CID" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as f:
    found = json.load(f)
with open(sys.argv[2], encoding="utf-8") as f:
    other = json.load(f)
assert [item["cid"] for item in found["components"]] == [sys.argv[3]]
assert other["components"] == []
PY

if "$LUME" registry resolve --cid "$CID" --target wasm32-wasi > "$TMP/target.json"; then
    echo "target-incompatible component resolved" >&2
    exit 1
fi
grep -q '"code":"R0004"' "$TMP/target.json"

python3 - "$TMP/forged.cert.json" "$ENTRY/proof.cert.json" <<'PY'
import hashlib
import json
import sys
with open(sys.argv[2], encoding="utf-8") as f:
    cert = json.load(f)
cert["proof_terms"][0]["term"] = "(implies (and) (>= 0 1))"
terms = json.dumps(cert["proof_terms"], separators=(",", ":"))
obligations = json.dumps(cert["obligations"], separators=(",", ":"))
payload = cert["source_hash"] + "|" + terms + "|" + obligations
cert["certificate_hash"] = "sha256:" + hashlib.sha256(payload.encode("utf-8")).hexdigest()
with open(sys.argv[1], "w", encoding="utf-8") as f:
    json.dump(cert, f, separators=(",", ":"))
    f.write("\n")
PY

if "$LUME" registry publish --adapter "$TMP/input/adapter.src" --source "$TMP/input/source.src" --tests "$TMP/input/tests.src" --proof "$TMP/forged.cert.json" > "$TMP/forged-publish.json"; then
    echo "forged and maliciously rehashed certificate was published" >&2
    exit 1
fi
grep -q '"code":"R0002"' "$TMP/forged-publish.json"
[ "$(find "$STORE/components" -mindepth 1 -maxdepth 1 -type d -name 'sha256:*' | wc -l)" -eq 1 ]

cp "$ENTRY/manifest.json" "$TMP/manifest.good.json"
python3 - "$ENTRY/manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as f:
    manifest = json.load(f)
manifest["artifacts"]["adapter_sha256"] = "sha256:" + "0" * 64
with open(sys.argv[1], "w", encoding="utf-8") as f:
    json.dump(manifest, f, separators=(",", ":"))
    f.write("\n")
PY
if "$LUME" registry resolve --cid "$CID" > "$TMP/adapter-hash.json"; then
    echo "mismatched adapter hash resolved" >&2
    exit 1
fi
grep -q '"code":"R0003"' "$TMP/adapter-hash.json"
cp "$TMP/manifest.good.json" "$ENTRY/manifest.json"

cp "$ENTRY/source.src" "$TMP/source.good.src"
printf '\n## stale source change\n' >> "$ENTRY/source.src"
if "$LUME" registry resolve --cid "$CID" > "$TMP/stale-proof.json"; then
    echo "stale proof resolved after source changed" >&2
    exit 1
fi
grep -q '"code":"R0002"' "$TMP/stale-proof.json"
cp "$TMP/source.good.src" "$ENTRY/source.src"

cp "$ENTRY/test-output.txt" "$TMP/test-output.good.txt"
printf 'forged test verdict\n' >> "$ENTRY/test-output.txt"
if "$LUME" registry resolve --cid "$CID" > "$TMP/stale-tests.json"; then
    echo "stale test evidence resolved" >&2
    exit 1
fi
grep -q '"code":"R0005"' "$TMP/stale-tests.json"
cp "$TMP/test-output.good.txt" "$ENTRY/test-output.txt"

python3 - "$ENTRY/manifest.json" <<'PY'
import json
import sys
with open(sys.argv[1], encoding="utf-8") as f:
    manifest = json.load(f)
manifest["toolchain_sha256"] = "sha256:" + "0" * 64
with open(sys.argv[1], "w", encoding="utf-8") as f:
    json.dump(manifest, f, separators=(",", ":"))
    f.write("\n")
PY
if "$LUME" registry resolve --cid "$CID" > "$TMP/stale-toolchain.json"; then
    echo "stale toolchain evidence resolved" >&2
    exit 1
fi
grep -q '"code":"R0004"' "$TMP/stale-toolchain.json"
cp "$TMP/manifest.good.json" "$ENTRY/manifest.json"

ATTACK="$STORE/components/.attack"
cp -R "$ENTRY" "$ATTACK"
printf '999 passed, 0 failed\n' > "$ATTACK/test-output.txt"
python3 - "$ATTACK" "$TMP/attack.cid" <<'PY'
import hashlib
import json
import pathlib
import sys
entry = pathlib.Path(sys.argv[1])
manifest_path = entry / "manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
manifest["artifacts"]["test_output_sha256"] = "sha256:" + hashlib.sha256((entry / "test-output.txt").read_bytes()).hexdigest()
material = (
    "lume-verified-component/0.1\n"
    f"adapter_sha256={manifest['artifacts']['adapter_sha256']}\n"
    f"source_sha256={manifest['artifacts']['source_sha256']}\n"
    f"proof_sha256={manifest['artifacts']['proof_sha256']}\n"
    f"tests_sha256={manifest['artifacts']['tests_sha256']}\n"
    f"test_output_sha256={manifest['artifacts']['test_output_sha256']}\n"
    f"target={manifest['target']}\n"
    f"toolchain_sha256={manifest['toolchain_sha256']}\n"
    f"capabilities={','.join(manifest['capabilities'])}\n"
    f"contract_signature={manifest['contract_signature']}\n"
    f"public_contracts={manifest['public_contracts']}\n"
    f"proof_kernel={manifest['evidence']['proof_kernel']}\n"
)
manifest["cid"] = "sha256:" + hashlib.sha256(material.encode("ascii")).hexdigest()
manifest_path.write_text(json.dumps(manifest, separators=(",", ":")) + "\n", encoding="utf-8")
pathlib.Path(sys.argv[2]).write_text(manifest["cid"], encoding="ascii")
PY
ATTACK_CID=$(cat "$TMP/attack.cid")
mv "$ATTACK" "$STORE/components/$ATTACK_CID"
if "$LUME" registry resolve --cid "$ATTACK_CID" > "$TMP/malicious-recompute.json"; then
    echo "maliciously recomputed CID bypassed test re-execution" >&2
    exit 1
fi
grep -q '"code":"R0005"' "$TMP/malicious-recompute.json"

cat > "$TMP/input/failing-tests.src" <<'EOF'
fn test_adapter_twice_is_wrong() = adapter_twice(1) == 3
EOF
if "$LUME" registry publish --adapter "$TMP/input/adapter.src" --source "$TMP/input/source.src" --tests "$TMP/input/failing-tests.src" > "$TMP/failing-tests-publish.json"; then
    echo "component with failing tests was published" >&2
    exit 1
fi
grep -q '"code":"R0005"' "$TMP/failing-tests-publish.json"

"$LUME" registry resolve --cid "$CID" > "$TMP/final-resolve.json"
cmp -s "$TMP/publish.json" "$TMP/final-resolve.json"
printf 'verified registry: publication, discovery, resolution, and adversarial checks passed\n'
