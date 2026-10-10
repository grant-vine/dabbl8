# Companion track client

Preparatory issue #11 client stage. `web/d8tracks.js` implements the versioned
in-memory track operations from [command 77](versioned-track-editing.md).
A user-facing companion page and physical MIDI transport remain to be built.
This module alone is not the completed editor or a firmware release.

## Negotiation and limits

`D8Tracks.connect(request)` first reads INFO. It queries command 75 only after
recognized final D8 schema-1 discovery. Capabilities must match INFO, the pinned
99-parameter/27-global/14-engine schemas, 64 steps and motion records, and eight
shared sounding voices. The new track feature and versioned-motion feature
must be advertised; expanded runtime must not claim safe legacy writes.

Untagged, old, default four-track, unknown-schema, malformed or mismatched
firmware remains identity-only/read-only; typed track requests refuse before
transport. Merely reporting eight tracks does not grant edits. Unknown feature
bits do not grant extra operations. Successful negotiation fixes immutable
limits; arrays and local index validation follow the reported count (up to
eight), not a fixed four or eight. The runtime compiler/tests exercise four and
eight actual tracks; five through seven client array shapes have mock coverage,
not separate target qualification.

## Typed API

The transport receives `[command, sevenBitArguments]` and resolves to a decoded
seven-bit reply payload. It must verify the reply command and convert a valid
D8_ERROR for that pending command to `TrackProtocolError(reason)`; reason 4 is
an explicit busy refusal. Physical MIDI framing/timeout/port ownership is the
next page/transport stage, not a capability inferred by this module.

After negotiation, the client exposes:

- `readTracks()` and `select(track)`: selected index and all track engine,
  preset, level, mute and armed metadata.
- `mix(track, update?)`: read or write level/mute; returns actual values.
- `parameter(track, id, value?)`: bounded parameter read/write, preserving
  signed biased-v14 values and the firmware's clamping/enum mapping.
- `dump(track)`: complete descriptor-count parameter array with engine/preset.
- `step(track, index, update?)`: full four-note/step-type/flags/velocity/drum
  masks/chance/ratchet read/write; bounds and local data validation precede send.
- `close()`: disable the session and prevent pending replies or queued requests
  from updating or continuing it. Reconnect requires a fresh instance/handshake.

Every edit uses only command 77 and one of the five allowlisted operations.
There is no public generic-send, project-save, sample/preset transfer, backup,
flash or global-write API. `editable` means this track subset only, never a
permission for legacy operations or new-format persistence. The existing
four-track browser remains unchanged and read-only on expanded firmware.

Whole transactions are serialized through acknowledgment validation, not just
transmission. Exact schema/operation/count/index/length and field ranges must
match before the next queued operation can send. An unknown, malformed,
mismatched or lost reply disables further editing until reconnect. Local
input errors send nothing and preserve a valid session. A well-formed busy
refusal is recoverable and keeps live mix edits available. Closing cannot undo
a request already transmitted or an already committed device edit; it ignores
that reply and prevents further queued traffic. Consumers must also reject
stale UI callbacks when replacing a connection.

## Validation and remaining work

`web/test_d8tracks.mjs` sends typed requests through a test-only line bridge
(`tests/track_client_bridge.c`) into the actual C USB/SysEx parser and editor.
The optimized and sanitizer bridges cover four-track read-only negotiation and
eight-track selection, isolated parameter edits, complete dumps, mixer and full
step round trips. Real song-busy responses are distinguished from connection
failure. Mock cases exercise malformed capabilities/acknowledgments,
negotiated counts five through seven, disconnect/reconnect and queued-write
cancellation. The source/bridge use no real MIDI, device backup or flash; all
real-handler replies assert zero simulated flash writes/erases. Both bridge
modes are integrated into the standard repository suite.

Remaining work: musical controls and labels in a responsive companion page,
physical transport with explicit port lifecycle handling, browser mock previews,
new-format project and backup conversion/round trips, and storage/runtime and
hardware acceptance gates. Do not close issue #11 or advertise a hardware-tested
editor from these host tests. No firmware, loader, flash map or reference hashes
are changed by this JavaScript/client-test stage.
