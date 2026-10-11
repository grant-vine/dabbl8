# First-device evidence plan

This is preparation for issues #13, #14 and #20, not a completed install or a
hardware-qualified release. Firmware main at preparation is
`fbadbca894419c9626306eb773f38ae22baed52d`. Optional timing instrumentation is
proposed separately in draft PR #113; its full CI and hardware qualification are
not implied here. The unchanged audited Felucca baseline remains
`276f72a4e6ea8a12499a7a6819aadf3165126755`.

## Current gates

The owner reports one disconnected FM-1 running V15, no saved projects and no
spare unit or RP2040 currently available. Physical revision, enumeration and
custom preset/sample inventory are not established. A locally retained vendor
restore package is not proof that this unit can actually be restored. Keep that
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

For diagnostic collection, retain start/stop/report responses and the exact D8T1
frame with its source/package identity. Reject incomplete/invalid/saturated frames.
Record p95/p99 as histogram bounds and the observed maximum, with nominal-clock,
quantisation and instrumentation exclusions. The inclusive interval omits recorder,
assembly wrapper and some entry/exit overhead; it cannot prove whole-interrupt
latency. Measure overhead/calibration and stack high-water separately before
claiming timing safety. The proposed diagnostic has only 1,180 RAM bytes free in
its current eight-track app-only target build; linked size is not a stack bound.

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
D8T1 currently lacks per-run late/UAC-overrun/elapsed snapshots. An exact aligned
window requires additional reviewed collection or an independent instrument.
Do not use the lifetime maximum as a run-specific maximum. Preserve failures and
skips, and keep #14 open until its actual physical criteria and prerequisites pass.

## Release evidence

Issue #20 additionally requires a second tester to reproduce install, import and
rollback, source/binary correspondence, complete notices/checksums, verified
compatibility and engine limits. Host CI, the browser simulator and a successful
compile cannot replace those results. Public website downloads and release claims
remain dependent on this evidence and separate publication instructions.
