# Current handoff — v028 diagnostic prepared; physical COMMIT cause open

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

Source publication is reconciled against draft PRs [#205](https://github.com/grant-vine/dabbl8/pull/205),
[#206](https://github.com/grant-vine/dabbl8/pull/206),
[#207](https://github.com/grant-vine/dabbl8/pull/207),
[#208](https://github.com/grant-vine/dabbl8/pull/208),
[#210](https://github.com/grant-vine/dabbl8/pull/210) and
[#211](https://github.com/grant-vine/dabbl8/pull/211). All 28 changed firmware,
test and host-tool files match existing PR blobs; the three CI Python files
match the combined published changes after adopting the missing runner checks.
The local runner passes 45 tests and all three selected steps. All 275
application inputs and seven generated inputs still match frozen v028.
Evidence: `build/rc-source-reconciliation-20261020/`. These are separate source
stacks, not a merged GitHub branch or a complete target build of each PR. Do not
create duplicate implementation PRs or repeat broad matrices from the dirty list.

Latest local follow-through: private v028 package `build/rc-v028-20261020/`, 610,106 B, SHA-256 `7e1df414687601d96650697c7b9aaaab042a0099ccbe88c27ccebe42be5cd549`. Format-2 retained diagnostics preserve TIMER4 CON/PRD at operation entry across warm reboot and boot reads. The exact checked application is reused with the unchanged Felucca loader. All 30 offline package steps and 38 fake-reader cases pass; the review found and fixed contradictory format-row acceptance. The fixed reader defaults to offline checks and captures only two additional 8 B clock spans when separately authorized. Coherent invalid-clock data is inspectable, not a health pass. Package handoff: `build/rc-package-v028-20261020/handoff.json`; seal receipt: `build/rc-package-v028-20261020/freeze.json`. No device batch is armed.

Trace 76 B, NOINIT margin 692 B; app 529,108 B, RAM text 12,248 B with no calls; globals/pool unchanged. All 25 selected steps pass, with the trace compile/run pairs refreshed after comment cleanup; the separate PR baseline passes 82 assertions per mode. Source draft [PR #208](https://github.com/grant-vine/dabbl8/pull/208) is distinct from the full dirty integration image. Evidence/review: `build/rc-timer-context-20261020/`. P33 comparison changes exactly two bytes for readback versus csync, but no hardware ordering contract proves a cause or fix; production P33 is unchanged. Installed v025, unresolved COMMIT/original readback and the no-device-access instruction remain in force. Prior sealed packages are untouched.

Vendor assessment: [official FM1 Open and GitHub reconciliation](https://github.com/grant-vine/dabbl8/pull/209). Nine issue bodies updated and 11 completed items checked; seven original tasks remain closed. All 12 unchanged vendor V14 APP units plus identity link with the cached Linux toolchain; the unchanged image validator passes. APP image 453,736 B; linker heap 247,372 B before runtime allocations. Exact Windows toolchain and hardware qualification remain open. Evidence: `build/fm1-open-link-20261020/`. No base switch or code import: preserving current Dabbl8 capabilities needs a deliberate port/licence review. The P33 barrier difference is a hypothesis, not a physical watchdog repair. No FM-1 access occurred.

Read [the three-advisor boundary review](commit-watchdog-boundary-review.md)
first. Local v027 fixes a proved inherited TIMER4 divider/period defect; it does
not establish the cause or repair of the v025 watchdog reset. Package
`build/rc-v027-20261020/`, SHA-256
`6b76115ced3c3d452657bd4fe779f56cf781fb6c3469da358380aced2482e757`.
Stable27-step local receipt, two new rejected timer mutations, exact RAM audit,
30 package commands and28 fake-reader cases pass. Prior broad evidence is reused.
Latest Felucca and BSP were separately fetched/pinned; no baseline upgrade or
newer verified COMMIT fix. Handoff: `build/rc-commit-investigation-20261020/handoff.json`.
Installed state remains v025 and original readback unverified. Do not discover,
access, restore, reset or flash the FM-1. No hardware batch is armed. Earlier
v026 findings and sealed package below remain valid historical evidence.

Latest local work: [SPI status investigation](spi-status-watchdog-investigation.md)
and `build/rc-spi-watchdog-20261020/`. v026 corrects a reproduced stale SPI-wait
trace and strengthens delayed-completion/PCLR tests. It does not establish a
physical watchdog repair. Private package: `build/rc-v026-20261020/`, SHA-256
`5f9fbfebbf41e0b70f03df91755acf2fb77e6fbbde9984cbf08f1307228bd4e9`.
Flash/first-use21 local steps, two rejected mutations, new target static/RAM
audit,30 package commands and28 fake-reader cases pass. Prior84 engine/effects,
42 emulator and host-overlay checks are reused. No hosted CI or FM-1 access.
Do not access, restore or flash the device now. Installed state below remains v025.

The owner authorized the focused v025 hardware batch on20 October2026. One
unchanged Felucca install succeeded, followed by60.04s warm and10.03s cold boot
checks with zero lifetime/entry/current-boot late counters and stable boot/USB
identities. Warm main boot2 became cold main boot1, establishing the cold cycle.
Do not repeat these passed checks or reflash.

Original restoration is now stopped after failure. Two confirmation requests
expired without SAVE/COMMIT, were closed normally and restaged only on fresh
owner instruction. On request3 the owner pressed SAVE, reported a reboot and
normal UI; the companion disconnected with `MIDI port disconnected or closed.`
The owner then authorized one read-only status/fixed RAM capture. That single
attempt succeeded with clean port closure, using only two status requests and
five fixed memr reads. Neither COMMIT completion nor original readback is
verified. Do not reconnect, retry restore, reset, send notes, change saved data
or flash. No further device access is authorized by the completed capture.
Latest session: `build/hardware-v025-20261020-074222/progress.json`.

The capture confirms main boot1→2, retained import boot1/current boot2, P3 reset
reason0x04 (watchdog), unfinished STORE at0x9D800,9 completed primitives and10
feed receipts. Guards/revokes/P33 failures are zero. The retained RAM driver
record is INFLIGHT PROGRAM, SPI transmit wait11 within WIP phase10, byte0x05,
MMIO0x29, no deliberate failstop. Both status observations retain zero
lifetime/entry/current-boot audio-late counters. Root cause remains open: the
last recorded wait is not an atomic final-instruction trace. The completed-
status reproduction proves this label can outlive its actual SPI wait.
Evidence: `build/hardware-v025-20261020-074222/restore-failure-capture-1/`.

Read [boot timing evidence](boot-timing-qualification.md), [RC status](release-candidate-status.md)
and the session above first; `build/rc-package-v025-20261019/handoff.json` is the
historical pre-install package handoff. Preserve the dirty
`dabbl8/felucca-161-bugfixes` checkout, both earlier freezes and all private backups.
The current worktree is `.local-baseline/felucca-161-integration` in the firmware
repository; no website/hosted CI/release/deployment work belongs to this task.

Local host follow-up: `build/rc-host-audio-epoch-20261019/handoff.json` records
the reviewed collector overlay. General timing/resource/clock/arrangement status
now requires the strict boot tuple for expected v0.2.5+, and package readers accept
the frozen private manifest format.67 host tests, seven clock groups and the
affected real resource-console fixture pass. Firmware, private package and unarmed
kit are unchanged; the overlay is separate from their frozen source snapshot.
Do not rerun those checks or apply it inside a sealed package. No device work is
authorized without fresh permission after this failure. The measured boot gate
has advanced; full timing/stack/recovery gates remain open.

## Prior installed diagnostic and evidence

`build/rc-v025-20261019/dabbl8-0.2.5-timing-diagnostic.fwsc`, FM-1_8025,
610106 bytes, SHA-256 `7a16ea2c4ec9c3e944b84e37a7f27ebefe3501139c6e6b54f17f88680a81fea5`.
The exact checked application is528908B; the reviewed6950B loader and Felucca
packer/installer are unchanged. Paired kit: `build/rc-hardware-v025-20261019/`.
Neither package nor plan grants hardware authority. Verify their seals before use.

Audio DMA now starts after saved-project adoption, with input scanning active
and external MIDI blocked until adoption completes. Lifetime late history is
preserved; coherent entry/current-boot counters distinguish old updater events.
New checks refuse any current-boot event or counter change. Physical attribution
of the two v024 events remains unverified. Exact-candidate warm/cold observations
now pass; this is not full target timing qualification.

## Reuse and remaining work

Focused actual-code startup/IRQ/adoption, counter coherence/wrap/parser and target
memory/static checks pass. The unchanged84 engine/effects cases,42 emulator
workflows, COMMIT/watchdog regressions, byte-identical RAM flash code/loader and
logical comparison codec evidence are reused. Do not rerun those broad matrices,
rebuild this package or use a full snapshot to compare the same five project roles.

The [focused batch](rc-hardware-test-plan.md) has reached original restoration
and stopped on the owner-reported reboot/disconnection. Preflight, one exact
installation, warm observation and cold power-cycle are already complete. Next
is local analysis of the successful read-only failure capture; no automatic
retry, reconnect, reflash or recovery write. The one authorized sealed v025
`read-completion.py` capture has already completed; do not run it again.
The canonical private originals have A/B, no C/shared autosave; cold boot therefore
precedes their restoration. End with exact originals and closed ports, no reboot.

Canonical archive: `build/hardware-v023-20261019-111007/preflight-original.d8bak`,
SHA-256 `f30eac2f7c7f1a8010874d9dfcded715005e5bf026717ad271e3fcaa5c872d4a`.
Never package, commit or substitute it with an export of the current partial state.
Full timing/ISR/clock uncertainty, stack ABI/nesting, independent failed-application
recovery and second-tester/release approval remain open. No RC claim is authorized.

Prior incident details and locators are preserved in
[the v024 handoff archive](history/local-work-handoff-through-v024-20261019.md).
Those are historical evidence, not current instructions to repeat device work.
