> Historical record only. Do not execute its old next-step instructions.
> Current authority: ../release-candidate-status.md and ../local-work-handoff.md.

# Local work handoff — current resume point

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


Read docs/dabbl8/release-candidate-status.md and
build/rc-durability-v019-20261018/handoff.json first. Original data is restored;
all ports are closed. Do not repeat completed timing, memory or restoration work.

Host fixes are committed in draft PR #199, stacked on #198:
https://github.com/grant-vine/dabbl8/pull/199
Commit cf046f716f019714e93c6ea2b6bb0295c6d16e47 contains only the two host fixes,
their tests and protocol documentation. The installed app remains v0.1.9-timing.
No new firmware was built, flashed, merged or released during host integration.

Reuse build/host-tools-review-20261018/review.json and input-review.json.
The source inventory differs from the frozen bundle only in the declared host
capture helper; all firmware/build/generation inputs remain unchanged. Original
bundle identity must not be relabelled. The host source addendum is separate.
The app could not attach PR #199 because this chat has reached its 100-artifact
limit; the normal GitHub link works. Do not retry attachment or remove other items.

Remaining gates are listed in the current status. No device action is pending;
next physical persistence work needs fresh owner readiness. No flashing or saved
data changes are authorized by this handoff. Actions and automation stay disabled/paused.

## Historical handoffs — context only

Older states below are superseded by the current resume point above.

# Local Work handoff

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


## Current handoff - 18 October 2026

### Authoritative resume point — v0.1.9 batch completed

Read `docs/dabbl8/release-candidate-status.md` and
`build/hardware-batch-v019-20261018/handoff.json` first. v0.1.9 is installed;
boot, seven-body and eight-channel timing/resource passes, all-data preservation,
natural quiet-guard export and final clean close passed. Original projects are
restored. Do not repeat the old DIST workaround, flash, export or restore because
an older checkpoint below calls them pending. Audio/screen owner feedback remains
unanswered; do not invent it. Historical checkpoints below retain their original
context and are not current device instructions.

Next local preparation is complete in `build/rc-durability-v019-20261018/`.
Its proposed archive preserves A/B, fills C/shared AS and uses the eight-body
project as live. Real current-renderer/full-pool and virtual firmware
restore/reboot/rollback checks pass. Adapter fault/grammar checks pass; readiness
stays FALSE. Read its `README.md` and `handoff.json` before any future use.
No additional flash is needed. Actual temporary setup needs owner approval and
availability for two SAVE confirmations and one deliberate power-cycle. Current
original live differs from saved slots and AS is empty, so do not power-cycle
casually under a promise of exact unsaved-state persistence.

### Historical checkpoints — superseded where stated above

Earlier checkpoint: full-pool local preparation passes. Read
`build/full-pool-preparation-20261018/handoff.json` and `README.md`. Proposed live
track 1 WHEEL/FULL ORGAN plus six FM6 notes and DRUM reaches eight voices/eight
bodies in every held burst, with clean release, no allocation/ownership failures,
unchanged proposal bytes and host codec rollback equality. Original/proposed
project-only archives and original import/virtual readback also pass; persisted
roles remain identical. Separate capture adapter passed 6,144 grammar cases and
ten controller fault cases; zero static errors/154 retained warnings. Its actual
hardware_ready remains FALSE and it refuses access before device constructors.
The native eight-track protocol intentionally lacks engine/preset load; do not
bypass it with legacy SET. Owner authorization/availability for setup and return
restore (two physical SAVE confirmations) is pending through the question tool.
No device access, polling, saved-data writes, target build or flash occurred.
Await the answer; do not execute the proposed capture or restore meanwhile.

Earlier acceptance audit: `build/rc-acceptance-audit-20261018/handoff.json` and
`remaining-gates.md`. Issue #11's two documentation items (own-format/version
policy and first-install reset disclosure) now link to the exact published source
`ae8b71f247bfa721ce095b43f0ee175bf6721e6d`. Its live body was compared before the
edit and verified afterward; issue remains OPEN with hardware gates unchecked.
No private device evidence, data operation, test rerun, code change or flash.
Remaining external gates are mapped issue-by-issue. One useful local preparation
still available is a precise eight-heavy-body configuration/workload and rollback
proposal, qualified offline before any permission request. Current sounds reach
seven bodies; hardware setup can autosave and remains prohibited. Do not access
the unit from this handoff or retry the blocked private issue-12 publication.

Earlier checkpoint: draft [#197](https://github.com/grant-vine/dabbl8/pull/197), commit
`ae8b71f247bfa721ce095b43f0ee175bf6721e6d`, adds before/after working-tree input
fingerprints to selective local receipts. Source edits/additions/deletions after
otherwise successful commands now fail instead of producing a passing receipt.
All 38 selector/receipt tests pass; 3,881 source hashes and seven explicitly
missing optional headers matched afterward, including after commit. No renderer
or device checks repeated. Read `build/rc-check-receipts-integration-20261018/`
`handoff.json` and `receipt-integration/REVIEW.md`; current issue bodies were
read in one API request and retained in `receipt-integration/open-acceptance.json`.
All source topics remain draft/unmerged; Actions disabled. Remaining roadmap
gates require actual hardware/control/routing/durability evidence and independent
reproduction, plus separate release authorization. Next audit the current local
acceptance/documentation coverage against those issue bodies; do not turn optional
expanded polyphony/donor research into a first-release prerequisite or repeat the
passing matrix. The original dirty source and all physical evidence are preserved.

Latest source group: draft [#196](https://github.com/grant-vine/dabbl8/pull/196),
commit `a8459733d9476da0b54df2f3a2690c172a42cb1a`, stacked on #195. Read
`build/rc-midi-workload-integration-20261018/handoff.json` and its review.
Fresh-checkout refusal on a missing private demo was reproduced; both actual
MIDI renderer fixtures now capture startup state in memory by default. Eight
channels, eight drum lanes, all 256 REC masks, bounded schedule/refusal/cleanup
checks, optional synthetic-file modes and 36 selector tests pass; core helper
typing reports zero errors/warnings. No device access or saved-data changes.
Actions remains disabled; PR is draft/open at the expected SHA, no checks queued.
The original dirty checkout keeps historical source; the committed topic and
copied logs/patch retain this fix. Next isolate the existing input-hash/stability
change to local CI receipts; reuse passing hardware/matrix evidence.
No retry of the blocked private issue payload, release or RC approval occurred.

Earlier checkpoint: combined local source checks passed and draft source PRs are open:
[#192](https://github.com/grant-vine/dabbl8/pull/192) restore,
[#193](https://github.com/grant-vine/dabbl8/pull/193) Mac MIDI,
[#194](https://github.com/grant-vine/dabbl8/pull/194) voice regression,
[#195](https://github.com/grant-vine/dabbl8/pull/195) engine/effects checks.
Read `build/rc-combined-integration-20261018/handoff.json` and `REVIEW.md`.
Head is `5278eee51bf2b69b29aa2323e70bb3d6e49faf51`; its base `5a41146` assembled
the exact restore patch locally with MIDI/voice topics. No GitHub PR was merged.
All 16 focused combined steps and 34 selector tests pass. The 84 matrix cases,
2,240 mixer cases and performance FX results were reused after matching compiler
dependency closures/flags; they were not rebuilt or rerun. Actions was disabled
before push; all four PRs are draft/open at their expected SHAs, with no hosted
status rollups. App attachments hit the existing 100-item limit; links above work.
No private device reports/backups/vendor binaries were published, no device or
saved-data operation occurred, and the blocked issue #12 payload remains pending.
Next source group is the bounded MIDI workload/schedule and routing/REC fixtures;
preserve current evidence and all outstanding hardware/durability/approval gates.

Latest isolated source group: voice regression commit
`54a9d2b796614076fb107a3e9d4f314db94bb80d`, branch
`dabbl8/voice-reclamation-regression`, stacked on the MIDI topic `6a2efe8`.
Read `build/rc-voice-regression-20261018/handoff.json` and `REVIEW.md`.
The generated demo now asserts exactly two released drum victims per bar/76
total; both controls yield zero, with no held-note victims. The exact fixture
strengthening is also in this original checkout. Focused allocator/overload
checks and 31 selector tests pass. The optional observer preprocesses away
identically; no allocator policy or target build flag changed. No device access,
target build/flash or public GitHub mutation. Next check the local combined source
with restore plus MIDI/voice changes, then extract engine/FX tests while reusing
their unchanged source-bound matrix. All release/hardware gates remain open.

Latest local integration: Mac MIDI guard/snapshot group is isolated in commit
`6a2efe8f42129c129c85c79ccd757751f8342b6d`, branch
`dabbl8/native-midi-qualification` (eleven files, parent `33c3b62`). Read
`build/rc-native-midi-integration-20261018/handoff.json` and `REVIEW.md`.
The environment-dependent CLI refusal test was reproduced and fixed; the exact
three-line fix is also applied to the original dirty checkout. Selected offline
checks pass: 15,363 inventory assertions, 17-role C snapshot/deadline/release cases
and 30 selector tests. The original dependency refuses before native import;
the repaired files qualify without import or port construction. No FM-1 access,
target build/flash, source push/PR/merge or independent review occurred. Keep the
existing source-bound physical evidence; no repeat is requested. Next isolated
group is the voice-allocation observer/regression fixture and its selective recipe.

Source integration follow-through: the already installed restore-confirmation
fix is isolated in local commit `0ac04f1f6886ba2048ce55613b52b225f4f371a8`, branch
`dabbl8/restore-confirmation-integration`, five files only. Read
`build/rc-source-integration-20261018/handoff.json` and `REVIEW.md`. Selected
restore/client/sample checks passed with source hashes verified unchanged through
commit; no target build or device access. The corresponding-source audit found
only the disabled voice test observer and host MIDI guard different from the
263 frozen recorded inputs; all seven generated headers match. Original dirty
checkout and frozen package are preserved. No push/PR/merge or independent
approval. Next separate group is the repaired native MIDI guard/snapshot work;
do not bundle the engine/effects test suite or private device data into that fix.

Earlier checkpoint: seven-body physical timing/resource stages passed. Read
`build/headless-seven-20261018/RESULTS.md` and `handoff.json` first. Six FM6 notes,
DRUM and ANALOG used peak seven bodies with no allocation failures; 6,292 timing
blocks, zero late/taint/reset; maxima render 51,168/inclusive 54,619 ticks,
uncalibrated. All 17 roles remained unchanged; note-on/off pairs matched, resource
stop acknowledged and contexts closed. No saved-data changes or flash. Reuse
this completed workload; no physical owner action or test retry is pending.
The last-census count is not a stop-time voice count; see the capture reference.
Public issue-update approval remains pending after automatic review rejection.
Do not retry that publication without approval of its exact retained draft.

The following earlier same-day records are retained for provenance.

Corrected headless timing and resource tests passed on installed v0.1.8-timing.
Resume from `build/headless-resource-20261018/handoff.json` and `RESULTS.md`.
Each stage sent 128 bounded note pairs on the existing eight-channel workspace;
timing measured 6,303 blocks with zero late publications/taint. Memory observed
two heavy slots, zero allocation failures, user/supervisor surviving stack
writes of 4,320/1,188 bytes. All 17 current/persisted roles match the original
17 October snapshot exactly; ports closed and resource stop was acknowledged.
No saved-data changes, reset or flash. Reuse these passing stages.

The first combined run's resource stage was refused locally before its start
command because Console retained the timing-only allowlist. That host wiring
was fixed and tested through the real command path; only the unfinished resource
stage was rerun. Failed receipts remain preserved. A proposed full-pool FM6 host
test observed the explicit six-voice per-track cap and did not reach eight slots;
its unqualified draft was never run on hardware. No device failure occurred.
RC still requires broader physical engine/FX, full-pool and deadline qualification,
write-dependent durability, integration/review and independent reproduction.
No device test is pending or armed. Do not use the draft FM6 capture helper.

Next local preparation is now grounded in a passing seven-body renderer proof:
`build/headless-fm6-20261018/LOCAL-RESULTS.md`, `seven_slot_test.c` and
`seven-host.stdout`. Six FM6 notes plus DRUM and ANALOG exercise eight voices/
seven heavy bodies without changing the captured workspace. The per-track FM6
cap is six, so the current sound configuration cannot fill eight bodies. See
[engine resource limits](engine-resource-limits.md) for all source caps and
evidence limits. Before physical use, qualify a fresh exact-schedule helper
and its cleanup; do not reuse the original eight-FM6 draft or change presets.

## Prior handoff - 17 October 2026

Latest headless attempt: `build/headless-workspace-20261017/RESULTS.md`.
Owner authorized autonomous testing of the connected unit. The actual current
workspace passed the local eight-channel renderer and physical read-only track
preflight, and all 17 roles matched the prior snapshot exactly. The run stopped
before sending notes: its host guard mistook the snapshot's expected READ timestamp
update for an unexpected operation. Boot/USB/audio/failure counters stayed stable.
The private guard correction and regressions pass locally; no hardware retry was
made. Preserve the failed receipts/executed source and keep device work stopped
until owner direction. No saved-data changes or firmware operation were requested.

The Mac MIDI guard is fixed at the shared backend constructor and CLI. The exact
repaired dependency has passed bounded local lifecycle/software-loopback checks;
see [native MIDI qualification and prepared snapshot](native-midi-lifecycle.md).
Use only the separate repaired environment. The old unpatched environment remains
blocked. No FM-1 access occurred during this local repair/qualification session.

The owner-authorized single read-only snapshot has now passed all 17 roles,
release acknowledgement and port closure. Evidence and local comparison:
`build/native-midi-qualification-20261017/fm1-snapshot-once/RESULTS.md`.
Saved B still exactly matches EIGHT DEMO and A matches prior verified persistence;
the current blank-name workspace differs. Track 4 has level 104/POLY rather than
demo level 72/MONO. No load, save, notes or flash was requested. Further workload
testing needs a separately authorized project load or a validated workload for
the current workspace. Do not access the device again from this handoff alone.
`tools/dabbl8_readonly_snapshot.py` still defaults to plan-only.
Do not use the stale eight-channel helper or its source pins. Its 16 October
read-only attempt stopped on two track-4 parameter differences, then the old
snapshot CLI refused before native import. Those failure records are preserved.
The new snapshot deliberately does not require demo equality: identify the current
workspace without changing it. No note playback, save/load, reset or flashing.

## Prior local evidence - 16 October 2026

Next: [read-only preflight for eight-channel qualification](rc-eight-channel-followthrough.md).
Independent frozen-bundle review APPROVE is saved in
`build/rc-followthrough-20261016/independent-package-review.md`; it excludes later
helper changes. The staged Python import failure was fixed; pure helpers/tests
type-check clean. `build/local-checks/run-gutjktqg/results.json` passed actual
256-mask recording query, 128-note-pair eight-channel renderer, matching project
bytes/cleanup and 37 selector tests. Do not repeat these or the 84-case matrix.
The old prepared helper and its offline evidence are retained as historical
inputs, not current hardware readiness. The 17 October handoff supersedes that
helper's proposed next step. Do not use the older DRUM adapter with the updated
core: its MIDI allowlist lacks the new command-27 REC checks.

Private qualification package is now frozen: `build/rc-freeze-20261016/`, archive
`dabbl8-v0.1.8-qualification.tar.gz`, SHA-256
`207fee0173dab0b4385616764deaaae9cad228c81c2a7fd77e24d31028e415aa`.
It reuses exact installed v0.1.8 bytes and original corresponding source; the
later validation source is a separate overlay, not a newly compiled application.
Read `SEAL.json`, bundle `README.md`/`verify.mjs`, and
[safe hardware test plan](rc-hardware-test-plan.md). Standalone and relocated
verification passed (263 target inputs/seven generated headers); six expected
damaged/incomplete bundle refusals passed. Bundle/archive are read-only. Do not
modify or overwrite them to accommodate later work; create a new identified
freeze if required. No device was accessed, firmware flashed or saved data
changed. No public RC/release approval or independent final review is claimed.
The existing package checker correctly refuses today's test-observer source;
use the frozen verifier against the frozen source instead of weakening it.
No FM-1 connection is requested now. New engine/effects setup can autosave;
current permission excludes such configuration changes and durability writes.

Local engine/effects follow-up completed: reuse the 84-scenario matrix at
`build/local-checks/run-5rtbcdfx/results.json` (187 relevant inputs matched).
New bounded banked performance checks passed in `run-ywfe1bqv`; optimized
performance FX and 36 selector tests passed in `run-wb58234s` under the same
`build/local-checks/` directory. Scoped self-review/evidence reuse is recorded in
`build/rc-local-review-20261016/REVIEW.md` and `report.json`. No source-bound
failure, hardware access, saved-device-data change, package build or flash
occurred in this follow-up. New CI recipes/tests/docs remain uncommitted.
The FM-1 is unplugged. Do not poll it or repeat completed checks. Prepare safe
physical workload and durability procedures before a device request; heavier
physical timing/stack, own-data recovery, exact integration/package review and
independent tester gates remain open. Hosted CI and follow-through remain off.

Start with [RC readiness](release-candidate-status.md), not the historical M0
prompt below. Firmware worktree:
`/Users/grantv/Code/dabbl8/.local-baseline/erase-trace-flicker-20261014/source`,
branch `dabbl8/render-cost-fix`, HEAD
`33c3b62b40ba1708ccfb6bfb14fb0191bf55bcea` with local changes. Preserve them.
PR #191 is open against `dabbl8/resource-diagnostic-package`.

Installed private package is v0.1.8, built from an exact dirty source snapshot;
do not substitute HEAD for its source identity. Restore, playback and saved A/B
reboot retention passed in bounded device checks. All 76 voice relinquishments
were reproduced locally as released drum-tail reclamation, with no production
allocator change. See the readiness record for original evidence and limits.

Keep checks selective/local. New run receipts bind working-tree inputs and
refuse source changes observed during execution. Reuse original evidence only
for its recorded scope; old receipts are not retroactively hash-bound. Do not
repeat the upstream baseline or external-data migration. No further flashing
is authorized by the current task. Resource/timing stress, broader own data and
interruption recovery, routing and independent tester gates remain open.

Website work belongs in `/Users/grantv/Code/dabbl`; this is firmware work only.

## Historical M0 startup prompt

This prompt is retained as history; M0 is complete. Do not restart it:

> Work on https://github.com/grant-vine/dabbl8. Clone or open a clean checkout and fetch dabbl8/develop. Read AGENTS.md if present, DABBL8.md, docs/dabbl8/README.md, issue-index.md, desktop-first-build.md and [baseline build issue #3](https://github.com/grant-vine/dabbl8/issues/3). Start with issue #3 only; the roadmap is issue #2. Confirm my actual macOS, workspace, Docker and toolchain. Review the pinned v1.1.5 versus main v1.1.5.1 difference before changing the baseline. Set up isolated dependencies, record toolchain/SDK/container versions and hashes, compile unmodified firmware and run required upstream tests. Retain map, logs, hashes and explicit skips/failures. Do not install firmware, change loader or flash boundaries, modify golden hashes, publish a release or deploy the website in this first session. Report evidence and update the issue when the build gate is satisfied; propose the next small implementation PR.

The website repository is grant-vine/dabbl; it is not the firmware checkout. Never embed private user backups or credentials in source, issues or logs. Issue completion requires evidence, not just a successful command exit. A blocked environment should leave the build issue open with a concrete blocker and next action.
