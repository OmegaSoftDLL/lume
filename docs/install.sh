#!/bin/sh
# Lume network installer — install/update the self-hosted toolchain from GitHub Releases.
#
#   curl -fsSL https://omegasoftdll.github.io/lume/install.sh | sh
#
# Re-run any time to update to the latest release (this is how Lume self-updates as it
# evolves). No C compiler needed; the downloaded toolchain is Lume-built and zero-C.
# Supported release platforms (#262): linux-x86_64, linux-aarch64, linux-riscv64,
# macos-x86_64, macos-aarch64, and windows-x86_64.
set -eu

REPO="${LUME_REPO:-OmegaSoftDLL/lume}"
PREFIX="${LUME_PREFIX:-$HOME/.lume}"

os=$(uname -s); arch=$(uname -m)
case "$os" in
    Linux)               OS=linux ;;
    Darwin)              OS=macos ;;
    MINGW*|MSYS*|CYGWIN*) OS=windows ;;
    *) echo "lume: no prebuilt toolchain for '$os' yet — build from source: https://github.com/$REPO" >&2; exit 1 ;;
esac
case "$arch" in
    x86_64|amd64)  ARCH=x86_64 ;;
    aarch64|arm64) ARCH=aarch64 ;;
    riscv64)       ARCH=riscv64 ;;
    *) echo "lume: no prebuilt toolchain for arch '$arch' yet — build from source: https://github.com/$REPO" >&2; exit 1 ;;
esac
PLAT="${LUME_INSTALL_PLAT:-$OS-$ARCH}"

need() { command -v "$1" >/dev/null 2>&1 || { echo "lume: '$1' is required" >&2; exit 1; }; }
need curl; need tar

if [ -n "${LUME_RELEASE_TAG:-}" ]; then
    TAG="$LUME_RELEASE_TAG"
else
    echo "lume: resolving latest release…"
    TAG=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" \
          | grep '"tag_name"' | head -1 | sed 's/.*"tag_name"[ :]*"\([^"]*\)".*/\1/')
fi
[ -n "${TAG:-}" ] || { echo "lume: could not resolve the latest release" >&2; exit 1; }

ASSET="lume-$TAG-$PLAT.tar.gz"
BASE="${LUME_RELEASE_BASE_URL:-https://github.com/$REPO/releases/download/$TAG}"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT

echo "lume: downloading $ASSET ($TAG)…"
if ! curl -fSL "$BASE/$ASSET" -o "$tmp/pkg.tar.gz" 2>/dev/null; then
    echo "lume: no prebuilt '$PLAT' binary in $TAG yet — build from source: https://github.com/$REPO" >&2
    exit 1
fi

# Verify SHA-256 against the release's SHA256SUMS (refuse on mismatch).
if curl -fsSL "$BASE/SHA256SUMS" -o "$tmp/SHA256SUMS" 2>/dev/null && grep -q "$ASSET" "$tmp/SHA256SUMS"; then
    if   command -v sha256sum >/dev/null 2>&1; then got=$(sha256sum "$tmp/pkg.tar.gz" | cut -d' ' -f1)
    elif command -v shasum    >/dev/null 2>&1; then got=$(shasum -a 256 "$tmp/pkg.tar.gz" | cut -d' ' -f1)
    elif command -v openssl   >/dev/null 2>&1; then got=$(openssl dgst -sha256 "$tmp/pkg.tar.gz" | awk '{print $NF}')
    else echo "lume: no sha256 tool to verify the download — aborting" >&2; exit 1; fi
    exp=$(grep "$ASSET" "$tmp/SHA256SUMS" | head -1 | cut -d' ' -f1)
    [ "$got" = "$exp" ] || { echo "lume: SHA-256 mismatch — refusing to install (expected $exp, got $got)" >&2; exit 1; }
    echo "lume: SHA-256 verified."
else
    echo "lume: no SHA256SUMS entry for $ASSET — aborting rather than install unverified" >&2; exit 1
fi

mkdir -p "$PREFIX"
tar -xzf "$tmp/pkg.tar.gz" -C "$PREFIX" --strip-components=1
echo "lume: installed $TAG to $PREFIX"

if [ -z "${LUME_SKIP_POST_VERIFY:-}" ]; then
    "$PREFIX/bin/lume" self verify >/dev/null && echo "lume: self verify OK (MANIFEST.sha256)"
    "$PREFIX/bin/lume" --version
fi

case ":$PATH:" in
    *":$PREFIX/bin:"*) ;;
    *) echo ""; echo "Add Lume to your PATH:"; echo "  export PATH=\"$PREFIX/bin:\$PATH\"" ;;
esac
echo "Re-run this installer any time to update to the latest release."
