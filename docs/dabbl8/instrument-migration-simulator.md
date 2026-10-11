# Original-preserving instrument migration simulator

This host-only test tool validates a reviewed shared-pool proposal, applies it to a simulated 1 MiB NOR image, or restores all twelve original raw allocations. It never communicates with a device, links into firmware, grants production ownership, or releases the persistence quarantine. GPL-3.0-only and inherited dependency notices remain unchanged.

A full version-1 instrument-capture archive contains seventeen independently preserved roles: twelve exact persisted raw allocations plus native live project, current PER4 preferences, two current preset-bank mirrors and current FM6 mirror. The host consumer validates ordered roles, maps, fixed filenames, lengths, CRC-32 and SHA-256 before retaining exclusive copies and invoking the executor. Logical RAM mirrors may legitimately have stale internal persistence CRCs; they are retained without normalization. Partial captures and raw-only captures cannot authorize this operation.

The capture is post-boot. It does not establish a pre-boot/vendor-original dump, device identity, authenticated transport or physically qualified recovery.

## Operations

`tools/dabbl8_instrument_migrate.py` offers simulation only. A separate, explicit reviewed-plan decision is required for APPLY; it is not physical-write permission or a migration-completion receipt. See its `--help` for the archive, simulated-current, plan and executor arguments.

The C executor interface is:

```
EXEC --simulate apply RETAINED_DIRECTORY PLAN40960 CURRENT1MiB OUTPUT [cut-operation [partial-bytes]]
EXEC --simulate restore RETAINED_DIRECTORY CURRENT1MiB OUTPUT [cut-operation [partial-bytes]]
```

OUTPUT is an exclusive-created full simulated NOR image, including a refused or interrupted result. Input originals remain untouched. Cut operations count erase/program calls from one; zero disables injection. Reads are bounded to 256 bytes, programs to one page of at most 256 bytes, and erases to an existing 4096-byte sector. There are no arbitrary-address write commands.

APPLY requires all twelve current raw allocations to match retained originals. It conservatively checks mapped legacy records, header/payload CRCs, generation ambiguity and frozen FUN1–FUN9 framing/size/FNV checksums before erase. Unknown residue, wrong types, corrupt sole copies or ambiguous generations refuse. The native proposal must contain a valid autosave object, available-reference-valid native objects and canonical erased unused blocks; the actual pool inventory and mapped load verify it before and after mutation. Only the five existing mapped blocks change: original project pairs 0–3 and noncontiguous historical autosave pair. The first-sector native commit header is written after both sectors' exact payload and slack.

This slice does **not** prove musical source-to-plan equivalence. The existing project converter and migration-bundle tooling can prepare a proposal, but reviewing its conversion reports and choosing any initialization for absent historical autosave remain explicit separate decisions. Receipt fields `source_plan_equivalence=false` and `plan_review_required=true` preserve that limitation. An entirely erased legacy pair may be accepted structurally, but initializing absent objects (especially shared autosave) is an explicit reviewed bootstrap decision; the externally supplied plan and its absence policy must be reviewed, never inferred from erase state. No native magic, partial pool, host receipt or boolean becomes production ownership.

RESTORE accepts backup-valid opaque originals, including torn records and unknown bytes. It recreates all twelve persisted roles, including original fourth pair, autosave, settings, both preset pairs, discontiguous FM6 sectors and all three complete 81920-byte sample allocations. Legacy sectors retain original bytes 32–255 and slack, with exact 32-byte commit headers last. Sample tails precede the first sector; its complete 512-byte header is written last, second page before the magic-bearing first page. This is externally recoverable using the retained archive, not atomic across twelve roles. Quarantine is a required production condition and a scoped simulator receipt flag, not an implemented device-wide polling lock. Current logical RAM adoption of the five separately retained current roles is not performed: production must keep settings/preset/sample/autosave polling quarantined until a separately reviewed adoption or reboot policy exists.

A callback failure may occur after commitment. The simulator rereads and compares the complete expected result and mapped native semantics before reporting completion; an incomplete result remains uncertain/refused and requires exact restoration before another APPLY. The host independently compares the complete output image, verifies every outside-map byte, hashes retained inputs and rejects false completion receipts. Even verified simulation receipts retain `quarantined=true`, `production_ownership=false`, `hardware_qualified=false`, and `device_writes=false`.

## Verification scope

The C tests use the actual executor and native mapped reader against synthetic NOR. They interrupt every 180-call APPLY position with two representative prefixes and every 1401-call RESTORE position, restore originals and retry, check complete source bytes and unchanged outside-map bytes, test all immutable FUN frames, header ordering, page/role bounds, corrupt payloads and readback failure/reconciliation. These are call-position cuts and representative prefixes, not every possible byte-level power-loss trajectory.

Host tests validate strict archive refusals, immutable retained originals, actual full-image APPLY/RESTORE, unsupported patterned legacy source, receipt tampering and cut/recovery. They use the real existing pool initializer rather than duplicate its format. Host tests provide no device deadlines, physical-NOR failure model, USB write protocol, durable ownership marker, firmware installation or recovery qualification.

Production activation remains gated on complete provenance/backup policy, reviewed source-to-native adoption, actual read-only capture and restore qualification, durable authority/revocation design, and suppression of stale RAM writers after raw restoration. Loader code, flash boundaries and golden hashes are unchanged.
