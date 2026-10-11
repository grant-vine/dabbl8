# Current source notice snapshot

This inventory reads immutable tracked blobs at main `e7945f4ed6958398645bdeee00ff4268ecc13cc7`, tree `ea56100f6ddc42d2e9c40f9cf1c6bb22cd4d8e15`. It records 357 code files, zero missing opening-comment SPDX declarations or incorrect declared identifiers or lost required upstream copyright/SPDX lines, 16 retained notice/asset pins and three byte-preserved port notice headers. The prior b246/f913 snapshots stay historical. This snapshot predates the new scanner, test and documentation files added by this maintenance change; later committed source needs a fresh scan before distribution.

Reproduce from this maintenance tool with:

```sh
python3 tools/license_inventory.py --source e7945f4ed6958398645bdeee00ff4268ecc13cc7 --output /tmp/dabbl8-e794-notices-new.json
python3 tests/license_inventory_test.py
```

The output path must not exist. The default source is committed HEAD, not uncommitted files. The checker reads Git blobs only, so it cannot ingest private untracked firmware, backups or credentials. The scoped extensions are C/H/inc, Python, shell, JS/MJS, HTML and CSS in firmware/tools/tests/web, plus build.sh. It also compares the 16 pinned notices/assets and the three port first-comment hashes. LICENSING.md may append text while its pinned original prefix remains exact.

The reference `tools/license_notice_baseline.json` derives required source notice lines and actual opening-comment SPDX identifiers from Felucca `276f72a4e6ea8a12499a7a6819aadf3165126755` and the notice/asset pins from ledger source `b2466bf9bc1c675ed8b8f667d37af8f5e4e4b377`. It enables shallow-checkout validation without downloading upstream history. Reference updates require explicit source review; there is no regeneration mode in the checker or normal tests.

Real-Git CLI tests use tracked public fixtures, including positive immutable/shallow checks and negative inherited notice loss, missing own-file SPDX, wrong own-file license identifiers (GPL-3.0-or-later, MIT or proprietary), embedded-string-only and empty header spoofs, duplicate identifiers, altered license asset, changed complete port header and overwrite refusal. Opening headers are parsed as language comments before the first executable token, with shell/Python shebang and HTML doctype support. New scoped project code requires GPL-3.0-only; inherited per-path identifiers preserve the three MIT/Apache port exceptions. They do not import private workspace material or establish new donor rights. Normal tests and CI run the validator against their own committed source.

The inherited line checks are path-based; renames and new donor provenance require source review. This is bounded source evidence, not a complete license/legal or release audit. External SDK/package inputs, actual emitted notices and matching corresponding source must be audited for the selected distribution. Experimental identity is implemented; issue #19 remains open because #14 physical recovery is unmet. No build, installation, device operation, public download or deployment follows from this snapshot.
