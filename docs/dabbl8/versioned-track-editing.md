# Versioned in-memory track editing

Preparatory issue #11 protocol work for experimental expanded firmware. The
shipping default remains four tracks. No eight-track project or backup format,
flash map, release package or hardware behavior is qualified by this change.

## Discovery and capability

Recognize the existing D8 INFO tag, then validate capability command 75 as
specified in [capability negotiation](protocol-capabilities.md). Feature bit 2
(value 4) advertises the schema-1 track envelope described here. It is offered
only when the runtime has more than four tracks. Expanded eight-track feature
bits are 5 (versioned motion and track envelope); the legacy-write bit stays
clear. Default four-track flags remain 3. Counts and parameter/engine schemas
must match INFO. Absence of the new flag never grants this operation.

This flag grants only the documented in-memory track operations. It does not
grant legacy writes, project/backup writes, preset or engine loads, sample
transfers or general UI/global changes. The existing four-track editor remains
read-only on expanded firmware; a companion must deliberately implement the
new envelope and capability-driven arrays before enabling its controls.

## Wire format

Use existing SysEx framing `F0 7D 46 4C ... F7`. Command 77 (`D8_TRACK`) requests
`[1, operation, payload...]` and replies with command 77 and
`[1, operation, result...]`. All data bytes are seven-bit. The operation is an
allowlisted existing track command number, not an arbitrary forwarded command.
The real upstream handlers produce the result and retain clamping, enum
mapping, motion capture, sync shadows and drawing refresh.

| Operation | Payload after schema/operation | Result after schema/operation |
|---|---|---|
| 27 TRACK | empty to read, or one track index to select | selected index, count, then the existing per-track engine/preset/level/mute/armed records |
| 28 TRACK_MIX | track, optionally level as biased v14 plus mute | track, level as v14, mute |
| 29 TRACK_DUMP | track | track, engine, preset, reported parameter-count values as v14 |
| 30 TRACK_STEP | track, step, optionally the existing 8/11/12/13-byte step payload | track, step, complete existing 13-byte step record |
| 31 TRACK_PARAM | track, parameter ID, optionally value as v14 | track, parameter ID, value as v14 |

Indices are zero-based: tracks 0 through reported count minus one, steps 0–63,
parameter IDs 0 through reported parameter count minus one. Values use the
unchanged 14-bit little seven-bit encoding biased by 8192. Parameter values and
step data retain the existing handlers' normalization; chance must be 0–100
and an explicit ratchet 1–4. Payload lengths must be exact, including the
supported historical short step forms. Unavailable indices, unknown operations,
missing fields and trailing bytes are refused before mutation. A full step
retains both eight-bit drum masks, probability and ratchet; no four-track
masking or aliasing is applied to its destination.

Command 76 (`D8_ERROR`) returns `[1, 77, reason]`: 2 unsupported schema, 3
malformed operation/length/index/value, or 4 busy. During song-chain playback or
arming, step writes and timing-parameter writes refuse with reason 4 instead
of reporting an unchanged state as a successful write. Reads and live mixer
parameters remain available. Selection keeps the existing watcher shadow
behavior. The legacy track write families stay refused on expanded firmware.
Default four-track firmware does not advertise or implement command 77; a
client must not send it without the flag.

## Evidence and remaining work

`tests/versioned_track_test.c` sends requests through the actual USB SysEx
parser and firmware editor handler at NPART=8, not a reimplemented mock. It
checks selection, parameter reads/writes, complete dumps, full last-step writes
and mixer writes for all eight tracks; other tracks remain unchanged. Every
non-allowlisted opcode is rejected without musical or flash mutation, including
nested envelopes and project/backup/sample commands. Malformed schema, lengths,
indices, chance/ratchet, busy guards and legacy-write refusal are covered in
optimized and ASan/UBSan modes by the standard suite.

Actual target compilation, memory/link checks and the full suite must pass
before this is called verified host preparation. None establishes real MIDI,
stack high-water bounds, ISR deadlines or physical audio. No new global state
array or flash writer is introduced. Hardware timing/recovery and the storage
and runtime acceptance gates remain open. The next editor stage must use the
new flag/counts, preserve read-only fallback on old or mismatched firmware,
validate replies, and keep unsupported save/backup controls disabled.
