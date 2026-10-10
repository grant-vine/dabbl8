# SLICE reverse windows in the shared engine pool

Preparatory issue #12 work stacked on PR35. Expanded reverse SLICE voices use a typed int16 window in the existing eight-body union, removing the independent eight-window array and owner metadata. Window size remains64 samples and is checked at compile time. Forward ADPCM playback reserves no shared body. Reverse liveness is published before allocation; new owners invalidate the decode cursor before any window read. Same-owner cached windows are retained, live bodies cannot be reclaimed, and a ninth unadmitted window refuses. The four-track path remains unchanged.

| Actual expanded target memory | Before | After | Capacity |
| --- | ---: | ---: | ---: |
| General RAM | 105508 | 104468 | 98304 |
| Engine pool | 333328 | 333328 | 344064 |

The actual target saves1040 general RAM bytes (1024 windows +16 owners). The linker still FAILS with6164-byte RAM overflow and BSS/pool overlap. No valid eight-track image/package exists. Pool free space remains10736 bytes; union56160 target bytes versus56960 host bytes reflects host alignment differences. No loader, linker-region or flash-boundary changes.

Four-track build/full upstream and extended suite PASS with exact prior firmware/package/loader/golden/CPU/target hashes: image447924/package610086, RAM92116/98304, pool331332/344064. Nonempty preprocessed four-track source lines are identical to PR35. No golden/reference hash is rewritten.

Seventeen actual-eight-track SLICE O2/sanitizer checks cover unique windows, every interleaved reverse sample against forward decoding, BREAK/PIANO isolated forward/reverse renderers, ninth-window refusal, reclamation,5000 reverse-loop steals and release. New transition checks prove reverse-to-forward retires ownership, a different reverse voice can reuse its body without touching the forward predictor/index, and forward-to-reverse reacquires an invalidated unique window. Synthetic ninth requests deliberately bypass the sounding-voice budget to verify defensive refusal/reclamation; normal rendering retains the eight-voice budget. Mixed-engine stress now covers WHEEL/GRAIN/PHYS/DRUM/FM6/reverse-SLICE with5000 notes and actual engine-switch fades. Other engine tests and all three fuzz tests pass; raw suite output ends ALL HOST TESTS PASSED.

Independent PR35 comparison covers BREAK/PIANO × forward/reverse × POLY/MONO/LEGATO (eight tracks)/UNISON (one part),256 blocks and retriggers100/200. All4194304 sample bytes match exactly, SHA-2564c6de01974fa9a9e4d327bd8cd731ee3e63f23a570237be2add7d8c2c57b4821; each case has audible samples. This covers these histories, not arbitrary parameters or hardware deadlines. PCM/binaries remain ignored locally; text commands and hashes are retained.

Pinned dependencies remain Python3.14.8/Pillow12.3.0/fonttools4.66.1/Apple clang21; JieLi pi32v2 clang4.0.1 archive SHA-256f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958; SDKd179b4484759423312073f5fbb232501aa491047; linux/amd64 Debian digest7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587. Existing sanitizer exclusions remain signed-overflow/shift/bounds/object-size/pointer-overflow.

Explicit skips: no DaisySP reference checkout, normal-suite V15 restore absent because vendor firmware remains outside Git, absent editor MENU settings and no Emscripten emulator. Hardware qualification SKIP. #9/#12 remain open for RAM fit, full workflows/combinations and physical high-water/deadline qualification. No flashing, merge, release or deployment occurred.
