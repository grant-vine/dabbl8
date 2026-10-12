# Native MIDI lifecycle qualification remains blocked

The command-line read-only adapter refuses before importing Mido or constructing
native objects. No native source/extension pair is currently qualified; its
allowlist is empty and there is no argument or environment bypass. The separately
injected Python helpers remain available to offline tests; they are not a generic
security or hardware-identity boundary.

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
MIT notice. It is a proposed dependency repair, not an installed or qualified
extension. Actual extracted C++ lifecycle bodies against fake CoreMIDI functions
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

Native build/import/port operations and physical capture are outside this change.
A future reviewed build must bind the exact patched source, generated wrapper,
compiler/headers and extension bytes, verify native cleanup, and separately qualify
physical transport/provenance. No user input, version string or file supplied to
the current CLI grants qualification or permission to install firmware.
