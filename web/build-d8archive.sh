#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Existing isolated compiler only. No dependency or firmware build.
set -eu
cd "$(dirname "$0")/.."
compiler=${EMCC:-emcc}
version=$("$compiler" --version)
case "$version" in *' 4.0.17 ('*) ;; *) echo 'Expected pinned Emscripten 4.0.17.'; exit 1;; esac
mkdir -p build/d8archive
"$compiler" -O2 -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_project_archive.c \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 \
  -sEXPORTED_RUNTIME_METHODS=FS,callMain -sSTACK_SIZE=1048576 \
  -o build/d8archive/dabbl8_project_archive.mjs
printf '%s\n' "$version" > build/d8archive/compiler-version.txt
python3 tools/d8archive_inputs.py --record
