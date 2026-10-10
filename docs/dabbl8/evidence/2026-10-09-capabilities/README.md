# D8-004: protocol capabilities

Source is stacked on issue #5 commit `db4fe1077b9b3e80cf7d029b6231ce33f6c27d26`. The audited baseline remains unchanged upstream v1.1.5 `276f72a4e6ea8a12499a7a6819aadf3165126755`; the [pinned environment](https://github.com/grant-vine/dabbl8/tree/fc4822e700b8d9f0c1aab8d52ca793eaa3cfac69/docs/dabbl8/evidence/2026-10-08-baseline) is reused. The [wire format and matrix](../../protocol-capabilities.md) specify family/schema/limits, stable IDs, errors and read-only fallback.

## Results

- Target build PASS, full host suite PASS, including existing legacy protocol/automation/project tests, audio hashes, ASan/UBSan and fuzz runs.
- Real C capability reply: `[68,56,1,4,64,8,64,99,27,14,1,1,1,9,3]`. Exported INFO and capability replies are included here.
- Unsupported motion schemas, malformed capabilities and unavailable indices return explicit errors before mutation. Expanded-runtime family policy rejects legacy writes; its eight-track boundary is a policy test, not eight-track playback.
- Final focused editor and web suites PASS after an additional independent compatibility observer was added. The unchanged upstream v1.1.5 Reader/INFO parser in tests/fixtures/protocol reads the real new firmware reply with every original field identical when the appended tag is omitted. The initial full suite predates this additional observer; final logs prove it separately.
- New editor / old firmware: no unsupported capability query, identity-only UI, mutation refused before MIDI. Unknown schemas/counts/feature limits, malformed replies, mismatched INFO and timeout all remain read-only. Supported current pairing permits its write path.
- Installed Chrome exercised the actual browser script at localhost against simulated devices only. A test-only closure hook (not committed to production HTML) verified that a read-only attempted write sends zero MIDI; supported connection loads normally. Both pairings emit no console/page errors. Desktop screenshots visually inspected and held locally under /private/tmp/dabbl8-editor-preview. No physical MIDI permission/device was used.
- Image 447,908 bytes (+172 relative to issue #5); package 610,086 bytes. General RAM 92,116 / 98,304 B; pool 331,332 / 344,064 B, unchanged. Artifact hashes, loader/golden/CPU/target-budget hashes in results.json; those reference files and loader remain unchanged.

The runtime still has four tracks. The existing editor deliberately refuses eight-track editing until #11 implements that client. D8P1 persistence, hardware qualification, fork USB identity and recovery remain later gates.

Optional full-suite skips remain explicit in tests.log: DaisySP reference, repo-root V15 filename, Emscripten. The separate stock restore simulation passed in issue #5 evidence; no hardware restore is claimed here. No flash/install, release, merge or deployment occurred.

## Development checks that initially failed

The first focused editor run failed its original suffix-relative INFO trailer assertion because three discovery bytes were appended. The test now excludes those final discovery bytes and checks all original bytes unchanged. Its initial log was overwritten by the focused rerun; this report records the failure explicitly rather than claiming that raw attempt is preserved.

The first focused web run retained in web-focused.log failed the isolated session-test harness: it extracted the new sessionRequest path without its negotiation helper. The harness now uses the real request guard and an explicitly supported session while separate pairing tests cover read-only behavior. web-focused-rerun.log and web-final.log pass. Initial Chrome verifier attempts could not access the editor's private closure; the final verifier adds a local-only hook, and chrome-pairings.json records the successful actual-page checks. The stale connect hint in the read-only view was removed and the final Chrome check repeated.

Full-build/test logs and final focused/browser evidence are retained as text only; no firmware binaries, private backups or vendor firmware are included.
