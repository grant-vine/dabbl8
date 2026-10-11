# Read-only post-boot instrument capture evidence

Executable source: `a660427945c28bc8f95eb0606ad9d41602b64378`, based on exact PR79 `2c20a784d3defb7c1b247e0c82ca7099b0c5890f`. The evidence commit adds only documentation/logs. No firmware flashing, loader/boundary change, restore/production ownership activation or release qualification was performed.

## Observed results

- Complete frozen local suite: exit 0, 673.97 seconds. All six pinned baseline artifacts and 29 generated inputs remained unchanged; tracked source stayed clean at the executable commit.
- New actual editor/native/storage/USB fixture: **67,341 checks, zero failures**, optimized and inherited ASan/UBSan, both focused and inside the full suite. Real source executes against virtual NOR/hardware primitives: all17role transfers, USB configuration0→1 with unchanged resets, upgrade requests before writes, failed/valid store attempts, current selection/name/settings/preset/FM6 changes, timeout/wrap/token/offset/canvas refusals and exact original byte retention.
- New host collector: **287 checks**, optimized and inherited-sanitizer actual C bridge, plus independently pinned reruns. Complete raw/full modes, independent physical-address byte oracle and current logical references, unchanged simulated NOR, partial originals/transcripts, malformed/replayed/reordered/CRC/END replies and actual late interruptions.
- Target four-track image: **447,924 bytes**, SHA-256 `b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b`; byte-identical to immutable upstream baseline. Default target budget passes.
- Target eight-track image: **461,308 bytes**, SHA-256 `5013615ad3516bcefb9744a43eade99d8bb7531ad424c4cd483a7049ecd72cec`. Static RAM **95,780/98,304 bytes**, pool **334,312/344,064 bytes**; builder/MMIO/`.ram_text` checks pass. Remaining static headroom is 2,524 bytes RAM and 9,752 bytes pool. Compared with parent 95,764/334,204, capture adds 16 bytes staticRAM and 108 bytes pool. No full-instrument RAM copy is allocated.
- Native target budget is **NOT qualified**: exit1 with existing `uac_tap48` cost 570 vs 504 (+13%), `rev_room` 296 vs 218 (+36%), `rev_spring` 176 vs 126 (+40%), above 10% limits. These failures were retained rather than changing budget/golden files. Fit in memory and host success do not resolve audio deadlines.

## Direct linked frame evidence

The inherited `ed_handle` direct frame is **2,024 bytes in both parent and capture** (52 saved-register bytes plus1,972 local bytes). New capture handling is inlined there. `ed_service` 60, `edc_ram` 40, `edc_read` 32, `edc_stopped` 12, `edc_valid` 8, `edc_reply` 16; native capture 20, capture-state 12, project-capture 44, native encoder 120, wire writer 124, storage read 28 bytes. These are direct linked prologues/call names, not a whole caller/IRQ/ROM stack bound.

The linker places stacks separately: user `0x01C74100..0x01C7A000` (24,320 bytes), supervisor `0x01C7A100..0x01C7C000` (7,936 bytes), each after a 256-byte guard. The 2,524-byte staticRAM remainder is **not** stack headroom. No device stack/timing qualification is inferred.

## Skips, failures and reproducibility

Explicit skips: DaisySP reference unavailable; optional editor MENU-settings case absent; official V15 restore file absent at the expected repository-root filename; browser emulator unavailable without emcc. Descriptive test lines containing “skip” are not all skipped checks; raw result JSON labels that distinction. External retained vendor firmware was not copied into the repository or counted as qualified recovery evidence.

Resolved setup failures are recorded in `setup-attempts.md`: missing generated inputs, missing test HAL linker stubs, incorrect test-only panel member, an invalid help invocation, and the initial contiguous-versus-spaced adapter hex mismatch. The stale previous-binary output after the panel compile failure was not counted as validation of edited source. Raw adapter failure remains included. Final freshly compiled checks supersede these setup failures.

Pinned environment: Apple clang 21.0.0 ARM Mac host; JieLi clang 4.0.1, toolchain archive SHA-256 `f686586bcfb45e0f0bb27fd2b39c7a7f313cb4f0e88a66a14da621ffa8225958`; AC79 SDK `d179b4484759423312073f5fbb232501aa491047`; Docker linux/amd64 `debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587`; isolated Python 3.14.8, Pillow 12.3.0, fonttools 4.66.1. Inherited sanitizer exclusions: signed-integer-overflow, shift, bounds, object-size, pointer-overflow; leak detection off. This is not strict full UBSan qualification.

`verify.py` binds the full-suite source, baseline artifacts and generated inputs; `target-verify.py` binds the separate4/8 target outputs and pins. Exact binaries/ELF/full linked disassembly and test archives remain private under `.local-baseline`, outside Git; no vendor firmware or personal backup files are committed. Public logs are synthetic-test/build evidence.

## Remaining issue gates

This is a read-only capture/collector implementation, not full restore/migration completion. Archives preserve the declared post-boot migration starting point and distinct current logical state, with CRC and host SHA identity; they do not authenticate transport/device provenance or prove earliest vendor/pre-upgrade originals. Twelve mapped musical allocations are not a complete physical flash image. Raw-only mode is explicitly incomplete current-state capture. Neither mode authorizes writes/ownership.

A qualified original-preserving APPLY/RESTORE executor, external original retention/recovery proof, durable production authority, actual device transport/power-loss/timing verification and the native audio budgets remain open. Issue11/13 are not closed by this evidence.
