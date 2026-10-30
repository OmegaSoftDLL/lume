#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
FIXTURES="$ROOT/tests/fixtures/c_adapter"
LUME=${LUME_CLI:-$ROOT/lume}

if [ "$(uname -s)" != "Linux" ] || [ "$(uname -m)" != "x86_64" ]; then
    echo "C adapter interop tests require Linux x86-64" >&2
    exit 77
fi
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

"$LUME" build "$FIXTURES/assertions.src" -o "$TMP/c_adapter_assert"
"${CC:-cc}" -shared -fPIC "$FIXTURES/buffer_math.c" -o "$TMP/libc_adapter_fixture.so"
"$LUME" adapt c --json "$FIXTURES/buffer_math.h" > "$TMP/adapter.json"
printf '@mode manifest\n@path %s\n@header %s\n@output %s\n' "$TMP/adapter.json" "$FIXTURES/buffer_math.h" "$TMP/generated.src" | "$TMP/c_adapter_assert"

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
    printf '@mode invalid\n@path %s\n' "$TMP/$name.json" | "$TMP/c_adapter_assert"
done

if "$LUME" agent bind "$FIXTURES/invalid/callback.h" > "$TMP/agent-callback.json" 2> "$TMP/agent-callback.err"; then
    echo "agent bind unexpectedly accepted a callback" >&2
    exit 1
fi
printf '@mode callback\n@path %s\n' "$TMP/agent-callback.json" | "$TMP/c_adapter_assert"

printf 'C adapter generation, CLI, rejection, provenance, and real shared-library interop passed\n'
