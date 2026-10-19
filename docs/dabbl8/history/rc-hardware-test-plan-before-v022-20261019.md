# RC hardware test plan — current scope, 19 October 2026

Current installed candidate: private v0.2.0-timing/FM-1_8020. Current authorization
is **local work only; no FM-1 access or flash**. This plan grants no permission.
Felucca's existing loader and installer remain the installation path.

The upgrade-preservation defect now has a narrow locally tested/reviewed fix.
Prepared successor: private v0.2.1-timing/FM-1_8021 in
`build/rc-v021-20261019/`; it is not installed. Read
[the remaining qualification plan](rc-remaining-qualification.md) first. A future
combined upgrade session needs separate installation authorization and current
owner readiness. No device action is queued. Timing preparation records omitted
interrupt costs; source/test approval is not hardware deadline qualification.

## Completed: do not repeat without a new cause

The current installation,60-second boot check, steady screen, original-project
restore, companion logical readback and retained COMMIT completion all passed.
Earlier engine/effects, heavy timing/memory and bounded MIDI results retain their
recorded source/configuration scope. Read release-candidate-status.md for evidence.
Do not add a full raw capture after a verified project restore just for reassurance.
Where a workload explicitly claims all17 raw/current roles unchanged, preserve
that test's original comparison requirement; do not weaken it to hide a failure.

## Remaining uncovered owner work — prepare before requesting it

The combined batch in `build/persistence-v020-20261018/` is complete: physical
local SAVE C, A→C recall, all three populated slots/nonempty shared autosave cold
reboot, and exact final original restoration passed. Do not repeat them.
Supported own-upgrade persistence and any broader settings/preset/sample claims
remain separate uncovered scopes. No new session or flash is authorized here.

Target performance qualification additionally needs agreed clock/observer bounds,
concurrent effects/transport/UI/routing coverage and complete stack bounds. Prepare
only uncovered workloads against the actual local renderer first. Explicitly state
which earlier results apply to the exact target source/configuration.

## Device safety and stopping rules

- Confirm current owner readiness, expected version/profile, unique endpoints,
  stopped/unarmed state and exact project/routing needed by the proposed test.
- No settings/preset/project edits under a no-write approval: autosave can persist
  them. No automatic retry, reconnect, reset, panic or reflash after a failure.
- Require stable boot/USB counters and healthy flash/timing state. Stop on the
  first transport, storage, deadline, unexpected packet or uncertain cleanup.
- Note workloads track potentially sent note-ons before send and attempt only
  matching bounded note-off cleanup. Stop only observers started by this session.
- Use existing restore protocol and physical SAVE; never bypass owner confirmation.
  Logical restore readback is the relevant project-restore completion evidence.
- Preserve raw errors, request/scan coordinates/timestamps and primary versus
  cleanup outcomes. Incomplete raw snapshots remain refused. Do not continue
  queries after a failure without owner direction.
- Do not deliberately interrupt storage without a separately approved recoverable
  procedure. Stock-v15 USB recovery was exercised; RP2040 recovery remains unqualified.

The optional snapshot's missing physical reply remains unresolved. Its host
reporting fix does not authorize retry or imply that the USB/storage cause is fixed.
[Historical plan](history/rc-hardware-test-plan-through-20261018.md) is evidence,
not current instructions.

## Prepared persistence inputs — no device action pending

Local package review matched the existing temporary archive to the current
verified original archive. The proposed set preserves original A/B byte-for-byte,
adds C/shared autosave, and has identical current/C/autosave content. Reuse it;
do not generate a replacement or rerun completed timing/memory workloads.

- Original private archive: `build/persistence-v019-20261018/original-set/verified/project-set.d8bak`,
  SHA-256 `99bd36dcae4ad784c4fbded37d7fd2789c4892410bb88ae0e80f69aaedbec399`.
- Temporary private archive: `build/rc-durability-v019-20261018/proposed-set/verified/project-set.d8bak`,
  SHA-256 `c779cdb662f7c00a4aed6617d13b64a67feb00eed7a346d04598f6bff75ea941`.
- Offline input comparison: `build/final-package-review-20261018/next-batch-input-review.json`.

The prepared fixtures have now completed their intended batch, including physical
local save/recall. Final original restoration and closed ports are recorded in
`build/persistence-v020-20261018/handoff.json`. Keep these inputs as evidence;
no setup, SAVE, reboot, restore or flash is queued. Own-upgrade durability remains
unqualified; do not infer it from normal reboot or project restore.
