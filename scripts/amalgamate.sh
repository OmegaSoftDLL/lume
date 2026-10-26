#!/bin/sh
# Amalgamate the stage-1 self-hosted compiler from its per-module sources.
#
# `compiler/stage1/lumec.src` is GENERATED, not edited by hand. The editable
# sources live in `compiler/stage1/lume/NN_*.src`; this script concatenates them
# IN LEXICOGRAPHIC (= numeric NN) ORDER into the single `lumec.src` that the
# bootstrap, the self-host tests and the `lume` CLI feed to the compiler.
#
# The concatenation is BYTE-IDENTICAL to the historical monolithic lumec.src, so
# the self-host fixpoint (stage 2 == stage 3, bit for bit) is preserved. This is
# the SQLite "amalgamation" model: split sources for humans, one blob for the build.
#
# To add or re-order a module, add/rename a `compiler/stage1/lume/NN_*.src` file;
# the NN prefix fixes its position in the generated file.
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
LC_ALL=C
export LC_ALL
SRC=compiler/stage1
OUT=$SRC/lumec.src
cat "$SRC"/lume/*.src > "$OUT"
