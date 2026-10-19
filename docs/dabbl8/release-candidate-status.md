# Dabbl8 RC status — 19 October 2026

**Combined diagnostic collection passes locally; public RC qualification is not complete.**
No FM-1 discovery, port access, notes, restore, reset or flash occurred in this
follow-through. No hosted CI, dependency download, release or deployment occurred.

## Current sources and last verified device

- Development: `dabbl8/felucca-161-bugfixes`, latest firmware source `c34263f`, in
  `.local-baseline/felucca-161-integration`. The original dirty v022 checkout is
  preserved. The new source adds diagnostic-only read-only clock/arrangement
  samples and guarded host comparison; no audio interrupt, storage or loader flow changes.
- Last verified device: **v0.2.2-timing**, originals restored with exact companion
  readback; COMMIT stage7/rc0/guards0/revoke0 and all ports closed. This is retained
  evidence, not a new observation of the current connection.
- Earlier frozen private diagnostic: **v0.2.3-timing / FM-1_8023**, source `e251aa4`, assembled from the exact
  locally checked application and loader without rebuilding either. Package
  610,106 bytes, SHA-256
  `c3ebf3933ee5b6088f990cefda17e56ca468e5dc4f619326dba09349898fac83`.
  Local freeze: `build/rc-v023-20261019/`; validation/review addendum:
  `build/rc-local-qualification-20261019/`. It is not installed or public RC firmware.
- New clock-capable component: `build/app-only/full/run-ki6fjgj1/`, 527,248-byte
  application only, not an installer. Its 180 firmware inputs match `95ac93f`.
  The frozen package above is unchanged and predates D8C1; the new collector
  refuses capture against it before opening a port. The consolidated package
  below supersedes this earlier app-only component for future review.
- Latest clock-and-arrangement component: `build/app-only/full/run-9n195ske/`,
  527,572 bytes. Diagnostic-only D8A1 coherent row status and strict observation
  model pass focused checks and unchanged target budgets. RAM/pool and conditional
  stack maxima are unchanged. See [the observation contract](arrangement-observation.md).
- Consolidated private package: `build/rc-v023-d8a1-20261019/`, the same unpublished
  v0.2.3 identity, with the exact latest application and unchanged reviewed loader.
  Package610,106 bytes, SHA-256
  `c2872ecc0aca96e4f92a8530d0e2682287fd48e40ab0749ecd22ff508720f8cb`.
  Firmware source `c34263f`, collector/readback `aeef693`; decode/CRC/corruption,
  virtual OTA/loader/native-pool preservation and17 completion-reader cases pass.
  The bundled clock helper now includes its exact source-header dependency.
  Its paired kit has282 input pins and remains `hardware_ready:false`. This is
  local preparation, not an installed build, public RC or permission to flash.
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

A later test-only follow-through extends the actual loader's virtual upgrade
fixture to nonempty synthetic settings/preset/FM6/sample allocations and all
flash bytes outside the app/known update-record sectors. Both optimized and
fatal ASan/UBSan runs pass; eight deliberate ancillary corruption controls per
mode are detected by both comparison layers. The normal host suite now includes
the fixture. This is raw-byte preservation evidence, not valid ancillary-format
reopening or physical power-cut recovery. Firmware, loader and both frozen
packages are unchanged; see private `build/rc-roadmap-audit-20261019/` receipts.

Current ancillary-format reopening now also passes a separate host fixture:
nondefault PER4 settings, both banks of 32 current FM6 presets/patches and all
three simple user-sample slots reopen through actual application readers after
the real loader's virtual upgrade. Both optimized/fatal-sanitizer positive runs
and 14 targeted invalidation controls pass, with poisoned caches and no reader
writes. See [the exact scope and limits](ancillary-upgrade-qualification.md).
These synthetic format checks do not establish ancillary writer atomicity,
physical cold boot or interrupted upgrade durability. The frozen image is unchanged.

## Remaining gates

The new `arrangement-stress` local scope covers the previously missing combined
workload: actual audio ISR, native arrangement, effects, live USB/TRS parser input
and asynchronous LCD drawing. Four scenarios pass in optimized and fatal
ASan/UBSan modes, including all16 natural scenes, fill/return and queued row,
external clock loss across timer rollover, eight voices/eight heavy slots,
matched note cleanup to silence and unchanged project/source bytes. Three
omitted-action controls are correctly rejected. This is simulated host evidence;
it does not close the target concurrent-playback, timing, stack or physical
routing gates. See [the combined scenario evidence](arrangement-stress-qualification.md).

The synthetic C proposal additionally passes actual native recall and the
SEQ-hold/SONG/PLAY preparation path, all16 natural scenes and automatic Stop in
both host modes. Its private combined archive preserves A/B and retains the old
MIDI workspace in live/shared autosave. Bounded observation/Stop/readback and
the newly pinned package/kit now pass local checks; no owner batch is armed.

The D8A1 producer/decoder and bounded row observer pass locally, including actual
C transitions and host refusal controls. Native timing/resource orchestration now
passes14 framing/ownership/cleanup/CLI cases; offline C-context readback passes10
cases through the actual native codec under fatal sanitizers. Exact A/B/C and
ancillary protection allows only the known live/shared-autosave transitions.
Existing console23 and selector45 cases pass. No hardware was accessed. A single
consolidated package/readers/kit is prepared; these results are not device
authorization or complete timing/stack evidence. See the [collector contract](arrangement-observation.md).

| Gate | Local result | Required before a full RC claim |
| --- | --- | --- |
| Timing | Historical inclusive maximum 57,151 ticks; source-documented half 69,632 ticks. Read-only D8C1 and conservative host-bracket math are locally tested. | Collect exact-candidate clock/cadence evidence with reference uncertainty; account for entry/recorder/exit and arrival latency; measure supported concurrent workloads. A narrow host-relative interval is not SI calibration or full ISR qualification. |
| Stack | Fixed-frame source model covers callback families and the two bounded cycles. Existing allocations remain 24,320 user/7,936 supervisor. | Establish hardware SP/SSP mode, implicit frames, permitted interrupt/fault nesting and model assumptions. Watermarks alone do not close this. |
| Independent recovery | Historical stock recovery remains valid. The supplied product listing is RP2040-Matrix; its manufacturer exposes GP0/GP1. Pinned transporter entry/upload/indicator behavior is reviewed. | Confirm the actual board, port XIAO-specific GPIO indicators, verify isolated wiring and application-independent recovery. No RP2040 test occurred; the external loader's underlying binary provenance remains unresolved. |
| New source on hardware | Locally checked private package and guarded batch prepared. | Separate owner authorization, fresh readiness, exact upgrade/reopen/readback and targeted timing/resource checks; original restoration afterward. |

The [single combined batch](rc-hardware-test-plan.md) is unarmed, with clock,
MIDI and all16-scene arrangement checks grouped into one visit and original
restoration at the end. [Independent recovery preparation](independent-recovery-preparation.md)
records the board and source-specific gates. Do not flash or
repeat narrow workloads just to obtain another uncalibrated pass. Its instrumented
workload cannot, by itself, close the clock/ABI/independent-recovery gates.

Earlier results and failed attempts are preserved in
[the archived status](history/rc-status-through-v022-20261019.md). Those entries
are historical, not instructions to repeat a SAVE, restore or installation.
