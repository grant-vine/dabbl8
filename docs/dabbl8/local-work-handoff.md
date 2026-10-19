# Current handoff — guarded clock preparation, no hardware access

Latest firmware source `95ac93f` on `dabbl8/felucca-161-bugfixes`. Read current
[RC status](release-candidate-status.md), [clock method](clock-measurement.md),
[independent recovery preparation](independent-recovery-preparation.md) and private
`build/rc-clock-20261019/handoff.json` first. Local command/collector, resource
interop, selector, target budget and conditional linked-stack checks pass.
Last verified installed firmware remains v022 with originals restored; no FM-1
discovery, ports, notes, saved-data changes, upload, reset or flash this round.

The original frozen v023 package remains immutable and predates D8C1. The new
clock-capable target component is application-only, not an installer. Do not
flash the old package to collect D8C1. A future consolidated freeze must repin
the package, completion reader and unarmed kit after resolving measurement
scope. Full ISR/arrival timing, stack ABI and independent physical recovery
remain gates. The product listing identifies RP2040-Matrix, but physical board
confirmation/port/wiring and recovery are not verified. Do not run transporter
`info`/`dump` as passive discovery: they can enter ROM and upload a RAM loader.

Reuse the 84 engine/effects scenarios and earlier physical persistence/stock
recovery evidence. Check owner availability before requesting any physical action.
No new hardware session is armed. Current instruction remains **do not flash**.

---

# Earlier handoff — local RC qualification and v023 preparation

Read [RC status](release-candidate-status.md),
[local follow-through](rc-local-qualification-20261019.md), and private
`build/rc-local-qualification-20261019/handoff.json` first. Firmware source remains
`e251aa4`; no firmware was changed in this follow-through. The private v023 package
is assembled from the exact checked app/loader. Its source/manifest freeze and
local review live in `build/rc-v023-20261019/` and the qualification addendum.

Last verified device is v022, originals restored, all ports closed. No new device
access occurred. Current direction is **do not flash yet**. The single future
[hardware plan](rc-hardware-test-plan.md) and kit are unarmed; check availability
before any physical request. Reuse passing evidence. Full clock/observer timing,
SP/SSP/implicit frame/nesting proof and independent unresponsive recovery remain
open; the prepared instrumented workload does not close those gates by itself.

The current 1,041-function source model accounts for main callback families and
bounded cycles (startup/main 7,512 bytes; audio 2,196; TIMER5 200), while retaining
its ABI limits. The 6,300 v022 timing samples and historical stock recovery inputs
are reused. New package negatives/native-pool preservation, fixed-layout reader,
batch refusal/fault grammar and exact current-source heavy schedule pass locally.
Do not repeat the 84 engine/effects cases, 42 emulator workflows or v022 persistence
and stock recovery trials without changed effective inputs.

---

# Prior development integration — selective Felucca 1.6.1 backports

Current development branch is `dabbl8/felucca-161-bugfixes` in
`/Users/grantv/Code/dabbl8/.local-baseline/felucca-161-integration`.
Read [the integration record](felucca-161-backports.md) and private
`build/upstream-161/` evidence. Baseline commit34c3b21 preserves the complete
current v022 source changes; new commits contain only the selective backports.
No physical device actions are authorized for this update round. Installed
v022 and its successful hardware evidence below remain unchanged and do not
qualify the modified source. The original checkout and backups are intact.

---

# Current handoff — v022 combined hardware batch complete, 19 October 2026

Installed v0.2.2-timing/FM-1_8022 using Felucca's unchanged flow and the pinned
reviewed package. Original project set is restored and exact companion readback
verified; final read-only COMMIT stage7/rc0/guards0/revoke0. All ports closed.
No further physical action or device retry is armed. Reuse completed evidence.

Warm60-second health, exact five-role upgrade readback, confirmed cold restart/
autosave reopening and exact cold readback pass. Two 18-second heavy MIDI windows
pass,6,300 timing blocks/no late/taint/aux, eight owned slots/no allocation failure,
all17 data roles unchanged. First cold guard refusal remains preserved; separately
authorized retry established a healthy cold epoch. Fresh original backup remains
private. Read `build/hardware-v022-20261019/handoff.json`, `completed-results.json`
and `docs/dabbl8/release-candidate-status.md` first. Calibrated timing/observer,
full stack bounds and independent recovery still block public RC qualification.
Next: local analysis of those specific gaps; no repetition of passing tests.

---

## Historical handoff (superseded by current record above)

Owner availability: AWAY (latest instruction). Finish local work only. Check
availability and wait for a reply before any physical request; do not flash,
connect, stage a restore or send a SAVE prompt while the owner is away.

# Latest handoff — corrected v022 package frozen, 19 October 2026

Current boundary: LOCAL ONLY. No FM-1 access, flash, notes or saved-data changes.
Device remains v021 with original projects verified restored and ports closed.

Frozen private candidate: `build/rc-v022-20261019-r2/`, productFM-1_8022, package SHA-256
`b3e137b9d4e918d32f6933e0bc92139e96e6d347df1570a3cacce8834cc238ff`.
Exact checked app and unchanged reviewed v021 loader reused; no new compile,
SDK/toolchain download or broad test rerun.62frozen files/3,889source files,
package corruption/decode checks, all checksums and17reader cases pass.
Final independent package review is APPROVE/no blockers; its addendum is outside the freeze in
`build/rc-package-v022r2-20261019/`; read its approval before any future device work.

The single future batch is `docs/dabbl8/rc-hardware-test-plan.md`, also copied as
frozenHARDWARE-BATCH.md. Execution kit `build/rc-hardware-v022r2-20261019` defaults
to plan-only/readinessfalse; offline10fault/6,144grammar checks pass. It keeps
existing routing/recording/project and17-role before/after snapshot guards.
TwoSAVE confirmations and onecoldpowercycle cover temporarysetup, own-upgrade,
automaticreopen, heavy timing/resource checks and exactoriginalrestoration.
Stopfirstfailure; do not automatically retry or restore after a stopped batch.
No actual test, prompt, installer or session is armed. Calibratedtiming/fullstack/
independentrecovery gates remain open. Reuse all unchanged passing evidence.

---

# Local work handoff — current resume point

**STOPPED physical batch:** v0.2.1-timing installed successfully; first status
reported audio_late1. The batch initially stopped; separately authorized read-only follow-up and
original restoration are now complete. No cold reboot was performed.
Authorized follow-up: all three saved projects/autosave exact, but live state
does not match saved autosave. Late remains1; boot/USB/flash health unchanged.
After separately authorized SAVE, originals are **restored and companion
readback verified**, with temporary C/autosave removed. See
`build/upgrade-v021-20261019/original-restore-result.json`. Read
`build/upgrade-v021-20261019/handoff.json` and late-counter-local-analysis.json
first. All ports closed; no automatic retry or flash. Prior v020 completion below
is historical and must not be mistaken for the current device state.

Read [release-candidate-status.md](release-candidate-status.md) for current facts
and remaining gates. Historical entries have been moved out of this handoff so
old installation/restore instructions cannot be mistaken for pending work.

## Current local work — startup correction, no device access

Latest owner direction is to fix late audio and automatic reopening locally,
add regressions, reuse passing evidence and continue toward RC. Do not access
or flash the FM-1. Installed firmware remains **v0.2.1-timing**, with original
projects restored and host ports closed.

Read `build/rc-startup-fix-20261019/audio/receipt.json` and `boot/receipt.json`.
The actual HAL/main phase fixture reproduces DMA starting before global IRQs;
split configuration/start removes that blackout in all 32 modeled phases for
four/eight tracks. USB attachment and UART MIDI acceptance now follow read-only
autosave adoption. A queued-packet fixture fails with the old shipping call
order and passes the corrected order without weakening input/storage guards.
The exact archived 6,599-byte upgraded autosave restores and recaptures identically;
13,347 boot checks pass with zero NOR writes. New result/policy/admission/time
fields make future refusal diagnosable. Physical attribution remains unproven;
these host results do not qualify FM-1 timing, stack or reopening.

**Target-cost blocker resolved locally:** final app-only link is 526,528 B,
RAM 94,760 B (3,544 B free), pool 335,688 B (8,376 B free). Target checks pass;
`rev_room` cost is 239 under the unchanged budget/tolerance. The initial cost-244
failure remains in `target-app/`. Controlled variants isolated new boot state
changing target global merging/register allocation. A target-only BSS subsection
for those globals retains all diagnostics and restores the prior hot-code score
without changing DSP source, budgets or golden hashes. See
`build/rc-startup-fix-20261019/budget/receipt.json` and
`target-app-final/result.json`. Final affected host checks pass at
`build/local-checks/run-v1hl5sr3/results.json`. Independent local startup code
review is **APPROVE**, with no local blockers; see
`.omo/evidence/rc-startup-fix-code-review.md`. Package freezing remains separate,
and old package approval cannot qualify this changed firmware. Exact-candidate
physical timing/stack/reopening remain open.

The 84 matrix scenarios are reused, not rerun: current effective preprocessed
input equals the retained passing input byte-for-byte. See
`build/rc-startup-fix-20261019/matrix-reuse-final.json`; the proof was refreshed
after the final target-only attributes. Existing audio/output-queue,
TIMER5 cadence/gating and emulator translation-unit checks have separate affected
receipts. No new installable package, public RC pass or device operation is implied.

## Prior v020 authorization and device state

Latest owner direction authorized the combined persistence batch and original
restoration, without flashing. **The batch is complete; no device action is pending.**
Read `build/persistence-v020-20261018/handoff.json` and completed-results.json.
Physical local SAVE C, recall A→C, and cold reboot with all three slots/nonempty
shared autosave passed. Final independent logical readback matches all five roles
of original99bd: original A/B/current restored, C/autosave absent. COMMIT trace
stage7/rc0/guards0/revoke0, boot1/USB1 stable after cold boot. All ports closed.
The prior extra command78 snapshot remains failed; its physical cause is unknown.
Do not repeat successful restores or add raw snapshots for reassurance.

## Prior v020 local review outcome

Final independent local package review **APPROVE**, no local blockers. Read
`build/final-package-review-20261018/handoff.json` first. The private reviewed kit
is `build/rc-review-kit-20261018/` and retains the exact installed diagnostic plus
a separate host addendum. Its root checksums bind the final review/scope files.
No firmware build, flash, hardware access or repeat of passing workloads occurred.
The84 matrix cases remain valid by equal effective preprocessed inputs.

The all-populated persistence batch is closed. Own-upgrade durability, calibrated
target performance/stack bounds and independent clean install/recovery remain
external gates. Prepare only uncovered checks locally; a future flash needs
separate owner authorization. No additional button press is needed now.

## Resume work without a device

- Source-only entry/nesting follow-through: `build/rc-owner-ready-20261019/stack-entry-followthrough.json`.
  TIMER5/audio nesting is explicit; retain hardware-frame/main callback gaps.
  Use the existing combined upgrade plan; no repeat stock restore or matrix.
  Device access/flash still needs separate approval and current readiness.


- Latest qualification work: `build/rc-qualification-20261019/qualification-summary.json`
  and [qualification-evidence-20261019.md](qualification-evidence-20261019.md).
  Historical stock restore reconstructed/readback verified; exact missing loader
  input recovered privately, cache-only runtime repaired,9 guarded tests pass.
  No device access/loader execution. Static IRQ frame coverage and retained timing
  audit are bounded results; full timing/stack and fresh physical recovery are not
  qualified. Do not repeat the historical restore or84 matrix for reassurance.

- Prior loader-cleanup work: `build/rc-upgrade-fix-20261019/handoff.json` and
  `build/rc-v021-20261019/`. Narrow owner-authorized cleanup exclusion is applied;
  exact pre-fix/fixed virtual-NOR regression and Felucca negative controls prove
  preservation. Source/test and local package review APPROVE (package-approval.json).
  That v021 diagnostic is now installed; originals are restored and no device
  action is pending. Current startup changes are a separate uninstalled successor.
  Do not repeat the 84 matrix or existing v020 persistence batch.
  Read [rc-remaining-qualification.md](rc-remaining-qualification.md) for remaining
  upgrade, timing/stack and independent reproduction gates.
- Read `build/snapshot-timeout-local-20261018/handoff.json` and its check receipts.
  The local correction preserves the timeout/request/scan coordinates and timings,
  separates cleanup errors, and reports host closure independently from acquisition.
  It adds no retry and changes no firmware. The physical missing-reply cause is
  unresolved; no speculative timeout/firmware changes are justified.
- Reuse the 84 engine/effects cases and source-bound transfer/client evidence.
  Do not rerun full suites, upstream baseline, completed restore, or package build
  without changed inputs or an uncovered gate.
- Local source/package review and scope records are complete for this artifact.
  Keep the frozen v020 bundle immutable; later host fixes are a separate addendum.
  Target durability/performance/independent reproduction remain external gates.
- Before requesting hardware, identify the exact uncovered requirement and prepare
  one bounded batch with rollback/cleanup. Obtain current owner readiness; this
  handoff grants no hardware authorization.

## Evidence and private data

- Prior v020 restore success: `build/restore-v020-20261018/companion-result.json` and
  `postcommit-diagnostic/completion.json`.
- Failed optional snapshot: `build/restore-v020-20261018/final-snapshot/`; never
  promote its unverified files to an accepted snapshot.
- Original backup: `build/persistence-v019-20261018/original-set/verified/project-set.d8bak`,
  SHA-25699bd36dcae4ad784c4fbded37d7fd2789c4892410bb88ae0e80f69aaedbec399.
- Installed exact frozen package/source: `build/rc-v021-20261019/`
  (v020 under `build/rc-v020-20261018/` is historical).
- Local-only checks and history remain under `build/`; no hosted CI or public release.
  Never commit private backups, vendor firmware or credentials; preserve GPL and notices.

[Archived earlier handoffs](history/local-work-handoff-through-20261018.md) are
historical evidence only, never a queue of actions to repeat.
