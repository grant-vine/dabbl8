# Offline native project-set archive (developer preview)

This host-only tool preserves an exact D8P1 live project and the current set in a
40960-byte simulated shared pool: three saved projects and one shared autosave.
It does not capture a device or constitute a full instrument backup. Settings,
user presets, samples and vendor recovery firmware are outside this format.
There are no firmware protocol, device-capability or flash-map changes.

## Build and use

From the repository root, build the standalone adapter with a C11 compiler:

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src \
  firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_project_archive.c \
  -o /tmp/dabbl8_project_archive
python3 tools/dabbl8_project_archive.py export POOL.bin NEW_EXPORT_DIR \
  --live LIVE.d8p --adapter /tmp/dabbl8_project_archive
python3 tools/dabbl8_project_archive.py import PROJECT_SET.d8bak NEW_IMPORT_DIR \
  --adapter /tmp/dabbl8_project_archive
```

Python uses only its standard library. The destination must not exist. These
inputs are offline files; obtaining them from physical hardware is not provided.
Always retain your input files independently. A refused oversized input remains
at its original path; the tool does not copy a truncated substitute.

Each attempt writes `report.json`, with the acceptance result, refusal reason and
hashes of retained originals. Bounded inputs are copied before interpretation;
import retains the exact archive before parsing it. Successful attempts publish
one `verified/` directory containing `project-set.d8bak` and
`proposed-virtual-pool.bin`. Refusals publish no verified directory. The proposal
is never written to an instrument. Publication uses a directory rename after all
checks, not a qualified crash/power-loss recovery mechanism.

## Compatibility and limits

The JSON format is `dabbl8-project-set`, version 1; maximum archive size is
262144 bytes. It is distinct from `felucca-backup` schema 1 and existing legacy
conversion bundles. There are exactly five ordered roles: `live`, `project-0`,
`project-1`, `project-2`, `autosave`. Each role has explicit presence, size,
CRC-32, SHA-256 and canonical base64 bytes. The archive also retains the entire
original virtual pool, including stale/torn records. Basename provenance is
informational and never becomes an output path. Duplicate JSON fields, unknown
header fields, unsafe provenance names and unsupported versions refuse.

Every present D8P1 payload must be supported by the current codec, fit 7936 bytes
and reference only saved projects present in the complete set. Autosave never
stands in for the unresolved historical fourth saved slot. Live and persisted
autosave may differ and are preserved separately. Unknown optional payload data
is archival-only: original files remain available but no usable proposal is
accepted. Unknown required fields and unsupported references also refuse.

Import verifies archived role bytes against the retained original pool with the
actual C inventory/load path before rebuilding. The simulated C pool writer then
rebuilds the whole set and reads every role back for exact byte equality and
absence. It never adopts runtime state or decodes/re-encodes project payloads, so
names, banks, scenes, arrangement, motion and otherwise valid noncanonical bytes
are retained. The rebuilt pool may have fresh record placement and generations;
it is not byte-identical to original history. The retained original pool is.

A damaged sole record cannot establish absence. Sparse pools with unrecognized
nonempty residue refuse conservatively, even if the residue could be a harmless
torn spare. A full known set may retain an unrecognized torn spare because no
missing role is inferred. A recognized damaged newer copy with a valid current
copy of the same identity can be retained without dropping that role.

## Verification and remaining gates

`tests/native_project_archive_test.py ADAPTER` exercises all sixteen sparse
presence masks, maximum bank/scene metadata, raw noncanonical identity, all four
persisted objects, independent live/autosave, checksums, header/path/size limits,
unknown data, unresolved references and damaged sole-copy absence. Normal and
strict AddressSanitizer/UndefinedBehaviorSanitizer variants run in the host
runner; the adapter has no inherited DSP sanitizer exclusions.

Physical capture/import, migration execution, ownership, native full instrument
backup, recovery qualification and installation remain open. This preview does
not close issue #11 or authorize flashing.

Frozen functional source `e50e479290cf537330cf5c61ee3db748c411e51c` passed
the Mac host runner: 374 archive checks in each normal/strict sanitizer mode and
92 unchanged golden renders. See [scoped evidence](evidence/native-project-archive-host.json)
for source/input/log hashes, preserved missing-OTA setup failure and all four
coverage omissions (DaisySP reference, browser emulator, official V15 restore
simulation without the vendor fixture, and this web editor’s absent MENU settings).
No new firmware build or browser-WASM converter parity is claimed by this host-only change.
