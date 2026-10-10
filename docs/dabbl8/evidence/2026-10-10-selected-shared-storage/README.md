# Selected shared-storage geometry evidence

Owner selected three projects and then explicitly shared autosave, not per-object A/B, on 2026-10-10. Source base `8a52e28892cfbe5a74aa8c406e6e62d2b7d7d133`; changed-source hashes are in manifest.json.

`python3 tools/dabbl8_storage_plan.py` passes all geometry assertions. Its output matches `docs/dabbl8/three-project-storage-plan.json` byte for byte. It validates five two-sector blocks, exact 40 KiB total, maximum-file capacity, alignment, nonoverlap and preservation of app/boot/OTA, sample, FM6/preset/settings/reserved areas. It neither modifies nor binds a NOR driver.

The existing independent abstract shared-spare model was rerun: **15,933 interrupted erase/program prefix states** plus **1,000 successive saves**, all assertions pass. Commands and output are retained here. It uses a TEST record header, Python CRC and opaque payloads. This verifies the selected strategy at model scope; it does **not** test the C D8P1 storage engine, callback errors, sequence ambiguity, migration, actual flash offsets, media wear or hardware. Its retained “No physical address map selected” limitation means the abstract model itself does not bind physical addresses; owner selection is recorded separately in the decision document.

No source affecting firmware behaviour, loader, flash boundaries or golden hashes changed. No build is required for this planning/geometry-only change; PR54 provides existing source build/test evidence, without qualifying this future shared-pool implementation. Hardware/installation/recovery and release are SKIPPED. Original legacy slot 4/autosave preservation and explicit migration remain gates.
