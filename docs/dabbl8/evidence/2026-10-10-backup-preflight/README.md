# Backup restore preflight evidence — 10 October 2026

Preparatory issue #11 work based on offline-preview commit
`b5e954b03e7396541401f54d3f461c135b1c5d55`. Only the browser backup helper,
its simulated-device tests and documentation change; firmware, flash layouts,
loader, test fixtures and golden/reference hashes do not change.

Node 26.11.0 `web/test_backup.mjs`: 35 checks passed. Unsupported FM6 patches
(including an empty object) stop before any destructive request; the source
archive is unchanged. Older archives remain compatible. A dishonest manifest,
malformed/unavailable manifest, wrong operation/object/status reply and wrong
sample offset/length/status cannot be reported as a successful restore. A full
synthetic sample (header and three data chunks) restores with exact length and
bytes; refused chunks stop before live music.

Related `web/test_web.mjs`: 270 checks passed; embedded-editor MENU support is
explicitly skipped. Its requests, updater and device are simulated. No real
MIDI access, backup restore, firmware flashing or installation occurred.
Commands: `node web/test_backup.mjs` and `node web/test_web.mjs`, from the
firmware worktree. Both complete raw logs and their SHA-256/source hashes are
recorded here. The final sample assertion also checks exact output length.

The whole C/host suite was not repeated locally for this JS-only change. The
preceding PR #40 complete local and hosted runs passed separately; a new draft's
hosted workflow remains pending. Do not attribute that preceding run to this
changed helper. No new target image or memory change is asserted.

See [guarantees and limitations](../../backup-restore-preflight.md). Restore
commits individual objects and cannot roll back earlier commits after a later
failure. New D8P1/expanded-track backup round trips and live editor controls
remain unfinished; issue #11 must remain open with #7/#9 prerequisites and
physical qualification unmet.
