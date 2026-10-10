# Native signature encoding cost

The native dirty-signature helper previously encoded its coherent snapshot once during capture, then encoded it again after excluding selected track and project-page controls. Both passes calculated chunk and whole-file CRCs. The optimized path shares a coherent structured capture with ordinary wire capture, validates the captured native state before normalization, and emits the canonical signature wire once. Invalid selected tracks still refuse. Ordinary wire capture retains its output/alias/capacity rules. No scheduler, automatic write caller, storage-ownership grant, driver, loader or flash-boundary behavior is added.

This supports [memory/timing issue #12](https://github.com/grant-vine/dabbl8/issues/12) and future [save-policy issue #13](https://github.com/grant-vine/dabbl8/issues/13). Evidence is in [the preserved test/cost audit](evidence/2026-10-10-native-signature-cost/README.md).

## Host measurement

On Apple M1 Pro/macOS 26.7.1 with Apple clang 21.0.0 at `-O2`, repeated measurements against exact parent `268981e672ae41ee12d96996a14b9f7e60437104` show:

| Valid native state | Parent median µs | Optimized median µs | Elapsed reduction | Instruction reduction |
| --- | ---: | ---: | ---: | ---: |
| Minimal, no motion/arrangement | 234.133 | 128.759 | 45.01% | 41.72% |
| Maximum fixture, 56 motion records | 272.604 | 149.286 | 45.24% | 41.52% |
| Full 64 records / 4 banks / 16 scenes / 16 rows | 274.655 | 150.673 | 45.14% | 41.35% |
| Full counts with active stored-base parameter | 274.261 | 150.858 | 44.99% | 41.35% |

Each source runs four cases, nine alternating-order batches of 1,000 calls per method/case, after 200 warm-up calls. Process instructions use macOS `proc_pid_rusage`; elapsed time uses `CLOCK_MONOTONIC`. The complete raw rounds, medians/ranges and compiler/environment are retained. Matching case lines verify unchanged signatures; the stored-base checks pass. The original earlier parent measurement is separately preserved privately, not overwritten. Wall-time noise and background Mac workloads remain possible; process counters and repeated ranges provide a second measure.

The portable optional [cost probe](../../tests/native_signature_cost_probe.c) uses actual runtime capture, not a simplified encoder. The checked-in maximum fixture has 56 motion records; the probe constructs eight valid nonduplicate records for the 64-record case without changing the fixture. `autosave_sig()` remains the historical comparison method, not a native replacement: it omits native arrangement and has different serialization policy. No performance threshold, CPU baseline or golden is rewritten.

These are host estimates, not target cycles, interrupt deadlines or LCD transfer measurements. The host cost case has no in-flight physical DMA; batch ranges are averages, not individual-call worst-case latency. Maximum counts are not proof of worst-case values. Do not select a production polling rate from these Mac timings alone.

## Validation and target tradeoff

Unchanged native-signature 13,774 checks / 3,579 musical mutations and adjacent runtime 8,911 checks pass in optimized and inherited ASan/UBSan modes. A new equivalence test adds 2,317 checks against the previous two-encode algorithm: canonical bytes/signatures, all 256 selected-track byte values, valid complete-step fields, invalid native state, short/overlapping output and preserved failure output. The same test also passes against exact parent. Existing signature tests cover active-canvas refusal and LCD late-start behavior. These tests do not establish target interrupt timing. Inherited sanitizers exclude signed overflow, shifts, bounds, object size and pointer overflow; this is not strict whole-engine UBSan.

Pinned default app remains byte-identical to the verified unchanged v1.1.5 baseline: 447924 bytes, SHA-256 `b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b`. Optimized eight-track app is 457188 bytes, SHA-256 `94e2d9122ea45da7e09c8ea4b673fbb043535e77515e4fdc026d4c3ac8c8ffc6`, 32 bytes above parent. General RAM remains 95732/98304 and pool 334164/344064. Interrupt RAM text remains 916 instructions/no calls. Image/memory/RAM-text and MMIO checks pass.

Linked target direct frames are signature 24 B, structured capture 12 B, ordinary wire capture 20 B, encoder 120 B and now-outlined native validator 56 B. Motion validation retains 36 B. The deepest verified synchronous signature-helper path is now 236 B through signature → encode → native validation → motion validation, versus parent 216 B. Thus this CPU optimization adds a 20-byte helper-only stack bound and 32 bytes of app code, while retained RAM/pool stay fixed. Neither figure includes caller/IRQ/context frames or proves real stack margin. No unresolved or indirect call was found in the audited linked helper scope; disassembly and call graph remain private with hashes.

Default target instruction budgets pass. Native target budget checks report rev_room and rev_spring overruns on both exact parent and changed image; their complete reports are byte-identical. These inherited native failures remain explicit rather than being hidden or recalibrated. They require broader native timing qualification, independently of this signature optimization. Fresh audio golden renders and the full repository suite were not rerun for this focused main-loop-only change; new-head CI and physical deadline/stack/recovery gates remain.

An initial orchestration check compared against a mutable research checkout's `source/build/felucca.bin`, which had become 447576 bytes / SHA-256 `d05d7de131b31956b2c154a096a78cd853c649be96976b02d026f393886b3524`. The comparison stopped despite the new default matching authoritative baseline `b69e74…`. The script was corrected to the immutable retained baseline hash; the raw initial failure is preserved. This was a comparator/reference failure, not a newly failing target image. No baseline binary was replaced to satisfy it.

The coherent capture still owns the existing LCD-fenced arena, rechecks transport after borrowing and captures under the existing interrupt-state guard. Validation before normalization retains original invalid-selection/native refusal behavior; encoding after normalization supplies the same canonical bytes. FNV32 remains a change detector with collision risk, not an integrity proof. Future scheduler integration must separately provide ownership, dirty/wear/retry policy, output-queue/tail/activity guards and boot/recovery semantics. No firmware was installed or released.

SPDX-License-Identifier: GPL-3.0-only. Preserve Felucca and dependency notices.
