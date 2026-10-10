# Offline legacy project conversion

This host-only workflow converts standalone FUN1–FUN9 projects through the pinned upstream importer and the staged D8P1 adapter. It creates project data, not firmware. Device upload, runtime adoption, arrangement playback, flash persistence and physical recovery remain unqualified. No flash map is selected.

Use the pinned baseline build environment described in `desktop-first-build.md`. Run `./build.sh` first to generate `build/gen/felucca_tables.h`, then compile the converter from the repository root:

```sh
cc -O1 -w -Ibuild/gen -Ifirmware/src \
  firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm \
  -o /tmp/dabbl8-project-convert
python3 tools/dabbl8_project_convert.py /path/to/project.bin /path/to/new-bundle \
  --converter /tmp/dabbl8-project-convert
```

The destination must be a new directory. Existing destinations are refused. The workflow snapshots the source as `original.bin`, records its byte count and SHA-256, and converts that snapshot. A successful bundle includes `project.d8p` and `report.json`. The emitted file is decoded and re-encoded through the actual C implementation; canonical bytes must match exactly. Upper four tracks start empty with native defaults; all eight tracks share eight sounding voices.

Historical chain entries refer to saved-project slots. Supply actual project files for every referenced slot, numbered 0 through 3:

```sh
python3 tools/dabbl8_project_convert.py /path/to/project.bin /path/to/new-bundle \
  --converter /tmp/dabbl8-project-convert \
  --reference 0=/path/to/saved-slot-0.bin \
  --reference 3=/path/to/saved-slot-3.bin
```

All supplied references are copied before conversion and independently converted and checked. Their backups are `slot-N-original.bin`; successful converted references are `slot-N.d8p`. Missing or invalid referenced slots refuse the bundle. Slot 2 or 3 never aliases another slot. Converted chain rows preserve their order and repeats through explicit slot-to-scene mappings. This representation does not implement playback.

Read `report.json` and require `accepted: true` before using any generated project data. Reports identify source format, engine migrations, removed incompatible automation, slot mappings, reference checks and original/output hashes. Conversion preserves canonical musical semantics rather than every inactive or reserved historical byte; keep the complete original files. Unknown formats and damaged checksums are refused with originals retained. Unknown optional D8P1 content cannot be converted through this legacy workflow.

On handled conversion or verification failure, generated `.d8p` files are removed and completed snapshots remain available with a refusal report. This is not a crash-consistent or power-cut-qualified backup system. Filesystem failures can interrupt copying, cleanup or report writing; an incomplete or absent report never establishes a complete backup. Keep independent source backups. Private user projects and vendor firmware belong outside Git.

The integrated tests exercise all thirteen frozen legacy fixtures against unchanged canonical references, exact original copies and hashes, engine migrations and automation removal, complete reference bundles, unavailable/corrupt references, malformed and oversized inputs, existing destination refusal, converter failures and late verification cleanup. Normal and upstream-configured ASan/UBSan modes both run these checks; the pure byte codec retains separate strict sanitizer coverage. This converter intentionally links the real upstream importer, whose existing DSP sanitizer exclusions still apply.
