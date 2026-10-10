# Native autosave session evidence

Source candidate based on exact PR74 fd2c1a458190b21f82f9dca69e4b3eae0c86f93c. See ../../native-autosave-session.md for caller authorization, policy, state reconciliation and limitations. Source SHA and file hashes are recorded in manifest.json.

Final optimized and inherited sanitizer tests pass with zero failures: session 275,985; automatic quiet/activity 594,684; actual logical output queues 249,563; mapped physical adapter 217,341; native pool runtime 525,110; canonical signature 13,774; native runtime 8,911 checks. This is actual application/runtime code against virtual NOR, synthetic clocks and actual queue code with synthetic disturbances, not hardware qualification. New session tests cover all 29 physical mutation cuts and pre/postcommit state changes. No golden fixtures or audio hashes changed.

Pinned default target: 447,924 bytes, SHA-256 b69e74e98721ca1f1124edec114c824e315102c65e1f89bf437619d8f048387b, RAM 92,116/98,304, pool 331,332/344,064. Native target: 458,608 bytes, SHA-256 3986f5720811f0aeeed3071675e83eae163ace82fd443788400bce90d4b46283, RAM 95,764/98,304, pool 334,204/344,064. Relative to exact PR74: app +1,080, general RAM +16, pool +40. Both build/mmio/static memory checks pass; .ram_text 916 instructions/no calls. Native leaves 2,540 general RAM and 9,860 pool bytes by static accounting, excluding stack/interrupt concurrency.

Direct linked frames (saved registers plus local allocation): begin 40, poll 44, reconciliation 40, flash snapshot 84, pool snapshot 112 bytes. These are only direct frames, not an additive whole-path bound. Linked excerpts retained; full symbols/disassembly/ELF/binaries stay private. Descendant, callback, caller and interrupt stack remains unqualified.

Preserved failure history: snapshot-initial-failure.log has 3 new-test failures expecting no reads from flash wrapper invalid-output calls. Opening the mapped backend legitimately scans before pool-level output validation. Tests were corrected to assert the pool API's actual before-access alias refusal; production source was unchanged. Earlier 72,521/274,505-check passing logs are preserved as intermediate matrices, not the final acceptance count.

Explicit skips/remaining gates: new-head full suite/CI; device flash writes/power cuts/endurance/erase timing; actual DAC/UAC peripheral drain and capture completion; device scheduler deadlines; every-instruction interrupt injection; whole caller/IRQ stack; complete migration/ownership activation; alternative target rates/TONE; release/install/merge/deploy. Existing native audio budget qualification remains a separate gate, not established by static memory checks. The separate PR73 signature optimization is not included. No public GitHub changes or firmware installation in this lane.

SPDX-License-Identifier: GPL-3.0-only.
