# Native MIDI lifecycle: bounded repaired-build qualification

As of 17 October 2026, one exact repaired arm64 CPython 3.9 extension has passed
local lifecycle and software-loopback checks. The shared backend and CLI both
call `tools/dabbl8_native_midi_guard.py` before native construction. It checks the
package initializer, version file, extension bytes and ABI, and rejects a foreign
already-loaded module. No environment/version-string override exists. The old
unpatched environment remains blocked. Injected offline test backends are test
seams, not hardware-identity attestation or a general security boundary.

The earlier private eight-channel helper bypassed the old CLI-only check. This
was a preparation defect: its shared backend constructor is now guarded too.
Keep the failed 16 October track-4 preflight and disabled-CLI snapshot attempt;
neither qualifies physical cleanup. Do not rerun the stale helper/source contract.

The official python-rtmidi1.5.8 source tag is
`f762d4d752f08493c6c0e90d8f4e48fb35308a14`, using RtMidi submodule
`f6ea40d406affaed540d16c54372a0230a84f22b`. Exact source archive SHA-256 is
`7f9ade68b068ae09000ecb562ae9521da3a234361ad5449e83fc734544d004fa`.
The relevant source files match that tag/submodule exactly. The installed pinned
arm64 wheel's instructions confirm the inspected initialization/cleanup behavior;
this is no complete reproducible source-to-wheel attestation.

`CoreMidiData` has no constructor. Both initialize methods allocate it without
value initialization and set only client and endpoint. Its port field remains
uninitialised. Destruction calls closePort, reads that field and may attempt
MIDIPortDispose with an indeterminate reference. Source and installed instructions
confirm this defect; a native runtime failure was not observed or induced.
Earlier successful endpoint inventories do not establish safe native cleanup.

The [minimal patch](../../tools/patches/python-rtmidi-1.5.8-coredata-value-init.patch)
value-initializes both allocations and preserves the original source's modified
MIT notice. Actual extracted C++ lifecycle bodies against fake CoreMIDI functions
use poisoned allocation to expose two invalid disposal attempts in the original,
zero after both changes, and failure when either relevant change is reverted.
The mock uses 32-bit MIDI references and opaque mock sysex fields; it is no real
CoreMIDI ABI or allocator validation. Actual seeded port cleanup remains tested.

Constructors may create a shared OS CoreMIDI client; deleting temporary RtMidi
C++ objects does not dispose that shared client. Source inventory APIs request
only counts and names. Explicit openPort performs MIDIInput/OutputPortCreate and
input connection; sendMessage performs transmission. Historical enumeration
made no explicit open/connect/send request. The dependency cleanup defect limits
stronger claims about its OS lifecycle. Raw duplicate refusal also cannot prevent
same-name replacement between inventory and open/send.

## Retained build and new local checks

The sealed manual build is retained under
`.local-baseline/native-midi-lifecycle-audit/manual-extension-build/` relative to
the outer firmware checkout. All 31 sealed files matched their hashes before reuse.
Its generated wrapper, five source/notice inputs, compiler/SDK/header closure,
compile commands, two-site patch and instruction inspection remain retained.
Extension SHA-256 is
`7b92ee608464cb1a1b34fef81906df90ffe807774bd786a496ed489b0f3c819e`.
The original Clang variable-length-array extension warning remains explicit.

The separate environment is
`/Users/grantv/Code/dabbl8/.local-baseline/venv-midi-repaired-20261017`.
It uses CPython 3.9.6, Mido 1.3.3 and packaging 26.3 from existing cached wheels;
the python-rtmidi 1.5.8 extension was replaced only there with the sealed repair.
The original environment/binary was preserved. No dependencies were downloaded.
The installed version string alone cannot distinguish this repaired build.

`build/native-midi-qualification-20261017/virtual-probe.stdout` records 100 pairs
of input/output construction and cleanup, three successful software-port SysEx
loops and four injected Python failure/cleanup cases. Every owned temporary UUID
destination disappeared after cleanup. Only freshly created software destinations
were opened or sent messages; no physical endpoint was opened. Successful object
deletion does not prove shared OS-client disposal, leak freedom or all failure paths.
The subsequent Mido binding import/hash check constructed no MIDI objects.

Offline guard tests refuse changed package/extension bytes, wrong ABI, missing
packages, foreign loaded modules and direct-constructor bypass. Existing strict
inventory/protocol checks and the actual C virtual-NOR collector remain separate
from native runtime observations. Physical FM-1 transport is not qualified by
these tests, and no firmware operation is authorized by this document.

## Snapshot runner and first observed physical result

`tools/dabbl8_readonly_snapshot.py` defaults to plan-only. The explicit capture
mode uses the guarded backend, requires unique exact `Dabbl8` endpoints, writes
to a fresh private directory, and captures all 17 roles without requiring the
workspace to match the demo. It enforces a 120-second capture deadline with a
bounded release opportunity, validates the release acknowledgement and closes
ports before accepting its receipt. No notes, edits, loads, restores, save,
reset, upload or flash operation is available. Native transport failure stops
the attempt; no retry or automatic restore follows.

After fresh owner readiness, a single authorized capture used this invocation:

```sh
/Users/grantv/Code/dabbl8/.local-baseline/venv-midi-repaired-20261017/bin/python tools/dabbl8_readonly_snapshot.py --capture build/native-midi-qualification-20261017/fm1-snapshot-once
```

That destination now exists and must not be reused. The 17 October physical
capture passed all 17 roles, release acknowledgement and successful port closure.
Read `build/native-midi-qualification-20261017/fm1-snapshot-once/RESULTS.md` and
its original receipts. This proves one successful physical capture, not complete
transport/lifecycle or firmware qualification. No notes or data writes were sent.
Saved project B exactly matches the demo; the current workspace is different.
Track 4 has level 104/POLY instead of 72/MONO. Do not load, edit or play notes
without the next owner-authorized test scope, and do not retry this capture.
