# Offline converter kit evidence

Source commit: `031fc6f1ea31b68c1f4a1ef32742b60656c4f720`, based on runtime shared-persistence commit `50cf903e67cd7cad6642f02acc729f9aedb648b1`. Source changes only host tooling, focused tests and documentation. Native converter source/schema/importer are unchanged. No core runtime, backup transport, full-suite runner, loader, flash boundary or golden hash changes.

The private Mac arm64 ZIP is 8,242,667 bytes, SHA-256 `0172f58309c46f9cdcd02ae62a9a2db932363869a99b54beb4b4cc00ec0b0afe`. Generated assets, binaries and private bundles remain outside Git. `manifest.json` reproduces the kit's source/compiler/platform/table/file provenance plus archive hash and patch source hashes. The artifact retains complete corresponding committed source and original licenses.

Commands run from this branch's isolated worktree, with the previously audited main workspace's generated baseline headers:

```sh
/Users/grantv/Code/dabbl8/.local-baseline/venv/bin/python tools/dabbl8_preview_kit.py /Users/grantv/Code/dabbl8/.local-baseline/parallel-tooling-check/dabbl8-offline-preview-macos-arm64.zip --tables /Users/grantv/Code/dabbl8/.local-baseline/main-workspace/build/gen
/Users/grantv/Code/dabbl8/.local-baseline/venv/bin/python tests/preview_kit_test.py /Users/grantv/Code/dabbl8/.local-baseline/main-workspace/build/gen
/Users/grantv/Code/dabbl8/.local-baseline/venv/bin/python tests/project_conversion_test.py /Users/grantv/Code/dabbl8/.local-baseline/parallel-tooling-check/existing-converter
git diff --check
```

Kit tests: 66 PASS, real native compiler twice, byte-identical archive comparison, all thirteen frozen originals from a relocated extracted kit, original/output hashes, source/notices/hash manifest, unknown-input refusal, existing-output preservation and modified-file detection. Existing conversion regressions: 314 PASS, using the identical unchanged converter source from parent commit. No duplicate full firmware/audio suite or browser/compiler download was run for this host-only packaging change. Prior browser parity is not new browser evidence here. No strict sanitizer run was added because no native converter source changed; existing converter sanitizer evidence remains separate.

The initial kit test discovered zero originals because it used an incorrect fixture glob; it failed explicitly, was corrected to the repository's thirteen `tests/fixtures/projects` originals, and rerun. That initial failure log is preserved in private evidence. No fixture or golden was changed.

Scope limits: host-specific unsigned developer artifact, only Mac arm64 tested. Packaging determinism compares identical local inputs and does not prove cross-platform compiler reproducibility. File hashes do not authenticate a publisher. Historical four-slot reference conversion does not implement the selected three-slot device migration. Issue #11 remains open for prerequisite #9, live editing, real backup/restore, migration and hardware round trips. No public release, upload, installation, merge or deployment occurred.
