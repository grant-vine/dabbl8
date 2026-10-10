#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run after ./build.sh; compiler must be isolated Emscripten 4.0.17.
set -eu
cd "$(dirname "$0")/.."
[ -f build/gen/felucca_tables.h ] || { echo 'Run the baseline build first.'; exit 1; }
compiler=${EMCC:-emcc}
version=$("$compiler" --version)
case "$version" in *' 4.0.17 ('*) ;; *) echo 'Expected pinned Emscripten 4.0.17.'; exit 1;; esac
mkdir -p build/d8converter
"$compiler" -O2 -w -Ibuild/gen -Ifirmware/src firmware/src/d8p1.c tools/dabbl8_project_convert.c \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 \
  -sEXPORTED_RUNTIME_METHODS=FS,callMain -sSTACK_SIZE=1048576 \
  -o build/d8converter/dabbl8_project_convert.mjs
printf '%s\n' "$version" > build/d8converter/compiler-version.txt
