# Dabbl8 RC status — 19 October 2026

**v0.2.5 is prepared and locally checked; RC and device work remain blocked.**
The owner is unavailable. No FM-1 access, restore or flash is authorized now.

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

Private corrected package: `build/rc-v025-20261019/`,610106 bytes,
SHA-256 `7a16ea2c4ec9c3e944b84e37a7f27ebefe3501139c6e6b54f17f88680a81fea5`.
Full profile, eight tracks/eight shared voices, engine mask0x3ffd, USB audio off;
all prior engines/effects retained. Application528908B, RAM94796/98304 (3508 free),
pool335696/344064 (8368 free), static link/budget PASS. The loader, flash map and
Felucca install flow are unchanged. Host results do not qualify hardware safety.

| Gate | Current evidence | Still required |
| --- | --- | --- |
| Startup timing | Old order fails32/32 phase cases; corrected4/8 order passes64. Actual adoption12254 checks pass with no DMA overlap or NOR writes. Counter coherence/wrap and strict host parsing pass. | Exact-candidate boot/cold observation; fail any current-boot late event. The original two events remain unattributed. |
| Own restore/COMMIT | Prior watchdog and retained diagnostics fixes pass local regressions. New package preserves the byte-identical RAM flash driver and loader. | Restore originals physically, acknowledged COMMIT, exact five-role readback and retained completion. Device saved state is unverified. |
| Engine/effects/workflows |84 engine/effects and42 emulator cases reused; actual ISR/DSP parity and affected startup checks pass. | Supported concurrent target workloads and arrangement qualification; no narrow demo claim. |
| Timing/stack | Prior clock/window/watermark observations reused; static memory remains within unchanged budgets. | Reference uncertainty, entry/exit/arrival/ISR bounds, SP/SSP ABI and interrupt nesting. |
| Independent recovery | Historical stock restore via working console remains verified. RP2040-Matrix preparation exists. | Actual board/wiring and application-independent recovery; external loader provenance remains open. |
| Release | Private package, notices, source, seals and local receipts prepared. | Second tester and separate release/publication authorization. |

The [hardware plan](rc-hardware-test-plan.md) is unarmed and groups the next
necessary physical checks into one visit. Do not ask for hardware while the owner
is away or repeat completed broad checks. Latest durable continuation state:
`build/rc-package-v025-20261019/handoff.json`.

The previous full evidence ledger remains in
[the archived v024 status](history/release-candidate-status-through-v024-20261019.md).
