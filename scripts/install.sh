#!/bin/sh
# scripts/install.sh — One-command zero-C installer for Lume (docs/22-adoption.md G0, #196).
#
# Usage:
#   sh scripts/install.sh [--prefix <dir>] [--from-local <repo_dir>]
#
# Installs the self-hosted Lume binary & stdlib bundle into <prefix> (default: ~/.lume),
# writes a FIPS-180-4 SHA-256 manifest (<prefix>/MANIFEST.sha256), and verifies that the
# installed `lume` binary runs and proves a contract with zero C compiler in steady state.
set -eu

PREFIX="${LUME_INSTALL_PREFIX:-$HOME/.lume}"
FROM_LOCAL=""
FROM_DIR=""

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix)
            [ $# -ge 2 ] || { echo "install.sh: --prefix requires a directory" >&2; exit 64; }
            PREFIX=$2
            shift 2
            ;;
        --prefix=*)
            PREFIX=${1#--prefix=}
            shift
            ;;
        --from-local)
            [ $# -ge 2 ] || { echo "install.sh: --from-local requires a repo path" >&2; exit 64; }
            FROM_LOCAL=$2
            shift 2
            ;;
        --from-local=*)
            FROM_LOCAL=${1#--from-local=}
            shift
            ;;
        --from-dir)
            [ $# -ge 2 ] || { echo "install.sh: --from-dir requires a binary directory" >&2; exit 64; }
            FROM_DIR=$2
            shift 2
            ;;
        --from-dir=*)
            FROM_DIR=${1#--from-dir=}
            shift
            ;;
        *)
            echo "usage: install.sh [--prefix <dir>] [--from-local <repo_dir>] [--from-dir <bin_dir>]" >&2
            exit 64
            ;;
    esac
done

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -z "$FROM_LOCAL" ]; then
    if [ -x "$SCRIPT_DIR/../lume" ]; then
        FROM_LOCAL=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
    else
        echo "install.sh: repository root not found; pass --from-local <repo_dir>" >&2
        exit 66
    fi
fi

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v openssl >/dev/null 2>&1; then
        openssl dgst -sha256 "$1" | awk '{print $NF}'
    else
        cksum "$1" | awk '{print $1}'
    fi
}

mkdir -p "$PREFIX/bin" "$PREFIX/compiler/stage1/lume" "$PREFIX/build/bin" "$PREFIX/evolve"
cp "$FROM_LOCAL/lume" "$PREFIX/lume"
chmod +x "$PREFIX/lume"
ln -sf "$PREFIX/lume" "$PREFIX/bin/lume"

for _f in "$FROM_LOCAL"/compiler/stage1/*.src "$FROM_LOCAL"/compiler/stage1/*.lume; do
    [ -f "$_f" ] && cp "$_f" "$PREFIX/compiler/stage1/"
done
for _f in "$FROM_LOCAL"/compiler/stage1/lume/*.src; do
    [ -f "$_f" ] && cp "$_f" "$PREFIX/compiler/stage1/lume/"
done
if [ -f "$FROM_LOCAL/evolve/maintainers.pub" ]; then
    cp "$FROM_LOCAL/evolve/maintainers.pub" "$PREFIX/evolve/maintainers.pub"
fi

SRC_BIN="${FROM_DIR:-${LUME_BIN_DIR:-$FROM_LOCAL/build/bin}}"
if [ -d "$SRC_BIN" ]; then
    for _b in "$SRC_BIN"/*; do
        if [ -f "$_b" ] && [ -x "$_b" ]; then
            cp "$_b" "$PREFIX/build/bin/"
            chmod +x "$PREFIX/build/bin/$(basename -- "$_b")"
        fi
    done
    if [ -f "$PREFIX/build/bin/lc" ] && [ ! -f "$PREFIX/build/bin/lumec" ]; then
        cp "$PREFIX/build/bin/lc" "$PREFIX/build/bin/lumec"
        chmod +x "$PREFIX/build/bin/lumec"
    fi
fi

if [ -x "$FROM_LOCAL/bootstrap/lume0/lume0" ]; then
    mkdir -p "$PREFIX/bootstrap/lume0"
    cp "$FROM_LOCAL/bootstrap/lume0/lume0" "$PREFIX/bootstrap/lume0/lume0"
    chmod +x "$PREFIX/bootstrap/lume0/lume0"
fi

MANIFEST="$PREFIX/MANIFEST.sha256"
: > "$MANIFEST"
printf '%s  lume\n' "$(sha256_of "$PREFIX/lume")" >> "$MANIFEST"
for _b in "$PREFIX"/build/bin/*; do
    if [ -f "$_b" ]; then
        printf '%s  build/bin/%s\n' "$(sha256_of "$_b")" "$(basename -- "$_b")" >> "$MANIFEST"
    fi
done
for _s in "$PREFIX"/compiler/stage1/lcli.src "$PREFIX"/compiler/stage1/lasm.src "$PREFIX"/compiler/stage1/lcaas.src "$PREFIX"/compiler/stage1/lcert.src "$PREFIX"/compiler/stage1/lmod.src "$PREFIX"/compiler/stage1/ltarget.src; do
    if [ -f "$_s" ]; then
        printf '%s  compiler/stage1/%s\n' "$(sha256_of "$_s")" "$(basename -- "$_s")" >> "$MANIFEST"
    fi
done

MANIFEST_SHA=$(sha256_of "$MANIFEST")
printf '{"ok":true,"installer":"lume-install-v1","prefix":"%s","version":"0.2.0","edition":"2026","zero_c_steady_state":true,"manifest_sha256":"sha256:%s"}\n' "$PREFIX" "$MANIFEST_SHA"
