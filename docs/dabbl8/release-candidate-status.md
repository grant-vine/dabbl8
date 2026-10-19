# Dabbl8 release candidate status — 19 October 2026

## Current hardware batch — v022 complete; originals restored

The authorized v022 combined batch is complete. Exact upgrade readback and the
60-second warm window passed. After a first cold-epoch guard refusal, the owner
separately authorized and confirmed a full off/on cycle: boot1/USB1/late0,
startup reopen result1/policy1/guards0 in1,330ms, and exact five-role readback.
The two18-second heavy MIDI passes passed:6,300 timing blocks, zero late
publications/taint/aux observations, eight owned heavy slots and zero allocation
failures. All17 before/after data sections match. Observed surviving stack writes
were5,472/24,320 user bytes and1,188/7,936 supervisor bytes; these are lower-bound
observations, not worst-case stack proofs.

The owner confirmed final SAVE. Companion verified exact readback of original
projects/autosave/current; final read-only COMMIT is stage7/rc0/guards0/revoke0,
with healthy late0/boot1/USB1. All host ports are closed. Temporary C/autosave
are removed according to the original archive. No more device action is armed.
Evidence: `build/hardware-v022-20261019/completed-results.json` and
`build/hardware-v022-workload-20261019/hardware-run/`.
Calibrated timing/observer/DMA, complete stack bounds and independent recovery
remain release gates. This scoped success is not a public RC qualification.

## Corrected installation package frozen — local package review APPROVE

Use **freeze revision2**. The first freeze is retained but rejected: its host
timing collector could repeat an unacknowledged stop command. The corrected
helper records stop-attempt state before sending, sends at most one stop, and
records uncertain cleanup while preserving the primary error. Three regressions
fail on the old helper; all23timing and24adjacent resource tests pass with the fix.
The firmware application/loader/package bytes are unchanged; revision2 updates
only host helper/source and its pins. Never use the rejected preparation.


Private **v0.2.2-timing / FM-1_8022** is assembled and frozen at
`build/rc-v022-20261019-r2/`:62 files with checksums and3,889 corresponding-source
files. Package610,106 B, SHA-256
`b3e137b9d4e918d32f6933e0bc92139e96e6d347df1570a3cacce8834cc238ff`.
It reuses the exact passing526,528-byte application and unchanged reviewed
6,950-byte v021 loader; neither was rebuilt during packaging. SDK/compiled
source inputs, decoded application/loader, boundaries/padding, corrupted/truncated
controls, reader17fake cases and every frozen checksum pass. The84 engine/FX
scenarios remain reused through exact effective inputs. Independent final package
review is **APPROVE**, with no local blockers, recorded externally under `build/rc-package-v022r2-20261019/`; do not
modify the frozen artifact to add an approval.

[One combined hardware batch](rc-hardware-test-plan.md) is prepared, with two
SAVE confirmations and one normal power-cycle, only after new owner authorization.
It groups upgrade/reopen/readback, cold persistence and the existing two18-second
heavy timing/resource windows before restoring originals. Its re-pinned adapter
passes10controller-fault controls and6,144grammar cases locally; default mode is
plan-only and hardware readiness remains false. Warm retained late1 is recorded
without clearing it; strictlate0 workloads run only after a healthy cold epoch.
The17-role before/after snapshots remain required by that workload’s unchanged
no-data-change contract; no extra full snapshot is added after a verified restore.

Owner is away. Do not issue a physical-action prompt or start a device session.
First check availability and wait for an affirmative reply; separate batch
authorization and fresh readiness are still required. No test is armed.

No device was accessed or flashed. Installed firmware remains v021, with original
projects restored. Local freeze/review does not qualify physical startup/reopen,
calibrated deadlines/observer costs, complete stack bounds or independent recovery.

## Local startup fixes — target checks and independent code review passed

Two startup defects now have local red/green reproductions. Audio DMA previously
ran while global interrupts were disabled; configuration and DMA start are now
separate, with DMA starting after IRQ enable. The actual HAL/main startup fixture
passes all 32 modeled phases in both four/eight-track configurations. USB attachment
and UART MIDI acceptance now follow read-only autosave adoption. Pending MIDI is
still refused and retained: the old shipping call order fails reopening when a
packet is queued, while the corrected order adopts the expected project with
zero NOR writes. No late counter was reset or suppressed.

The exact 6,599-byte autosave retained from the failed v021 upgrade decodes,
restores and recaptures byte-for-byte in the actual C path under idle admission;
the expanded boot fixture passes 13,347 checks. Preference/unclean-boot policy,
late MIDI/OTA/transport input, held controls, busy workspace, damaged storage,
I/O failure and read-only write denial remain covered. Four startup diagnostic
fields retain the result, policy, admission snapshots and completion time.
These reproductions establish defects in startup sequencing, but do not prove
which condition caused the observed physical late event or missed reopening.

Evidence: `build/rc-startup-fix-20261019/audio/receipt.json` and `boot/receipt.json`.
The 84 existing engine/effects scenarios are reused without rendering again:
`matrix-reuse-final.json` proves the effective preprocessed input remains byte-identical
(SHA-256 `125c5ec383a30830777af9605433cb8ed7ff9a24ac62c94b0d4098331bf9d9e7`).
Audio/overload/output-queue, TIMER5 gating/cadence and emulator translation-unit
checks are separate affected evidence; they do not establish target deadlines.

The final app-only successor links at 526,528 B; RAM 94,760/98,304 and
pool 335,688/344,064 leave 3,544 B and 8,376 B free respectively. Target static
checks now pass, with `rev_room` restored to cost 239 under the unchanged
budget 218 and existing 10% allowance. The initial cost-244 failure remains in
`target-app/`; controlled target variants isolated new boot diagnostics affecting
whole-module global merging and hot-code register allocation. Keeping the new
boot globals in a target-only BSS subsection preserves diagnostics and restores
the prior score. DSP source, budgets and golden hashes were unchanged.

See `build/rc-startup-fix-20261019/budget/receipt.json` and
`target-app-final/result.json` with its static-budget output. The final affected
host run passes at `build/local-checks/run-v1hl5sr3/results.json`; the 84-case
reuse proof was refreshed after the target-only attributes in
`matrix-reuse-final.json`. That check covered an app-only build; its exact bytes are now packaged above,
without any new release qualification. Independent local code review is **APPROVE**,
with no local blockers (`.omo/evidence/rc-startup-fix-code-review.md`). The package review and exact-candidate physical timing/stack/reopening checks
retain their separate evidence and authorization gates. Current physical state remains v021 with the original project set
restored; no FM-1 access, flash or saved-data change occurred.

## Latest physical batch — stopped after v0.2.1 installation

Owner authorized the combined upgrade batch. The fresh original backup matched
all five logical roles; temporary setup was physically confirmed and exact
readback passed, with COMMIT stage7/rc0/guards0/revoke0. Felucca installation
completed with exit0 and identity FM-1_8021. The first post-upgrade status
reported audio_late1, so the batch stopped before further queries, reboot or
original restoration. Current installed version is **v0.2.1-timing**. A separately owner-authorized read-only follow-up found the late counter still1
with unchanged boot/USB/flash health. Its logical backup proves all three saved
projects and shared autosave exact; current live state does not match saved
autosave, so automatic reopen acceptance remains failed. After separate owner authorization and physical SAVE, original restoration
completed with companion exact readback verification. Original A/B/current are
restored and temporary C/autosave removed. See `original-restore-result.json`. All host
ports are closed; no retry, reset, restore or flash is queued.

The late counter is retained across warm boots at the same .noinit address in
v020/v021. One observation cannot attribute the event to update staging, handover
or new boot, or establish an audible fault. Keep the failure; do not clear or
ignore the counter to pass. Evidence: `build/upgrade-v021-20261019/handoff.json`
and `late-counter-local-analysis.json`. Further device work needs owner direction.


## Prior v0.2.0 state and scope

Previously installed private diagnostic: **v0.2.0-timing**, product **FM-1_8020**. This is not
a public release or a release-qualified build. Eight tracks share eight sounding
voices; engine mask0x3ffd, USB audio off, CDC on. Felucca's loader and installer
flow are retained. No new flash or device access is authorized by this document.

The original project restore **succeeded**: the owner confirmed SAVE and the normal
interface; the companion read back and verified all saved projects, shared
autosave and current project. The retained COMMIT trace is stage7/rc0/guards0/
revocation0, with boot3 and USB reset1 unchanged. Reuse this evidence; do not
repeat restore or add another full-memory capture merely to verify this result.

An optional independent command78 snapshot subsequently lost one reply during
its second consistency scan of sample allocation2. It remains an unsuccessful
snapshot, not a failed restore. Device work stopped. The missing reply's physical
cause remains unknown; no firmware fix or hardware retry is implied by the local
host error-reporting correction. Full raw snapshots remain required where a
specific test claims all17 persisted/runtime roles unchanged; they are not an
extra project-restore acceptance gate.

## Combined persistence batch completed

On 18 October, the owner-authorized v020 batch passed physical local SAVE to C,
A→C recall, and cold reboot with A/B/C plus nonempty shared autosave preserved
byte-for-byte. The live project after reboot exactly matched saved autosave.
The final original restoration passed companion readback and an independent
logical backup comparison: original A/B/current exact, C/autosave absent.
COMMIT stage7/rc0/guards0/revoke0; boot1/USB1 stable after the cold boot; ports
closed. Evidence: `build/persistence-v020-20261018/completed-results.json` and
handoff.json. No flash occurred and no further device operation is pending.
One final confirmation expired before SAVE; the owner authorized a fresh request.
A comparison initially refused the expected selected-slot change1→3; offline
byte/CRC analysis proved all musical data unchanged. Neither was a device failure.
These results do not establish own-upgrade durability or resolve older failures.

## Evidence to reuse

| Scope | Evidence and limits |
| --- | --- |
| Prior v020 install, steady screen, 60-second stopped boot check | `build/hardware-v020-20261018/handoff.json`; no unexpected reset, flash failure or late-audio event in this window |
| Successful original restore and logical readback | `build/restore-v020-20261018/companion-result.json` and `postcommit-diagnostic/completion.json`; no independent full raw snapshot or reboot-durability claim |
| Frozen exact package/source/notices | `build/rc-v020-20261018/`; package SHA-25630ce914f59916f7bc23d7c31eef1c6022e3eb7f58b6ef5722625f344f3a5f037; keep immutable |
| Local transfer and client checks | `build/rc-v020-20261018/validation/reuse.json`:111,539 actual-C and312 browser/C/WASM checks,585 relevant hashes compared |
| Engine/effects matrix |84 existing scenarios; retain their recorded source/configuration binding, not a claim of current target timing |
| Older physical heavy timing/memory | `build/rc-durability-v019-20261018/completed-results.json`:6,317 blocks, no recorded late/taint/reset, eight heavy slots/no ownership or allocation failure; uncalibrated ticks and lower-bound stack watermarks only |
| Older own-data persistence | `build/hardware-trial-v018/reboot-persistence-result.json`:A/B plus absent C/autosave survived a reboot; not all-populated durability |
| Installed v021 static memory | RAM94,740/98,304:3,564 free. Pool335,688/344,064:8,376 free,184 above reserve. Stack allocations24,320/7,936; worst-case bounds unqualified |
| Snapshot timeout/local correction | `build/snapshot-timeout-local-20261018/`; exact failed request/context, primary/cleanup errors separate; actual-C simulated loss and no-write checks. Does not establish physical cause |

See [the RC review guide](rc-review-guide.md) for scope, provenance, installation
and recovery limits. The final local review artifacts are under
`build/final-package-review-20261018/`; public qualification stays separate.

## Prior v020 package review complete

Independent local review: **APPROVE**, no local package blockers. The self-contained
private review kit is `build/rc-review-kit-20261018/`; its index and SHA256SUMS bind
the unchanged installed image, exact corresponding source, notices, host-only
addendum, current scope/guide and independent verdict. No new build or device
operation was performed. This closes local package preparation/review for this
diagnostic; it does not approve public release or a different future binary.

The84 engine/effects cases were reused from run-qibkv0eu after comparing exact
effective preprocessed inputs; they were not rerun. See
`build/final-package-review-20261018/matrix-reuse.json` and gate-review.md.
The prepared persistence fixtures were used successfully in the combined batch.
Original data is restored; do not repeat that batch without changed inputs.

## Additional qualification evidence — 19 October

[Timing, stack and recovery audit](qualification-evidence-20261019.md) completes
bounded offline checks without device access. It inspects1,033 linked functions,
accounts for all40 engine callbacks on the interrupt paths, reparses39,529 prior
physical timing samples and reconstructs the saved stock-v15 readback exactly.
Nine repaired-runtime recovery guard cases pass. The scoped historical stock
recovery evidence is independently checkable; no repeat restore is needed merely
to reconfirm it. Exact-candidate timing/observer/DMA, full stack/ABI/nesting and
unresponsive-device/RP2040 recovery remain unqualified. No public RC pass implied.

## Remaining release gates — dependency order

**Upgrade defect fixed locally:** the narrow cleanup exclusion protects the
existing E5000–E7000 native pool block. The exact final regression fails against
retained old loader source and passes with the fix (all five blocks and newest
autosave); the checksum-broken control and existing Felucca loader tests pass.
Independent source/test review: APPROVE. No application, staging, calibration or
flash boundaries changed. Private **v0.2.1-timing / FM-1_8021** is built and
validated offline under `build/rc-v021-20261019/`; it is now installed, as recorded
in the physical batch above, but is not RC qualified. App526,056 B, RAM94,740 B, pool335,688 B unchanged; loader6,950 B.
Package SHA-256 `c76b95afd830089f58884a238dddc2a3a4c1309e2781b38053a78353f264b1ce`.
Independent **local package review APPROVE**:128 frozen files,3,887 source files,
267 recorded inputs, exact loader decode, package negatives and17 fake-console
cases verified. Approval is an external addendum at
`build/rc-upgrade-fix-20261019/package-approval.json`; the freeze remains unchanged.
Source inputs/generated headers bind reuse of the84 matrix cases. See
[remaining qualification](rc-remaining-qualification.md) for scoped evidence,
timing accounting and one future upgrade batch. No device action is queued.

1. **Final public revision/scope.** The local package is reviewed. Keep its exact
   binary/source and later host addendum distinct; any future changed firmware
   needs affected evidence and review. Select the public version/scope and
   distribution authorization only after remaining gates. No new build is needed
   for host-only fixes. The startup successor changes firmware and needs its own
   affected review; earlier timings do not qualify that binary.
2. **Own-data durability.** All three populated slots/nonempty shared autosave
   reboot and physical local save/recall now pass. The v020→v021 upgrade preserved
   all saved projects/shared autosave exactly; live automatic reopening failed.
   Its startup correction still needs hardware verification. Broader
   settings/preset/sample durability remains unqualified.
   Retain virtual-NOR torn-write tests; deliberate interruption needs a recoverable
   procedure. No migration from other firmware is a release gate.
3. **Target performance and controls.** Clock/observer calibration, supported
   heavy engine/effects/transport/UI/routing envelope and complete stack bounds.
   Reuse completed workloads; only measure uncovered combinations.
4. **Release scope and independent reproduction.** State USB-audio-off limits
   explicitly; an enabled build needs separate qualification. Independent clean
   install/own restore/stock rollback remains required. Owning an RP2040 does not
   qualify loader-unavailable recovery.
5. **Distribution.** Final compatibility/limits, corresponding source, dependency
   notices and checksums, then separate publication authorization. No RC tag or
   public download yet.

No FM-1 action is needed for local host verification or evidence consolidation.
See [the current hardware plan](rc-hardware-test-plan.md) before any future owner
session. Older records are preserved in
[history](history/release-candidate-status-through-20261018.md); their old actions
and authorization must not be resumed.
