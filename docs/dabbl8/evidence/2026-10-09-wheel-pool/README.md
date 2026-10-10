# WHEEL state in the shared engine pool

Preparatory issue #12 work stacked on PR33. Expanded builds allocate each sounding WHEEL voice from the existing eight typed shared bodies alongside GRAIN/PHYS/DRUM. The WHEEL member fits without increasing the union. Per-track rotor/bar caches and the original eight-voice WHEEL cap remain intact. Same-owner phases and gain ramps survive retriggers; new owners receive cleared state, live owners cannot be reclaimed, and unadmitted bodies fail closed. The unchanged four-track path retains its original array.

| Actual expanded target memory | Before | After | Capacity |
| --- | ---: | ---: | ---: |
| General RAM | 113780 | 107892 | 98304 |
| Engine pool | 333328 | 333328 | 344064 |

Removing the 64 × 92-byte WHEEL side array saves 5888 bytes of general RAM. The target linker still FAILS with 9588 bytes of RAM overflow and a BSS/pool overlap. No valid eight-track image/package exists. The pool has 10736 bytes free. Target values come from actual link diagnostics; the shared union is 56160 target bytes and 56960 host bytes because host pointers/alignment differ. No linker regions, loader code or flash boundaries changed.

The four-track target build and full upstream/extended Mac suite PASS. Firmware remains 447924 bytes, package 610086 bytes, RAM 92116/98304 and pool 331332/344064. Firmware/package/loader and golden/CPU/target reference hashes exactly match PR33. Normalized four-track preprocessing is identical to the previous source. No reference hashes were rewritten.

Eleven actual-eight-track WHEEL checks pass at O2 and ASan/UBSan: eight audible destinations, independent rotor caches, retained nonzero phases/ramps, preserved live bodies on stealing, ninth-body refusal, new-owner clearing, eight-body unison, single-trigger percussion, retained per-track cap, 5000 mixed notes with actual engine-switch fades, and completed release tails. Existing shared-engine, voice, PHYS, FM6 and SLICE tests and all three fuzz tests also pass. Raw full-suite output ends ALL HOST TESTS PASSED.

An independent comparison compiles PR33 and this source for eight tracks without altering the reference. All five factory WHEEL presets × POLY/MONO/LEGATO (eight active tracks) and UNISON (one part/eight voices), 256 blocks with retriggers at 100/200, produce exactly equal 5242880 sample bytes, SHA-256 d1654d3caa9dd286ecc612e851026c9fa284081c609de4c163289c6e023cbf99. This covers those presets/history, not all parameters, reclaimed-owner startup or hardware timing. Commands/hashes are committed; PCM and firmware binaries remain ignored locally.

Pinned dependencies remain Python 3.14.8, Pillow 12.3.0, fonttools 4.66.1, Apple clang 21; JieLi pi32v2 clang 4.0.1 archive SHA-256 f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958; SDK d179b4484759423312073f5fbb232501aa491047; linux/amd64 Debian digest 7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587. Upstream sanitizer exclusions remain signed-overflow/shift/bounds/object-size/pointer-overflow.

Explicit omissions: no DaisySP reference checkout, normal-suite V15 restore skipped because vendor firmware stays outside Git, absent editor MENU settings, and no Emscripten emulator. Hardware qualification is SKIP. Issues #9/#12 remain open for RAM fit, workflows, combinations and physical high-water/deadline evidence. No flashing, merge, release or deployment occurred.
