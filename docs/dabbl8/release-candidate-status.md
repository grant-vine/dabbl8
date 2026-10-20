# Dabbl8 RC status — 20 October 2026

Private v028 diagnostic package prepared: `build/rc-v028-20261020/`, 610,106 B, SHA-256 `7e1df414687601d96650697c7b9aaaab042a0099ccbe88c27ccebe42be5cd549`. It adds retained TIMER4 CON/PRD entry context (76 B trace; 692 B NOINIT headroom) and a reviewed fixed-span reader. All 30 offline package steps, 38 fake-console cases and the exact target RAM audit pass. This improves observability; it does not prove a physical watchdog repair or RC readiness. Prior broad passing evidence is reused. See [trace format 2](retained-flash-trace.md), `build/rc-timer-context-20261020/` and `build/rc-package-v028-20261020/handoff.json`. No device access, flash or restore retry is authorized; no hardware batch is armed.

Source publication is tracked in [PR #205](https://github.com/grant-vine/dabbl8/pull/205)
(retention), [#206](https://github.com/grant-vine/dabbl8/pull/206) (startup),
[#207](https://github.com/grant-vine/dabbl8/pull/207) (host guards),
[#208](https://github.com/grant-vine/dabbl8/pull/208) (clock context),
[#210](https://github.com/grant-vine/dabbl8/pull/210) (timer contract) and
[#211](https://github.com/grant-vine/dabbl8/pull/211) (completed SPI trace).
These are draft source stacks. The private v028 build has its own complete
input receipt; a passing isolated source PR is not an attestation of the whole
integration image. [PR #209](https://github.com/grant-vine/dabbl8/pull/209)
records the separate vendor feasibility assessment and no base change.

## Installed v025 and preceding correction history

**v0.2.5 installed and passed warm/cold boot checks; original restoration failed and RC remains blocked.**
Earlier local candidate v027 fixes a reproduced inherited-clock defect; three
advisors and live-pinned upstream comparison have not established the physical
COMMIT reset cause.27 stable local steps,1540 timer checks per mode, target RAM
audit,30 package commands and28 fake-reader cases pass. All broad unchanged
evidence is reused. See [current boundary review](commit-watchdog-boundary-review.md)
and `build/rc-commit-investigation-20261020/handoff.json`. No device access or
new restore/flash is authorized. The v026 package below is preserved history.
Earlier local v026 fixes a reproduced stale completed-SPI-wait label
and passes focused optimized/sanitized and package checks. It does not prove
a physical watchdog repair. No device access is authorized now.
See [investigation and candidate](spi-status-watchdog-investigation.md).
The owner authorized the focused hardware batch on 20 October. One unchanged
Felucca installation completed. Four warm observations over60.04s and two cold
observations over10.03s had stable boot/USB identities and zero lifetime, entry
and current-boot late counters. These results qualify the observed boot windows,
not full timing/stack or application-independent recovery.

Two original-restore confirmations expired while the owner was away; neither
reached COMMIT. Each was closed normally, then retried only on fresh owner
instruction. For the third request the owner pressed SAVE and reported a reboot
back to the main interface. The companion reported `MIDI port disconnected or
closed.` We stopped without reconnecting or issuing further device commands.
Original projects and COMMIT completion are unverified; normal UI is not proof.
One separately authorized fixed read-only capture completed and closed cleanly.
It confirms main boot1→2 and a retained previous-boot unfinished COMMIT in
STORE:9 completed primitives,10 feed receipts, zero P33 failures/guards/revokes.
The RAM driver was in flight programming0x9D800; its last retained point is SPI
transmit wait (stage11) inside WIP polling (phase10), outgoing status command0x05.
Reset reason0x04 is the P3 watchdog bit. No deliberate failstop was recorded,
and both captured status observations have zero audio-late counters. The exact
stalled instruction and underlying cause remain unproven: a local reproduction
shows that wait label surviving completed transfers, and the non-atomic
retained trace is not a continuous instruction history. No project readback was
attempted. Do not retry restoration or flash. Session evidence:
`build/hardware-v025-20261020-074222/progress.json`.
Raw journal, fixed RAM bytes, decoded COMMIT and successful close receipt:
`build/hardware-v025-20261020-074222/restore-failure-capture-1/`.

The v024 update installed successfully but the first health query showed two
lifetime late-audio detections. We stopped before the planned cold boot/restore.
The counter survives updates; its reading alone cannot attribute those events to
v024 boot or audible playback. Local investigation also reproduced a genuine
startup overlap: DMA was running while native saved-project adoption blocked
interrupts. The corrected order adopts before starting DMA, and preserves the
lifetime counter while publishing coherent entry/current-boot diagnostics.
See [the source-backed findings and tests](boot-timing-qualification.md).

The host collectors have also been corrected locally to distinguish inherited
history from current-boot failures, retain that history across observations and
read the actual frozen manifest format.67 host tests, seven clock groups and the
affected resource-console fixture pass. This separate host overlay changes no
firmware bytes, grants no device authority and clears no physical release gate.

Private corrected diagnostic: `build/rc-v026-20261020/`,610106 bytes,
SHA-256 `5f9fbfebbf41e0b70f03df91755acf2fb77e6fbbde9984cbf08f1307228bd4e9`.
Full profile, eight tracks/eight shared voices, engine mask0x3ffd, USB audio off;
all prior engines/effects retained. Application529008B, RAM94796/98304 (3508 free),
pool335696/344064 (8368 free), static link/budget PASS. The loader, flash map and
Felucca install flow are unchanged. Host results do not qualify hardware safety.

| Gate | Current evidence | Still required |
| --- | --- | --- |
| Startup timing | Old order fails32/32 phase cases; corrected4/8 order passes64. Actual adoption12254 checks pass with no DMA overlap or NOR writes. Counter coherence/wrap and strict host parsing pass. Exact v025 warm60.04s/cold10.03s observations pass with zero boot late events. | Broader target qualification remains open. The original two v024 events remain unattributed. Reuse the passed boot windows. |
| Own restore/COMMIT | Prior watchdog and retained diagnostics fixes pass local regressions. Physical v025 restore lost MIDI after SAVE. One read-only retained capture confirms watchdog reset during program/WIP polling at0x9D800. | Investigate the RAM SPI/status-wait path locally using this capture. Acknowledged COMMIT and exact five-role readback are absent. Device saved state is unverified; no automatic retry. |
| Engine/effects/workflows |84 engine/effects and42 emulator cases reused; actual ISR/DSP parity and affected startup checks pass. | Supported concurrent target workloads and arrangement qualification; no narrow demo claim. |
| Timing/stack | Prior clock/window/watermark observations reused; static memory remains within unchanged budgets. | Reference uncertainty, entry/exit/arrival/ISR bounds, SP/SSP ABI and interrupt nesting. |
| Independent recovery | Historical stock restore via working console remains verified. RP2040-Matrix preparation exists. | Actual board/wiring and application-independent recovery; external loader provenance remains open. |
| Release | Private package, notices, source, seals and local receipts prepared. | Second tester and separate release/publication authorization. |

The frozen [hardware plan](rc-hardware-test-plan.md) and package handoff retain
their historical unarmed state. The newer owner-authorized session above is
now stopped at its first unexpected device failure. Reuse passing checks and
preserve both frozen packages and the session evidence; do not repeat boot tests.

The previous full evidence ledger remains in
[the archived v024 status](history/release-candidate-status-through-v024-20261019.md).
