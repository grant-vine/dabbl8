# Shared storage pool implementation

This is the first C implementation of the owner's three-project/shared-autosave decision, for [issue #13](https://github.com/grant-vine/dabbl8/issues/13). It is a standalone, callback-driven record engine. It is not wired into project menus, boot restore, a physical NOR driver or the shipping app. No device has been migrated or flashed.

`firmware/src/d8pool.c` manages five virtual 8192-byte blocks. Objects 0–2 are saved projects; object 3 is the one logical autosave. Four current objects share one replacement block. Read the [selected physical geometry](three-project-shared-storage.md) before implementing a backend: block 4 is noncontiguous, and treating this as contiguous physical storage would overwrite unrelated data.

## Publication and recovery

Scan all blocks and validate each committed header and streamed payload CRC. Select the newest unambiguous generation per object. Before a save, validate the staged D8P1 wire and its three-project reference mask. Refuse unknown optional chunks for writes, unavailable references, foreign/future headers, ambiguous generation sets, read errors and running transport before any erase. A recovered generation near a half-range boundary must also remain unambiguous after increment; otherwise refuse before writing.

Choose a block which is not current for any object. Erase its two sectors, write the payload in pages of at most 256 bytes and program the commit header last. Check the destination again after publication. The preceding save remains committed throughout replacement. The next save can reuse its obsolete block, so no permanent per-object A/B pairs or permanent redundant copy is promised. An error after publication can leave a new committed save; rescan before retrying.

Generation comparison uses unsigned 32-bit modular ordering. Equal generations in different blocks, exact half-range differences and cyclic sets are refused, even if equal-generation payloads match. Corrupt payloads are ignored as candidates. An old copy is recoverable only while it still exists. Once reclaimed, corruption of a sole current record can lose that object.

## Wire header

The 256-byte header reservation is followed by at most 7936 bytes of D8P1 wire. Only the first 32 header bytes are programmed; remaining reserved bytes stay erased. Integers are little endian.

| Offset | Field |
| --- | --- |
| 0 | Magic `D8SP` |
| 4 | Version 1 |
| 5 | Virtual block index |
| 6 | Two zero reserved bytes |
| 8 | Object ID, 0–3 |
| 12 | Generation |
| 16 | Payload length |
| 20 | Payload CRC32 |
| 24 | Reserved `0xFFFFFFFF` |
| 28 | CRC32 of preceding 28 bytes |

A CRC-invalid header with magic bits consistent with interrupted programming/erasure is treated as uncommitted. Unrecognisable foreign bytes or CRC-valid unknown metadata prohibit pool use. This recognition is deliberately limited to an explicitly migrated new-format allocation; it is not a reliable legacy-format detector or permission to erase arbitrary storage. A CRC is accidental-corruption detection, not authentication.

## Caller and backend contract

Use synchronous stopped-main-loop operations with one writer, no callback reentry and immutable, separately staged input wire. Backend reads/erases/programs return zero only for success. Map virtual blocks individually, bound every access, implement actual NOR page/sector constraints and verify stopped transport before each mutation. Input/output storage must not overlap the backend or its callback state. Load rejects overlap between its wire destination and published length. It validates wire, CRC and reference availability before publishing that length; failed reads can change destination bytes, so runtime adoption must remain a separate validated step.

Inventory publishes its index only after a successful full scan. A missing object leaves the caller's record untouched. Save/load reference masks admit only project identities 0–2 (`0x7`); a historical fourth-project reference is refused. Synthetic test copies remap references only to exercise maximum-sized records; committed historical fixtures remain unchanged. This is not a user-data migration policy.

The engine uses no allocation or project-sized automatic buffer; payload validation scans with a 256-byte page buffer. JieLi `-Os` object evidence reports 1662 bytes of text and zero data/BSS. Visible save → inventory → scan frames total 800 bytes; load → current → inventory → scan totals 752 bytes. These are prologue arithmetic, excluding parser/CRC/backend descendants, callers and IRQ frames, not complete stack upper bounds. Caller staging, complete call-chain stack, real flash latency, wear, IRQ interactions and physical read-error handling still need qualification.

## Verification and remaining integration

`tests/d8pool_test.c` uses guarded virtual NOR and the real D8P1 parser. It exercises each mutation boundary, every programmed-byte cut, every sequential erase prefix in both destination sectors, individual interrupted magic-bit states, stopped-state transitions, read errors, failed/protected writes, unknown metadata, generation wrap/ambiguity, corruption fallback and 1000 rotating saves across all four logical objects. These simulations are not a physical power-loss qualification.

The normal and strict ASan/UBSan suites run the test. Evidence records exact source hashes, target object compilation, unchanged default app/package/loader and untouched audio goldens/budgets. The sanitizer leak detector is unavailable on this Mac; address and undefined-behavior checks remain enabled. Failed verification attempts are retained with their corrections.

Before runtime binding, implement a reviewed migration that preserves/exports all four original projects and autosave, validates three-project references and never silently mounts or erases legacy data. Then connect menus, editor transfers, boot restore and autosave scheduling, verify stopped adoption and measure target memory/timing. Keep #13 open until its complete acceptance criteria pass. Physical qualification belongs to #14 with separate installation authorization. Release and website claims depend on #20/#21.

SPDX-License-Identifier: GPL-3.0-only. Preserve upstream and dependency notices.
