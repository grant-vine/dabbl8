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

## Current-main qualification receipt

The exact integration source `9b8c1798c40e25fab528ce2ffb431050db4972ca`
(tree `d28501fd59ead24ed2c96ecb0ae1da60b2e87e9d`) has parents current main
`1466d4b4f0fb4244f0491fbf3cb4103bfd3b675d` and original adapter
`441fb281e69000bb432a84c7bdc8f8d944ccde04`. Adapter/tool test bytes match the
original; the actual capture translation unit changed with profile/UI/voice/project
hooks, so earlier capture execution was not reused. Fresh FULL four/eight-track,
FM4-off/SLICE-on normal and inherited-sanitizer runs passed 246 checks each.
This does not qualify reduced-profile capture or the complete test suite.

Each run retained complete 17-role/raw12 captures and three hostile READ cases:
correct operation/token with wrong role, offset or payload CRC. All hostile cases
refused without verified publication and retained exact private transcripts; all
virtual NOR bytes remained unchanged with zero write callbacks. Original and new
raw receipts remain separate. The compact [receipt](evidence/2026-10-11-readonly-midi-adapter/receipt.json)
binds source, logs and scope; actual data/transcripts and host executables stay
outside Git. A targeted command79 guard-removal probe supplements the original
broad negative control: unchanged code sends nothing, while removal of the exact
request barrier emits79 and fails its explicit no-send assertion. The original
broad control failed earlier on an empty-request fake-reply exception and must not
be cited as that isolated79 proof. Duplicate/token/size controls remain unchanged.

## Raw CoreMIDI inventory hardening

The optional CLI wraps the explicit RtMidi CoreMIDI backend with fresh,
per-direction native inventories. It retains duplicate names rather than using
Mido 1.3.3's coalesced name list. Inventories allow at most 256 endpoints, each
with a nonempty name of at most 1,024 characters and no ASCII control characters.
Malformed API bindings, import/scan failures or failed temporary-object deletion
refuse without falling back to coalesced names. Temporary RtMidi C++ objects are deleted on
the owning thread; their shared OS client can outlive those objects. No explicit
open/connect/send API is requested by inventory. The pinned native cleanup defect
blocks native CLI qualification; see [lifecycle evidence](native-midi-lifecycle.md). Inventory is repeated
before each explicit open and exchange, without caching trusted names.

Mido's actual open implementation obtains another native list and uses the first
matching name's index. These separate checks cannot prevent endpoint replacement
or a duplicate arriving between the last inventory and open/send. A unique name
is not physical identity, ownership or atomic pairing. A separately authorised
physical capture must still establish provenance and exclusivity.

Optional dependency versions, original notices and the earlier Mido-reported
name observation are recorded separately in the CoreMIDI readiness documentation
(PR144). That earlier coalesced count is not a raw endpoint count.

`tests/raw_coremidi_inventory_test.py` exercises coalescing fake backends,
API/import/cleanup failures, disappearing endpoints and actual C virtual-NOR
full/raw collector paths. It never imports native MIDI. The existing collector
retains role/offset/CRC/provenance validation unchanged. Native name enumeration
is a separate readiness observation, not physical capture qualification.

The [raw inventory receipt](evidence/2026-10-11-raw-coremidi-inventory/receipt.json)
binds fresh wrapper and existing collector executions for FULL four/eight tracks,
FM4-off/SLICE-on normal and inherited sanitizers. Actual C binaries were reused
only after every dependency byte in the prior 16 translation-unit configurations
matched; this is not a complete runner or reduced-profile qualification.
Separately, pinned native read-only enumeration observed one raw `FM-1` input
and one raw `FM-1` output, with no ports opened or messages sent. A sandbox client
creation abort was preserved before the authorised enumeration succeeded.

A generic wrapper-recheck bypass mutant failed the broader fixture before the
disappearance case; it is not isolated disappearance evidence. The separate
[probe](evidence/2026-10-11-raw-coremidi-inventory/isolated-disappearance.json)
accepts an external inventory, removes the input, then checks wrapper open: the
unchanged wrapper refuses with zero opens, while its exact recheck removal
delegates an open and fails the explicit assertion. Coalescing and cleanup bypass
controls independently fail their relevant assertions.

The native CLI is now blocked before imports until a source/extension pair is
qualified. Historical endpoint observations and offline collector receipts remain
unchanged; the new [lifecycle audit](native-midi-lifecycle.md) supersedes claims
that object deletion guarantees disposal of an ephemeral OS MIDI client.
