# Additive read-only engine availability negotiation

Preparatory issues #53 stage 2 and #11 exchange. This observes the engines compiled
into the current firmware. It does not authorize a write, enable a profile, prove
engine exclusion/memory savings, or qualify a device. Production profile override
guards remain closed. The immutable mask is the existing compatibility-aware
`D8ENGINE_EFFECTIVE_MASK`, not a requested card configuration.

## Wire request and exact response

Use the existing `F0 7D 46 4C cmd args... F7` framing. Command 75 with **no
arguments** retains its exact schema-1 15-byte payload byte for byte, including
existing feature bits and compiled counts. Existing clients do not query or infer
this new capability, and the INFO discovery/trailer is unchanged.

After recognized D8 schema-1 INFO discovery and validation of the existing
schema-1 reply against INFO, an explicitly opted-in client may query command 75
with the single argument `[2]`. Its exact eight-byte reply is:

| Offset | Meaning | Value |
| --- | --- | --- |
| 0–1 | Family | ASCII D8 (`68,56`) |
| 2 | Reply schema | 2 |
| 3 | Stable engine ID schema | 1 |
| 4 | Stable ID slot count | `NENGINES` (13 without SLICE, 14 with it) |
| 5–6 | Effective compiled-engine mask | unsigned bits 0–6 then bits 7–13 |
| 7 | Authorization/qualification flags | 0 |

All bytes are seven-bit. Bit `i` describes stable engine ID `i`; IDs are never
compacted and the slot count is not the number of available engines. DIGITAL ID
1 remains a reserved slot when FM4 is disabled (bit 1 clear). With the inherited
FM4 compatibility build enabled, DIGITAL is implemented and bit 1 is set. FM6
stays ID 12; optional SLICE stays ID 13 and is outside the slot count when omitted.
A mask describes engine availability only, not sample/patch presence, capacity,
performance, safe combinations, project compatibility or hardware verification.

Flags 0 grant **no new write/profile authorization**. This reply cannot activate
legacy writes or be parsed as the schema-1 track capability. A future profile or
write negotiation requires its own reviewed contract. Neither timeout nor missing
support implies a full-engine mask.

## Refusal and state preservation

For nonempty command-75 requests, a first byte other than 2 returns command 76
payload `[1,75,2]` (unsupported request schema). Schema 2 with any trailing bytes
returns `[1,75,3]` (malformed shape). The schema check precedes the shape check;
therefore `[3,0]` is an unsupported schema. No-argument command 75 remains the
original valid schema-1 request.

The new query and these refusals dispatch before capture revocation, migration
session checks, workspace borrowing, editor shadow changes or other handlers.
Reply buffer/MIDI output changes are intentional; runtime, voices, patches, undo,
transport, cache, captured snapshot, owned arena and stores remain unchanged.
Legacy storage/write-family quarantine is untouched.

## Independent browser API

`web/d8engines.js` exports `parseEngineAvailability(payload)` and
`D8Engines.connect(request)`. The request callback uses the existing typed
`[command, sevenBitArguments]` convention and must verify matching wire commands,
reject command-76 errors and provide timeout/lifecycle handling. Negotiation sends
only INFO, command 75 `[]`, then command 75 `[2]`. The returned object exposes
`available`, immutable `caps` (`schema`, `engineSchema`, `slots`, `mask`, immutable
`ids`), `has(stableId)`, `reason` and `close()`. It has no write or generic-send API.

Missing discovery prevents both capability queries. Unsupported/malformed reply,
INFO/count/schema mismatch, firmware refusal or timeout leaves `available=false`
and `caps=null`; no fallback mask is invented. Stable IDs are checked before
membership inspection. The helper validates both optional SLICE slot counts and
does not infer permissions from schema-1 feature bits. The schema-1 feature
byte is intentionally ignored by this read-only helper; existing track-client
feature/write validation is unchanged. Every indexed reply byte must be a
seven-bit integer, including rejection of sparse-array holes. This independent helper remains explicitly opt-in. The visible companion now opts into the same observational exchange through its own single serialized session; see [companion-editor-page.md](companion-editor-page.md). Other session users retain the original default handshake.

## Evidence and remaining integration review

The host test uses the actual USB parser and editor, retaining the original
schema-1 bytes, all 128 request schemas and malformed trailing bytes across
four/eight tracks and both FM4/SLICE switches. It starts actual simulated sounding
voices and snapshots runtime/UI/undo/patch/preset state on refusals. A separate
actual native eight-track capture fixture verifies full capture/session and
workspace bytes, caller input, owner/generation and NOR read/write counters
survive exact queries/refusals against virtual NOR, across both FM4/SLICE
switches in normal and inherited sanitizer modes. It covers all 128 schemas
with one, two and three argument bytes during initial capture, after one poll
and with an exclusively owned migration arena. Four-track firmware has no native
capture/owned-arena protocol; its separate actual editor fixture covers response
and runtime-state preservation. See [supplemental matrix evidence](engine-capability-capture-matrix-evidence.md). These tests do not perform physical
MIDI, flash or target qualification.

The Node/browser-module test runs through the real C bridge for four/eight-track,
FM4-enabled and SLICE-disabled configurations; hostile/old replies and timeouts
use mocks. This is module execution in Node, not a rendered Chrome page or real
Web MIDI session. Existing track-client regression tests remain required.

The visible companion now displays availability through its owned transport with separate [host evidence](companion-engine-availability-evidence.md). It offers no engine-choice writes. Before claiming complete stage-2 integration, review all engine selection/filtering and unavailable stored-record preservation paths. Separate startup/default,
all publication/ref-project guards, true exclusion, target reserves and physical
performance/recovery gates remain. This exchange alone does not complete #53 or
#11 and provides no firmware download/install authorization.
