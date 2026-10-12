# Offline native project-set archive (developer preview)

This host-only tool preserves an exact D8P1 live project and the current set in a
40960-byte simulated shared pool: three saved projects and one shared autosave.
It does not capture a device or constitute a full instrument backup. Settings,
user presets, samples and vendor recovery firmware are outside this format.
There are no firmware protocol, device-capability or flash-map changes.

## Current own-format compatibility

Reviewed against development source `b633ecc53b593d0f0dc00416afbc649c0d5b1830`
on 12 October 2026. Firmware release labels and project-format versions are
separate; no released or physically qualified upgrade is established here.

| Own format | Supported version | Scope |
| --- | --- | --- |
| `D8P1` project | Container version 1 | Eight tracks, 64 steps, 99 parameters/27 globals, parameter/engine schemas 1; known required chunks and supported optional bank/scene chunks; at most 7936 bytes. |
| `D8SP` shared pool record | Record version 1 | Three saved projects and one shared autosave across five existing blocks; each usable payload must pass the native project checks. |
| `dabbl8-project-set` archive | JSON version 1 | Offline live project plus saved-project/autosave set; not a full instrument backup or connected restore protocol. |

Current runtime adoption requires known writable D8P1 data, valid checksums,
all referenced saved projects and engines available in the selected build.
Changing engine profiles can therefore make a preserved project unplayable;
missing engines or references refuse rather than silently replacing them.
No other own container/record/archive version is supported or automatically
converted. See the [codec](../../firmware/src/d8p1.c) and
[runtime adoption](../../firmware/src/d8p1_runtime.c) guards. Unknown required data refuses. Unknown optional chunks can be
inspected and retained as original bytes, but cannot be adopted or rewritten
by dropping those chunks.

Normal reboot and supported application upgrades must preserve recognized
Dabbl8 data. The first-use classifier skips formatting for a recognized own
record, including a checksum-valid future record header or damaged committed
payload; subsequent catalog/adoption checks can refuse it. Readiness is not
permission to rewrite an unsupported store. Damaged or unfinished headers can
instead produce an uncertain-storage disclosure: reset requires explicit
physical SAVE confirmation and is destructive, never an automatic upgrade
conversion. Retain backup bytes before choosing reset for recovery. Existing
host/simulator checks do not prove updater behavior, physical persistence or
recovery on an FM-1.

First installation from previous firmware is a clean reset: previous projects,
autosave, settings, user presets and user sample contents are erased after the
first-use disclosure and confirmation. The three sample-slot capacities remain.
Migration from vendor/Felucca/other firmware is not a v0.1 prerequisite; historical
converters and their recorded evidence do not establish a supported upgrade path.
The [first-use classifier](../../firmware/src/native_first_use.c) and
[panel workflow](../../firmware/src/native_first_use_session.c) define that distinction.

## Connected editor, capture and restore limitations

The current browser keeps ordinary eight-track track/project editing closed;
its separately negotiated command-80 sample replacement is a narrow exception,
not native project import or full backup restoration. Legacy project/backup
writes remain refused on eight tracks. Legacy BACKUP_GET/PUT must not be treated
as a D8P1 project-set round trip.

Command 78 provides read-only capture preparation: twelve raw-store roles, or
seventeen roles including the live D8P1 project and current ancillary state.
Raw capture is post-boot data, not an installable firmware image or restoration
proof. The offline project-set archive validates/rebuilds only supplied files
and a virtual pool. The supplied Mac CoreMIDI capture executable is currently
disabled because no native backend is qualified. A validated connected own-project
import/restore workflow is still missing; command-79 simulation/migration tooling
and command-80 sample replacement do not supply it. Physical connected capture,
import, restore and recovery remain open in issue #11; this documentation does
not close that issue or authorize device commands. See
[read-only capture preparation](readonly-midi-capture.md) and the
[current browser negotiation](../../web/editor.html).

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

Physical capture/import, own-data restore, native full instrument backup,
recovery qualification and installation remain open. External-firmware migration
is historical developer tooling, not a release prerequisite. This preview does
not close issue #11 or authorize flashing.

Frozen functional source `e50e479290cf537330cf5c61ee3db748c411e51c` passed
the Mac host runner: 374 archive checks in each normal/strict sanitizer mode and
92 unchanged golden renders. See [scoped evidence](evidence/native-project-archive-host.json)
for source/input/log hashes, preserved missing-OTA setup failure and all four
coverage omissions (DaisySP reference, browser emulator, official V15 restore
simulation without the vendor fixture, and this web editor’s absent MENU settings).
No new firmware build or browser-WASM converter parity is claimed by this host-only change.
