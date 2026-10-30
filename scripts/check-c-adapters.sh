#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
FIXTURES="$ROOT/tests/fixtures/c_adapter"
LUME=${LUME_CLI:-$ROOT/lume}

if [ "$(uname -s)" != "Linux" ] || [ "$(uname -m)" != "x86_64" ]; then
    echo "C adapter interop tests require Linux x86-64" >&2
    exit 77
fi
command -v python3 >/dev/null 2>&1 || {
    echo "missing C adapter test dependency: python3" >&2
    exit 77
}
${CC:-cc} --version >/dev/null 2>&1 || {
    echo "missing C adapter test dependency: ${CC:-cc}" >&2
    exit 77
}

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
if [ -z "${LUME_BIN_DIR:-}" ]; then
    LUME_BIN_DIR="$TMP/toolchain"
    export LUME_BIN_DIR
fi

"${CC:-cc}" -shared -fPIC "$FIXTURES/buffer_math.c" -o "$TMP/libc_adapter_fixture.so"
"$LUME" adapt c --json "$FIXTURES/buffer_math.h" > "$TMP/adapter.json"
python3 - "$FIXTURES/buffer_math.h" "$TMP/adapter.json" "$TMP/generated.src" <<'PY'
import hashlib
import json
import pathlib
import sys

header = pathlib.Path(sys.argv[1]).read_bytes()
adapter = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
assert adapter["ok"] is True
assert adapter["adapter_contract"] == "c_abi_foreign_v2"
assert adapter["source_hash"] == "sha256:" + hashlib.sha256(header).hexdigest()
assert adapter["capabilities"] == ["ffi"]
assert adapter["provenance_guard"] == "A0003"
assert adapter["return_provenance"] == "unverified"
assert adapter["diagnostics"] == []
assert len(adapter["adapters"]) == 2
buffer_fn = adapter["adapters"][0]
assert buffer_fn["symbol"] == "c_buffer_weighted_sum"
assert buffer_fn["return"] == {
    "c_type": "int64_t",
    "lume_type": "unverified[int]",
    "provenance": "foreign",
}
assert buffer_fn["effects"] == ["ffi"]
assert buffer_fn["parameters"][0] == {
    "name": "bytes",
    "c_type": "const char *",
    "lume_type": "str",
    "ownership": "borrowed_read_only",
    "lifetime": "call",
    "extent": "count(c_bytes)",
    "length_parameter": "bytes_len",
}
assert buffer_fn["parameters"][1]["ownership"] == "value"
assert adapter["extern_decls"][0] == (
    'extern "c" fn c_buffer_weighted_sum('
    "c_bytes: str, c_bytes_len: int, c_weight: int) -> int"
)
assert adapter["foreign_wrappers"][0] == (
    "pub fn foreign_c_buffer_weighted_sum(c_bytes: str, c_weight: int) -> "
    "unverified[int] / {ffi} = c_buffer_weighted_sum(c_bytes, count(c_bytes), c_weight)"
)
assert "verify(" not in adapter["generated_lume"]
assert "ensures" not in adapter["generated_lume"]
pathlib.Path(sys.argv[3]).write_text(adapter["generated_lume"] + "\n", encoding="utf-8")
PY

"$LUME" adapt c "$FIXTURES/buffer_math.h" > "$TMP/generated-stdout.src"
"$LUME" adapt c "$FIXTURES/buffer_math.h" -o "$TMP/generated-output.src"
cmp "$TMP/generated.src" "$TMP/generated-stdout.src"
cmp "$TMP/generated.src" "$TMP/generated-output.src"
cp "$FIXTURES/lume.toml" "$TMP/lume.toml"
{
    printf '@capability(allow: ["ffi"])\n'
    cat "$TMP/generated.src"
    cat "$FIXTURES/caller_tail.src"
} > "$TMP/caller.src"
"$LUME" build "$TMP/caller.src" -o "$TMP/caller"
LD_LIBRARY_PATH="$TMP:${LD_LIBRARY_PATH:-}" "$TMP/caller" > "$TMP/actual-output.txt"
diff -u "$FIXTURES/expected_output.txt" "$TMP/actual-output.txt"

{
    printf '@capability(allow: ["ffi"])\n'
    cat "$TMP/generated.src"
    printf 'pub fn bad_trusted_return(bytes: str) -> int / {ffi} = foreign_c_buffer_weighted_sum(bytes, 1)\nprint 0\n'
} > "$TMP/bad-provenance.src"
if "$LUME" build "$TMP/bad-provenance.src" -o "$TMP/bad-provenance" > "$TMP/bad-provenance.out" 2> "$TMP/bad-provenance.err"; then
    echo "foreign provenance unexpectedly discharged" >&2
    exit 1
fi
grep -q 'A0003' "$TMP/bad-provenance.err"
[ ! -e "$TMP/bad-provenance" ]

for header in "$FIXTURES"/invalid/*.h; do
    name=$(basename "$header" .h)
    printf 'preserve-existing-output\n' > "$TMP/$name.src"
    if "$LUME" adapt c "$header" -o "$TMP/$name.src" > "$TMP/$name.json" 2> "$TMP/$name.err"; then
        echo "invalid header unexpectedly accepted: $name" >&2
        exit 1
    fi
    printf 'preserve-existing-output\n' > "$TMP/$name.expected"
    cmp "$TMP/$name.expected" "$TMP/$name.src"
    python3 - "$TMP/$name.json" "$name" <<'PY'
import json
import pathlib
import sys

result = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
assert result["ok"] is False, sys.argv[2]
assert result["generated_lume"] == "", sys.argv[2]
assert result["diagnostics"], sys.argv[2]
assert all(d["code"] == "I0003" and d["severity"] == "error" for d in result["diagnostics"])
PY
done

if "$LUME" agent bind "$FIXTURES/invalid/callback.h" > "$TMP/agent-callback.json" 2> "$TMP/agent-callback.err"; then
    echo "agent bind unexpectedly accepted a callback" >&2
    exit 1
fi
python3 - "$TMP/agent-callback.json" <<'PY'
import json
import pathlib
import sys

result = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
assert result["ok"] is False
assert result["generated_lume"] == ""
assert any("callback" in d["message"] for d in result["diagnostics"])
PY

printf 'C adapter generation, CLI, rejection, provenance, and real shared-library interop passed\n'
