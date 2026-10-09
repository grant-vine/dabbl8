# Bounded PHYS voice state — partial target-memory improvement

Preparatory work for #12, prompted by the actual eight-track link failure recorded in PR29. Runtime remains four tracks; this change does not complete #9 or #12. Source base and checked source-file hashes are in manifest.json; raw logs and exact verification commands are retained here. Dependencies are the unchanged issue3 baseline: Python3.14.8, Pillow12.3.0, fonttools4.66.1, Apple clang21, JieLi pi32v2 clang4.0.1 from archive SHA256 f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958, SDK d179b4484759423312073f5fbb232501aa491047 and linux/amd64 Debian digest7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587.

Expanded builds allocate eight PHYS bodies for the eight shared sounding voices, instead of three bodies for every track. Each body has one track/voice owner; retriggers retain the correct body. Only inactive or no-longer-PHYS owners can be reclaimed. A full pool refuses another unadmitted voice rather than aliasing a live body. The existing PHYS cap of three voices per track is preserved. Four-track allocation and rendering are unchanged. Lookup scans at most eight owners; this bounded host behavior is not a target deadline measurement.

| Actual target memory | Before PHYS pooling | After PHYS pooling | Capacity |
| --- | ---: | ---: | ---: |
| General RAM | 132740 | 132756 | 98304 |
| Engine pool | 451024 | 382288 | 344064 |

The exact expanded target probe still FAILS the linker: RAM overflows34452 bytes and pool overflows38224 bytes. PHYS storage falls from103104 to34368 bytes, saving68736 pool bytes and adding16 ownership bytes in general RAM. No valid eight-track image/package is produced. No linker region, loader code or flash boundary was changed. The probe uses a separate output directory and the same generated headers; it does not overwrite the valid four-track test image.

Four-track target build and complete upstream/extended host suite PASS. Image447924 bytes and package610086 bytes are byte-identical to PR27; loader, audio golden, CPU baseline and target-budget hashes match exactly. RAM92116/98304 and pool331332/344064 remain unchanged. The suite reports92 golden renders with0 changed and0 health failures, and ALL HOST TESTS PASSED.

Real eight-part PHYS tests PASS at O2 and with ASan/UBSan: distinct eight-track bodies, shared render budget, retriggers, ninth-note steals, engine/model changes, stale-owner reuse, full-pool refusal,5000 mixed-model steals, release and reset. Existing actual eight-part voice-budget tests also PASS under sanitizers. Sanitizer flags preserve the upstream exclusions for signed-overflow, shift, bounds, object-size and pointer-overflow; the exact flags and logs are retained. This is host/simulator evidence, not hardware qualification.

Explicit full-suite omissions: DaisySP reference comparison is unavailable; stock V15 restore is skipped because vendor firmware is deliberately kept outside Git (a separate earlier FakeFM1 restore/refusal test passed); the editor has no MENU settings test; Emscripten emulator is unavailable. Hardware qualification is SKIP. No golden files were rewritten and no firmware was flashed, merged or released. Remaining RAM/pool reductions, engine combination caps, target pool/stack high-water measurements and #9 acceptance remain open.
