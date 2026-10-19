# Dabbl8 RC status — 19 October 2026

**Clock-measurement preparation has advanced; public RC qualification is not complete.**
No FM-1 discovery, port access, notes, restore, reset or flash occurred in this
follow-through. No hosted CI, dependency download, release or deployment occurred.

## Current sources and last verified device

- Development: `dabbl8/felucca-161-bugfixes`, latest firmware source `95ac93f`, in
  `.local-baseline/felucca-161-integration`. The original dirty v022 checkout is
  preserved. The new source adds a diagnostic-only read-only clock sample and
  guarded host comparison; no audio interrupt, storage or loader flow changes.
- Last verified device: **v0.2.2-timing**, originals restored with exact companion
  readback; COMMIT stage7/rc0/guards0/revoke0 and all ports closed. This is retained
  evidence, not a new observation of the current connection.
- Frozen private diagnostic: **v0.2.3-timing / FM-1_8023**, source `e251aa4`, assembled from the exact
  locally checked application and loader without rebuilding either. Package
  610,106 bytes, SHA-256
  `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83`.
  Local freeze: `build/rc-v023-20261019/`; validation/review addendum:
  `build/rc-local-qualification-20261019/`. It is not installed or public RC firmware.
- New clock-capable component: `build/app-only/full/run-ki6fjgj1/`, 527,248-byte
  application only, not an installer. Its 180 firmware inputs match `95ac93f`.
  The frozen package above is unchanged and predates D8C1; the new collector
  refuses capture against it before opening a port. Consolidate/review a new
  package and repin readers/kit before any future installation.
- Eight tracks/eight shared sounding voices; all current engines, mask0x3ffd;
  USB audio off. RAM 94,812/98,304 leaves 3,492 bytes. Pool 335,696/344,064 leaves
  8,368 bytes, only 176 above its reserve. No budget or golden hash was changed.

## Completed checks to reuse

The independent `e251aa4` source gate is APPROVE. Its 84 engine/effects scenarios,
42 emulator workflows, native/WASM audio parity, UI, settings, flash and target
link/MMIO/RAM checks remain valid for unchanged effective firmware inputs. Do not
repeat the broad suite or historical physical persistence/stock recovery trials.

The `95ac93f` delta passes focused local clock/console/collector checks, six
optimized/sanitized resource-console configurations, 24 resource-client tests,
39 selector tests, unchanged target budgets and main-session review. Python3.9
compatibility of the existing console helpers is retained. Its linked model
keeps audio 2,196/TIMER5 200 bytes; startup/main is now 7,516. These are still
conditional software-frame contributions. See [the clock method](clock-measurement.md)
and private `build/rc-clock-20261019/` receipts; the earlier source gate does not
independently review this new delta.

The earlier `e251aa4` follow-through audited 1,041 linked functions and closed the main-loop callback
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
| Timing | Historical inclusive maximum 57,151 ticks; source-documented half 69,632 ticks. Read-only D8C1 and conservative host-bracket math are locally tested. | Collect exact-candidate clock/cadence evidence with reference uncertainty; account for entry/recorder/exit and arrival latency; measure supported concurrent workloads. A narrow host-relative interval is not SI calibration or full ISR qualification. |
| Stack | Fixed-frame source model covers callback families and the two bounded cycles. Existing allocations remain 24,320 user/7,936 supervisor. | Establish hardware SP/SSP mode, implicit frames, permitted interrupt/fault nesting and model assumptions. Watermarks alone do not close this. |
| Independent recovery | Historical stock recovery remains valid. The supplied product listing is RP2040-Matrix; its manufacturer exposes GP0/GP1. Pinned transporter entry/upload/indicator behavior is reviewed. | Confirm the actual board, port XIAO-specific GPIO indicators, verify isolated wiring and application-independent recovery. No RP2040 test occurred; the external loader's underlying binary provenance remains unresolved. |
| New source on hardware | Locally checked private package and guarded batch prepared. | Separate owner authorization, fresh readiness, exact upgrade/reopen/readback and targeted timing/resource checks; original restoration afterward. |

The [single combined batch](rc-hardware-test-plan.md) is unarmed and needs a new
package/kit freeze for the D8C1 delta. [Independent recovery preparation](independent-recovery-preparation.md)
records the board and source-specific gates. Do not flash or
repeat narrow workloads just to obtain another uncalibrated pass. Its instrumented
workload cannot, by itself, close the clock/ABI/independent-recovery gates.

Earlier results and failed attempts are preserved in
[the archived status](history/rc-status-through-v022-20261019.md). Those entries
are historical, not instructions to repeat a SAVE, restore or installation.
