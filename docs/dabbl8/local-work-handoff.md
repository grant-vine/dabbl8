# Current handoff — consolidated diagnostic ready for review

Read [RC status](release-candidate-status.md), the [review guide](rc-review-guide.md)
and private `build/rc-consolidated-v023-20261019/handoff.json` first. They identify
one prepared, unarmed owner batch. Current instruction: **do not flash or access
the FM-1**. No device discovery, port, notes, write, restore or restart is pending.

## Exact current inputs

- Worktree: `/Users/grantv/Code/dabbl8/.local-baseline/felucca-161-integration`,
  branch `dabbl8/felucca-161-bugfixes`. Preserve the original dirty v022 checkout.
- Latest firmware source: `c34263f07c67a8b5bece85abbe041e661b655e38`.
  Collector/readback source: `aeef693b9cafe0ca859ab3243b50fd5225d368f4`.
- Frozen private package: `build/rc-v023-d8a1-20261019/`, v0.2.3-timing/FM-1_8023,
  SHA-256 `c2872ecc0aca96e4f92a8530d0e2682287fd48e40ab0749ecd22ff508720f8cb`.
  It includes D8C1 and D8A1; the earlier v023 freeze does not. Keep both immutable.
- Unarmed kit: `build/rc-hardware-v023-d8a1-20261019/`,282 source/input pins,
  `hardware_ready:false`. Private original and temporary archives stay outside it.
- Last verified device: v0.2.2-timing, originals exactly restored, COMMIT
  stage7/rc0/guards0/revoke0 and ports closed. This is not a current observation.
- Review stack: [#200](https://github.com/grant-vine/dabbl8/pull/200),
  [#201](https://github.com/grant-vine/dabbl8/pull/201),
  [#202](https://github.com/grant-vine/dabbl8/pull/202). Do not create duplicates,
  merge them or enable hosted CI. The app's attachment cap does not remove a PR.

## Reuse completed evidence

Reuse84 engine/effects scenarios,42 emulator workflows, source-bound actual
renderer/arrangement tests and prior physical persistence/stock-restore evidence.
All181 firmware inputs and effective compile/generator inputs match the checked
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

Only prepare a new owner session after fresh availability, authorization and
readiness. Follow the [single combined batch](rc-hardware-test-plan.md): original
backup, temporary setup/SAVE, exact reviewed upgrade, one cold restart, clock
samples, guarded MIDI passes, C-load/SEQ-hold/PLAY, natural Stop/readback, then
original restore/SAVE. Stop on any device or uncertain-cleanup failure. Never
silently retry an expired prompt, restore, reset or reflash.

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
