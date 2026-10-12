# D8-004: capability negotiation

The development protocol retains the existing `F0 7D 46 4C ... F7` frame and stable parameter/engine IDs. It identifies the fork explicitly through a separate `D8` capability family. USB branding remains a later identity task (#19). Track-count expansion is not enabled by this change.

## Discovery and capability reply

The existing INFO reply retains all its fields and appends the final three-byte discovery tag `44 38 01` (ASCII `D8`, schema 1). A client must not infer Dabbl8 support from a firmware name or track count. If the tag is absent or its schema is unsupported, do not send new commands; use a read-only identity connection.

Command 75 (`D8_CAPS`) takes no arguments. Its exact 15-byte reply is:

| Offset | Meaning | Current value |
|---|---|---|
| 0–1 | Protocol family | ASCII D8 |
| 2 | Capability schema | 1 |
| 3 | Runtime tracks | 4 (maximum this family supports: 8) |
| 4 | Steps per track | 64 |
| 5 | Shared sounding voices | 8 |
| 6 | Shared motion records | 64 |
| 7–9 | Parameter / global / engine counts | 99 / 27 / 14 |
| 10–11 | Parameter / engine ID schemas | 1 / 1 |
| 12 | Motion section schema | 1 (D8M1) |
| 13 | Live project-save schema | 9 (frozen FUN9; D8P1 is not live yet) |
| 14 | Feature bits | Bit 0: versioned motion command 74; bit 1: original legacy writes are safe; bit 2: expanded in-memory track envelope 77; bit 3: versioned own-sample replacement command 80 (only when compiled) |

Every byte is seven-bit. Counts and schemas must match the separate INFO report. Unknown capabilities, unsupported layouts, timeout, malformed replies or inconsistent limits cannot grant writes. Unknown feature bits are ignored, rather than treated as an existing feature. Bit 1 is advertised only while the runtime retains at most four tracks. It must be cleared when eight tracks are enabled. Experimental expanded firmware advertises bit 2 for the separately versioned [track envelope](versioned-track-editing.md); it does not grant project, backup or other legacy writes. Default four-track firmware does not advertise that bit. The limits describe current behavior, not future implementation or hardware qualification.

Parameter and engine ID schema 1 retains the audited upstream IDs: P_COUNT 99 / P_E0 91, G_COUNT 27, retired DIGITAL engine ID 1 reserved, FM6 ID 12 and SLICE ID 13. New capabilities do not renumber them. The existing browser uses descriptors for values and retains its original four-track request layout.

## Unsupported-operation errors

Command 76 (`D8_ERROR`) carries `[1, original_command, reason]`:

- 1: the legacy write family cannot safely edit this runtime.
- 2: unsupported request schema.
- 3: malformed request or unavailable index.
- 4: busy, used by the expanded versioned track envelope when song playback/arming prevents a step or timing edit.

The versioned motion command validates before mutation and sends explicit schema/bounds errors. Invalid legacy commands retain their historical behavior on the current four-track runtime. The client request queue recognizes a valid error for its pending command and rejects immediately; an unrelated or malformed error cannot complete another request.

When the runtime expands beyond four tracks, the firmware checks command-family policy before invoking handlers. Legacy sound, preset, project, sample, backup and targeted track writes are refused. Identity and read requests remain available. Versioned motion is a distinct schema-validated command. This is a compatibility gate, not a session authentication system; later versioned mixer/project/editor operations must advertise their own supported limits.

## Editor negotiation and fallback

The browser obtains INFO first, queries command 75 only after recognized discovery, validates the exact capabilities, and enables its existing editing UI only for matching schemas/counts, no more than four tracks, and the advertised legacy-write feature. Its central session request path refuses every mutation before MIDI transmission when negotiation is absent or read-only. Reads use an explicit allowlist; unknown commands cannot acquire write access by default.

A fallback connection displays identity and its reported track count, hides the editor controls, and remains disconnectable. It does not attempt legacy bank or project edits. The UI does not present a four-track editor as capable of editing eight tracks. The eight-track editor is a later task (#11); that task must deliberately consume the negotiated new operations.

## Verification matrix

- Old editor / current new four-track firmware: original INFO data and commands remain compatible; appended discovery ignored by the legacy parser.
- New editor / old firmware: identity-only, no unsupported query, no write MIDI.
- Supported new editor / current new firmware: capability bytes from the real C handler match the mock; supported request path operates normally.
- New editor / eight-track capability report: this four-track UI remains read-only.
- Unsupported schemas, fields, lengths, values or mismatches: read-only or explicit error with state unchanged.
- Expanded-runtime command policy: legacy writes denied, reads and versioned schema path allowed. This policy check does not establish eight-track sequencing or target performance.

Chrome preview tests exercise the actual browser script against mock devices only. A test-only hook exposes the closure to the verifier; no test hook is added to production files and no real MIDI permission is requested.

## Additive engine observation query

The separate [schema-2 engine availability query](engine-availability-capability.md)
uses command 75 with exact argument `[2]`. The original no-argument schema-1
15-byte response and INFO discovery remain unchanged. The new eight-byte reply
contains stable-ID slot count and immutable compiled-engine mask with flags 0;
it grants no write/profile authorization. Explicitly opted-in clients must treat
unsupported/malformed replies or timeout as unavailable, never assume a full mask.

## Native sample replacement (development schema 1)

Command 80 is a separate bounded transaction; legacy sample writes 11–14 remain refused on eight tracks. Advertise feature bit 3 only when the actual native sample module is included. Capability negotiation authorizes the protocol family, not a physical flash operation. PREPARE stages a validated 480-byte FSMP header; the instrument shows REPLACE SAMPLE and requires SAVE press/release before erasing. No remote packet or cached READY flag replaces that consent.

Every request starts `[1, operation]`. PREPARE (operation 1) appends `slot0..2, pack7(header480)`. STATUS (2), COMMIT (4), CANCEL (5) append a five-byte seven-bit token (top byte at most15). WRITE (3) appends token, five-byte offset, then pack7 of at most256 bytes. Offsets start512, are256-aligned and strictly sequential; the final packet is exactly the remaining length. Only an identical most-recent page retry is accepted without another erase.

The exact reply is `[1, operation, rc, token5, phase, next_offset5]` (14 payload bytes). rc0 is success; 1 malformed, 2 busy, 3 stale/notready, 4 I/O, 5 cancelled. Phases are0idle,1consent,2release,3upload. COMMIT verifies data CRC and the immutable header, writes header bytes4..479 first and magic bytes0..3 last, then checks readback. Repeating successful COMMIT or CANCEL preserves completed status.

Replacement is in place. The previous sample is lost when the first sector is erased. Interruption can leave the selected slot empty; a completed final magic write with a lost/error acknowledgement can leave the complete new sample. There is no automatic restoration or retained A/B sample copy. Keep the source audio or own backup and restart from PREPARE after a cancelled/uncertain transfer. Other sample slots and project/autosave storage remain outside the transaction. This development protocol is not device/recovery qualification.
