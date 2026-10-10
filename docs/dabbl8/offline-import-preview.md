# Offline legacy import preview

Status: preparatory work for issue #11. This tool does not produce a loadable
Dabbl8 project, connect to MIDI, restore a backup or establish hardware support.
The shipping build remains four tracks. Storage #7 and expanded-runtime #9
acceptance gates remain open.

## Editor approach

Keep the capability negotiation and write guards already in the firmware
repository's editor/backup code. Prepare an offline companion import stage using
the pinned firmware's actual FUN1–FUN9 importer rather than duplicating historical
conversions in JavaScript. A future editor can consume the versioned preview,
but device editing and round trips still need an approved save schema and map.
This is the first stage, not a finished editor or a final release architecture.

The separately maintained `hugelton/Felucca-WebApp` was reviewed at research
commit `05c901d802d14832fd1aedb1a91f661b9d4a329a` and observed current commit
`8a8477672b3919f74da9fb33490d78120be3b00b`. Both declare GPL-3.0-only, with
Leo Kuroshita and Hügelton Instruments credits. Their dependency notices cover
Inter Tight (OFL-1.1), Fukiai (MIT) and msfa/FM6 (Apache-2.0). The mixer already
creates strips from the reported track count, but direct legacy writes need the
Dabbl8 capability guards. Current upstream protocol code includes newer
FUN10/FUNA and 111-parameter handling; the pinned firmware has FUN9 and 99
parameters. Do not import current upstream wholesale or advertise its newer
features as Dabbl8 capabilities. No upstream web code or extra dependency is
copied by this change. Audit file hashes are recorded with the evidence.

## Usage

After the normal build has generated `build/gen`, build the host-only importer
from the firmware repository root (the standard test suite also builds it):

```sh
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/dabbl8_import_preview tools/dabbl8_import_preview.c -lm
python3 tools/dabbl8_import_preview.py /path/to/project.bin .local-baseline/imports/project-preview
```

The destination must be a new directory. Private projects belong outside Git;
`.local-baseline` is ignored. Do not add device backups to fixtures or commits.
Only Mac/Linux little-endian hosts used by the baseline are qualified here.

The directory contains:

- `original.bin`: the exact input snapshot, including reserved and unknown bytes.
- `report.json`: original size/SHA-256, success or refusal, warnings, historical
  engine changes and count of automation records removed by the importer.
- `preview.json`: only for accepted standalone FUN1–FUN9 files. A versioned JSON
  view of globals, names, tracks, parameters, 64 steps, FM6 patches, chain and
  motion; this is not D8P1 or an uploadable project.

Historical tracks 1–4 retain the canonical importer result. FUN1 originally has
one instrument and fills its other original tracks using the upstream rules.
New tracks 5–8 have explicit default parameters, power-on sound markers, init
FM6 patches and empty REST steps. All eight tracks still share eight voices.
The `parts` field retains historical importer metadata; `track_count` is the
preview's eight-track shape, not a device capability declaration.

DIGITAL converts to FM6, SAMPLE PERC and historical PHYS DRUM convert to DRUM,
and older GM drums become a synth part according to the upstream importer.
Some DIGITAL/PERC engine automation is removed; the report counts this and the
original retains it. Older parameter IDs, default fills, drum grids and names
follow the pinned C importer. The semantic preview is not a lossless substitute
for the original. Unsupported/reserved interpretations remain recoverable only
from that original. A new or corrupt format is refused, never silently upgraded
or treated as an empty valid project. Even refusal or a missing importer keeps
an original snapshot and report; no preview is written. An existing destination
is refused without overwriting it.

## Validation and remaining gates

`tests/offline_import_test.py` checks all 13 immutable baseline fixtures against
their canonical FUN9 captures for the four historical tracks and other musical
fields, verifies explicit empty new tracks, exact original/hash preservation,
existing-directory refusal, corrupt/truncated/oversized/future/random records,
and a synthetic DIGITAL migration that drops one incompatible event while
preserving a track-4 step-64 signed parameter lock. Fixtures and golden audio
hashes are unchanged. Optimized and ASan/UBSan runs use the normal exclusions.

Live capability-driven editor controls, backup restore with unsupported chunks,
D8P1 serialization, upload/download and hardware round trips remain unfinished.
Do not close issue #11 based on this preview. No flash map, loader or boundary
changes are included.
