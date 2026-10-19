# One combined v023 hardware batch — prepared, unarmed

Current instruction: **local work only; do not flash yet**. No discovery, port,
MIDI, restore prompt, installer or device retry is armed. Last verified firmware
is v0.2.2-timing with originals restored. Check owner availability before requesting
physical action; then obtain authorization for the complete concrete batch.

This replaces the completed v022 plan. Do not repeat its demo, local SAVE/recall,
stock restoration, broad engine survey or earlier warm/cold trials for reassurance.
Changed firmware still requires its own targeted hardware acceptance.

## Local prerequisites

- Frozen private candidate `build/rc-v023-20261019/`, product FM-1_8023,
  package SHA-256 `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83`.
  Verify SHA256SUMS, source/archive/loader pins and the external local review
  before execution. Version strings are not independent binary attestation.
- Execution kit `build/rc-hardware-v023-20261019/` is plan-only with
  `hardware_ready:false`. All 275 effective source/kit inputs are pinned, including
  the new UI header. Use a new session copy; never carry readiness or authorization
  forward or modify the frozen package. The existing helper and note schedule
  remain unchanged apart from identity/input pins.
- Existing temporary A/B/C/shared-autosave set has workspace SHA-256
  `678dba08d7f40c4a5e8c8223d703511466ef0aeb4e4d80987e406ef2b8f375c3`.
  Current-source actual rendering proves its eight-voice/eight-slot schedule and
  bounded cleanup. Take one fresh original logical backup at authorized preflight;
  never substitute an older archive if the owner has made edits.
- Resolve the measurement method before treating this as the final RC session:
  independent clock/audio cadence and omitted ISR work, and SP/SSP/exception/nesting
  evidence. The existing D8T2/D8R1 helpers supply useful instrumented comparisons,
  not a complete timing/stack certificate. No arbitrary GPIO wiring or new board
  pin mapping is approved by this plan.

## One owner session

Two SAVE confirmations and one confirmed normal power-cycle, with automated work
between them. Announce SAVE only after the prompt is ready and the owner is present;
it expires after two minutes. Never silently retry an expired request.

1. **Preflight and original backup.** After fresh authorization/readiness, check
   unique expected endpoints, v022/profile, stopped/unarmed transport/recording,
   flash health, routing and counters. Back up the current logical projects once.
   Compare the temporary archive against that fresh backup: preserve the owner's
   A/B and other data. If they differ from the prepared originals, stop and update
   the temporary set offline. Keep the stock recovery artifacts available locally;
   no recovery write or deliberate fault injection is part of preflight.
2. **SAVE #1: temporary project set.** Stage through the existing companion,
   obtain one physical confirmation, require COMMIT completion and exact logical
   readback before advancing. A timeout is uncertain, not success.
3. **Upgrade once through Felucca's flow.** Close observers/ports, install only
   the exact reviewed package after separate flash authorization, then close the
   installer. Check steady boot/USB/flash health for 60 seconds, no new late event,
   successful read-only autosave reopening and exact A/B/C/autosave/current bytes.
   No factory reset or first-use bypass. New loader behavior only rejects a
   wrong-sized board image; existing native-pool cleanup exclusion is preserved.
4. **One cold restart.** After all ports close, unplug USB, switch fully off,
   confirm dark screen, wait five seconds, switch on, reconnect after normal UI.
   Require a fresh healthy epoch, automatic reopen and exact saved/current bytes.
   Do not compare retained counters across epochs or clear counters to pass.
5. **Targeted automated measurements.** With MASTER low and no recording or
   settings/preset edits, use the unchanged schedule: six FM6 voices on channel2,
   DRUM channel4, WHEEL channel1. Two 18-second timing/resource passes, one 17-role
   full snapshot before and one after, exact routing/live-project guards, matching
   note-off cleanup and confirmed no holds. Capture stack observations and counters
   in the same session. Use the new fixed completion reader only for v023; its
   trace address is 0x01C1D250, validated against the exact ELF, not the old v022
   address. Independent clock/observer/ABI measurements must be recorded separately
   with uncertainty; absent evidence stays unqualified, regardless of clean counters.
6. **SAVE #2: originals.** Restore the fresh original archive once through the
   companion, obtain physical confirmation and require its exact logical readback.
   Record stage7/rc0/guards0/revoke0, unchanged health and closed ports. Do not add
   another full snapshot or reboot after verified restoration.

## Independent recovery and failure handling

The demonstrated stock-v15 Mac restore depended on a working console to enter
ROM mode. Reuse that evidence; do not erase a working unit merely to repeat it.
Broader unresponsive-application recovery requires a separately reviewed physical
entry method. RP2040 ownership is not proof of compatible board pin mapping,
wiring, readback or restore. Qualify those with the exact board and primary
transporter instructions; do not assume XIAO labels apply to the Waveshare board.
Group any separately authorized hands-on preparation into this owner visit where
practical, but keep recovery writes and deliberate power interruption outside
the normal qualification batch. A read-only identification/backup alone does not
prove restore from an unresponsive application.

Stop on the first device/communication/storage/deadline/taint/allocation or uncertain
cleanup failure. No automatic retry, reset, reconnect, reflash or restore. Retain
the primary error and any cleanup error. If temporary data remains installed,
report it and obtain direction before attempting restoration. A clean batch ends
with exact originals restored; a stopped batch may not.

This batch covers the changed candidate's update/reopen and bounded workload.
It does not automatically qualify every preset/effect combination, UAC, all
ancillary saved data, interruption recovery, full stack or calibrated deadlines.
Do not call it RC-complete while a stated release gate remains open.

Historical reference: [completed v022 plan](history/rc-hardware-test-plan-v022-completed-20261019.md).
