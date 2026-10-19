> Historical record only. Do not execute its old next-step instructions.
> Current authority: ../release-candidate-status.md and ../local-work-handoff.md.

# Dabbl8 release candidate status — 18 October 2026

## Latest hardware result — original restore succeeded; extra snapshot failed, stopped

On the owner-authorized fresh request (after one missed SAVE confirmation expired),
the owner pressed/released SAVE on v0.2.0. The companion reported: "All restored
projects, autosave and the current project were read back and verified."
The companion was disconnected. The frozen read-only reader then passed with
COMMIT stage7 (complete), raw rc0, guards0 and revocation0; boot3/USB reset1
unchanged. This establishes a successful COMMIT and companion logical readback
for this attempt, not the cause/fix of the previous v0.1.9 failure.

An additional independent command78 snapshot failed during verification with
"Capture reply timed out; reconnect required". Its release acknowledgement could
not be obtained on the closed link. The host closed on error and exited1; its
conservative success/cleanup receipt remains false. No further device queries,
retry, note, restore, reboot or flash occurred after this failure. Captured raw
objects are unverified and must not be presented as an accepted snapshot.
No firmware flashing occurred in this restore session.

Device work is stopped. Next: inspect snapshot timeout evidence locally; further
hardware access requires owner direction. No release qualification or independent
raw snapshot success is claimed. Evidence: build/restore-v020-20261018/handoff.json,
postcommit-diagnostic/completion.json and final-snapshot/snapshot/report.json.
Earlier entries below are historical and do not override this stopped state.

## Current device — v0.2.0 installed; boot/read-only checks pass

The owner authorized the exact frozen v0.2.0-timing diagnostic installation using
Felucca's existing MIDI loader flow. Package SHA-256
30ce914f59916f7bc23d7c31eef1c6022e3eb7f58b6ef5722625f344f3a5f037
and all frozen checksums/decoded-package controls passed before device access.
The stopped v0.1.9 preflight passed; the existing installer completed once with
exit0 and returned the expected FM-1_8020 identity. No forced install or retry.

Four read-only samples spanning60,063 ms passed: boot3 and USB reset1 remained
stable, flash detected, first-use status0, flash failures0, late-audio0 and voices
shed0. The frozen candidate-specific RAM reader then passed, with stable duplicate
reads and completion stage/raw-code/guards/revocation all0. These zeros describe a
fresh diagnostic boot; they do not recover the previous v0.1.9 failure history.
All console/MIDI handles closed. No notes, SAVE action, project restore/save or
reset command was sent; no restore retry occurred. The owner confirms the normal interface
and steady screen with controls untouched. Private evidence: build/hardware-v020-20261018/handoff.json.

The original-project restoration and underlying COMMIT cause remain unverified.
This boot result does not qualify worst-case timing, stack or release readiness.
Any restore experiment requires fresh owner direction. Older entries below are
historical; the frozen package README describes its pre-install preparation state.

## Current candidate — v0.2.0 handover diagnostic, not installed

A distinct private v0.2.0-timing (FM-1_8020) application and Felucca-format package
have been built locally. Target link/image/memory and unchanged static budgets
pass. Application526,056 B; RAM94,740/98,304 (3,564 free, +16 B diagnostic state);
pool335,688/344,064 (8,376 free,184 above its8,192 reserve). Stack allocations
remain24,320/7,936 B; worst-case stack and physical timing remain unqualified.
The unchanged6,930 B Felucca loader and flash/calibration boundaries are preserved;
automatic initialization is off. Full mask0x3ffd, UACoff/CDCon, collectors stopped.

Package SHA-256:30ce914f59916f7bc23d7c31eef1c6022e3eb7f58b6ef5722625f344f3a5f037.
Exact decoded application, every entry CRC/length, padding and damaged/truncated
package controls pass locally. Actual-C111,539/browser312 test evidence is reused
with matching relevant input hashes. The candidate-specific RAM reader defaults
to a plan, pins package/ELF/symbols/helper hashes and passes17 simulated-console
cases including refusal, stability and cleanup. Source and notices are included
under build/rc-v020-20261018; check its manifest/SHA256SUMS for frozen status.

No device access, restore retry, flashing, release publication or hosted CI was
performed. Installed firmware staysv0.1.9. The handover fault is unresolved and
original restoration remains unverified. No automatic hardware action is pending;
installation and any restore experiment require separate owner direction. Read
build/rc-v020-20261018/README.md and validation/review.json for the prepared plan.
The lower entries are historical and do not override this current candidate state.


## Current local investigation — completion trace added, hardware cause unresolved

The owner authorized read-only investigation only, with no restore retry or flash.
One fixed-allowlist RAM/status capture passed and closed the console. Retained
state shows the original requested engine layout in RAM, all four roles marked
committed, epoch302 + own64 == current366, no lease, revoked1, native mode2/ready0
and autosave not ready. Boot2/USB1 stayed stable. This supports a post-adoption
handover failure but does not retain its exact return code or revocation source.
Complete original restoration remains unverified; no device data writes occurred.

Local source now preserves stage/raw-code/guard/revocation diagnostics (16 B BSS),
without relaxing any safety gate or changing uncertain outcomes. New actual-C
regressions fail on the old transfer implementation and pass on the local source:
111,539 firmware checks and 312 browser/C/WASM checks (run-b7qezy3g). Binding IO,
deadline, transport revocation and autosave-start failures remain offline even
with exact bytes. The separate captured-data rollback exercised 9,432 simulated
interrupt-enable/audio boundaries without reproducing the physical fault.

This fixes missing diagnostic provenance; the firmware COMMIT fault is **not
fixed or release-qualified**. The diagnostic source is not flashed or packaged,
and target memory/timing checks remain pending for any later candidate. No
further device action is pending or authorized. Read build/restore-completion-
20261018/handoff.json and decoded-session.json first. Lower entries are historical.


## Current state — original restore stopped at COMMIT

The locally repaired companion staged the fresh original99bd archive once. The
owner confirmed the visible prompt and pressed/released SAVE. The companion then
reported native status4 at command81/7, phase6, selector0/offset0: COMMIT was not
acknowledged. This now identifies a firmware COMMIT failure before host readback;
the exact firmware substep remains unknown. The companion disconnected. All
device work stopped: no query, retry, reboot or flash after this failure.
Current device data is unverified; original restoration must not be claimed.
Original backup99bd remains intact. New owner direction is required for further
device access. Read build/restore-completion-20261018/handoff.json first.

Local fixes passed 312 browser/C/WASM checks and 99,742 firmware transfer checks.
The captured temporary -> original private virtual-NOR rollback passed 1,013,855
checks, including exact logical/live bytes and unchanged unrelated NOR. Actual-C
fault/deadline injections show that exact bytes alone do not prove completion.
The companion retains command/phase context, COMMIT acknowledgement and a primary
failure even if cleanup also fails. No safety guard or production firmware was
changed by this investigation. The target failure remains a release blocker.
The lower entries are historical and do not override this current stopped state.


## Current persistence state — read-only verification complete

The owner authorized one status and full snapshot after the uncertain restore.
Both passed and closed their ports. v0.1.9 remains at boot2/USB1, stopped; status
reported flash1, fl_failure0, audio_late0 and voices_shed0. These are current
observations, not proof of every earlier operation. The native pool codec and
exact-byte comparisons independently confirm presence15 and all five proposed
logical objects. A/B remain original; C/shared AS/live match the heavy proposal.
All11 unrelated persisted/runtime settings, preset and sample roles match the
fresh original baseline. No further write, retry, power-cycle, note or flash
occurred. Original restoration remains pending; original archive is preserved.

The firmware-reported uncertain outcome is an unresolved release blocker even
though data matches. The capture does not identify whether final adoption,
binding/autosave handover or a subsequent export/readback produced the error.
Do not treat this as just the earlier companion timeout or ignore the refusal.
Next investigate locally before any new device action. Read
build/persistence-v019-20261018/handoff.json, uncertain-status.json,
uncertain-snapshot/receipt.json and uncertain-comparison.json.


## Latest persistence batch stopped — 18 October

The fresh baseline matched all17 roles of the verified original. A new native
archive was validated and preserved at build/persistence-v019-20261018/
original-set/verified/project-set.d8bak (SHA-256
99bd36dcae4ad784c4fbded37d7fd2789c4892410bb88ae0e80f69aaedbec399).
The authorized proposal was staged once and the owner pressed SAVE. The companion
then reported “Restore outcome is uncertain. Retain the archive and verify or
retry the complete set.” No successful readback was observed. Device work stopped
without any further query, retry, intentional power-cycle or flash. Current
project contents are unverified; the prior original-restored state is historical.
Read build/persistence-v019-20261018/handoff.json and failure.json first.
Owner direction is required for guarded read-only status/snapshot before any
further write. Do not retry because the UI suggests retry as an option.


## Current state

Installed firmware is private v0.1.9-timing. It is not release-qualified.
The original live project and A/B are restored exactly; C and shared autosave
are absent again. All 11 unrelated persisted/runtime data roles are unchanged.
The companion and native ports are closed. No device operation is pending.

## Completed evidence to reuse

- Local engine/effects matrix: 84 scenarios; reuse the existing source-bound pass.
- Heavy timing: 6,317 blocks, zero recorded late events/taint/reset; maximum
  render/inclusive intervals 49,992/53,468 ticks. Clock is not calibrated.
- Memory: eight heavy slots exercised (six FM6, DRUM, WHEEL), zero allocation
  failures/invalid owners. User/supervisor surviving written depth was
  5,772/24,320 and 1,188/7,936 bytes. These are lower bounds, not max-SP proof.
- Full 17-role equality before/after memory; original restoration independently
  verified with the native pool codec and a separate full snapshot.
- Host fixes: 303 project checks, 85 MIDI checks, 21 console transport tests,
  24 resource-capture tests, plus actual console checks across three profiles.
  Delayed/missing replies, no-retry behavior and startup/cleanup failures are covered.
- Draft host PR #199: https://github.com/grant-vine/dabbl8/pull/199
  (two commits, six files), stacked on firmware quiet-tail PR #198. Both unmerged.
  Self-review reused seven QA records and matched 156 compiler dependencies.
  This is not independent final release approval. GitHub Actions stays disabled.

Private receipts: build/rc-durability-v019-20261018/completed-results.json,
restored-final-comparison.json and handoff.json. Host review and source-only
addendum: build/host-tools-review-20261018/. All earlier failures are retained.

## Remaining release gates

| Gate | Remaining work |
| --- | --- |
| Source and package integration | Review/integrate the stacked drafts and independently review final source/notices/package. Host fixes do not change the installed application. Keep the original frozen bundle immutable; use the separately identified host source addendum. |
| Own-data persistence | Physical reboot with all three slots and nonempty shared autosave; physical local save/recall and supported own-upgrade durability. Virtual NOR passes do not close these gates. |
| Timing and memory limits | Calibrate clock/observer overhead, cover concurrent effects/transport/UI/routing loads, and establish complete stack bounds. Static spare RAM and watermarks are insufficient alone. |
| USB audio scope | Installed v0.1.9 is UAC-off. Choose that release scope explicitly or qualify a separate UAC-enabled candidate. |
| Recovery and reproduction | Independent install/backup/restore/rollback reproduction; retain the tested stock-v15 USB recovery scope. RP2040 recovery is not qualified merely because the board is owned. Interrupted-write testing needs a specific recoverable procedure. |
| Distribution | Final compatibility/limits, source and notices, checksums and publication authorization. No public RC tag, download or release claim yet. |

The next owner session should batch temporary setup, one deliberate power-cycle,
and original restoration. Do not repeat successful timing or memory workloads.
No new flash is required for that persistence check. Do not stage anything until
fresh owner readiness and authorization; hardware_ready remains false.

## Historical checkpoints — context only

The records below preserve earlier failures, plans and superseded versions.
The current state and gates above take precedence; do not execute old instructions.

# Release candidate readiness - updated 18 October 2026

## 18 October: corrected console, full heavy-pool memory and original restoration PASS

The corrected CDC startup passed on the FM-1 with a strict fresh greeting and
status; boot2/USB1 remained unchanged. Full readiness snapshot matched all17
regions of the verified temporary set. Only the missing resource pass ran; the
previous passing6317-block timing stage was reused. Resource collection observed
8/8 heavy slots (sixFM6, oneDRUM, oneWHEEL), zero allocation failures and zero
invalid ownership. Its17-role before/after snapshots matched the fresh readiness
snapshot exactly. Recorded stack surviving written depths were5772/24320 user
and1188/7936 supervisor bytes; these are lower bounds, not complete max-SP proof.

The owner pressed SAVE at the original-set restore prompt. Corrected companion
reported “All restored projects, autosave and the current project were read back
and verified.” It then disconnected. A separate final status passed; a separate
full snapshot plus actual native pool codec independently verified the exact
original live/A/B objects, original absent C/shared-AS (presence3), and all11
unrelated persisted/runtime settings, preset and sample roles unchanged. Pool
generations/history legitimately differ; logical objects and unrelated bytes
are verified. All native ports closed successfully. No further firmware flash
or intentional power-cycle occurred in this continuation. Original restoration
is COMPLETE; do not follow historical pending-restore instructions below.

Evidence: build/rc-durability-v019-20261018/completed-results.json,
resource-comparison.json, restored-final-comparison.json, final status/snapshot
receipts and handoff.json. Console local regression evidence remains under
build/console-reopen-20261018/; companion local evidence under
build/companion-timeout-20261018/. Previous failures are retained, not relabelled.

Release qualification remains false. Next local work is source review/integration
for host fixes and RC evidence consolidation. Physical nonempty-AS reboot
durability was not performed in this resumed memory/restore session; calibrated
timing, complete stack bounds and other final release gates remain open.
Automation remains paused. No new device action is needed for this completed
continuation; reuse these matching passing results.


## 18 October: console reopening — second local correction prepared; device stopped

The owner authorized local console fixes, remaining memory checks and original
restoration, stopping on any device failure. Retained-high DTR was reproduced
locally and a rising-edge handshake added. The actual C console fixture confirms
that retained-high DTR produces no greeting until its main loop observes low.
The first physical read-only check then received the exact greeting, sent one
status command, and received another identical greeting before the status reply.
Strict framing refused “Multiple console prompts / stale reply”. Device work
stopped immediately. No notes, resource collection, reboot, restore or flash
followed. Process exited; host lsof found no tty owner. This is not an accepted
status result or a successful original restoration. Raw journal preserved.

A second local correction now clears unrequested startup input while DTR is high
before requesting a fresh greeting. It sends no payload during startup, retains
strict echo/prompt/version checks and restores original DTR/termios on close.
Both initial DTR states and injected low/high/settling/input-flush failures are
covered. The stale automatic-greeting regression failed before this correction;
21 transport and 24 resource capture tests pass afterward. Actual console checks
passed in three profiles, optimized and ASan/UBSan. Relevant unchanged C binary
evidence was reused for final resource tests. No target firmware source changed.
The 100ms settling intervals are bounded handshake waits, not a hardware timing
guarantee. The second correction has NOT been tried on the FM-1.

Evidence: build/console-reopen-20261018/final-results.json and
build/rc-durability-v019-20261018/reopen-stop.json. Readiness remains false;
automation remains paused. Original backup is preserved. Next authorized device
session should start with guarded read-only status and full data comparison,
then only the remaining resource workload and restoration after clean results.
Reuse the passing 6317-block timing stage; do not repeat it.


## 18 October: companion fix passes locally; heavy batch stopped at resource greeting

The companion COMMIT acknowledgement now has a bounded30s wait while ordinary
requests retain their configured/default1.5s deadline. Timeout errors identify
command/op and mark COMMIT outcome uncertain. No retry or protocol/firmware change.
The actual C/WASM interoperability regression failed before the fix when holding
a completed COMMIT reply past1500ms, then passed303checks. A missing reply closes
at30000ms with no retry/CLOSE/export; late replies cannot revive the session.
General MIDI framing/lifecycle suite passed85checks. Prior physical timeout's
exact command/duration is unrecorded; physical corrected-restore QA remains pending.
Evidence: build/companion-timeout-20261018/results.json and red/green logs.

Owner authorized continued batched tests and original restoration, stopping on
failure. The prepared timing stage passed on installedv0.1.9:6317blocks, max
render49992/inclusive53468ticks, zero late publications/taint, unchangedboot2/USB1,
128matched note pairs and clean timing-stage closure. These are uncalibrated
scoped observations, not worst-case qualification. Resource stage then failed
waiting for its fresh serial greeting BEFORE notes or collector commands. The
session stopped: no reboot, rollback, new restore or flash followed. Resource
peak/stack and full AFTER data comparison are missing. Original rollback archive
is preserved; latest full verified data was the exact temporary set before notes.
Failed session receipt cannot attest full cleanup; process exited and host lsof
found no tty owner. No post-failure device query was sent.

Both hardware readiness and release qualification remain false. See
build/rc-durability-v019-20261018/heavy-session-stop.json and handoff.json.
Next device action requires owner direction: read-only status/snapshot to establish
current state before original restoration. No automatic retry, reset or format.
Firmware emits greeting on a DTR rising edge; reopening with original DTR retained
is a local hypothesis for the missing greeting, not a confirmed hardware cause.


## 18 October: read-only follow-up verifies temporary restore

After explicit owner authorization for one status and snapshot, both completed
and closed their ports successfully. v0.1.9 boot2/USB1 remained unchanged, playback
was stopped, and no flash failure or audio-late event was reported. Offline native
codec inventory of the captured pool found presence15. All five logical objects
(live/A/B/C/shared AS) exactly match the proposed set: original A/B preserved;
C/AS/live SHA-256 678dba08d7f40c4a5e8c8223d703511466ef0aeb4e4d80987e406ef2b8f375c3.
All 11 unrelated settings/preset/sample persisted/runtime roles match baseline.
The temporary set is therefore installed and verified; **the original live/C/AS
state has not yet been restored**. No notes, reset, new restore, saved-data writes
or flash occurred during this follow-up. hardware_ready stays false; device work
remains stopped pending direction after the failed companion transaction.

Evidence: build/rc-durability-v019-20261018/post-timeout-status.json,
post-timeout-snapshot/receipt.json, post-timeout-comparison.json. The missing reply
stage/root cause remains unconfirmed. Source inspection finds a default 1500ms
mandatory request deadline and synchronous whole-set COMMIT before its reply;
this is a hypothesis to test locally, not proof to merely increase a timeout.
No original restoration or heavy-workload/persistence pass may be claimed.


## 18 October: latest full-pool batch STOPPED after SAVE

Fresh v0.1.9 status passed (boot2/USB1), and all 17 original raw data roles
matched the preserved backup. The owner authorized temporary setup/restoration,
and pressed SAVE at the verified RESTORE OWN PROJECTS prompt, reporting a normal
steady screen. The companion then showed “MIDI reply timed out; reconnect before
editing.” and disconnected. No successful restore/readback was observed.
**Current project/AS contents are unverified; do not assume original or temporary
data.** No workload, intentional reboot, rollback, retry or flash followed.
Both archives remain preserved. Evidence: build/rc-durability-v019-20261018/
failure.json and handoff.json. Keep hardware_ready false. Stop device work pending
owner direction; next useful action is a separately authorized guarded read-only
status/snapshot to locate the failure, not another restore. Browser error/warn
logs were empty. Source shows missing mandatory MIDI acknowledgements close the
session without retries; this does not identify the timed-out command or prove
whether flash writes completed.


## v0.1.9 physical batch — installed and scoped checks passed

The owner authorized a batched installation/test session on 18 October. Private
`v0.1.9-timing` / `FM-1_8019` was installed once through unchanged Felucca flow.
Four read-only checks over 60 seconds passed. All 17 raw data roles matched the
fresh pre-install snapshot; no formatting or physical SAVE was required.

Two workloads each passed separate timing/resource stages with bounded note
cleanup: six FM6 plus DRUM/ANALOG (seven heavy bodies), and notes on all eight
tracks (two heavy bodies). Four passes totalled 512 matched note pairs. Full
17-role before/after equality passed for both; no late publications, resets,
allocation failures, invalid owners or uncertain port cleanup were reported.
The heavy timing window contained 6,323 blocks (maximum render/inclusive
47,256/50,401 ticks); eight-channel window 6,311 (47,232/50,492 ticks).
These remain nominal-clock scoped observations with the recorded IRQ/observer
exclusions, not calibrated worst-case deadlines or complete stack clearance.

The specific quiet-guard regression passed physically: read-only own-project
export after the workloads succeeded naturally, without DIST adjustment or SAVE.
Logical current/saved/autosave data matched the original archive, and the export
raw pool matched fresh pre-install bytes. A browser download-wait control timeout
was resolved by observing the already-completed verified backup and validating
its downloaded file; no export retry occurred. The companion disconnected and
a final read-only status passed with boot2/USB1 unchanged. No project save,
restore, reset or second flash was requested. The quiet-tail fix is now installed.

Local private evidence: `build/hardware-batch-v019-20261018/RESULTS.md`,
`batch-results.json`, `quiet-export-comparison.json` and individual receipts.
Owner listening/screen feedback remains pending; no audible/visual pass is
claimed from counters. No manual power-cycle was done. Current live differs from
saved slots and shared autosave is empty: prepare a controlled own-save/durability
session rather than casually rebooting this workspace. All-three-slot/nonempty
shared-autosave durability, eight-heavy-body setup on this binary, broader
physical effects/transport/UI, independent review/reproduction and recovery gates
remain open. UAC stays off. No hosted CI, public release or GitHub evidence upload.

## Next combined durability/full-pool session — prepared locally

`build/rc-durability-v019-20261018/` prepares a proposed own-project set with
A/B preserved exactly, C populated, nonempty shared autosave and current live
set to the same eight-heavy-body project. Presence changes from 3 to 15.
Original rollback is the verified post-workload backup from the v0.1.9 batch.
The actual current renderer passed the eight-body fixture and codec rollback;
actual firmware transfer/boot code passed virtual-NOR import of all three slots,
AS recovery after virtual reboot, exact logical rollback and unchanged bytes
outside the approved pool map. The many byte assertions are not independent
hardware scenarios. The native archive adapter is reused by its source/hash
binding; no target rebuild or flash was needed. Offline controller checks pass
10 fault cases and 6,144 message-grammar cases. Hardware readiness stays FALSE.

A new supervised batch can cover setup, full-pool notes, shared-AS reboot and
original restoration using two physical SAVE confirmations and one power-cycle.
It writes project C/autosave temporarily and requires owner availability and
explicit temporary-data approval. It does not prove a physical local SAVE/recall
gesture, power-loss interruption safety or general timing/stack clearance. No
new device access or saved-data change occurred during this preparation.

Quiet-tail source integration is now draft [#198](https://github.com/grant-vine/dabbl8/pull/198),
commit `26b3ad5e6a70d489ac9a191630dbe303f1d0792d`, stacked on #197. Only fx.c
and its owning regression are changed. The clean source branch matches all 267
v0.1.9 target inputs and all 104 compiler-derived quiet-test inputs; exact check
arguments match the reused passing receipt. No repeated renderer run or target
rebuild was needed for source integration. Actions remains disabled. No merge,
private backup/device-log publication or additional device operation occurred.

## Corrected private package — preparation record (before installation)

Private `v0.1.9-timing` / `FM-1_8019` is locally built and reviewed in
`build/rc-v019-20261018/`. Its package SHA-256 is
`4ad55e9192c565f3285281a63042ac2d29cc0a555084cb03e267c8be998860d5`.
The app is 525,352 bytes (16 smaller); linked RAM and pool allocations are
unchanged. Existing target static budgets pass without edits. All 84 affected
engine/effect scenarios, 11,328 mixer checks and performance-effects checks pass
freshly. The 130,200-check quiet regression and before/after transition evidence
are reused after all 104 compiler-derived inputs matched. The exact 267 target
manifest inputs match the corresponding-source archive. Loader SHA, flash bounds,
entry CRCs, decoded app bytes, padding and malformed-package controls pass.

Review is a scoped main-session self-review, not independent final release
approval. Four inherited target compiler warnings remain. Package decoder alone
does not reject OTA truncation; the supplied verifier separately checks every
entry length, CRC and exact loader bytes. The initial decoder-only negative
assertion failed and that limitation is explicitly retained in the package notes.
No production loader/installer/decoder change was made. No FM-1 access, saved-data
change, flash, download or hosted CI occurred in this preparation. USB audio
remains disabled. Installed v0.1.8 evidence is historical and cannot qualify this
new binary. See the package README, `validation/review.json` and
`review-handoff.json` for the next separately authorized hardware gate.

Quiet-guard follow-through identified a concrete installed-build blocker:
track1 DIST=0 with tail16 and retained filter values36/-45/-46. The mixer
short-circuits on bypass before decrementing the tail, so inactive state never
retires and the stricter storage quiet guard refuses export/import indefinitely.
Fixed locally for eight tracks by retiring the idle countdown even during bypass;
the guard and DSP memory remain intact. The focused selector passes 130,200
checks with stable inputs; the isolated pre-fix case fails 428 assertions. Active
and enabled-tail audio comparisons, including the immediate next note after
re-enable, match old code exactly. Idle re-enable intentionally suppresses the
old stale ghost tail; no claim that all transitions are byte-identical. The v0.1.8 investigated in this historical record was unpatched; the later
v0.1.9 installation and physical results are recorded above. Evidence and transition
review: `build/quiet-guard-20261018/`.

A read-only project export reproduced the refusal and device work stopped. The
owner then authorized one expanded RAM capture, which completed/closed safely.
Original-project restoration is complete. The owner-authorized physical
DIST0->1->0 adjustment passed, returned to zero, preserved all eight track dumps,
and closed its ports. A subsequent read-only snapshot matched all 17 raw data
roles against the temporary pre-workload setup. The first original-restore
request was cancelled before physical confirmation; its exact cause remains
unconfirmed. After explicit authorization for one fresh request, the owner
confirmed the restore prompt and SAVE press/release. The companion read back
and verified all three saved projects, shared autosave and current project
against the preserved original archive. The companion was then disconnected.
Evidence: `build/quiet-guard-20261018/original-restore-success.json`.
No firmware was flashed or reset during that restoration task. The subsequent
v0.1.9 batch installed the idle-tail fix as recorded above. The controller passes 19 offline fault scenarios and 17,668 grammar
cases; selected static checks report 0 errors/119 retained dynamic warnings
(not strict-clean). Do not claim the local fix or prior 84-case evidence
qualifies a changed target image.

Full-pool physical timing/resource stages now pass on installed v0.1.8: six FM6
notes, DRUM and WHEEL/FULL ORGAN reached eight of eight heavy bodies. Owner
authorized the temporary project setup and restoration and confirmed SAVE for
setup. Each stage sent 128 matched note pairs over 18 seconds. Timing captured
6,295 blocks, maxima render 49,488/inclusive 52,804 uncalibrated ticks, with zero
late publications, taint or auxiliary pending observations. Boot/USB counters
stayed 1/1; allocation failures and invalid censuses were zero. All 17 configured
data roles matched before/after notes; collectors stopped and ports closed.
Stack surviving-write observations increased to user 5,772/24,320 bytes and
supervisor 1,188/7,936; these remain lower bounds, not stack-clearance proofs.

Historical full-pool run initially stopped at a PT_BUSY quiet-guard refusal.
That incident is resolved: the original project set was restored and read back
before the v0.1.9 installation, and v0.1.9 now passes natural quiet export after
its workloads. The temporary WHEEL setup is not currently installed. Retain
`build/full-pool-physical-20261018/RESULTS.md`, `handoff.json` and
`DEBUG-JOURNAL.md` as historical failure evidence; they must not trigger a repeat
of the completed workaround or restoration. The current original project is
identified by the latest v0.1.9 batch handoff above.

Draft #197 (`ae8b71f247bfa721ce095b43f0ee175bf6721e6d`) strengthens selective
local receipts with before/after source fingerprints and fails observed input
mutation even when commands succeed. All 38 receipt/selector tests pass. This
does not retroactively bind old receipts or add hardware qualification. Evidence:
`build/rc-check-receipts-integration-20261018/`. No firmware/device operation;
Actions remains disabled and all source PRs remain unmerged. Current issue
acceptance bodies were reread; the external release/durability gates remain open.

Additional source-only draft PR #196 (`a8459733d9476da0b54df2f3a2690c172a42cb1a`)
makes bounded MIDI regressions independent of private demo files. Actual host
eight-channel/drum, 256 REC-mask and schedule cleanup/refusal checks passed,
alongside 36 selector tests and zero core type errors/warnings. Optional file
modes passed using synthetic startup data. Evidence is retained in
`build/rc-midi-workload-integration-20261018/`. No device operation or firmware
change occurred; this does not expand physical qualification. Actions disabled,
PR unmerged, existing release gates unchanged.

Reviewed source groups are now separate draft PRs #192 (restore), #193 (native
MIDI guard/snapshot), #194 (voice regression) and #195 (engine/effects checks).
They remain unmerged and do not establish a public RC. The combined local tree
at `5278eee51bf2b69b29aa2323e70bb3d6e49faf51` passed focused cross-boundary checks;
34 selector tests pass. The 84 engine matrix cases and two performance fixtures
were reused only after actual compiler dependency/flag matches, without reruns.
Evidence: `build/rc-combined-integration-20261018/REVIEW.md` and `handoff.json`.
Actions is disabled; no target build, device operation, release or deployment.

Subsequent heavier physical workload also passed: six FM6 notes, DRUM and
ANALOG, 128 pairs per stage; seven of eight heavy bodies, zero allocation failures;
6,292 timing blocks, no late publications/taint/reset, maxima render 51,168 and
inclusive 54,619 ticks (uncalibrated). All 17 current/persisted roles stayed
unchanged; resource stop/closure passed, no saved-data changes or flash.
See `build/headless-seven-20261018/RESULTS.md` and `handoff.json`. The report's
last-census count is not final voice state; no owner listening or direct final
active-voice measurement was obtained. This completes the seven-body scenario,
not all-engine/all-preset, eight-body or full deadline qualification. Reuse it.

Latest bounded physical follow-through passed: corrected eight-channel MIDI
timing and resource stages, 128 note pairs each, no late publications or new
boot/USB resets. Timing: 6,303 blocks, maxima render 49,632/inclusive 52,954 ticks;
uncalibrated. Resource: two of eight heavy slots, zero allocation failures;
surviving stack writes user 4,320/24,320 and supervisor 1,188/7,936 bytes, lower
bounds only. All 17 captured current/persisted roles remain byte-identical to
the original 17 October snapshot; note cleanup, acknowledged resource stop and
port closure passed. No saved-data changes or flash. Evidence and remaining
limits: `build/headless-resource-20261018/RESULTS.md` and `handoff.json`.
The first resource attempt was refused by a host command-profile error before
device start; corrected offline coverage passed and only that stage was repeated.
An eight-slot FM6-only proposal stopped locally at its explicit six-voice engine
cap; no extra hardware test was attempted. Full-pool and broader configured
engine/FX device timing, deadline calibration, write-dependent durability and
final integration/review remain open. This supplements the historical scope
records below; it does not qualify a public RC or require another flash.

Status: private diagnostic validated in the scopes below; **RC not yet qualified**.
Follow-through: independent review approved the frozen private bundle/plan only;
report is `build/rc-followthrough-20261016/independent-package-review.md`.
It does not approve newer helper code or physical qualification. The subsequent
test import failure is preserved in `build/rc-followthrough-20261016/staged/STOPPED.md`;
its import fix now type-checks with zero errors/warnings. Selective checks passed
in `build/local-checks/run-gutjktqg/results.json`: actual TRACK query covers all
256 recording masks without selection/storage writes; eight-channel MIDI runs
128 note pairs through the actual renderer, peak eight voices/two heavy slots,
no held notes and byte-identical project; 37 selector tests pass.
The new read-only preflight and guarded capture helper have separate offline
evidence under `build/eight-midi-qualification-20261016/`. See
[eight-channel follow-through](rc-eight-channel-followthrough.md).
These helpers are a later supplement; the frozen v0.1.8 bundle is unchanged.
Next required device action is a fresh connection/read-only state check, not a
flash or project load. Broader engine/FX setup and data-durability writes remain
outside current permission. Do not infer a full-project match from track dumps.

An exact private qualification bundle was frozen on 16 October under
`build/rc-freeze-20261016/`. Its archive is
`dabbl8-v0.1.8-qualification.tar.gz` (17,369,114 bytes), SHA-256
`207fee0173dab0b4385616764deaaae9cad228c81c2a7fd77e24d31028e415aa`.
`SEAL.json`, `verification.json` and `negative-checks.json` retain the results.
The 186-file bundle contains the original binary, exact captured source,
263 verified target inputs, seven generated headers, unchanged SDK inputs,
notices, build/static reports and selected existing validation evidence.
It passed standalone verification outside the checkout and after archive
relocation; six damaged/missing/extra/path/symlink controls refused as expected.
The expanded directory and outer archive are read-only; checksums are integrity
records, not signed authenticity. No new firmware application was built.
The later validation-source overlay is explicitly separate from the app's
corresponding source, including the undefined test observer conditional.
The package remains byte-identical to installed v0.1.8; no version/RC relabel,
release publication, independent final package approval or clean-commit claim
was made. Original historical manifest fields remain unchanged.

The [safe hardware test plan](rc-hardware-test-plan.md) is prepared. It separates
eligible future read-only/note-only checks from engine/FX edits that can autosave
and durability tests that require writes. `status` alone does not expose REC;
verify that state independently before any new note schedule. No device request
is pending, and no hardware was accessed for the freeze. Existing no-flash and
no-saved-data-change constraints still apply.

Local work resumed on 16 October; the FM-1 remains unplugged and untouched.
The latest local actual-renderer matrix passed in
`build/local-checks/run-5rtbcdfx/results.json`: 13 enabled engines plus one mixed
eight-track layout, each with dry, chorus, delay, ROOM, SPRING and combined effects
(84 scenarios). Eight shared voices, bounded pool ownership, requested sends/wet
output and released-note cleanup passed under ASan/UBSan with the existing DSP
signed-overflow/shift exclusions. This is host behavior, not device timing or an
all-presets proof. New fixture `tests/engine_fx_matrix_test.c` and its selective
`engine-fx` recipe remain local. The relevant 187 source/generated inputs were
verified unchanged; the 84 scenarios were reused without rerunning them.
No FM-1 access, flash or saved-device-data changes were performed for this matrix.
Resume from this receipt; do not repeat the passing matrix without changed inputs.
Follow-up checks passed: `build/local-checks/run-ywfe1bqv/results.json` adds
11,328 assertions across 2,240 actual banked solo/mute dry/stereo/send cases;
`build/local-checks/run-wb58234s/results.json` adds the existing optimized
eight-track performance-effects fixture and 36 selector tests. Repeat/reverse,
tape stop, freeze, filter/crush/throw and harmonizer checks passed. The host
instruction counter was available and its unchanged assertions passed; its
inherited device percentage estimates are not physical FM-1 measurements.
The new `performance-buses` and `performance-fx` recipes keep those checks
separate from the passing engine matrix. No production firmware changes or
new application/package build were needed in this follow-up.

The scoped main-session review of the matrix, affected recipes, selector tests
and claims passed; see `build/rc-local-review-20261016/REVIEW.md` and `report.json`.
This is not independent package review, complete software acceptance or RC
approval. Final source integration/review, heavier physical timing/stack and
concurrent routing/UI workloads, own-data durability/interruption recovery,
USB-audio scope and independent reproduction remain the gates listed below.
The frozen private bundle and test plan above complete this local preparation;
current permission does not allow saved-data changes or flashing. Follow-through
remains paused at the remaining qualification gates.
This is the current handoff. The [14 October acceptance map](software-acceptance-status.md)
and earlier package documents retain historical source-specific findings.
GitHub issue bodies were read on 15 October; #9-#15, #19 and #20 remain open.
Unchecked planning prerequisites do not imply missing implementation, and a short
device test cannot close a broader acceptance requirement.

## Candidate and reused evidence

Installed version: `v0.1.8-timing`, product `FM-1_8018`, eight logical tracks sharing
eight voices. Full engine mask `0x3ffd` retains 13 engines, including SLICE; FM4
and USB audio are disabled. This is a diagnostic configuration, not a final
declaration of supported release combinations.

Source base: `33c3b62b40ba1708ccfb6bfb14fb0191bf55bcea` **plus the captured working-tree
changes**. Never describe the installed package as a clean build of that commit.
Its exact source snapshot, patch and manifests remain under
`build/timing-diagnostics/resource-v018-xx2XYR/`.

Package SHA-256:
`6de9bfbb6ec41d0ef7467df3666783c22de7d133a515ce38cfc1da1be845e353`.
Application SHA-256:
`f01bafba8b1605c94dbaca708b4d68ca79069ba437e971a9ac5415cbd7394376`.
The unchanged Felucca loader and approved boundaries passed offline validation;
installation used Felucca's existing installer. No new flashing flow is needed.

All evidence paths below are local to the firmware worktree. Private backups and
vendor firmware remain outside Git. Reuse these results only for their recorded
inputs and configuration; do not rerun the upstream baseline without a new cause.

| Established scope | Retained evidence |
| --- | --- |
| Restore screen stays drawn, held SAVE/release accepted, timeout and unsafe-input/fault refusal | `build/local-checks/run-arzwwb42/results.json`: 84,098 transfer checks, 297 browser/C/WASM checks, sample-upload checks; `run-1sj2vfmm` covers read-only boot and save consent |
| Exact app/package/source snapshot, CRCs, unchanged loader, corruption refusal and notices | `build/timing-diagnostics/resource-v018-xx2XYR/package-validation.json` and `package-check.mjs` |
| Link/static budget pass for this UAC-off target | `build/app-only/full/run-nv6ahg_x/app-only-manifest.json` and `static-budget.stdout`; application 525,368 bytes, general RAM 94,724/98,304, pool 335,688/344,064 |
| Stable startup before restore, no additional late events during the check | `build/hardware-trial-v018/readonly-boot-stability.log`: four samples over 60 seconds; startup late counter stayed at 4, not zero |
| Physical SAVE, steady restore prompt, original project A preserved, demo B/current restored and exact fresh readback | `build/hardware-trial-v018/handoff.json`; owner confirmed steady prompt and normal interface |
| Internal eight-track demo plays/stops cleanly, owner hears steady sound | `build/hardware-trial-v018/internal-playback-20261015/`: 3,453 blocks, zero late publications/taint, stable boot/USB counters; inclusive maximum 51,360 ticks; clock uncalibrated |
| Saved A/B and explicit C/autosave absence survive controlled power cycle | `build/hardware-trial-v018/reboot-persistence-result.json`: exact role/presence comparison; unsaved workspace is separate, no last-project auto-resume claim |
| 76 relinquishments reproduced as released drum-tail reclamation, no held-note victims or budget overflow | `build/voice-relinquishment-20261015/`: exact demo and two controls; `build/local-checks/run-27ktqq4o/results.json` adds allocator stress and four/eight-track overload tests |
| First physical idle pool/stack and timing baseline | `build/measurements-v018-20261015/RESULTS.md`: D8R1 generation 1, zero allocation requests/failures, user/supervisor observed written depths 3,340/956 bytes; D8T2 3,451 blocks with zero late/taint, maximum render/inclusive 12,864/13,745 ticks. No heavy workload or stack-safety qualification. |
| Demo resource measurement and sustained playback | Same measurement directory: pool peak 2/8 (DRUM/FM6), 83,765 reuses with zero allocation failures; observed user/supervisor stack depths 3,632/1,188 bytes. Separate 60-second D8T2: 20,687 blocks, zero late/taint/reset/shedding, maximum render/inclusive 48,120/51,601 ticks. Resource observer paused during timing; heavier combinations and calibration remain open. |
| Guarded external MIDI drum workload, 16 October | `build/measurements-v018-20261015/DRUM-WORKLOAD.md`: both stages accepted, 128 matched note pairs per stage and successful port closure. Timing: 6,239 blocks, zero late/taint/shedding/reset/storage failure, maximum render/inclusive 44,856/48,014 ticks. Pool: peak 1/8, zero allocation failures; observed user/supervisor stack depths 4,116/1,188 bytes. Narrow DRUM workload, not all-engine qualification. |

The hardware lifetime voice counter has no per-event cause trace. Its matching
local reproduction explains the observation without proving each physical event
or the inaudibility of every possible voice steal. No production allocation policy
changed. The later conditional observer and host/CI/documentation edits are not a
new installed binary. Old run receipts without source hashes remain historical
evidence, not automatically reusable matches against any dirty checkout.

## Remaining gates and concrete next work

| Gate | What remains | Work possible before another flash |
| --- | --- | --- |
| Exact reviewed source and package | Exact private v0.1.8 package/source bundle is frozen and verified above. Restore fixes/test changes remain local; PR #191 remains open. Final integration and independent package review are pending. | Review/integrate the small fixes without relabelling the frozen binary. Reuse source-bound evidence. Build a new app only for changed target inputs/configuration; qualify that new identity separately. |
| #12/#14 resource and timing envelope | Heavy engine/effect combinations, sustained playback with MIDI/UI/USB load, observer overhead/clock calibration, supported caps. General RAM has 3,580 bytes spare; pool has 8,376, only 184 beyond its 8,192 reserve. These margins are not stack clearance. | Existing v0.1.8 already exposes D8R1/D8T2. Prepare captures with the existing tools in plan-only mode; later measurements can reuse the installed build without reflashing. Stack watermark is a surviving-write lower bound, not maximum-SP proof. |
| #9/#10/#15 controls, routing and arrangement | Physical mixer/bank gestures, simultaneous routing/recording, arrangement/fill stress and maximum size/timing. A simple demo does not cover these. | Reuse host fixture results for unchanged inputs; prepare targeted scenarios. Do not add optional weighted polyphony or donor features to this release path. |
| #11/#13 own data durability | All three populated slots, nonempty shared autosave, physical local save/recall, settings/presets/samples across a supported own upgrade; interrupted format/save recovery. | Existing virtual-NOR cut tests are retained. Physical power-cut tests need a specific recovery/test procedure; do not perform them incidentally. The successful restore/reboot test does not establish these broader cases. |
| USB audio scope | Installed kit is UAC-off. Both USB audio sample rates remain unqualified in #14. | Keep this explicit. Qualify an enabled candidate or obtain a deliberate release-scope decision before claiming USB audio support; this kit cannot pass those rate tests. |
| #14/#20 recovery and independent reproduction | Stock v15 restoration and sound/controls were owner-confirmed earlier; loader-unavailable recovery and RP2040 setup are not qualified. A second tester still must reproduce clean install, own backup/restore and rollback. | Retain stock firmware privately and existing Mac recovery evidence. Document the actual tested recovery scope. Owning an unwired RP2040 is not a verified recovery path. |
| #19/#20 distribution | Exact corresponding source, notices, checksums, supported compatibility/limits and final authorization. | Reuse the package license/source checks; prepare final records after the integration revision is frozen. No public RC/release tag or download claim yet. |

## Next device measurements

Idle, demo memory/timing and clean-stop checks are complete in the scopes above;
do not repeat them without changed inputs or a new failure. A note-only eight-lane
DRUM workload has now passed the actual host renderer at peak eight voices with
byte-identical project state and no remaining notes. The guarded MIDI helper,
exact routing/track refusal, partial-send cleanup and malformed/timeout controls
pass offline checks; see local `build/measurements-v018-20261015/DRUM-WORKLOAD.md`.
The core and its tests pass type checking with zero errors/warnings. This does
not qualify the private adapter's complete native-I/O lifecycle or device timing.

The initial follow-through found the device disconnected. After explicit owner
reconnection/readiness on 16 October, the guarded drum timing and memory stages
passed on installed v0.1.8. They are complete; do not repeat them. Follow-through
remains paused at the broader configuration and physical calibration gates.
Next qualification needs additional engine/effect combinations and concurrent
transport/UI loads; note-only bursts on this demo cannot cover eight heavy-pool
slots. Retain external elapsed/boot evidence and D8T2/D8R1 for new workloads. Stop
on resets, late events, invalid ownership, storage errors or failed cleanup. Only
then test additional own-save/autosave and routing cases. This document authorizes
no connection, reset, save, erase, upload or flash operation.

The untouched Felucca baseline, external-format migration, optional language ports,
genre cards, weighted extra voices and donor features are not work to repeat or
add merely to make the v0.1 queue longer. Current own-data and hardware gates take
priority. Hosted CI remains disabled.
