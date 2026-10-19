# Combined v0.2.2 diagnostic hardware batch — prepared, not authorized

Current device: v0.2.1-timing/FM-1_8021, original A/B/current restored and
verified; temporary C/autosave absent. Owner is currently away: first check availability and wait for an affirmative
reply before requesting any physical action. Current instruction permits local package
work only. Do not enumerate, open or flash the FM-1 now. The successor is private
v0.2.2-timing/FM-1_8022, full engine mask0x3ffd, eight tracks/eight shared voices,
USB audio off, CDC on. Use Felucca's unchanged installer flow and the byte-identical
reviewed v021 loader. No first-use bypass or factory reset is part of this batch.

This is one future owner session, with **two SAVE confirmations and one normal
power-cycle**, only if each preceding phase passes. Book the owner for these
three short actions; issue a SAVE request only when the owner is present. The
physical restore prompt expires after two minutes. Do not queue a prompt while
waiting for the owner, or silently retry an expired request. A timeout/cancel is
not evidence that a restore completed.

## Inputs and reuse

- Candidate: `build/rc-v022-20261019-r2/`; check SHA256SUMS, frozen manifest and
  external package approval before any device action. A directory or version
  string alone does not attest the running image.
- Retain prior successful originals at
  `build/upgrade-v021-20261019/preflight-original.d8bak` (SHA-256
  `9955977bd65298adef17a96f5ca0f4fe576c1e6bdc2ef5ef32f7d4f4ac5621ce`).
  Take one fresh logical backup in the future authorized preflight; retain it as
  the restoration authority if the owner has since made edits. Never overwrite it.
- Reuse the validated temporary set at
  `build/rc-durability-v019-20261018/proposed-set/verified/project-set.d8bak`
  (SHA-256 `c779cdb662f7c00a4aed6617d13b64a67feb00eed7a346d04598f6bff75ea941`).
  It preserves original A/B and populates C/shared autosave/current identically.
  Compare against the fresh original before staging. If A/B changed, stop and
  prepare an updated temporary set offline; do not overwrite new work.
- Reuse the 84 engine/effects scenarios, startup red/green, exact 6,599-byte
  autosave replay, voice-allocation, save/recall and historical recovery evidence.
  Do not repeat the broad matrix, local save/recall, stock rollback or raw full
  snapshots simply for reassurance. Earlier physical timing describes its exact
  older versions, not this candidate.

## Ordered batch and gates

1. **Automated preflight, future authorization required.** Verify unique expected
   endpoints, v021 identity/profile, stopped/unarmed transport and recording,
   flash health, routing and quiet readiness. Read boot/USB/late/failure counters
   and take the fresh logical backup once. The last recorded late counter is1;
   record the actual baseline without clearing it. A changed/unexplained baseline
   stops the batch for analysis. Validate recovery artifacts and package pins
   locally, with no recovery writes or deliberate fault injection.
2. **Owner SAVE #1: temporary setup.** Stage the already validated archive through
   the existing companion. Announce the prompt only once it is ready and the owner
   is present. Require companion COMMIT completion and exact logical readback of
   A/B/C/autosave/current before advancing. No further queries after a failure.
3. **Automated upgrade through Felucca's flow.** Close all observers and ports,
   install the exact frozen v022 package once, then close the installer. Check
   identity, flash health and boot/USB stability over a 60-second stopped window.
   Require no new late event relative to preflight. The warm retained value1 is
   historical evidence, not a new failure or permission to reset it. Require
   `startup_restore_result 1`, `startup_restore_policy 1`, guards0 and a plausible
   recorded completion time. Independently compare all saved projects/autosave
   to the prepared logical bytes, and current to saved autosave. Do not compare
   current against unrelated pre-upgrade live edits. Stop on any mismatch.
4. **Owner action #2: one normal cold power-cycle.** After host connections are
   closed, unplug USB, switch off, wait five seconds, switch on, and reconnect USB
   only after the normal interface appears. Capture one stopped status window and
   logical backup. Require successful automatic reopen and exact A/B/C/autosave/
   current matching the saved set; no unexpected restart, flash fault or new late
   event. A power loss may clear retained RAM; preserve warm and cold baselines
   separately and do not subtract across reset epochs. The pinned completion
   reader requires late0 and is usable only in the healthy cold epoch, not the
   historical warm late1 epoch.
5. **One bounded, automated workload window.** Only if all prior gates pass:
   reuse the previously qualified eight-voice heavy-state MIDI schedule on this
   verified heavy project/routing (six FM6 voices on channel2, DRUM on channel4,
   WHEEL on channel1), with low volume and no recording/settings/
   preset changes. Capture aligned timing/resource windows using the existing
   tested helpers, exact candidate ELF/symbols and immutable workload contract.
   Track possibly sent note-ons before sending; issue only their bounded matching
   note-off cleanup. Require zero new late/reset/flash/ownership/allocation faults,
   valid unsaturated/untainted observations and confirmed note cleanup. Close all
   observers and connections before the original restoration. Do not repeat the old demo
   playback or a full preset survey. Retain the original workload contract’s
   two full 17-role snapshots before/after: these are required to substantiate its
   no-data-change claim, not extra restore-completion checks. Stop on any missing
   snapshot reply; do not weaken the comparison or retry. This is comparative instrumented evidence;
   it does not establish timer calibration or full stack/interrupt bounds.
6. **Owner SAVE #2: restore originals.** Stage the fresh verified original archive
   once, obtain the physical SAVE confirmation and require companion exact logical
   readback. This restores original A/B/current and removes temporary C/autosave
   when those were originally absent. Record COMMIT stage7/rc0/guards0/revoke0,
   unchanged health counters and closed host ports. Do not add another raw snapshot
   or power-cycle after verified restoration just for reassurance.

## Failure handling and limits

Stop on the first device, communication, storage, deadline or cleanup failure.
Do not retry, reset, reconnect, power-cycle, reflash, weaken a guard or start a
restore automatically. Preserve primary/cleanup errors and exact phase. If a
failure occurs while temporary data is installed, retain both backups and request
owner direction for a separate recovery/restoration action. Restoration after a
failed test is not automatically safe. A normal batch ends with originals restored;
a stopped batch must report honestly if restoration remains pending.

This session can cover the corrected startup, own-project upgrade/reboot
persistence and a bounded heavier workload together. It does **not** qualify every
engine/preset/effect combination, saved samples/presets/settings, USB audio,
calibrated full-interrupt deadlines, complete worst-case stack bounds, deliberate
power-loss recovery or loader-unavailable/RP2040 recovery. Existing stock-v15 USB
recovery evidence remains scoped; owning an RP2040 is not independent recovery
verification. These release gates cannot be converted to passes by this plan.

Historical plan: [previous v021 plan](../history/rc-hardware-test-plan-before-v022-20261019.md).
No device action is armed, scheduled or authorized by this document.

## Prepared execution kit — offline checks only

`build/rc-hardware-v022r2-20261019/eight_capture.py` defaults to plan-only, with
`hardware_ready:false`; capture modes refuse before opening ports. It retains
two 18-second timing/resource passes, the eight-body/256-event schedule, exact
project/routing checks, all17-role pre/post snapshots and bounded matching note
cleanup. Its version and package/source pins now target v022. Use it only after
the cold epoch with late0: it deliberately refuses the warm retained late1.
The kit’s offline adapter test covers10 controller faults, exact MIDI bytes and
6,144 message-grammar cases without opening native ports. Logs and readiness
refusal proof are in `build/rc-package-v022r2-20261019/`.

After future authorization, make a new private session copy of this kit under
`build/`, preserve its relative layout, verify its original hashes, and record
owner readiness and exact setup readback before changing only the copy’s readiness
flag. Keep the frozen package and preparation originals unchanged. Do not carry
forward a prior readiness or authorization record. No session copy is armed now.

The corrected host collector attempts each timing stop at most once. Lost or
malformed stop acknowledgements are retained as uncertain cleanup and stop the
batch. Revision1 is preserved but rejected for its host-helper stop retry.
