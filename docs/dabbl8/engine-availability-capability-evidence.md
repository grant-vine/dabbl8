# Engine availability exchange: local host evidence

Based on sound-selection prerequisite `9fcf06d`. Final executable source is
identified by the hashes below; the handoff includes the local topic commit.
Apple clang 21.0.0 (`clang-2100.1.1.101`), arm64 macOS, Python 3.14.8;
Node v26.11.0. No dependency download, target build or device activity.

## Executed checks

- Actual C USB/parser/handler: 4/8 tracks × FM4 0/1 × SLICE 0/1,
  optimized and ASan/UBSan: 16 runs, 1,943 assertions each, **31,088 PASS**.
  Tests begin simulated sounding voices and check all 128 schema bytes,
  malformed schema-2 lengths, exact no-argument 15-byte schema-1 payload,
  immutable compiled mask and retained runtime/UI/undo/patch/preset/store state.
- Actual simulated capture/owned arena: optimized and ASan/UBSan,
  **1,476 checks each PASS**, active capture survives every version request
  and malformed query; owned workspace, owner and virtual NOR stay unchanged.
- Read-only browser module: actual C bridges for four/eight tracks,
  FM4-enabled and SLICE-disabled; final **787 checks per mode PASS**,
  malformed/old/version/count replies and timeout mocks, and sparse holes at
  every INFO/schema-1/schema-2 byte position. This executes in Node, not a
  rendered browser or real Web MIDI session.
- Existing companion track client regression: **144 checks per mode PASS**.
- Shell syntax and whitespace checks PASS. All seven reused generated header
  hashes remain unchanged; no generation/download performed.

Sanitizer runs retain existing DSP exclusions: signed integer overflow, shift,
bounds, object-size and pointer-overflow. ASAN uses detect_leaks=0 and halt-on-error;
UBSAN halts on errors. No sanitizer policies, golden hashes or budgets changed.

Root's independent static review found one browser validation flaw: Array.some
skipped sparse holes. A pre-correction query with byte 5 deleted was accepted as
mask 16256. Dense for-of validation now covers INFO and both capability replies;
all sparse positions refuse, and malformed INFO prevents any capability query.
Schema-1 feature bits remain intentionally ignored by this observation-only
helper; the existing track client's write-feature contract is unchanged.

A private modified-candidate control moved the new dispatch after capture
revocation. It compiled, then failed 129 preservation checks as expected
(first: retained ed_capture changed). This is a guard-placement control, not a
pristine-parent comparison. The parent lacks the additive exchange entirely.

## Receipts and exact inputs

Full source/log/binary/generated-header hashes and exact commands:

- `/private/tmp/d8-engine-capability/run-z167pnid/receipt.json`: all C matrices,
  capture fixtures, earlier browser checks and existing track regression.
- `/private/tmp/d8-engine-capability/sparse-followup/receipt.json`: final browser
  correction and repeated module tests; C source and reused bridge hashes
  verified unchanged from the first receipt. No repeat of those C tests claimed.
- `/private/tmp/d8-engine-capability/late-dispatch-control/receipt.json`:
  private mutated source hash and raw expected-failure log.

| Tested source | SHA-256 |
| --- | --- |
| `firmware/src/editor.c` | `48653c8fe01cf650cef9319f6aa0ee984e6a00b2053757708c5c66d925d47619` |
| `tests/engine_availability_caps_test.c` | `a3c3502374aec9db58b896d090954935dc63eb7662c14d88097d31356ce0b5e2` |
| `tests/engine_availability_capture_test.c` | `22bdae5583dc58cc0763b0ba8334b843c662087f7c5805b6a40a78b594563083` |
| `tests/track_client_bridge.c` | `68d0f0eb8461b7cee5fdbade8adbc0e4a1ab7947d1dc9e20968a1456fc2e5a19` |
| `tests/run_tests.sh` | `ff1e1f756d2dc5da9f75af962260a56031c1cec1ed700718be2f06095ec8c7d0` |
| `web/d8engines.js` | `fef80770df5e305c674a746a22bc2f2df42aa22a6ec8c3fc223a0df12bd265fe` |
| `web/test_d8engines.mjs` | `7db2d72563d9263208aeb3f3d0cd2d6cd89b8b1ffa8710947b0d16b3f94a0de8` |

Generated inputs were borrowed read-only from
`/Users/grantv/Code/dabbl8/.local-baseline/uac-array-bounds/build/gen`.
Their seven exact hashes are in the retained first receipt.

## Scope and remaining review

Only production change: early, observational schema-2 dispatch in editor.c.
Original INFO and schema-1 reply stay unchanged. Stable slots include retired
DIGITAL, with its bit set only when the inherited FM4 implementation is compiled.
Flags zero authorize no writes or profiles. Timeout/refusal never implies all
engines. Production mask override guards and native/legacy quarantine stay closed.

The independent browser helper is explicitly opt-in; existing companion and
transport behavior is unchanged. Visible companion transport ownership/queueing,
engine selection/filter integration, safe startup/defaults, referenced project
policy and all remaining publication paths require review before complete #53
stage-2 claims. This exchange does not complete #11/#53, implement engine
exclusion, save memory, qualify hardware or authorize release downloads.

No target ELF/app/package, memory/stack report, physical audio/MIDI timing,
backup/restore, recovery, golden suite or full source-union CI was executed here.
Root performs later target/source-union verification separately. No loader,
flash-boundary, production-storage, release or website changes.
