# Offline converter developer-preview kit

This local artifact packages the existing original-preserving FUN1–FUN9 converter for use outside a firmware checkout. It contains a host-specific native executable, Python launcher, file verifier, complete committed corresponding source and original license notices. It is experimental project tooling, not firmware, an installation package or qualified device migration. No MIDI connection or device writes are made.

## Build and use

The kit builder needs Python 3.12 or newer, Git, the host C compiler and generated headers from the audited baseline build. It downloads nothing. Use the isolated Python environment described in [desktop-first-build.md](desktop-first-build.md), and generate `build/gen` using the baseline build first. From the repository root:

```sh
python3 tools/dabbl8_preview_kit.py /path/outside-git/dabbl8-preview.zip --tables build/gen
```

The ZIP destination must be new. `--revision COMMIT` selects exact committed corresponding source; default is `HEAD`. Uncommitted edits are not packaged. Generated header hashes, compiler version, source commit, build flags, platform and a real codec smoke check are recorded in `manifest.json`. The command prints the whole ZIP SHA-256 and size. Retain that output alongside the artifact. Packaging identical source, headers and compiler output is deterministic; this does not promise cross-compiler or cross-platform binary reproducibility. Keep generated ZIPs outside Git. Creating this local artifact does not authorize a public release.

Extract the ZIP to a new directory and run:

```sh
python3 verify.py
python3 convert.py /path/to/project.bin /path/to/new-bundle
```

Python 3.10 or newer is sufficient for the extracted kit. The executable only supports its recorded host OS and architecture. Mac arm64 is tested here; other hosts need their own build and checks. If an extractor drops executable permissions, run `chmod +x bin/dabbl8_project_convert`. Run the verifier before using the kit. File hashes establish kit consistency; they do not authenticate a publisher or establish hardware safety. `source.tar` and license notices provide corresponding source; README.txt records the rebuild command and baseline setup references.

The launcher locates its native executable relative to itself, so the kit works from another working directory. It supports all existing CLI reference arguments, for example `--reference 0=/path/to/slot0.bin`. These are **historical four-slot indices**, not the selected three-slot device layout. An accepted legacy bundle with slot-3 references is not ready for device storage until explicit migration resolves those references without aliasing or deleting originals. This kit does not implement that migration.

Read `report.json` and require `accepted: true`. Unknown/damaged input and unavailable references are refused with readable originals retained and no converted files. Existing output directories are refused. Conversion can normalize reserved/inactive bytes and remove incompatible engine automation; the report describes changes and original files retain those bytes. Missing/incomplete copies or reports are not verified backups. Keep independent originals. [Offline project conversion](offline-project-conversion.md) describes semantics and failure limits; [browser conversion](browser-project-conversion.md) remains a separate companion workflow.

## Validation and remaining gates

`tests/preview_kit_test.py /path/to/audited/build/gen` builds the real kit twice, compares archive bytes, extracts it, checks every listed file, invokes the relocated launcher on all thirteen frozen legacy originals, verifies original and output hashes, checks unknown-input refusal and existing-destination preservation, and detects a deliberately altered kit file. Temporary synthetic/fixture bundles are not committed. The existing converter tests separately cover deeper conversion/reference semantics.

No firmware, schema, flash boundary, loader, DSP or golden hash changes are included. Issue #11 remains open for capability-driven device editing, actual transport, migration, three-slot reference policy and qualified hardware round trips. Neither this kit nor prior browser tests establishes those outcomes. Public release packaging still needs review, supported-host testing and authorization.
