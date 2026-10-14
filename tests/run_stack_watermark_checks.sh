#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host checks by default. D8_STACK_TARGET=1 adds offline startup/object checks.
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
out=${1:?Usage: tests/run_stack_watermark_checks.sh ABSOLUTE_EVIDENCE_DIRECTORY}
mode=${2:-checks}
case "$mode" in checks|--target-plan) ;; *) printf 'Unknown mode: %s\n' "$mode"; exit 2;; esac
# Never overwrite an earlier run. The parent directory must already exist.
mkdir "$out"
out=$(CDPATH= cd -- "$out" && pwd)
cc=${CC:-clang}
run()
{
    name=$1; shift
    printf '%s\n' "$@" > "$out/$name.argv"
    if "$@" > "$out/$name.log" 2>&1; then code=0; else code=$?; fi
    printf '%s\n' "$code" > "$out/$name.exit"
    cat "$out/$name.log"
    [ "$code" -eq 0 ] || exit "$code"
}
reject()
{
    name=$1; shift
    printf '%s\n' "$@" > "$out/$name.argv"
    if "$@" > "$out/$name.log" 2>&1; then code=0; else code=$?; fi
    printf '%s\n' "$code" > "$out/$name.exit"
    cat "$out/$name.log"
    [ "$code" -ne 0 ] || { printf 'Expected rejection: %s\n' "$name"; exit 1; }
}
if [ "${D8_STACK_TARGET:-0}" = 1 ]; then
    toolchain=${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN to the existing cached toolchain}
    image=${JIELI_DOCKER_IMAGE:-mirror.gcr.io/library/debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587}
    run target-cached-image docker image inspect "$image" --format '{{.Id}} {{.Architecture}} {{.Os}}'
    read -r image_id image_arch image_os < "$out/target-cached-image.log"
    case "$image_id" in sha256:*) ;; *) printf 'Invalid cached image ID\n'; exit 2;; esac
    [ "$image_arch" = amd64 ] && [ "$image_os" = linux ] || { printf 'Expected cached linux/amd64 image\n'; exit 2; }
    image=$image_id
    if [ "$mode" = --target-plan ]; then
        run target-plan printf '%s\n' docker run --rm --pull never --network none --platform linux/amd64 \
            -v "$toolchain:/opt/jieli:ro" -v "$out:/work" -w /work "$image" \
            /opt/jieli/pi32v2/bin/clang -target pi32v2 -DD8_STACK_WATERMARK_DIAGNOSTIC=1 \
            -c candidate/firmware/crt0.S -o candidate-1.o
        printf 'STACK_WATERMARK_TARGET_PLAN_ONLY\n'
        exit 0
    fi
elif [ "$mode" = --target-plan ]; then
    printf 'Target plan requires D8_STACK_TARGET=1\n'
    exit 2
fi
run host-compiler "$cc" --version
run source-head git -C "$repo" rev-parse HEAD
run local-selector python3 "$repo/ci/local_checks.py" --paths docs/dabbl8/stack-watermark-diagnostic.md --check docs
for flag in unset 0 1; do
    set --
    [ "$flag" = unset ] || set -- "-DD8_STACK_WATERMARK_DIAGNOSTIC=$flag"
    run "host-$flag-build" "$cc" -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -O2 "$@" \
        -I"$repo/firmware/hal" "$repo/tests/stack_watermark_test.c" -o "$out/host-$flag"
    run "host-$flag-run" "$out/host-$flag"
    run "san-$flag-build" "$cc" -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -O1 -g \
        -fno-omit-frame-pointer -fsanitize=address,undefined "$@" \
        -I"$repo/firmware/hal" "$repo/tests/stack_watermark_test.c" -o "$out/san-$flag"
    run "san-$flag-run" env ASAN_OPTIONS=halt_on_error=1 \
        UBSAN_OPTIONS=halt_on_error=1 "$out/san-$flag"
done
for flag in 2 banana 1u '0+0' '1+0' '0&&1' '1<<0' '(0)' '(1)' '' \
            '0&&banana' '1&&banana' '0*banana' '0==banana'; do
    reject "host-invalid-$flag" "$cc" -std=c11 -D_DEFAULT_SOURCE -fsyntax-only \
        "-DD8_STACK_WATERMARK_DIAGNOSTIC=$flag" -I"$repo/firmware/hal" "$repo/tests/stack_watermark_test.c"
done
reject host-loader-enabled "$cc" -std=c11 -D_DEFAULT_SOURCE -fsyntax-only \
    -DD8_STACK_WATERMARK_DIAGNOSTIC=1 -DFELUCCA_LOADER=1 -I"$repo/firmware/hal" "$repo/tests/stack_watermark_test.c"
run host-loader-disabled "$cc" -std=c11 -D_DEFAULT_SOURCE -fsyntax-only \
    -DD8_STACK_WATERMARK_DIAGNOSTIC=0 -DFELUCCA_LOADER=1 -I"$repo/firmware/hal" "$repo/tests/stack_watermark_test.c"
if [ "${D8_STACK_TARGET:-0}" = 1 ]; then
    baseline=${D8_STACK_BASELINE_REF:-1d896793ed688090bc5930e5884b83b2de2f7858}
    mkdir -p "$out/baseline/firmware" "$out/candidate/firmware/hal"
    git -C "$repo" show "$baseline:firmware/crt0.S" > "$out/baseline/firmware/crt0.S"
    cp "$repo/firmware/crt0.S" "$out/candidate/firmware/crt0.S"
    cp "$repo/firmware/hal/fm1_stack_watermark.h" "$out/candidate/firmware/hal/fm1_stack_watermark.h"
    cp "$repo/firmware/app.ld" "$out/app.ld"
    printf '.section .text.stub,"ax",@progbits\n.globl fm1_cstart\nfm1_cstart:\ngoto .\n' > "$out/stub.S"
    printf '#include "fm1_stack_watermark.h"\nfm1_stack_watermarks_t read_stack(void) { return fm1_stack_watermark_read(); }\n' > "$out/readout.c"
    target()
    {
        set -- docker run --rm --pull never --network none --platform linux/amd64 -v "$toolchain:/opt/jieli:ro" \
            -v "$out:/work" -w /work "$image" "$@"
        printf '%s\n' "$@" > "$out/$name.docker.argv"
        "$@"
    }
    run target-compiler target /opt/jieli/pi32v2/bin/clang --version
    run target-baseline target /opt/jieli/pi32v2/bin/clang -target pi32v2 -c baseline/firmware/crt0.S -o baseline.o
    for flag in unset 0 1; do
        set --
        [ "$flag" = unset ] || set -- "-DD8_STACK_WATERMARK_DIAGNOSTIC=$flag"
        run "target-$flag" target /opt/jieli/pi32v2/bin/clang -target pi32v2 "$@" \
            -c candidate/firmware/crt0.S -o "candidate-$flag.o"
        run "target-$flag-preprocess" target /opt/jieli/pi32v2/bin/clang -target pi32v2 "$@" \
            -E candidate/firmware/crt0.S
        run "target-$flag-disassembly" target /opt/jieli/pi32v2/bin/objdump -d -r "candidate-$flag.o"
        run "target-readout-$flag" target /opt/jieli/pi32v2/bin/clang -target pi32v2 -Os -fno-builtin -Wall -Wextra -Werror "$@" \
            -Icandidate/firmware/hal -c readout.c -o "readout-$flag.o"
        run "target-readout-$flag-disassembly" target /opt/jieli/pi32v2/bin/objdump -d -r "readout-$flag.o"
    done
    run baseline-unset-identical cmp "$out/baseline.o" "$out/candidate-unset.o"
    run baseline-flag0-identical cmp "$out/baseline.o" "$out/candidate-0.o"
    for flag in 2 banana 1u '0+0' '1+0' '0&&1' '1<<0' '(0)' '(1)' '' \
                '0&&banana' '1&&banana' '0*banana' '0==banana'; do
        reject "target-invalid-$flag" target /opt/jieli/pi32v2/bin/clang -target pi32v2 \
            "-DD8_STACK_WATERMARK_DIAGNOSTIC=$flag" -c candidate/firmware/crt0.S -o invalid.o
    done
    reject target-loader-enabled target /opt/jieli/pi32v2/bin/clang -target pi32v2 \
        -DD8_STACK_WATERMARK_DIAGNOSTIC=1 -DFELUCCA_LOADER=1 -c candidate/firmware/crt0.S -o invalid.o
    run target-stub target /opt/jieli/pi32v2/bin/clang -target pi32v2 -c stub.S -o stub.o
    run target-link target /opt/jieli/pi32v2/bin/ld -T app.ld candidate-1.o stub.o -o startup-only.elf
    run target-matching-link target /opt/jieli/pi32v2/bin/ld -T app.ld candidate-1.o readout-1.o stub.o -o matching.elf
    reject target-mismatched-link target /opt/jieli/pi32v2/bin/ld -T app.ld candidate-0.o readout-1.o stub.o -o mismatched.elf
    run target-linked-disassembly target /opt/jieli/pi32v2/bin/objdump -d startup-only.elf
    run target-linked-symbols target /opt/jieli/pi32v2/bin/nm -n startup-only.elf
    run target-matching-symbols target /opt/jieli/pi32v2/bin/nm -n matching.elf
fi
printf 'STACK_WATERMARK_CHECKS_PASS\n'
