# Selected three-project shared storage

On 2026-10-10 the owner selected **three projects**, then explicitly selected **shared autosave, not A/B**, for space efficiency. This supersedes the unselected two-/four-project alternatives for the initial storage direction in [issue #7](https://github.com/grant-vine/dabbl8/issues/7). Retain all three user sample slots and one logical autosave. Keep the unchanged upstream baseline, loader and flash boundaries.

## Selected geometry

Five 8192-byte blocks occupy the existing 32768-byte project allocation and 8192-byte autosave allocation. Four current objects (projects 1–3 plus autosave) share one write spare. There are no permanently assigned A/B pairs for any object.

| Pool block | Existing allocation, inclusive | Bytes |
| --- | --- | ---: |
| 0 | `0x97000–0x98FFF` | 8192 |
| 1 | `0x99000–0x9AFFF` | 8192 |
| 2 | `0x9B000–0x9CFFF` | 8192 |
| 3 | `0x9D000–0x9EFFF` | 8192 |
| 4 | `0xE5000–0xE6FFF` | 8192 |

These are existing areas, not a widened application/data boundary or an implemented device binding. Pool block indices are virtual: block 4 is physically noncontiguous. A future backend must map each block separately and split reads crossing virtual block boundaries. It must never treat the pool as a contiguous 40 KiB device range.

Each block reserves 256 header bytes, leaving **7936 payload bytes**. The modelled maximum D8P1 file is **7705 bytes**, leaving 231 bytes of payload headroom. Total allocation is **40960 bytes**. Sample areas `0xA0000–0xDBFFF`, FM6/preset/settings and boot/OTA areas remain outside the pool. Compression is optional; maximum-capacity storage must work without it.

`tools/dabbl8_storage_plan.py` emits and validates the selected geometry. It asserts block/sector alignment, maximum capacity, disjoint blocks/reserved areas and the exact total. It neither opens a device nor changes firmware constants. [Selected plan JSON](three-project-storage-plan.json) records the same constraints and unimplemented/qualification status.

## Save and recovery semantics

Scan object IDs and generations to select one committed current record per object. Choose an erased or obsolete block not currently owned by any object. Erase both destination sectors, write bounded payload pages, then publish the checksummed commit header last. Validate the replacement before reclaiming its predecessor. Save synchronously while stopped.

During interrupted replacement, the previous valid save remains current. Once its block is reused, a permanent fallback copy is no longer guaranteed. This is shared-spare commit-last storage, **not per-object A/B**, firmware dual-bank rollback or a hardware recovery guarantee. Corruption of the sole current record after its predecessor is reclaimed may lose that object; verified external backups remain necessary.

The existing `d8store.c` engine in PR #52 handles a two-copy 16 KiB region. It is retained as historical preparation and **does not implement this selected pool**. A new shared-pool engine must handle all four objects, sequence wrap/ambiguity, future/unknown records, read failures, source immutability, interrupted erases/programs, post-commit errors, full/dirty storage and bounded stopped-operation callbacks before device integration. Stale/corrupt records must never cause erasure of another object's current record. Do not mistake the abstract five-block research model for a target NOR driver.

## Migration and remaining gates

Legacy project slot 4 must be exported and retained. Its references must not wrap, alias slot 1 or be silently discarded. Three saved project identities permit reference mask `0x7`; a legacy fourth-slot reference requires a separately reviewed migration/remap or explicit refusal with its original retained. Autosave reuses the pool and therefore cannot simultaneously preserve the old autosave allocation as a separate writable legacy store.

Before physical adoption, verify complete project/preset/sample backups, handle all four original project slots and autosave, validate converted references and the new pool, and make the destructive migration step explicit. No automatic mount/erase of legacy bytes is implemented by this decision. Final retained-memory/cache behaviour must match the new three-slot count without changing historical four-track structure layouts.

Issue #13's acceptance now concerns shared-spare publication and recovery of the last valid save throughout replacement; it must not require or advertise permanent A/B copies. Physical fault injection, wear/read-error policy and #12 stack/IRQ evidence remain open. #14 requires a separate authorised install session; #20 release and #21 website download claims require verified qualification and separate publication instructions.

GPL-3.0-only and dependency notices remain unchanged. No binary, vendor firmware or private backup is included in this decision.
