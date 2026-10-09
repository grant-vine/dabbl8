# FM6 operators in the shared engine pool

Preparatory issue #12 work stacked on PR34. Expanded builds use an FM6 typed member of the existing eight-body engine pool, removing the separate eight-note array and owner metadata. Per-track patches, macro-adjusted patches and LFOs remain independent. Same-owner operator phases survive retriggers; new owners receive cleared FM6 state, live bodies cannot be reclaimed and a ninth unadmitted body refuses. The original six-voice per-track cap and shared eight-sounding-voice budget remain intact. Four-track source retains its original include order and allocation.

| Actual expanded target memory | Before | After | Capacity |
| --- | ---: | ---: | ---: |
| General RAM | 107892 | 105508 | 98304 |
| Engine pool | 333328 | 333328 | 344064 |

The actual target saves 2384 general RAM bytes (2368-byte note array plus 16 owner bytes). The linker still FAILS with 7204-byte RAM overflow and BSS/pool overlap. No valid eight-track image/package exists. Pool free space remains 10736 bytes. The shared allocation is 56160 target bytes and 56960 host bytes; host alignment is not a target-memory measurement. Loader code, linker regions and flash boundaries are unchanged.

Four-track build/full upstream and extended suite PASS, retaining exact firmware/package/loader/golden/CPU/target hashes: image447924/package610086, RAM92116/98304, pool331332/344064. Nonempty preprocessed four-track source lines are identical to PR34. An initial strict source comparison differed by exactly one blank line before FM6_INIT; that normalization failure is explicitly retained, with no firmware edit needed. No golden/reference hash was rewritten.

Twelve actual-eight-track FM6 O2/sanitizer tests retain unique note bodies, nonzero operator phases, zeroed reclaimed state, ninth-body refusal,5000 mixed-patch steals, completed release and the six-voice cap. The existing mixed-engine stress now includes FM6 along with WHEEL/GRAIN/PHYS/DRUM and runs5000 notes with actual engine-switch fades. All other voice/engine checks and three fuzz tests pass. Raw suite output ends ALL HOST TESTS PASSED.

Independent prior-source comparison covers every FM6 factory preset and all four voice modes: POLY/MONO/LEGATO on eight tracks and UNISON on one part with its original six-voice cap,256 blocks with retriggers100/200. All8388608 sample bytes match exactly, SHA-25632246037fab2ca5a06668ace5928af53508442469943d117194a2e80ed4ec292. All cases produce nonzero audio. This proves those histories only, not arbitrary parameters, reclaimed-owner startup or hardware deadlines. PCM/binaries remain ignored locally; commands and hashes are retained here.

Pinned environment remains Python3.14.8/Pillow12.3.0/fonttools4.66.1/Apple clang21; JieLi pi32v2 clang4.0.1 archive SHA-256f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958; SDKd179b4484759423312073f5fbb232501aa491047; linux/amd64 Debian digest7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587. Apache-2.0 FM6 dependency notices remain intact; only expanded include placement changes. Upstream sanitizer exclusions for signed-overflow/shift/bounds/object-size/pointer-overflow remain.

Explicit skips: no DaisySP reference checkout, normal-suite V15 restore absent because vendor firmware stays outside Git, absent editor MENU settings, no Emscripten emulator. Hardware qualification SKIP. #9/#12 remain open for RAM fit, complete workflows/combinations and physical high-water/deadline evidence. No flashing, merge, release or deployment occurred.
