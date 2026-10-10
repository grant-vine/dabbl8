# Cold capture and rotating native autosave recovery — host evidence

Functional test commit `06d86ba506d33f478306a8d4a31dbb71ced9018e` adds 112 lines only to `tests/instrument_capture_test.c`, on frozen integration parent `6f73dba1576ad12bdc3fa30eac1509cd674dc0a5`. Existing normal and sanitizer runner entries already execute this fixture and collector; no runner or firmware change is needed.

Both focused capture builds pass **407,512 checks**, including two cold cases and all five actual writer-chosen autosave blocks. Both collectors pass **287 checks** through the actual C bridge. Logs are retained alongside exact source, unchanged reference and seven generated-header hashes in `results.json`. No new full-suite or target-build result is claimed.

The cold fixture uses the existing actual host audio/UI `cold()` initializer plus explicit global resets. With native cache invalid, unbound and arrangement bytes poisoned, actual full-current editor capture preserves all eight distinct tracks, selected track/name and zero native arrangement. A nonempty unconverted historical chain explicitly refuses full-current capture while raw capture succeeds without changing that chain. This is **not** actual firmware `main()`/`persist_boot()` cold boot or pre-upgrade capture proof.

The pool writer itself selects each of the five physical blocks for native object 3; the fixture does not relocate headers or forge generations. Public GET replies for raw roles 0–4 are independently decoded and checked against actual mapped addresses. Actual C inventory/load then recovers persisted autosave solely from the reconstructed pool and distinguishes it from unsaved live role 12. Raw role 4 alone is insufficient in four of the five cases. Capture and recovery leave live state, virtual NOR and native binding/session readiness unchanged.

## Reproduction

From the functional checkout, with the seven pinned generated headers in `build/gen` and an output directory `OUT`:

```sh
cc -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/capture-opt" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_capture_test.c -lm
"$OUT/capture-opt"
python3 tests/instrument_capture_collector_test.py "$OUT/capture-opt"
cc -O1 -g -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow -fno-sanitize-recover=undefined -w -Ibuild/gen -Ifirmware/src -o "$OUT/capture-san" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_capture_test.c -lm
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$OUT/capture-san"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 python3 tests/instrument_capture_collector_test.py "$OUT/capture-san"
```

Mac Apple clang 21.0.0 and isolated Python 3.14.8 were used. The DSP-containing fixture retains five existing sanitizer exclusions; these checks do not prove strict whole-DSP undefined-behavior safety. No new build/test failure occurred.

## Remaining gates

Virtual NOR/read-window checks are host evidence, not physical transfer, flash power-cut, DMA/FIFO/codec completion or timing qualification. No production ownership grant, runtime migration adoption, autosave activation, loader/map change or release claim is introduced. Actual pre-upgrade originals require a separate boot-wide persistence quarantine and verified device capture. Existing native static cost failures remain UAC 570/504, ROOM 296/218 and SPRING 176/126; issue #12 and hardware gates remain open. Audio goldens and cost references are unchanged.
