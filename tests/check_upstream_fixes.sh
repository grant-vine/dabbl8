#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local virtual-NOR/UI/DSP checks; supplied package is read as fixture data only.
# Usage: sh tests/check_upstream_fixes.sh EXISTING_PACKAGE.fwsc [NEW_OUTPUT_DIR]
set -eu
cd "$(dirname "$0")/.."
PKG=${1:?Supply an existing FM-1 package as read-only fixture data}
OUT=${2:-build/upstream-fixes}
test -f "$PKG"
test ! -e "$OUT"
mkdir -p "$OUT"
COMPILER=${CC:-cc}
for mode in optimized sanitized; do
    FLAGS='-O2'
    DSP=''
    if [ "$mode" = sanitized ]; then
        FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        DSP='-fno-sanitize=signed-integer-overflow,shift'
    fi
    for tracks in 4 8; do
        # DSP keeps only the repository's existing arithmetic exclusions.
        "$COMPILER" $FLAGS $DSP -w -DNPART="$tracks" -DFELUCCA_FM4=0 -DFELUCCA_SLICE=1 \
            -Ibuild/gen -Ifirmware/src tests/upstream_ui_fixes_test.c -lm -o "$OUT/ui-$tracks-$mode"
        "$OUT/ui-$tracks-$mode" > "$OUT/ui-$tracks-$mode.log" 2>&1
    done
    for fixture in ota ldr; do
        "$COMPILER" $FLAGS -Wall -Wno-unused-function -DOWN_PKG=1 \
            "tests/${fixture}_test.c" -o "$OUT/$fixture-$mode"
        if [ "$fixture" = ldr ]; then
            "$OUT/$fixture-$mode" "$PKG" "$PKG" > "$OUT/$fixture-$mode.log" 2>&1
        else
            "$OUT/$fixture-$mode" "$PKG" > "$OUT/$fixture-$mode.log" 2>&1
        fi
    done
    for fixture in speaker native_settings_session; do
        SOURCES=''
        if [ "$fixture" = native_settings_session ]; then
            SOURCES='firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c'
        fi
        "$COMPILER" $FLAGS $DSP -w -DNPART=8 -DFELUCCA_FM4=0 -DFELUCCA_SLICE=1 \
            -Ibuild/gen -Ifirmware/src $SOURCES "tests/${fixture}_test.c" -lm -o "$OUT/$fixture-$mode"
        "$OUT/$fixture-$mode" > "$OUT/$fixture-$mode.log" 2>&1
    done
    "$COMPILER" $FLAGS -w -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c \
        firmware/src/d8pool_mapped.c tests/ldr_native_pool_preservation_test.c -o "$OUT/pool-$mode"
    "$OUT/pool-$mode" "$PKG" "$PKG" > "$OUT/pool-$mode.log" 2>&1
done
printf 'Selective upstream regressions passed; logs: %s\n' "$OUT"
