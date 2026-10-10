# D8-003 automation addresses

Runtime `motion_event_t.place` is an unsigned 16-bit value `(track << 6) | step`. Valid schema addresses are 0–511, eight tracks times 64 steps. Runtime validation additionally restricts the track to the enabled `NTRK`; this change leaves that count at four. Parameter IDs remain below 128, with bit seven retaining the lock flag. Values remain signed 16-bit and valid from -64 through 127. The shared event pool remains 64 records; 512 addressable positions do not mean 512 simultaneous records.

## Consumers audited

- `core.h`: runtime record and frozen historical record types.
- `motion.c`: validation, counts, masks, locks, capture, play/unlock, snapshots/replacement, find/sort/move and engine-change filtering. Address writes no longer narrow to eight bits.
- `project.c`: legacy compact motion import/export, parameter-ID migration, DIGITAL and SAMPLE PERC filtering, song source restore and autosave comparison.
- `song_chain.c`, `ui.c`, `ui_tools.c`: saved motion, undo snapshots and pattern transformations (the transformation write is widened).
- `ui_graph.c`, `ui_events.c`: track/step display and editing.
- `editor.c`: explicit track and step fields in requests/replies; historical command retained, new command/schema distinct.
- `web/editor.html`: legacy command 64 and four-track request limits retained until capability negotiation and the eight-track editor work. No browser path silently opts into the new schema.

## Frozen legacy records

FUN7–FUN9 motion remains 260 bytes: count, four-track enabled mask, two reserved bytes, then 64 four-byte records (one-byte place, parameter, signed LE16 value). `motion_legacy_store_t` asserts the old extent; explicit codec functions replace runtime-struct memcpy. Track-five-and-later addresses cannot be encoded in FUN9. The immutable historical fixtures must remain byte-exact. The wider runtime structure is not a disk format.

## D8M1 save section

The new standalone motion section starts with ASCII `D8M1`, a count byte (0–64), eight-track enabled mask, and two zero reserved bytes. Each record is five bytes: explicit track (0–7), step (0–63), parameter with lock bit, signed LE16 value. The exact section length is `8 + count * 5`, at most 328 bytes. Reject unknown magic/version, nonzero reserved bytes, invalid counts/lengths/track/step/parameter/value, and duplicate (track, step, parameter ID) records, including automation/lock duplicates. A failed decode preserves the destination; a failed encode preserves output.

This is a versioned section codec for the later D8P1 project format. It does not install a new flash-storage format or reinterpret an old project ID. Current live saves still emit frozen FUN9 for four tracks. D8P1 framing, size and atomic multi-sector persistence belong to #7 and #13.

## Versioned transport

SysEx command 74 (`ED_D8_MOTION`) starts requests and replies with schema byte 1. The remainder uses the existing explicit track/step operation shape from command 64, including operations 5–7 for locks/kinds. Requests to unavailable runtime tracks or unknown schemas are refused before mutation. Command 64 stays limited to original tracks 0–3; its reply bytes remain unchanged. Command 74 uses the existing `7D 46 4C` framing during development; capability advertisement and fork identity are subsequent #6/#19 gates. New clients must negotiate support before sending it.

## Verification scope

Exhaustive codec checks round-trip every one of the 512 positions independently, for automation and locks, in bounded pool batches. They test version, reserved bytes, duplicates, lengths, track/step boundaries, invalid parameters and signed value boundaries, and refusal without mutation. Real editor-handler tests check the version-prefixed path and unsupported schema/track refusal. Historical byte fixtures, motion and migration tests run unchanged apart from expressing their historical disk extent explicitly. Full eight-track playback and UI/editor behavior are later tasks; this codec does not claim them.
