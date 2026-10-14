#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Cached generated inputs, existing Emscripten/cc/Node only. No firmware/device build.
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/emu-workflows}
EVIDENCE="$OUT/evidence"
for tool in emcc cc node; do
    command -v "$tool" >/dev/null 2>&1 || { echo "eight-emu: missing local $tool; no dependency download"; exit 2; }
done
mkdir -p "$EVIDENCE"
EMU_OUT_DIR="$OUT" sh web/emu/build.sh --dabbl8 0.1.1 > "$EVIDENCE/build-eight.log" 2>&1
node web/emu/eight_workflow_test.mjs "$OUT/felucca.wasm" "$EVIDENCE" > "$EVIDENCE/eight-workflows.log" 2>&1
cc -O2 -ffp-contract=off -std=gnu99 -w -DNPART=8 -DDABBL8_IDENTITY=1 \
    -DFELUCCA_FM4=0 -DFELUCCA_SLICE=1 '-DFELUCCA_VERSION="v0.1.1"' \
    -Ibuild/gen -Ifirmware/src web/emu/native_eight_check.c -lm \
    -o "$OUT/native_eight_check" > "$EVIDENCE/build-native-eight.log" 2>&1
"$OUT/native_eight_check" "$EVIDENCE/initialized-nor.bin" "$EVIDENCE/native-eight-active.f32"
node web/emu/compare_audio_test.mjs "$EVIDENCE/eight-active.f32" "$EVIDENCE/native-eight-active.f32" "$EVIDENCE/native-eight-parity.json" > "$EVIDENCE/native-eight-parity.log" 2>&1
node web/emu/preset_settings_test.mjs "$OUT/felucca.wasm" "$EVIDENCE/initialized-nor.bin" "$EVIDENCE" > "$EVIDENCE/preset-settings.log" 2>&1
node web/emu/worklet_first_use_test.mjs "$OUT/felucca.wasm" "$EVIDENCE" > "$EVIDENCE/worklet-first-use.log" 2>&1
node web/emu/source_receipt.mjs "$OUT"
printf 'eight emulator selected checks passed; evidence: %s\n' "$EVIDENCE"
