#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Felucca in the browser: the firmware's sources as the host tests build them (web/emu/felucca_web.c), compiled
# to WebAssembly with Emscripten, plus the page. After X0X's web/emu/build.sh (charlesvestal/fm1-x0x, GPL-3.0).
#   web/emu/build.sh [--dabbl8 X.Y[.Z][-suffix]]   ->  build/emu/{index.html, worklet.js, felucca.wasm, fonts/}
# Run after one ./build.sh (build/gen: the tables, the fonts, the CC0 samples).
# -ffp-contract=off: no fused multiply-adds, as the host build. EMU_OPT replaces -O2, EMU_CFLAGS adds flags
# (EMU_CFLAGS=-msimd128: let the compiler vectorise; every browser with AudioWorklet has wasm SIMD).
# The page shows the version (FELUCCA_VERSION) marked "(preview)" before its release; EMU_PREVIEW=0 drops the mark.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "emu: run ./build.sh once first (build/gen)"; exit 1; }
command -v emcc >/dev/null 2>&1 || { echo "emu: no emcc (Emscripten)"; exit 1; }
APP=Felucca
STORE=felucca-emu-flash
MIDI=true
NAV='<a href="../installer/">Install</a><a href="../editor/">Editor</a><a href="https://github.com/hugelton/Felucca">Source</a>'
MODE_FLAGS=''
if [ "$#" -ne 0 ]; then
    [ "$#" -eq 2 ] && [ "$1" = --dabbl8 ] || { echo "emu: usage: web/emu/build.sh [--dabbl8 X.Y[.Z][-suffix]]"; exit 2; }
    VERSION=$(python3 -c 'import sys; sys.path.insert(0,"tools"); from d8identity import dabbl8_identity; print(dabbl8_identity(sys.argv[1]).version)' "$2") || { echo "emu: invalid Dabbl8 version"; exit 2; }
    APP=Dabbl8
    STORE=dabbl8-emu-flash-v1
    MIDI=false
    NAV='<a href="https://github.com/grant-vine/dabbl8">Source</a>'
    MODE_FLAGS='-DNPART=8 -DDABBL8_IDENTITY=1 -DFELUCCA_FM4=0 -DFELUCCA_SLICE=1'
else
    case "${EMU_CFLAGS:-}" in
        *-DNPART=8*|*-DDABBL8_IDENTITY=1*) echo "emu: eight-track identity requires --dabbl8 VERSION"; exit 2 ;;
    esac
    VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
fi
OUT=${EMU_OUT_DIR:-build/emu}
mkdir -p "$OUT/fonts"
# shellcheck disable=SC2086
emcc ${EMU_OPT:--O2} -ffp-contract=off -std=gnu99 -w ${EMU_CFLAGS:-} $MODE_FLAGS \
    "-DFELUCCA_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o "$OUT/felucca.wasm" web/emu/felucca_web.c
[ "${EMU_PREVIEW:-1}" = 0 ] && PREVIEW='' || PREVIEW=' (preview)'
[ "$APP" = Dabbl8 ] && PREVIEW=' (experimental preview)'
sed -e "s/@VERSION@/$VERSION/g" -e "s/@PREVIEW@/$PREVIEW/g" \
    -e "s/@APP@/$APP/g" -e "s/@STORE@/$STORE/g" -e "s/@MIDI@/$MIDI/g" \
    -e "s|@NAV@|$NAV|g" web/emu/index.html > "$OUT/index.html"
cp web/emu/worklet.js "$OUT/"
cp web/emu/fonts/DotGothic16-subset.woff web/emu/fonts/OFL.txt "$OUT/fonts/"
echo "emu: $OUT ($(wc -c < "$OUT/felucca.wasm" | tr -d ' ') B wasm, $VERSION)"
