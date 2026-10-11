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

## Experimental offline engine-mask workflow v1

The ordinary `legacy`, `check` and `canonical` modes and default Python bundle
behavior remain unchanged. An explicitly optional C mode now accepts two separate
masks:

```text
dabbl8_project_convert legacy-profile-v1 SOURCE REFERENCE_SLOT_MASK ENGINE_MASK
```

The slot mask still describes original saved slots 0–3. The engine mask describes
actual compiled stable engine IDs and never implies available sample assets,
qualified patches, resource fit, timing or a reduced firmware build. For example,
slot mask 9 means slots 0 and 3; engine mask 5120 means FM6 ID 12 and DRUM ID 10.
FM4/SLICE compatibility follows the converter's real compile configuration;
empty, unknown or uncompiled bits refuse.

The optional mode uses the separately named profile-v1 staging API. All original
four canonical historical tracks, their private patches and actual migration
semantics remain intact. An unavailable original or late sentinel target refuses.
Only newly inserted tracks 5–8 can select a different default. Metadata records
requested/stored engine, stored preset, fallback and default kind separately from
first-four historical migration metadata. Available ANALOG retains the historical
preset-255 deferred default and unchanged bytes; a fallback uses its actual
factory preset 0 and matching parameters/private patch.

The C converter emits proposed D8P1 bytes on stdout only on success and structured
JSON on stderr, including profile schema, exact engine mask and status code/name.
It creates no files. Codes are 0 success, 1 malformed/unavailable-reference data,
2 invalid engine mask, 3 unavailable original/late engine, 4 unsupported source
engine, and 5 invalid arguments. Unsupported source diagnostics identify an
unsupported field in an exact checksum-valid envelope, not a certificate that
all other source fields are valid.

`tools/dabbl8_project_convert.py` exposes `--experimental-engine-mask-v1 MASK`
and the keyword-only `prepare(..., engine_mask=MASK)` seam. It applies one mask to
primary and every supplied reference, preserves all original snapshots before
attempting conversion, verifies every complete result and keeps the existing
exclusive-directory/no-overwrite policy. Any conversion or late readback failure
removes all proposed `.d8p` artifacts. Structured profile refusals are retained in
`profile_refusal`; invalid argument shapes refuse before creating a directory.
Profile reply schema, both masks, exact stable IDs, track/voice counts, default
metadata and explicit no-device/no-firmware claims are validated before accepting
results. Booleans cannot masquerade as numeric IDs or schema/mask fields.

The same optional argument is available in `tools/dabbl8_migration_bundle.py` for
all emitted live/project/autosave proposal objects. An unavailable original in
any emitted source refuses the complete proposal, including the virtual pool.
Original archive, sectors and decoded objects remain retained. The fourth legacy
saved slot is archival-only: it retains ordinary structural validation and exact
original bytes, is never substituted or emitted as a native slot, and cannot be
used to satisfy native reference identity. The existing three-project/shared
single-autosave proposal and write quarantine remain unchanged.

Browser UI/wrapper selection is not wired in this slice: existing browser calls
remain ordinary conversions, with rebuilt WASM/native regression parity. Firmware
profile overrides, availability capability negotiation, engine/state exclusion,
physical timing and target qualification remain closed gates. This completes an
optional offline CLI/Python workflow, not the full issue #53 firmware/card plan.

Host evidence: `evidence/2026-10-11-engine-offline-profile-workflow/`. Normal CI
verifies frozen ordinary CLI receipts from exact parent `370ac2d`; receipt
generation is separate and never runs implicitly in CI. Direct rebuilt-parent
comparisons passed before freezing those project-conversion receipts. Existing
audio goldens and resource/timing thresholds are unchanged.
