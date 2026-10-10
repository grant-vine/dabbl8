# Guarded mapped backend evidence — 2026-10-10

Functional implementation commit `1a7e2f811c30ad5f2f0620beeb8ddc164810f61e`, parent migration `96d7737d32a2fef28a3a7aceea1092bffba20758`. Actual Mac target builds and full suite at that implementation pass. Post-review changes refine tests to match upstream non-nesting IRQ helpers, add final-inventory/postcommit/range cases, clarify comments and change the plan's implementation/activation metadata. Final optimized/sanitizer focused runs pass; migration report regression127checks passes. No executable firmware behavior changed after the target/full-suite run.

- Final standalone mapped session: **93359 checks**, zero failures in optimized and strict ASan/UBSan.34mutation cuts,34late-stop cuts,109inventory read cuts,80rotating saves.
- Final actual runtime/physical adapter: **72571 checks**, zero failures in optimized and upstream-equivalent ASan/UBSan.29driver cuts,29PLAY injections at write critical-section entry.
- Mapping verified against all five exact existing addresses; all nonpool regions of 1MiB simulated NOR byte-identical. Unauthorized, blank, legacy, malformed/foreign, busy, invalid/page/overflow and failed reopen sessions refuse access; close revokes.
- Stopped-state loss at final inventory read revokes authorization. Loss after commit may leave a valid new record with an error; readback after rescan verifies that documented case.
- Runtime capture/save/load/boot-policy flag checks use actual eight-track state; round trips retain track8, pending requests/active display refuse, successful manual identity publishes after commit and autosave has no manual slot. Physical hooks are simulated; no device used.
- Full suite:92unchanged goldens,0health/voice/CPUbudget failures,2existing SAMPLE timing notes and a WHEEL static cost note; no budgets/hashes rewritten. Explicit four omissions: DaisySP reference, private vendor fixture, editor MENU settings and browser emulator. Full log private `.local-baseline/mapped-backend-check/full-upstream-suite.log`; direct summary committed.
- Default4image447924SHA b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b byte-identical. Eighttrackimage454616SHA6156abeb3c64ba3c83343acd69f7b1e7380778526df171b93cc03661684679d1; RAM95716/98304,pool334100/344064,ramtext916instructions/no calls. New adapter symbols confirmed linked.
- Actual partial local/register prologues recorded separately, not complete caller/callback/driver/IRQ stack bounds. Default package/loader/golden/CPU/target hashes remain unchanged.

Compiler/runtime/dependencies retain the audited baseline: Appleclang21,Python3.14.8,Node26.11.0,Pillow12.3.0,fonttools4.66.1; JieLi20250324.1 archiveSHAf686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958,SDKd179b4484759423312073f5fbb232501aa491047,linux/amd64Debian digest7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587. Runtime sanitizer retains upstream exclusions for signed-overflow,shift,bounds,object-size,pointer-overflow; standalone mapping/codec/pool sanitizer has none.

Commands from repository root:

```sh
clang -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8pool_mapped_test.c -o /private/tmp/d8mapped
/private/tmp/d8mapped tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p
clang -O2 -w -Ibuild/gen -Ifirmware/src firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8p1_flash_runtime_test.c -lm -o /private/tmp/d8flash
/private/tmp/d8flash
sh tests/run_tests.sh
```

Standalone sanitizer adds `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined`; runtime sanitizer additionally uses the exclusions above and `ASAN_OPTIONS=detect_leaks=0`. Both tests are in normal/sanitizer runner sections. Pinned target build uses existing tools.build.build_app/check with separate build/mapped-backend-4/8 destinations, not release/package/install commands.

Initial harness failure retained: protected-range length wrongly included the authorized autosave block, and a one-byte magic corruption intentionally qualifies as a torn header rather than proving foreign data. Corrected the independent protected range and used literal FELU bytes; no runtime bypass. Initial runtime-test compile lacked a mapped-address declaration; included the header. Review then identified the test's initial nesting-preserving IRQ model differed from actual helpers; final test models unconditional STI and driver inner critical sections. These are evidence corrections, not waived failures.

Native ownership policy, actual snapshot capture/provenance, approved interruption-safe migration executor/restore, editor/menu/cache/transfer and boot/autosave scheduling remain open. IRQ-enabled serialized main-loop only; no general nesting, hardware performance, live saving or release qualification claimed. GPL and notices preserved.
