# Dabbl8 RC status — 19 October 2026

**Local qualification follow-through is complete; public RC qualification is not.**
No FM-1 discovery, port access, notes, restore, reset or flash occurred in this
follow-through. No hosted CI, dependency download, release or deployment occurred.

## Current sources and last verified device

- Development: `dabbl8/felucca-161-bugfixes`, firmware source `e251aa4`, in
  `.local-baseline/felucca-161-integration`. The original dirty v022 checkout is
  preserved. Later documentation-only commits do not change these firmware bytes.
- Last verified device: **v0.2.2-timing**, originals restored with exact companion
  readback; COMMIT stage7/rc0/guards0/revoke0 and all ports closed. This is retained
  evidence, not a new observation of the current connection.
- Private next diagnostic: **v0.2.3-timing / FM-1_8023**, assembled from the exact
  locally checked application and loader without rebuilding either. Package
  610,106 bytes, SHA-256
  `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83`.
  Local freeze: `build/rc-v023-20261019/`; validation/review addendum:
  `build/rc-local-qualification-20261019/`. It is not installed or public RC firmware.
- Eight tracks/eight shared sounding voices; all current engines, mask0x3ffd;
  USB audio off. RAM 94,812/98,304 leaves 3,492 bytes. Pool 335,696/344,064 leaves
  8,368 bytes, only 176 above its reserve. No budget or golden hash was changed.

## Completed checks to reuse

The independent `e251aa4` source gate is APPROVE. Its 84 engine/effects scenarios,
42 emulator workflows, native/WASM audio parity, UI, settings, flash and target
link/MMIO/RAM checks remain valid for unchanged effective firmware inputs. Do not
repeat the broad suite or historical physical persistence/stock recovery trials.

This follow-through audits 1,041 linked functions and closes the main-loop callback
families in a source-conditioned model. Bounded fixed-frame contributions are
7,512 bytes for startup/main,2,196 for audio, 200 for TIMER5, and 400 for fatal
software entry including its vector stub. They are not complete physical bounds.
It reparses 6,300 retained v022 timing samples, verifies the recovery input hashes,
and reuses nine unchanged recovery guard cases. See
[the detailed evidence and limits](rc-local-qualification-20261019.md).

The exact future MIDI schedule was additionally exercised against the current
actual renderer under fatal ASan/UBSan: 24,823 blocks, eight voices/eight heavy
slots, no allocation failures or remaining holds, unchanged proposal and exact
host rollback. Its proposed workspace hash is unchanged. The package's strict
OTA/loader and native-pool upgrade tests pass. The newly repinned read-only
completion reader passes 17 fake-console cases; batch fault/message tests and
default no-access refusal pass. No hardware timing claim follows from these tests.

## Remaining gates

| Gate | Local result | Required before a full RC claim |
| --- | --- | --- |
| Timing | Historical inclusive maximum 57,151 ticks; source-documented half 69,632 ticks. The 12,481-tick difference is not full deadline headroom. | Exact-candidate clock/cadence bounds from an independent reference; account for entry/recorder/exit and arrival latency; measure supported concurrent workloads. |
| Stack | Fixed-frame source model covers callback families and the two bounded cycles. Existing allocations remain 24,320 user/7,936 supervisor. | Establish hardware SP/SSP mode, implicit frames, permitted interrupt/fault nesting and model assumptions. Watermarks alone do not close this. |
| Independent recovery | Exact stock-v15 package, historical full readbacks and repaired tool/loader hashes match. Working-console stock recovery already succeeded. | Verify an entry/recovery path independent of the running application if that is the support claim. The owner's Waveshare RP2040 wiring/pin mapping and recovery remain untested; the external loader's underlying binary provenance remains unresolved. |
| New source on hardware | Locally checked private package and guarded batch prepared. | Separate owner authorization, fresh readiness, exact upgrade/reopen/readback and targeted timing/resource checks; original restoration afterward. |

The [single combined batch](rc-hardware-test-plan.md) is unarmed. Do not flash or
repeat narrow workloads just to obtain another uncalibrated pass. Its instrumented
workload cannot, by itself, close the clock/ABI/independent-recovery gates.

Earlier results and failed attempts are preserved in
[the archived status](history/rc-status-through-v022-20261019.md). Those entries
are historical, not instructions to repeat a SAVE, restore or installation.
