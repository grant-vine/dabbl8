# Offline import preview evidence — 10 October 2026

Preparatory issue #11 work, based on mixer commit
`76e585b98e3cf550d03707227b3226b283fe44e7`. No firmware source, loader,
flash map or golden/reference fixture changes.

The final targeted optimized and ASan/UBSan runs each passed 312 assertions
(`tests/offline_import_test.py`), covering 13 immutable legacy fixtures,
canonical four-track musical data, explicit empty preview tracks 5–8,
exact original bytes and SHA-256, unsupported/corrupt input preservation,
non-overwriting output, missing-importer reporting and counted DIGITAL motion
removal with an unrelated signed track-4/step-64 lock intact. Standard sanitizer
exclusions remain unchanged. The initial expectation corrections and compiler
version are recorded in `results.json`; those corrections were not golden or
fixture rewrites.

Both checks are added to `tests/run_tests.sh`. The complete suite is pending
under worker 96630; do not claim its final result from these targeted checks.
Its raw log is retained outside Git under `.local-baseline/offline-import-check`.

Commands used from the firmware worktree:

```sh
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/dabbl8_import_preview tools/dabbl8_import_preview.c -lm
python3 tests/offline_import_test.py build/host/dabbl8_import_preview
cc -O1 -g -w -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow -fno-sanitize-recover=all -Ibuild/gen -Ifirmware/src -o build/host/dabbl8_import_preview_san tools/dabbl8_import_preview.c -lm
python3 tests/offline_import_test.py build/host/dabbl8_import_preview_san
```

The read-only upstream WebApp audit records exact commits, observed filenames,
byte sizes and SHA-256 in `editor-audit-manifests.json`. No WebApp implementation
or fonts were copied. See [approach and usage](../../offline-import-preview.md).

No device is connected or written. Preview JSON is not a loadable D8P1 project
or a complete backup conversion. Issue #11 stays open: #7/#9 prerequisites,
capability-driven live editing, unsupported-data backup/restore and hardware
round trips remain incomplete. Shipping default four-track memory/build evidence
remains in the preceding baseline/mixer records; no target image is produced by
this host-only addition.
