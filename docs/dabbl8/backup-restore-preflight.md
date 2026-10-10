# Backup restore support preflight

Preparatory issue #11 work for the existing legacy `felucca-backup` version 1
archive. This does not enable eight-track project or backup writes, select a
flash map, or qualify physical recovery. The editor's negotiated read-only
policy and firmware legacy-write guards remain in force.

The inherited restore path could skip object 9 (user-preset FM6 patches) when
an older device rejected it and still finish with live music restored. The
Dabbl8 browser backup helper now refuses that unsupported destination before
any destructive request:

1. Validate every source object, including size, base64, CRC and sample headers.
2. Read and validate the destination's complete `BACKUP_LIST` manifest.
3. Require every source object ID, including empty ones, to be supported by that
   manifest. An unsupported object, malformed manifest or unavailable reply
   aborts before any object PUT, sample begin or sample erase.
4. Restore supported objects in the existing order, with settings and live
   music last. Older 11-/12-object archives still restore to a compatible
   13-object destination.
5. Check exact operation/object echoes, length and seven-bit status bytes for
   each object PUT. Check exact slot and chunk-offset echoes for sample replies.
   A later refusal is an error, never a skipped object or successful restore.

The helper does not alter the source archive. Keep that original available;
no converted or reduced archive replaces it. Unknown archive schemas or
unrecognized object sets are refused by the existing strict archive parser,
not copied into a partial supported subset. The destination manifest reports
supported IDs and current contents, not prospective capacity or semantic
compatibility with a future D8P1 schema. No automatic down-conversion is offered.

Restore remains non-atomic across objects: a lying/stale destination, failed
chunk, disconnect or malformed later reply can stop after earlier objects
committed. A failed object chunk sends the existing abort request; the helper
stops before continuing to live music. This preflight does not claim rollback
of already committed objects or power-loss safety. Issue #13 tracks atomic
project storage and physical power-cut testing separately.

`web/test_backup.mjs` uses simulated requests only. It covers original legacy
archive reading/restoring and capture CRC checks, unsupported FM6 objects
refused before writes, unchanged original data, empty unsupported objects,
malformed/unavailable manifests, dishonest advertised support, incorrect
object/operation/status echoes, and a complete synthetic sample restored over
three data chunks. Offset/length/status corruption stops before sample commit
or live music. Existing editor/package regression checks exercise the
capability guards separately; no real MIDI access or firmware installation is
performed by these tests.

Remaining issue #11 work includes capability-driven eight-track controls,
actual new-format backup round trips without unsupported-data loss, approved
storage/runtime prerequisites, and physical recovery qualification. This
legacy compatibility improvement alone does not satisfy those gates.
