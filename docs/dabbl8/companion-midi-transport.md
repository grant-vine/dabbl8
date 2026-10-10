# Companion MIDI transport

Preparatory issue #11 stage, based on the bounded [track client](companion-track-client.md).
`web/d8midi.js` provides `D8MidiSession.connect(input, output, options)` for explicitly
selected Web MIDI ports. It does not request browser MIDI permission, choose ports
by device name, provide a visible editor, or perform project/flash operations.
The page must request SysEx access following a user action and pass selected ports.
Port handling follows the [Web MIDI specification](https://www.w3.org/TR/webmidi/).

The resolved session exposes `tracks`, the existing typed `D8Tracks` API, and
`connected`. Only INFO, recognized capability discovery and command-77 track
requests can leave this transport. There is no public generic request/send API.
Default four-track or unsupported firmware stays read-only through the same client.

## Framing and failures

Outgoing frames use `F0 7D 46 4C command arguments F7`, with seven-bit data and
bounded payloads. Incoming editor frames must be complete, at most 600 bytes,
seven-bit clean and match the sole pending command. Known legacy WATCH pushes
are ignored; notes and unrelated manufacturers' SysEx do not acknowledge requests.
Malformed frames, mismatched commands, send failures and missing acknowledgments
close the connection and disable its client. No mutation is retried automatically:
a missing reply cannot prove an edit was not applied.

A firmware error must be exactly command 76 with `[1, pendingCommand, reason]`,
where reason is 1 through 4. These become typed `TrackProtocolError` refusals.
The client keeps a valid busy refusal (4) recoverable, and disables editing on
other refusals. Its existing exact operation/index/count/length validation still
runs before the next typed transaction sends. A semantically invalid reply can
disable client editing even when the MIDI ports remain open; the page must display
`tracks.editable` and `tracks.reason`, not infer edit permission from `connected`.

## Port ownership and cancellation

Each selected port has at most one session in this module. Successful connections
explicitly open both ports. A physical disconnect or externally closed port,
`close(reason?)`, or optional AbortSignal invalidates the session, rejects pending
requests, removes only its own listeners, closes both ports and releases ownership.
Opening one port unsuccessfully also cleans up both. `close` returns the same
cleanup promise on repeated calls; resolved per-port settled results expose any
browser close failure. Await cleanup before attempting another session on those
ports. This in-page ownership does not establish exclusivity against other tabs,
applications or copies of the module; users must avoid simultaneous editors.

Abort during opening prevents negotiation, and cleanup waits for both native
`open()` promises to settle before closing ports. Web MIDI supplies no cancellation
of those native promises. A browser whose open/close promise never settles can
therefore leave connection or cleanup pending; reply timeouts do not claim to bound
native port acquisition. Default reply timeout is 1500 ms, configurable from 1 to
30000 ms for tests or a page's deliberate connection policy.

Closed session callbacks cannot restart traffic. A new session always renegotiates,
and the page must also check its connection generation before applying asynchronous
UI results. The wire format has no request identifier or connection epoch: it cannot
prove rejection of arbitrarily delayed, identical valid replies delivered to a new
session by a device or driver. These tests cover old listeners and queued client
callbacks, not such physical timing. Awaiting cleanup and renegotiation is required;
physical transport/reconnect qualification remains an acceptance gate.

## Evidence and remaining work

`web/test_d8midi.mjs` tests framing, ownership, negative signed values, all refusal
codes, malformed/mismatched messages, WATCH/alien traffic, timeouts with no retries,
queued writes, port disconnect/closure, partial opening, cancellation during open
and handshake, listener preservation, cleanup failure and fresh negotiation. Its
simulated Web MIDI ports also route production transport frames through the actual
four/eight-track C parser and handlers: all eight selection/mixer destinations and
signed parameter round trips, with zero simulated flash writes/erases checked on
every real-handler response. Both optimized and sanitizer bridge modes are included
in the repository suite. This is host evidence, not a physical MIDI connection.

The separate [visible editor stage](companion-editor-page.md) now supplies musical
controls and browser simulator evidence. New-format save,
backup conversion/round trips, storage/runtime prerequisites and hardware gates
remain unfinished. Do not close issue #11 or claim a qualified firmware release.
No loader, flash boundary, firmware source, fixture or golden is changed here.
