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

## Local COMMIT fault investigation (18 October 2026)

The installed v0.1.9 returned status4 on command81/7. A subsequently authorized
RAM-only capture passed with stable boot2/USB1 and a successfully closed console.
The retained session marked all four roles committed, epoch302 + own64 matched
current epoch366, the lease was released, and the original requested engine
layout had reached RAM. The session was revoked and native catalog/autosave
access was offline. This narrows the failure to post-adoption handover; it does
not identify the revoking event or independently verify the complete restored
set. No restore retry or firmware flash was made during this investigation.

The local source now retains `d8pt_diag`, four 32-bit observation words (16 bytes
in BSS when own-project transfer is compiled). It grants no authority and does
not alter COMMIT status, timeout, quiet, readback or revocation policy. It records
stage (0 none, 1 baseline, 2 store, 3 verify, 4 live adoption, 5 binding/catalog,
6 autosave startup, 7 completed), raw signed return code represented as u32,
failed guard bits, and sticky revocation observations. Raw return-code meaning
is stage-specific (D8RT for adoption, D8POOL for binding/autosave).

Guard bits: 0 revoked, 1 flash unavailable, 2 canvas active, 3 transport/count-in,
4 automatic quiet refused, 5 physical/pending input, 6 migration editor busy,
7 encoder residue, 8 USB state/requests, 9 store epoch, 10 LCD fault count,
11 authorization deadline. Revocation bits 0..3 observe transport request,
queued MIDI, MIDI overflow and held notes respectively at the actual audio
block admission; bit31 marks the shared revocation entry point. These bits are
observations, not proof of the first event; direct UI/display revocation can
have no bit31 marker. They survive terminal cleanup and read-only export; a
successfully opened new import resets them. Read RAM only using that candidate's
linked symbols, never the installed v0.1.9 addresses for a changed binary.

Local real-handler tests distinguish binding IO, binding deadline, transport
revocation and autosave-start IO even when persisted/live bytes match, and keep
all failures uncertain/offline. This fixes missing failure provenance, **not the
unreproduced hardware COMMIT fault**. The source is not installed, packaged or
target memory/timing qualified. The 16-byte diagnostic cost must be included in
the next candidate's memory check. No further flash or restore is authorized.

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

The companion waits up to 30 seconds for the synchronous whole-set COMMIT
acknowledgement; ordinary requests retain the 1.5-second default. COMMIT may
already have written and verified the set when its reply is lost or delayed.
A timeout closes the session and reports an uncertain outcome, never retries
the commit, and requires read-only verification before another restore. This
host deadline is not a measured hardware worst-case guarantee.

Native refusals retain the failed command, phase, selector and offset in the
companion error. A restore failure separately records whether COMMIT was
acknowledged; successful COMMIT alone is not verified restoration. The companion
still requires a fresh full-set export and exact logical readback before reporting
success. If closing the session also fails, the original failure remains primary
and the secondary exception is available as `cleanupError`. Neither failure is
retried automatically. A terminal IO result may follow exact stored and live
bytes if the final native binding/autosave handover fails; byte equality alone
must not turn that result into success.

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

## Local companion delivery

`web/d8companion.html` includes own-project backup/restore when command-75
capabilities advertise feature 16 and command-81 schema 1 matches exactly.
The existing eight-track controls remain in-memory edits. During a project
transaction those controls are disabled and the MIDI session refuses their
requests. A selected archive is retained across cancellation or disconnection.
Restore validates the full archive using the actual C codec compiled to WASM,
waits for physical SAVE press/release, and verifies a fresh logical export after
commit. No arbitrary flash addresses, firmware installation, or external-firmware
conversion are exposed in this workflow. Historical converters remain developer
reference tools outside the main companion.

Build only the codec with the already-installed Emscripten 4.0.17:
`EM_FROZEN_CACHE=1 EMCC=/absolute/path/to/emcc sh web/build-d8archive.sh`.
No dependencies are downloaded. Serve the repository root on localhost, rather
than opening the page as a file; its generated codec is under `build/d8archive/`.
The helper records source/artifact hashes, and selective local CI refuses stale
codec inputs rather than regenerating them automatically.

Inspect the plan, then explicitly run the bounded interoperability checks:
`python3 ci/local_checks.py --paths web/d8projects.js --check own-project-client`
and the same command with `--run`. This exercises the actual command handler
against private virtual NOR, framed simulated MIDI and native/WASM codecs.
Physical transport, first installation, flash interruption, playback timing and
rollback acceptance remain open. Project archives exclude settings, presets,
samples and restore firmware; they do not constitute a full instrument backup.
