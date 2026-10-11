# First-device evidence plan

This is preparation for issues #13, #14 and #20, not a completed install or a
hardware-qualified release. The current plan was refreshed on 2026-10-11 against
the experimental diagnostic candidate in draft PR #138, `53d20f03cea3959b730c0afb9ae2b0baf779301e`, tree
`6e981b76a50818d22b3623a7b47f8b0709bd6a6b`. Timing instrumentation is implemented,
including the D8T2 aligned window from merged PR #133; current curated-profile
and diagnostic build integration has separate draft PRs #136 and #138. These
statuses do not establish installation or hardware qualification. The original
plan was prepared at `fbadbca894419c9626306eb773f38ae22baed52d`; its references
to draft PR #113 and D8T1-only collection are historical. The unchanged audited
Felucca baseline remains
`276f72a4e6ea8a12499a7a6819aadf3165126755`.

## Current gates

The owner initially reported one disconnected FM-1 running V15, no saved projects
and no spare unit or RP2040. The device has since been connected, with M-UPGRADE
reporting V15 in the observed screen. That establishes an observed connection and
reported version, not functional MIDI/audio operation, precise physical revision,
complete descriptors, custom preset/sample inventory or recovery readiness. A
locally retained vendor restore package is not proof that this unit can actually be restored. Keep that
package and all private backups outside Git. Recovery equipment and the applicable
upstream procedure must be verified before a separate owner-authorised install.
No automatic boot migration or production native-write activation is qualified.

The selected new storage direction is [three saved projects, one shared autosave
and one write spare](three-project-shared-storage.md), retaining all three user
sample slots. Permanent per-object A/B pairs are historical preparation, not the
selected initial design. After validated replacement, a reclaimed predecessor
is no longer a fallback copy. Retain original fourth-slot/autosave bytes and refuse
unresolved historical references. Shared-spare fault-injection software tests do
not establish physical erase/program, wear, recovery or safe live saves. Loader
source and flash boundaries remain unchanged.

## Evidence to retain before any install

Record the device revision/labels and reported stock firmware, Mac/macOS version,
normal USB descriptors and interfaces, MIDI ports and actual supported audio
rates. Retain exact original project/preset/sample exports and verification results
or explicitly state that each category is empty/unverified. Record a known-good
rollback package's source, length and SHA-256 plus the recovery method and equipment
actually available. Record the candidate source commit/tree, compiler/archive/SDK/
container/dependency versions, raw build logs, app/package hashes, linked memory
report and unchanged budget failures. Hardware install is a separate authorised
session; this plan authorises no update, reset, rescue operation or storage writes.

## Measurements after a separately authorised install

Use repeatable projects covering tracks 1–8 independently and together, a realistic
kit/bass/chords/lead arrangement and maximum admitted heavy-engine/FX combinations.
Repeat stopped/playing, MIDI bursts over each physical input, rapid UI/knob edits,
USB audio at both requested sample rates, and simultaneous MIDI/UI/USB load. State
which cases are unsupported or skipped; do not interpret missing inputs as passing.
Retain exact project/settings, duration, host applications/routes, audio captures,
UI/USB logs, reboot/recall outcomes and audible faults for every case.

For diagnostic collection, retain start/stop/report responses and the exact D8T2
frame with its source/package identity and external build manifest. D8T2 does not
identify the engine profile by itself. Historical D8T1 frames remain decodable but
do not contain the aligned window. Reject incomplete/invalid/saturated frames.
Record p95/p99 as histogram bounds and the observed maximum, with nominal-clock,
quantisation and instrumentation exclusions. The inclusive C interval omits recorder,
assembly wrapper/prologue and some entry/exit overhead; it cannot prove whole-interrupt
latency. The nominal 24 MHz clock, microsecond quantisation and render interval
exclusion of nested TIMER5 work require external clock/wrapper/recorder calibration.
Measure that overhead and stack high-water separately before claiming timing safety.
The current FULL8 diagnostic package has only 1,100 linked RAM bytes free; linked
size and fixed stack allocation do not establish dynamic stack bounds. The older
1,180-byte app-only margin belongs to the historical pre-D8T2 source described in
[timing-diagnostic.md](timing-diagnostic.md).

The existing `status`/`dbg` replies expose `audio_late`/`late` and USB audio glitch
counters. Preserve before/after replies with timestamps and boot identity. These
replies print fields while interrupts continue and have no atomic snapshot or CRC;
they do not share exact histogram boundaries. Treat differences as an outer
observation window, including console activity, rather than exact per-histogram
counts. Truncation, reboot, counter wrap or USB reconfiguration invalidates an
unqualified delta. Record `usb_resets`, `uac_alt`, `uac_hz`, `uac_rate_sets`,
`uac_starts`, packet/underrun/overrun/missed/stall counts, and the host's requested
rate. A stream restart changes the fill range; a rate change resets packet/frame
rate counters. Separate each stable-rate window instead of combining them.

The existing late counter checks whether DMA moved on during a rendered half; zero
late does not exclude every missed interrupt, transport fault or audible glitch.
D8T2 now carries aligned late/UAC/elapsed snapshots with validity and saturation
checks. These logical boundaries are tested in host fixtures; they do not prove
physical timer calibration, interrupt deadlines or USB interleavings. D8T1 lacks
this window and must retain its narrower interpretation.
Do not use the lifetime maximum as a run-specific maximum. Preserve failures and
skips, and keep #14 open until its actual physical criteria and prerequisites pass.

## Current FULL8 diagnostic candidate

The existing FULL package path was actually run at the exact candidate above with
`--dabbl8 0.1.0 --timing-diagnostic --engine-profile full`, pinned pi32v2 tooling,
a sparse AC79 SDK with all three required package inputs verified, and fresh
sample/header generation. The app is 484,152 bytes; `.data+.bss` uses 97,204 of
98,304 bytes, and pool state uses 335,240 of 344,064 bytes. The loader raw image and
wrapped 6,930-byte `ota.bin` are byte-identical to the audited Felucca baseline.
Loader source, package construction and flash boundaries are unchanged.

| Candidate artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| FULL8 diagnostic app | 484152 | `ea544b6c95dccce3ade0599c17098fa19847be2d3bf134c4a78fda96ba58b428` |
| Unchanged loader `ota.bin` | 6930 | `d71d1b7c12adede951605e758a3580dacccedc427d49e1fceaf51decccc68a4b` |
| `dabbl8-0.1.0-timing-diagnostic.fwsc` | 610086 | `9044a5919ac5c327d8462c8998482388c6082965c1c9cb304ec4091d7429576f` |

Build/package integrity checks passed; unchanged static budgets still fail:
audio IRQ score 55,772 versus 37,502 and UAC resampler 562 versus 504. These
heuristic scores are not physical timings. The package identity is `FM-1_8010`,
version `v0.1.0-timing`; identity alone is insufficient to bind a measurement to
this exact package. Retain its checksum and external manifest. Core/Breaks package
construction remains refused; their app-only artifacts are not installation packages.

The compact [build receipt](evidence/2026-10-11-full8-diagnostic-package/report.json)
binds the exact source, dependency pins, compiler commands, loader comparison and
limitations. Raw logs, ELF/disassembly, SDK/vendor inputs, package and private backups
remain outside Git in ignored local evidence. This is a reviewable diagnostic
candidate, not a shipping release, successful install or authorisation to install.

## Phased qualification and remaining software gates

The roadmap checklists are planning gates. Operational saved slots in #9 need #13,
#13 needs #12, and #12 lists #9; this is not a reason to weaken acceptance criteria
or enable writes. Collect bounded volatile/read-only #12/#14 measurements only after
separate owner installation authorisation and recovery preparation, while leaving
final issue acceptance open. Original V15 data need not already be migrated to the
native pool to measure volatile eight-track audio.

Production application flash writes and the native migration coordinator's write
backend are both intentionally closed. Read-only verified `RESUME` can release its
workspace and establish catalog/autosave policy; that policy does not grant physical
write authority. An actual physical native write backend, per-operation authority,
restore interface and qualified persistent Mac capture/session transport remain
separate implementation/qualification work before native migration or persistence.
The current instrument-session command is simulation-only. Do not infer physical
SAVE, replacement-cut, reboot/recall or rollback evidence from its host tests.

## Release evidence

Issue #20 additionally requires a second tester to reproduce install, import and
rollback, source/binary correspondence, complete notices/checksums, verified
compatibility and engine limits. Host CI, the browser simulator and a successful
compile cannot replace those results. Public website downloads and release claims
remain dependent on this evidence and separate publication instructions.
