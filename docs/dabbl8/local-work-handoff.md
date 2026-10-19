# Current handoff — v025 prepared locally; no device access

The owner is unavailable and explicitly forbids FM-1 access or flashing. Do not
discover devices, open ports, stage restores or request physical actions. Installed
firmware is v0.2.4-timing, last observed main boot2/USB1 with lifetime audio_late2.
That batch stopped at its first boot-health query. Saved device contents and
original restoration remain unverified; do not infer them from normal UI.

Read [boot timing evidence](boot-timing-qualification.md), [RC status](release-candidate-status.md)
and `build/rc-package-v025-20261019/handoff.json` first. Preserve the dirty
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
authorized, and the remaining physical gates below have not changed.

## Corrected private candidate

`build/rc-v025-20261019/dabbl8-0.2.5-timing-diagnostic.fwsc`, FM-1_8025,
610106 bytes, SHA-256 `7a16ea2c4ec9c3e944b84e37a7f27ebefe3501139c6e6b54f17f88680a81fea5`.
The exact checked application is528908B; the reviewed6950B loader and Felucca
packer/installer are unchanged. Paired kit: `build/rc-hardware-v025-20261019/`.
Neither package nor plan grants hardware authority. Verify their seals before use.

Audio DMA now starts after saved-project adoption, with input scanning active
and external MIDI blocked until adoption completes. Lifetime late history is
preserved; coherent entry/current-boot counters distinguish old updater events.
New checks refuse any current-boot event or counter change. Physical attribution
of the two v024 events and hardware acceptance of the fix remain unverified.

## Reuse and remaining work

Focused actual-code startup/IRQ/adoption, counter coherence/wrap/parser and target
memory/static checks pass. The unchanged84 engine/effects cases,42 emulator
workflows, COMMIT/watchdog regressions, byte-identical RAM flash code/loader and
logical comparison codec evidence are reused. Do not rerun those broad matrices,
rebuild this package or use a full snapshot to compare the same five project roles.

A future owner-authorized session uses the [focused batch](rc-hardware-test-plan.md):
healthy stopped v024 preflight (known lifetime2 must not increase), one exact
Felucca installation, strict v025 boot observation, one cold power-cycle, then
one final original-project restore/SAVE and exact logical readback/COMMIT capture.
Stop on any failure; no automatic retry, reconnect, reflash or recovery write.
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
