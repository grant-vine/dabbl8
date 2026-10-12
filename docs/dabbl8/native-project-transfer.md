<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Connected own-project transfer (command 81, schema 1)

This interface transfers Dabbl8's complete own-project set: three manual saved
projects (roles 0–2), shared autosave (role 3), the separate live project, and an
exact 40,960-byte original virtual-pool snapshot. These are the existing `.d8bak`
archive roles. Settings, user presets, user samples, vendor firmware and other
instrument assets are **not** included. Preserve those dependencies separately.
A single D8P1 payload is not a `.d8bak` archive.

The host/virtual-NOR implementation is not physical-device qualification.
Connected MIDI transport, physical erase/write timing and recovery still need
verification before installation or release claims. First installation resets
previous-firmware projects, autosave, settings, presets and samples. External
firmware migration is not a prerequisite. Ordinary own-firmware upgrades and
reboots preserve supported own records.

## Format and storage

Project payloads remain D8P1 version 1. Positive D8SP pool records remain version
1. Known D8SP version 2 records are deletion tombstones: the same 32-byte header,
length and payload CRC zero, canonical reserved fields, erased remainder, and
header written last. Inventory separately tracks logical `present` roles and
current `anchors` (positive records plus tombstones). Both consume a current
allocation; five existing blocks retain one shared spare. No permanent A/B pair,
new flash allocation, loader change or flash-boundary change is introduced.

An all-absent set remains positively owned through tombstone anchors and boots
as a supported empty own store. Older firmware that understands only record
version 1 refuses version 2. Unknown/future headers, ambiguous generations,
unknown optional project chunks and unavailable engines refuse connected restore.
A damaged committed sole payload is not reclassified as an absent role. The
restore-only recovery path accepts recognized interrupted noncurrent candidates
and unresolved references in a partial own set; arbitrary committed corruption
is not a supported recovery promise.

## Protocol

INFO schema-1 feature bit 4 (`16`) advertises this module only when enabled.
Command 81 has its own CAPS query; later command-75 queries are not used.
All integers below marked `u32` use five little-endian seven-bit bytes. A count
uses two seven-bit bytes. Data uses existing pack7 groups, at most 256 decoded
bytes per packet. No operation accepts a physical address.

| Op | Request after `[schema=1, op]` |
| --- | --- |
| 0 CAPS | None |
| 1 EXPORT_BEGIN | None |
| 2 IMPORT_BEGIN | present-mask, u32 live-length, u32 live-CRC, u32 original-pool-CRC |
| 3 READ | u32 token, selector, u32 offset, count |
| 4 WRITE | u32 token, selector, u32 offset, packed bytes |
| 5 PREPARE | u32 token |
| 6 STATUS | u32 token |
| 7 COMMIT | u32 token |
| 8 CANCEL/CLOSE | u32 token |

Selectors are original pool (0), live (1), and manual 0/1/2/autosave read views
(2–5). Only selectors 0 and 1 accept WRITE. READ count zero queries length/CRC;
absent logical roles return length and CRC zero. Receive order is the complete
pool followed by live, sequential exact pages and a final partial page. Only an
identical immediately previous chunk may be retried; retry performs no flash
write. The companion does not automatically retry requests.

Every reply starts with:
`[1, op, status, u32 token, phase, selector, u32 offset, u32 total, u32 CRC]`.

Statuses: OK 0, BAD 1, BUSY 2, STALE 3, IO/uncertain 4, CANCELLED 5,
UNSUPPORTED 6, INCOMPLETE 7. Phases: done/closed 0, export 1, receive 2,
visible consent 3, SAVE held awaiting release 4, approved 5, partial/error 6.
CAPS appends 19 bytes: `[15,3,3,u32 40960,u32 7936,u32 256,2]`.
EXPORT_BEGIN appends 16 bytes: `[present,u32 live-length,u32 live-CRC,u32
original-pool-CRC]`. STATUS appends `[source-present, committed-role-mask]`.
READ appends packed data. Other successful replies have no suffix.

WRITE acknowledges its requested selector and ending offset, that selector's
whole length and expected CRC. PREPARE, COMMIT and CLOSE return total/CRC zero.
IMPORT_BEGIN starts pool selector 0 at offset zero. STATUS reports the current
receive cursor, switching to live after all pool bytes arrive. Completion releases
the arena: verify through a **fresh EXPORT_BEGIN**, not a stale token.

## Confirmation, interruption and verification

IMPORT_BEGIN proves recognized own ownership and exclusively leases the existing
main workspace. PREPARE validates the entire original pool, all current roles,
live, CRCs, supported schemas, engines and references against the complete source
manifest before granting any destructive primitive. Mutually referenced incoming
projects therefore work on a fresh own store even when neither referenced role
currently exists. A packet never grants consent: the actual display must finish,
then the user must press and release SAVE. Notes, other input, lost quiet, USB
reset, stale ownership or display failure cancel that exact proposal.

The existing 59,520-byte arena holds decoded state 9,536 bytes, wire 7,936 bytes,
and pool plan 40,960 bytes (58,432 bytes total in the Apple fixture). After original
source validation, a genuinely noncurrent source block stages live; the resulting
working-plan seal is distinct from the original snapshot CRC. Decoded state is
then disposable bounded LCD scratch under the same owner and DMA fences. No
extra whole-set globals are allocated. Full scans run with interrupts enabled;
private primitive fences check exact role/block/sector/page/header/sequence,
epoch, USB, quiet and source-page identity. Handler reentry is excluded through
commit, final readback and adoption.

Each saved role replacement or deletion commits atomically into the shared spare.
The **whole set is not atomic**: interruption can expose a mixture, absent roles,
or temporarily unresolved references. Ordinary loading can refuse a partial set.
Keep the full host archive and retry the complete restore after a new visible
confirmation; known partial own sets have a bounded restore-only recovery route.
A reported I/O failure may still have committed the newest role, so never infer
absence or success from the transport result alone.

Final verification compares logical presence, each current role's exact payload,
and separate live. Historical physical block order, record sequence and spare
bytes are intentionally rebuilt, not restored byte-for-byte. Live adoption uses
the existing guarded runtime path; autosave establishes live as its change
baseline while preserving the independently restored autosave until a real
musical edit. A successful virtual test is not a complete issue-11 or release gate.
