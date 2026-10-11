# Read-only Mac capture adapter preparation

`tools/dabbl8_readonly_midi.py` is an optional persistent full-SysEx transport for
`dabbl8_instrument_capture.py`. Its first qualification uses injected MIDI ports
and the actual C capture handler against virtual NOR. Native CoreMIDI, physical
port enumeration, device capture and recovery remain untested. This is transport
preparation for #11/#13/#14; it does not grant permission to connect, install,
restore, migrate or activate storage writes.

The adapter accepts only the complete version-1 command-78 read-only capture
request grammar: capabilities, BEGIN, scan, descriptor, bounded read, END and
abort. It rejects command 79, update/sample/preset/project-write commands,
`#consent` and arbitrary bridge input before sending. Each request uses the same
explicitly selected input/output pair; absent or duplicate names refuse. There is
no auto-selection, virtual port, broadcast or reconnect fallback. Endpoint names
are not attestation of hardware identity or exclusive ownership across other apps.

One pending request has one bounded reply slot. Wrong command/operation, token,
phase, size, duplicate/unsolicited SysEx, timeout, send failure or a closed/missing
endpoint poisons the adapter and requires a new connection. Ordinary MIDI notes
and clocks are not replies. No retries occur. Callback code does not close ports;
exchange and cleanup use the owning thread, and failures closing either port remain
observable. A reply's complete role/chunk/phase/provenance validation still belongs
to the unchanged collector. The protocol cannot distinguish every replay with an
identical operation and token; collector echo/two-scan checks remain mandatory.

The collector retains all 17 full-state roles (or explicitly 12 raw-store roles),
CRC/SHA-256, two scans, publication readback and its normal abort cleanup unchanged.
The adapter writes bounded received SysEx frames to private stderr diagnostics;
oversized backend messages are refused without retaining an unlimited payload.
The existing collector stores transport logs and original bytes in the requested
private destination. Keep captures/transcripts outside Git: they can contain user
projects, samples and presets. A successful collection is post-boot declared
musical state, not the entire physical flash or a known-good vendor restore image.
Capture uses volatile workspace/lease state; read-only means no NOR programming,
not absence of transient firmware state changes.

## Optional dependency setup for a separately authorised collection

Use a separate Python environment with `mido` and `python-rtmidi`; record the actual
installed versions/backend and source/tool hashes in that session's evidence. The
adapter lazily selects `mido.backends.rtmidi/MACOSX_CORE`; it does not inherit an
arbitrary backend from environment settings. No native dependency was installed or
imported during the offline qualification. No exact dependency version is claimed
qualified by fake-port tests.

[Mido's primary backend documentation](https://mido.github.io/mido/backends/rtmidi.html)
describes API suffix selection, lazy backends and callback support, and notes that
port close is not thread-safe. [python-rtmidi's primary API documentation](https://spotlightkid.github.io/python-rtmidi/rtmidi.html)
defines the Mac CoreMIDI API. OS port lifetime, startup delay and disconnect events
still require separately authorised physical observation.

The existing collector can invoke the executable adapter with explicit arguments:

```text
python tools/dabbl8_instrument_capture.py PRIVATE_NEW_DIRECTORY \
  --adapter /ABSOLUTE/PATH/tools/dabbl8_readonly_midi.py \
  --adapter-arg=--bridge \
  --adapter-arg=--input-port --adapter-arg="EXACT SELECTED INPUT" \
  --adapter-arg=--output-port --adapter-arg="EXACT SELECTED OUTPUT" \
  --provenance="UNQUALIFIED physical adapter; record exact device/session evidence"
```

This example is not executed by tests and is not authorisation to send commands.
The executable's Python environment must provide the optional dependencies.
The current instrument-session CLI remains simulation-only. This adapter refuses
its command-79 traffic; a physical read-only RESUME handoff would need separately
reviewed explicit physical SAVE/status polling, persistent transport/provenance,
abort/revocation and authority semantics. APPLY/physical writing is outside scope.
The existing collector's `_verified_handoff` foundation is preserved, not enabled
for production sessions by this transport.

## Offline verification

`tests/readonly_midi_adapter_test.py COMPILED_CAPTURE_FIXTURE` uses injected ports
and an actual C bridge. It covers full/raw capture, whole-NOR equality and zero
mutation callbacks plus malformed request/reply, duplicate, unsolicited, token,
timeout/late reply, disconnect, send/open/close failure, owner-thread and hex-line
cases. The normal and inherited-sanitizer CI capture blocks call it with the actual
fixture. Separate local four/eight-track runs retain their exact source/generation
receipts. Guard-removal negative controls must fail; neither this test nor the C
fixture opens MIDI ports or proves native USB/CoreMIDI behaviour.
