# Current handoff — v0.2.4 installed; boot timing gate stopped

## Latest v0.2.4 session — stopped before power-cycle or restoration

The owner authorized the combined batch and connected the FM-1 on19 October.
Private receipts: `build/hardware-v024-20261019-160901/progress.json`.
Both frozen checksum inventories, the canonical original archive and local stock
recovery input passed their hash checks. The current v023 read-only preflight
passed (boot1/USB1, stopped, all tracks unarmed, audio-late0, flash detected).
The unchanged Felucca updater installed the exact pinned v024 package once in
67.68 seconds and reported FM-1_8024.

The first v024 status at14,935ms uptime reported **audio_late2**, so the boot-health
gate failed immediately and the batch stopped. Status also showed stopped
playback, flash1/fl_failure0, main boot2/USB1 and COMMIT stage0 with no transaction
record. This single status does not identify when or why the late events happened,
establish audible failure, or establish an unexpected reboot; main boot2 includes
the intended updater restart. The60-second observation did not pass.

No owner power-cycle, SAVE request, project restore, fixed-RAM capture or retry
followed. The serial context closed without a reported cleanup error and the
companion had already been disconnected. The observer's raw `closed:false` field
is an incomplete success-path flag; the separate cleanup interpretation preserves
that distinction without another device access. Canonical originals remain safe
and unchanged; **device saved contents and original restoration remain unverified**.
Further device access needs owner direction. Preserve both frozen package/kit seals
and the earlier passing evidence. RC remains blocked by timing, unverified corrected
restoration, full stack/ISR qualification and independent failed-application recovery.

## Previous preparation and v023 incident (historical)

Read [RC status](release-candidate-status.md), the [review guide](rc-review-guide.md)
and private `build/hardware-v023-20261019-111007/progress.json` first. The latest
authorized batch installed v023, passed boot/upgrade/cold persistence, then stopped
on a final read-only snapshot timeout. A separately authorized original restore
encountered a watchdog reboot. Current instruction: **no further device access,
restore retry, notes, reset, reboot or flash without fresh owner direction**.
The one authorized post-failure diagnostic capture is complete and its ports are
closed. Original restoration is unverified; the original archive remains safe.

## Corrected package and next owner session

The owner has unplugged the FM-1 and will tell us when hardware is available.
Do not discover devices, open ports or ask for a physical action meanwhile.
The corrected private diagnostic is v0.2.4-timing/FM-1_8024 at
`build/rc-v024-20261019/`, package SHA-256
`adfb2355dd30496b5a28af101e7d301926796a839500d53caceb5f5cf410adb9`.
It reuses the exact reviewed application and unchanged loader through Felucca's
existing packer/installer flow. Neither component was rebuilt for packaging.
The paired `build/rc-hardware-v024-20261019/` is unarmed, with no device execution
mode in its offline batch-plan verifier. Verify both checksum inventories.

Package decoder/CRC/corruption and actual virtual OTA/loader/native-pool checks
pass in optimized/fatal-sanitizer modes. The repinned56-byte diagnostic reader
passes23 fake-console cases. Its COMMIT boot counter is independent of the main
boot counter; the reader compares it with `pt_boot`, not `boots`. Four offline
logical-readback controls cover equal roles, changed pool layout, changed live
music and damaged checksums. The original archive validates through the native
C codec and remains private outside the package and source archive.

Use the [single focused batch](rc-hardware-test-plan.md) only after fresh owner
availability and authorization. It needs one cold power-cycle before the final
restore, then one SAVE confirmation. The canonical originals have no C/autosave;
do not invent a cold-reopen promise for unsaved live music or add a second restore
after reboot. End with all five logical roles verified and ports closed. Stop on
failure; no automatic retry or recovery action. Broad timing/stack/recovery and
arrangement gates remain open. Do not resume the failed v023 batch.

Freeze/review receipts and the latest continuation state are in
`build/rc-package-v024-20261019/`; read its `handoff.json` and `local-review.md`.

## Local correction under test — no device work

The working source now services the watchdog before each admitted COMMIT flash
primitive, while XIP is available and IRQs are masked. It refuses the operation
if the P33 feed reports a timeout; busy RAM waits still never feed. The actual
dispatcher regression changed from zero watchdog calls over116 primitives to116
checked feed boundaries. This closes a demonstrated progress gap, but does not
establish the cause of the physical watchdog reset.

The volatile COMMIT observation record is now56 bytes in diagnostic `.noinit`,
with boot identities, pending result, primitive progress and feed-failure evidence
available through read-only `status`. Startup clears live authority, not the
retained incident. A same-boot export/revocation regression also preserves it.
Early unsampled SPI stalls now publish at their first unsuccessful poll; loader
behavior, flash boundaries, storage formats and consent remain unchanged. See
[retained diagnostics](retained-flash-trace.md).

Current local investigation and negative-control receipts are in
`build/rc-erase-watchdog-20261019/`. That correction produced the application-only
component reused by the private package above; neither is hardware qualified. Preserve both v023 freezes and
their hardware evidence. Reuse the completed engine/effects/emulator matrix;
no FM-1 discovery, read or write has been performed during this correction.

Final focused sanitizer run `build/local-checks/run-urktiia1/` passes the existing
111,539-check own-transfer fixture and new24,443-check watchdog/retention fixture.
The flash-driver/first-use optimized and sanitizer results in
`build/local-checks/run-b277flfj/` remain passing for those unchanged inputs;
that run's later fixture-model failure is preserved, not relabeled as a full pass.
Runner/startup checks pass in `build/local-checks/run-3c0d0t2z/`. Deliberate no-feed,
stale-observer and old-SPI-trace controls fail as expected. The unchanged original
progress probe now passes with116 feeds. Its exact earlier zero-feed failure is
preserved under the advisor-budget evidence.

Final non-installable target: `build/app-only/full/run-mmubc7xz/`, version label
v0.2.4-timing. Application528,840 bytes; RAM94,796/98,304 (3,508 free); pool
335,696/344,064 (8,368 free,176 above reserve). The56-byte COMMIT record is in
`.noinit`, outside both startup-cleared regions; no loader/package was built.
Use its source-hashed manifest and the independent `advisor-budget/fix-review.md`
before any further preparation. The earlier `run-d31oi5a8` application is
superseded by this final local build and must not be substituted.
Independent `advisor-ram/report.md` binds all275 source inputs and94,671 decoded
instruction rows to the final ELF. All574 RAM branches remain inside RAM; the
NOLOAD diagnostic boot updates precede BSS/pool clearing. `.noinit` has700 bytes
of headroom. Retained addresses shift across builds; frozen v023 readers stay
pinned to v023. Status/erase local-frame changes of8/4 bytes are static observations,
not complete stack qualification. The investigation's `summary.json` is the
current machine-readable local completion record.

## Exact current inputs

- Worktree: `/Users/grantv/Code/dabbl8/.local-baseline/felucca-161-integration`,
  branch `dabbl8/felucca-161-bugfixes`. Preserve the original dirty v022 checkout.
- Frozen installed firmware source: `c34263f07c67a8b5bece85abbe041e661b655e38`.
  Collector/readback source: `aeef693b9cafe0ca859ab3243b50fd5225d368f4`.
- Frozen private package: `build/rc-v023-d8a1-20261019/`, v0.2.3-timing/FM-1_8023,
  SHA-256 `c2872ecc0aca96e4f92a8530d0e2682287fd48e40ab0749ecd22ff508720f8cb`.
  It includes D8C1 and D8A1; the earlier v023 freeze does not. Keep both immutable.
- Unarmed kit: `build/rc-hardware-v023-d8a1-20261019/`,282 source/input pins,
  `hardware_ready:false`. Private original and temporary archives stay outside it.
- Last verified device: v0.2.3-timing, boot2/USB1; normal interface and healthy
  stopped audio counters after a watchdog reset. The prior-boot flash trace
  identifies an interrupted0x9D000 sector erase at WIP polling. COMMIT/session
  RAM was cleared by startup. Saved-project state remains unverified.
- Latest capture: `build/hardware-v023-20261019-111007/restore-failure-diagnostics/`.
  Do not overwrite the frozen package or the failed-session receipts. The separate
  session kit `build/hardware-v023-20261019-111007-kit/` is disarmed.
- Review stack: [#200](https://github.com/grant-vine/dabbl8/pull/200),
  [#201](https://github.com/grant-vine/dabbl8/pull/201),
  [#202](https://github.com/grant-vine/dabbl8/pull/202). Do not create duplicates,
  merge them or enable hosted CI. The app's attachment cap does not remove a PR.

## Reuse completed evidence

Reuse84 engine/effects scenarios,42 emulator workflows, source-bound actual
renderer/arrangement tests and prior physical persistence/stock-restore evidence.
The frozen installed181 firmware inputs and effective compile/generator inputs match the checked
527,572-byte target application. RAM94812/98304 and pool335696/344064 are unchanged.
No new build is needed for test/documentation changes alone.

The collector and C-context readback, package decode/corruption checks, virtual
OTA/loader/native-pool preservation,17 fixed-reader cases, batch fault/message
controls and no-access defaults pass. The package/source/kit checksum inventories
are sealed. Do not restart collector implementation, package assembly or these
passing cases merely because an older handoff says they are unfinished.

The later raw-ancillary preservation extension passes optimized/fatal sanitizer
runs plus eight deliberate corruption controls per mode. It covers nonempty
synthetic settings/preset/FM6/sample allocations and all bytes outside approved
app/update-record sectors. It does not prove valid ancillary format reopening or
physical interrupted-write recovery. Its addendum is private
`build/rc-roadmap-audit-20261019/`; no candidate or loader bytes changed.

A subsequent [current-format reopening fixture](ancillary-upgrade-qualification.md)
closes that specific local reader gap: two positive modes and 14 deliberate
invalidation controls pass for current settings, 32 FM6 presets/patches and three
simple sample slots, with actual loader/readers and poisoned caches. Commands
and 100 source/header pins are in `build/rc-ancillary-reopen-20261019/`. Ancillary
writer atomicity, physical cold boot and power-loss qualification remain open;
do not repeat this fixture or rebuild the unchanged diagnostic to reconfirm it.

## Next actions and gates

The local correction review, linked-RAM/retention verification, application
memory checks and private package preparation pass. Reuse the frozen v024 package
and unchanged reviewed loader; do not rebuild them merely to start the next visit.
The incident capture itself did not establish a physical root cause.
The installed COMMIT record was ordinary BSS; its zero-after-reset values were
not evidence of a completed stage. The local correction above retains it, but
has not been installed. Package preparation is authorized; device access is not.
Reuse the successful v023 upgrade, cold persistence, clock and timing observations;
do not restart the [combined batch](rc-hardware-test-plan.md). Its final snapshot,
arrangement phase and original restoration remain incomplete. Any future device
session needs separate owner direction, fresh availability and a concrete reviewed
plan. Never silently retry an expired prompt, restore, reset or reflash.

Full timing still needs reference uncertainty, omitted ISR/arrival work and
supported concurrent target workloads. Stack still needs SP/SSP mode, implicit
frames and nesting evidence; the linked model and watermarks are conditional.
Independent recovery still needs actual RP2040-Matrix confirmation/port/wiring,
application-independent entry and verified recovery. Transporter info/dump are
not passive: they can enter ROM and upload a RAM loader. No recovery action is
armed. A working-console stock restore does not close failed-application recovery.

Live issue audit: #3–#8 and #18 are closed;12 implementation issues remain open.
Respect their prerequisites and unverified acceptance. #16 weighted expansion
and #17 donor work remain later experiments, not additional v0.1 features to
invent while the core qualification gates are open. A second tester and separate
release authorization are also required by #20; #21 needs verified release text.

Earlier append-only entries are preserved in the
[historical handoff](history/local-work-handoff-through-d8a1-preparation-20261019.md).
They are evidence locators, not current instructions to repeat completed work.
