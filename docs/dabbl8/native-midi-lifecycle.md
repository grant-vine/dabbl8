# Native MIDI lifecycle: exact repaired-build qualification

The shared read-only backend and CLI both require the exact locally tested
repaired arm64 CPython 3.9 dependency before native construction. The guard
hashes the package initializer, version file and extension without importing
their code, then checks loaded-module paths. An environment setting or version
string cannot grant qualification. Other builds remain refused. Injected
offline test backends are not hardware identity or a general security boundary.

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
MIT notice. One exact local extension built with this repair is allowlisted;
that does not qualify an arbitrary rebuilt wheel. Actual extracted C++ lifecycle bodies against fake CoreMIDI functions
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

The allowlisted extension SHA-256 is
`7b92ee608464cb1a1b34fef81906df90ffe807774bd786a496ed489b0f3c819e`.
Its local source/wrapper/compiler/SDK/header closure and bounded construction,
software-loopback and failure-cleanup evidence are retained outside Git. These
observations do not prove shared-client disposal, all failure paths, physical
endpoint identity or firmware qualification. Dependency binaries are not bundled
with this source change, and the tools do not download or rebuild dependencies.

`tools/dabbl8_readonly_snapshot.py` defaults to a plan that opens no MIDI port.
Explicit `--capture NEW_PRIVATE_DIRECTORY` uses unique exact Dabbl8 endpoints,
the fixed read-only command-78 protocol and all 17 roles. The directory must be
new and is created private. A 120-second capture deadline permits a bounded
release attempt; acceptance requires complete data verification, release
acknowledgement and successful port closure. Failure stops without retries or
automatic restoration. Notes, settings edits, project load/save/restore, reset,
erase, upload and flashing are not exposed by this runner.

Use an existing qualified environment to inspect the plan:

```sh
python tools/dabbl8_readonly_snapshot.py
```

Capture requires separate owner authorization and readiness. No CLI argument,
file, version string or allowlisted extension grants that authorization. Keep
private snapshots outside Git. The `midi-native` local recipe tests injected
inventory and real virtual-NOR collection, deadline and release cases without
importing native MIDI or opening physical/software ports. Its CLI refusal test
injects the refusal explicitly so it also works in a qualified environment;
separate shared-guard tests check actual file-hash and ABI refusals.
