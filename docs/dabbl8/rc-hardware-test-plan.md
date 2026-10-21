# Hardware hold — physical COMMIT watchdog cause unresolved

**Owner decision, 21 October 2026:** defer the deliberate catastrophic-failure
recovery drill, including unbootable-application/RP2040 testing, for the initial
release candidate. It remains unverified and is not an initial RC prerequisite.
Earlier statements requiring that drill are superseded. Retain stock firmware,
existing working-console rollback evidence and upstream recovery guidance.

The actual COMMIT watchdog restart during ordinary project restoration remains
an unresolved defect. Normal boot, save/restore and upgrade integrity, timing/
stack qualification and publication approval remain required. This decision
grants no device access, restore or flash authority; do not request RP2040
availability as a prerequisite for the next ordinary firmware test.

The v025 batch below is historical, not the next operation. Current work is in
[the boundary review](commit-watchdog-boundary-review.md). v027 fixes a local
timer contract defect but is not a proved physical COMMIT repair. No new batch
is armed. Do not discover/access/restore/reset/flash the FM-1 or automatically
repeat this sequence. Preserve installed v025, incident receipts and canonical
originals; original restoration/readback remains unverified. A future device
test needs a concrete causal question and fresh owner direction.

## Historical v025 batch

The owner is unavailable. No device discovery, port access, flash, reset or
restore is authorized. This is a future plan, not a queued operation. Await fresh
owner availability and direction; do not poll the FM-1 or ask for button actions.

## Frozen inputs and preserved state

- Candidate: `build/rc-v025-20261019/dabbl8-0.2.5-timing-diagnostic.fwsc`,
  FM-1_8025/v0.2.5-timing,610106B, SHA-256
  `7a16ea2c4ec9c3e944b84e37a7f27ebefe3501139c6e6b54f17f88680a81fea5`.
- Exact checked app528908B; unchanged reviewed loader6950B, SHA-256
  `d6a2726256681ce39ccc61a4089df885ed32b387d6d26393056b4c69c6d9229a`.
  Use the unchanged `tools/fm1_install.py` Felucca device-requested flow and
  qualified existing Python/CoreMIDI runtime, one exact unique named port, no force.
- Paired `build/rc-hardware-v025-20261019/` verifies inputs only and refuses
  execution flags. Check both seals and create a fresh authorized session record;
  never flip flags in either frozen bundle or resume an old failed session.
- Last observed v024: main boot2/USB1, stopped, lifetime audio_late2 at14935ms,
  flash1/fl_failure0, startup restore1. Normal UI does not verify saved contents.
- Canonical originals: private
  `build/hardware-v023-20261019-111007/preflight-original.d8bak`, SHA-256
  `f30eac2f7c7f1a8010874d9dfcded715005e5bf026717ad271e3fcaa5c872d4a`.
  A/B present, C/shared autosave absent. Do not use a current partial export as
  replacement. If new owner content exists, stop and revise preservation first.

## One future owner visit

Expected physical actions: one normal power-cycle, then one SAVE confirmation.
No temporary project set, notes/workloads, settings/preset edits, extra full
snapshot or repeated broad survey belongs to this focused correction batch.

1. **Read-only preflight after fresh authorization.** Owner connects, MASTER low,
   stopped/unarmed and controls untouched. Verify unique expected endpoints,
   stopped v024, flash detected, paused diagnostics, no new flash/boot/USB error.
   The known lifetime late count2 must not increase during the bounded preflight.
   Do not demand zero lifetime here, clear it, or accept a higher count as known.
   Close all observers/companion ports. Recheck originals and recovery-input hashes.
2. **Install once.** Use the exact package through unchanged Felucca updater.
   Expected two-stage updater transitions belong to that operation. Any unexpected
   timeout/mismatch/disconnect/cleanup failure stops; no force or retry.
3. **New-boot health before SAVE.** Require normal steady UI, stopped playback,
   flash1/fl_failure0, paused diagnostics, full8/UAC-off profile and stable boot/USB
   over60 seconds. Parse the exact `audio_late`, `audio_late_entry`,
   `audio_late_boot` tuple through the bundled strict `boot_audio_status` helper.
   Entry is captured before any IRQ/audio starts; current_boot must be0 and
   lifetime-minus-entry must match it modulo2^32. Lifetime/entry must not change
   between observations. Nonzero inherited history is recorded, never reset or
   used to excuse a new event. New events, missing/inconsistent fields or instability
   stop. The packaged `read-completion.py` performs one fixed RAM/status capture
   with that same tuple gate and new ELF/symbol pins. First-use/reset/storage-error
   UI stops without SAVE; do not format or bypass it.
4. **Cold boot before final restoration.** Close all connections; ask owner to
   unplug USB, switch fully off (screen dark), wait5 seconds, on, then reconnect
   only after normal UI. Require healthy stopped v025 and coherent zero current-boot
   late diagnostics again. COMMIT retention is warm-reset observation, not a
   power-loss journal. Existing content is still unverified; no restoration claim.
5. **Restore canonical originals once.** Connect unchanged private localhost
   companion, select the verified canonical archive, stage one restore. Only when
   prompt is ready and owner present, request SAVE alone for about1 second then
   release once. Prompt limit2 minutes; absence/expiry is cancellation, not retry
   authority. Require acknowledged COMMIT, normal UI and logical readback archive.
   Take one packaged pinned read-only completion capture: stage7/raw_rc0, guards0,
   revoke0, p33_failures0, record_boot==pt_boot, completed==feeds. Count is set-dependent;
   never hardcode116. Preserve any pending/error diagnostic without a new access.
6. **Compare and finish.** Bundled `compare-projects.py` validates original and
   readback through the unchanged native codec and requires all five logical roles
   and presence/absence to match; raw archive/pool hashes need not. Save receipts
   and close connections. No final reboot: canonical originals have no shared
   autosave, so unsaved live content has no cold-reopen promise. Do not add a full
   command78 snapshot to compare the same data a second time.

## Stop condition and limits

First unexpected device, health, storage, communication, deadline or cleanup
failure ends device work. Preserve primary failure/cleanup and partial evidence.
No automatic reconnect, restore, reset, reflash, recovery write or follow-up
capture. Further device activity requires owner direction. Restoration/readback
failure leaves device data unverified even if UI is normal; original archive stays
safe. Do not recycle historical late counts as evidence that a new event is safe.

A pass would verify this candidate's normal boot and corrected original restore
on this FM-1. It cannot close complete timing/ISR/clock or stack ABI/nesting bounds,
interrupted-write durability, independent failed-application recovery, external
loader provenance, all concurrent workloads or second-tester/release approval.
The historical stock restore required a working console. No ROM entry, RAM loader
upload or RP2040 recovery is part of this batch.
