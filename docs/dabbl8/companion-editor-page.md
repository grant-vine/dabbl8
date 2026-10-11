# Visible companion editor

Preparatory issue #11 stage. `web/d8companion.html` is a standalone firmware-repository
web tool, separate from the public Dabbl website. It uses the
[typed track client](companion-track-client.md) and
[port-owning MIDI session](companion-midi-transport.md). Physical MIDI qualification,
new-format persistence and required storage/runtime gates remain unfinished.

## Current controls

The negotiated track count drives the cards and index limits. Each track shows its
engine, level, mute, record-armed state and device selection. Selection and mix
changes use only the bounded command-77 operations. Eight tracks still share eight
sounding voices. Play, record, engine/preset loads and global controls remain on the
FM-1; the companion does not infer permissions for these from track-edit capability.

The selected track's complete 99-parameter dump supplies a musical parameter
selector and native ranges, including signed pan. All 14 engine slots have their
own eight parameter descriptors; common parameters remain the pinned 91 entries.
`tools/dabbl8_descriptors.py build/host/desc.json web/d8descriptors.js` regenerates
that snapshot from the actual host export and refuses a different schema. The
model tests compare the exported data exactly and check all 1,386 engine/parameter
combinations. This does not import the newer upstream 111-parameter schema.

All negotiated steps (64 in this schema) are available. The step editor preserves
four stored notes and active count, note/tie/rest, accent/slide, velocity, eight drum
hits and accents, chance and ratchet. Drum accent selection also enables its hit;
removing a hit clears its accent, so inconsistent masks cannot be submitted.
UI inputs use native musical ranges and note names; no parameter IDs or wire
schema choices appear in the user flow.

## Synchronization and lifecycle

Apply operations capture form values, use the typed client, and reload actual
acknowledged state. Invalid parameter values refuse before transport; typed mix/step validation
prevents malformed writes. Pending
operations disable other editing. Refresh replaces unapplied forms with device
values; there is no background synchronization or automatic project persistence.
Refresh after device-side changes. Engine/preset preflight before parameter or step
writes discards cached step data and requires explicit refresh/review if the sound
changed. This read-before-write guard is not an atomic compare-and-write: the wire
has no expected-engine token. Avoid simultaneous engine/preset changes while
applying an edit; physical race/transport qualification is still pending.

Discovery requests browser SysEx access only after Find MIDI ports. The user
explicitly chooses input/output ports, then connects. The page is intended for
Chrome on localhost/HTTPS with Web MIDI support. Denied access leaves discovery
available; unsupported/four-track firmware is identity-only with a legacy-editor
link. This page does not offer a generic-send, legacy write, project/sample/preset
transfer, backup, installer or firmware download API.

Connection generations and AbortSignal invalidate late discovery/connect results,
old rendered controls and asynchronous view callbacks. Disconnect removes editing
controls; reconnect renegotiates. Page exit/destroy also cancels and closes the
session. The model checks closure between reads and before post-preflight writes.
The MIDI session's documented native-promise and wire-epoch limitations still apply.

## Local preview and evidence

Serve only the repository's public `web` folder on localhost to preview
`d8companion.html` in a disconnected state; no device or MIDI permission is required
to view it. Do not serve private backups, vendor firmware or the repository root.
The page is not yet copied into the upstream release-site generator or deployed.
It provides no installation instructions for an unqualified firmware release.

Both optimized and sanitizer host-C model sections pass 48 checks: all eight
selection/mixer/pan destinations, independent values, complete last-step capture,
exact musical descriptors, range refusal, changed-sound review, concurrency and
closure between follow-up reads. This supplements the existing 85 transport and
144 client checks, not physical MIDI evidence.

Chrome preview used injected simulated ports and the production MIDI transport,
with only commands 1/75/77 forwarded to the actual eight-track host C parser.
Track 8 mix/pan and step 64 notes/drum/chance/ratchet round-tripped; other mixer
values remained unchanged. Desktop 1440×900 and mobile 390×844 had no horizontal
overflow. Four-track capability shape, access refusal and cancelled late discovery
were mock browser cases; actual four-track refusal is separately covered by the C
bridge tests. No actual MIDI permission, device port, flash, backup or installation
was used. See [preserved evidence](evidence/2026-10-10-companion-page/README.md).

The full repository suite passed locally. Required new-format project/backup round
trips, #7/#9 and physical acceptance gates remain; keep issue #11 open. No firmware,
loader, flash map, fixture or golden changes are part of this page stage.

## Read-only compiled engine availability

The companion explicitly opts into `D8MidiSession.connect(..., { engineAvailability: true })`. The default transport still sends only INFO and the exact schema-1 capability request. The companion reuses those replies to validate discovery, then sends one command-75 `[2]` request through its existing serialized port owner and listener, before exposing the track client. It does not create a `D8Engines` connection or another MIDI session.

A separate Engine availability panel shows stable IDs as Available or Unavailable only after exact schema-2 validation and matching INFO/schema-1 slot counts. Slot 13 is unavailable when outside a verified 13-slot build. Historical ID 1 is labeled DIGITAL; its advertised bit can be available on an FM4-enabled compatibility build. These observational IDs are not a canonical engine selection policy. The current firmware still compiles the full permitted configuration; this display offers no profiles, engine writes, exclusions, performance qualification or hardware claim.

Unsupported discovery, refusal, invalid capability payload or a missing optional reply shows Unknown, never an assumed full mask. Unknown availability does not authorize writes and does not replace the existing track client's independently negotiated editing contract. Four-track/unsupported track clients retain their identity-only connection. Disconnect, port loss, cancellation and page teardown clear the observation.

The optional read has a bounded, non-retried timeout that leaves the independently negotiated connection usable. Command 75 is never requested again by this session after the optional exchange completes. Late or duplicate well-framed command-75 replies and exact refusals for that completed query are discarded, including during a later command-77 transaction; they cannot acknowledge an edit. Invalid framing, alien command replies, malformed refusals, send failures and normal track transaction timeouts retain the original session-closing policy. Any future reuse of command 75 needs a new correlation design rather than reusing this handshake-only exception.

Evidence: [companion-engine-availability-evidence.md](companion-engine-availability-evidence.md). Physical MIDI and browser rendering qualification remain pending for this addition.
