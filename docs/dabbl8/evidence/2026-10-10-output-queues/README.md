# Logical output queue gate evidence

Source commit 43b413cb99955f231c4f31db0f2265b38efd391d, exact parent a686c29a7acb298ed38ab85b486e7a7b6c7f2b5f. See ../../native-output-queues.md for invariants and remaining gates.

Optimized and inherited sanitizer queue tests: 249,563 checks, zero failures, 1,444 independent real-buffer oracles; seven queue states at all 29 late mutation entries plus postcommit restores. Actual benchmark adapter refusal: 73 checks, zero failures. Relevant existing optimized/sanitizer activity 594,684, resident tails 1,256,615 and physical adapter 217,341 checks pass. Existing audio and USB ring/FIR tests pass, including native USB accounting.

Pinned default target byte-identical (447,924 bytes, SHA-256 b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b). Native target 457,396 bytes (+372 from parent); RAM 95,748/98,304 (+16), pool 334,164/344,064 unchanged. Native target SHA-256 c5c9d020d7c54ffd509f45df6b2f6209820bdbc83ce312351e1db9702ca92550. Full linked symbols/disassembly remain private; the guard excerpt shows fresh bounded reads after IRQ-off mutation entry.

Actual native audio_block final DAC/USB-tap output across 1,200 blocks: parent/current 614,400 bytes, identical SHA-256 b3b012b65b38628d4753351302a8f85ec0b11624d007d379514bb51c71d16a73. This is a finite host workload, not device continuity proof.

Initial failing new-test logs are retained: rate-switch silence packet did not clear the old underrun frame (correct conservative refusal), and a test initially expected a re-prime publication on every steady render (the actual steady path has one tap publication). Corrected expectations use actual zero production and observed publication boundaries. No production/golden data was changed to accommodate these failures.

Explicit skips: full new-head CI/full suite, actual storage_hw.c silence execution in host fixture, every-instruction interrupt injection, no-48-kHz/TONE target variants, peripheral FIFO/codec/host capture completion, actual erase/program deadlines, flashing/installing, ownership/migration activation, scheduler, release/merge/deploy. Indefinite deferral on stranded ring/history state or sticky uncertainty is an intentional conservative availability limit.

Parallel integration must retain PR #71's canonical signature helper and both test-runner entries. No pushes or GitHub mutations were made by this lane.

SPDX-License-Identifier: GPL-3.0-only.
