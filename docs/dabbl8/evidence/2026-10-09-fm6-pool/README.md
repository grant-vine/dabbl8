# Shared FM6 operator state — partial memory improvement

Preparatory #12 work stacked on PHYS PR30; #9 and #12 remain incomplete. Expanded builds keep eight operator-envelope bodies for the eight shared sounding voices rather than six bodies for every track. The existing six-voice per-track cap, per-track patch/macros/LFO and default four-track allocation are preserved. Ownership scans at most eight entries; same-owner retriggers retain state, inactive or changed-engine destinations can be reclaimed, newly assigned bodies are cleared, and a full pool refuses an unadmitted ninth request instead of aliasing a live body. No dynamic allocation is introduced.

| Actual eight-track target memory | Before FM6 pooling | After FM6 pooling | Capacity |
| --- | ---: | ---: | ---: |
| General RAM | 132756 | 120932 | 98304 |
| Engine pool | 382288 | 382288 | 344064 |

FM6 bodies fall from14208 to2368 bytes;16 ownership bytes yield a net11824-byte RAM saving. The actual expanded target still FAILS the linker by22628 RAM bytes and38224 pool bytes. No valid eight-track image/package is produced. Loader code, linker regions and flash boundaries were not changed. The target probe only overrides NPART=8 in a separate app output directory and reads existing generated headers. This is not eight-track firmware acceptance or target deadline/high-water qualification.

Four-track build and full upstream/extended suite PASS with image447924B,package610086B,RAM92116/98304,pool331332/344064. Firmware, package, loader and golden/CPU/target-reference hashes are byte-identical to the preceding verified Mac build;92 golden renders have0 changes and0 health failures. The complete suite ends ALL HOST TESTS PASSED and includes actual eight-part voice, PHYS and FM6 tests at O2 and ASan/UBSan, plus all three fuzz tests.

FM6 tests exercise eight distinct bodies, retriggers, ninth-note steals, changed-engine reclamation with complete state clearing, return to FM6, full-pool refusal,5000 mixed-patch steals, operator-envelope release/reset and the unchanged six-voice single-track cap. After the full suite, the same-destination retrigger assertion was strengthened to require an unchanged nonzero operator phase, rather than only the same pointer. This stronger test passes separately at O2 and ASan/UBSan; phase-check.json records both test revisions and exact commands. No firmware source changed after the complete-suite run. Standard test-runner integration retains this stronger check in future CI.

Dependencies remain the original audited Mac baseline: Python3.14.8/Pillow12.3.0/fonttools4.66.1/Apple clang21, JieLi pi32v2 clang4.0.1 from archive SHA256 f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958, SDKd179b4484759423312073f5fbb232501aa491047 and linux/amd64 Debian digest7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587. Exact commands/results/hashes are preserved as text. Sanitizers retain upstream signed-overflow/shift/bounds/object-size/pointer-overflow exclusions.

Explicit omissions: DaisySP reference comparison unavailable, official V15 restore skipped because vendor firmware is kept outside Git (earlier separate FakeFM1 restore/refusal evidence exists), editor MENU settings absent, Emscripten emulator unavailable. Hardware is SKIP. Remaining memory reductions, declared engine combinations, pool/stack high-water and target-performance evidence stay open. No golden rewrites, flashing, PR merges, releases or deployment occurred.
