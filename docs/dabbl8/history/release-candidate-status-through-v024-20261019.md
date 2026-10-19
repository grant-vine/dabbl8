# Dabbl8 RC status — 19 October 2026

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

**RC is blocked by a watchdog reset during project restoration on v0.2.3.**
The authorized hardware batch installed the exact consolidated diagnostic and
passed boot, upgrade and cold-start project preservation checks. It stopped on
a final snapshot timeout; the separately authorized original-restoration attempt
then lost MIDI and rebooted. Restoration remains unverified. No hosted CI,
dependency download, public release or deployment occurred.

**Local correction prepared, physical failure still open:** the actual COMMIT
fixture now services116 admitted primitive boundaries instead of zero, refuses
a write on a reported P33 feed timeout, and retains its56-byte observation record
across modeled warm reboot. New tests cover interrupted erase, refusal, failed
NOR I/O and same-boot export isolation. SPI waits publish their first unsuccessful
poll, addressing ambiguity in the saved stage10 trace. No device was accessed.
The application was subsequently packaged privately as described below;
successful host tests do not qualify a restore or authorize retrying one.
Correction evidence: `build/rc-erase-watchdog-20261019/`.

**Corrected private package prepared:** v0.2.4-timing/FM-1_8024 at
`build/rc-v024-20261019/`,610,106 bytes, SHA-256
`adfb2355dd30496b5a28af101e7d301926796a839500d53caceb5f5cf410adb9`.
Existing Felucca assembly/installation flow, exact reviewed application and
unchanged loader; no component rebuild or device access. Exact package-consumer
checks,23 repinned read-only helper cases and four logical-readback controls pass
locally. The paired `build/rc-hardware-v024-20261019/` is unarmed. The owner has
unplugged the unit and will announce availability. No hardware request is pending.
See the [focused batch](rc-hardware-test-plan.md): one cold boot before restoring,
then one original-project SAVE and exact logical readback. No temporary set or
broad repeated workload; full timing/stack/independent recovery remain open.

Final local target `build/app-only/full/run-mmubc7xz/` is528,840 bytes with
RAM94,796/98,304, pool335,696/344,064 and no link/memory errors. Static budget
passes; RAM code has3,933 instructions and no calls. The existing own-transfer
fixture passes111,539 checks and the new watchdog/retention fixture24,443 checks
under fatal ASan/UBSan, retaining only inherited DSP arithmetic exclusions.
The original zero-feed regression now observes116 feeds and passes; deliberate
no-feed, stale-observer and old-trace controls fail as expected. No golden hash,
budget, engine/effect selection or frozen v023 package was changed.
Independent ELF/disassembly review binds all275 inputs and94,671 instruction
rows;574 RAM branches stay in RAM, and the diagnostic is NOLOAD `.noinit`
with700 bytes remaining. Its boot updates precede BSS/pool clearing. The observed
local frame growth (status8 bytes, erase4 bytes) does not qualify full stack
use. These corrections have not been flashed; current device data remains unverified.

## Current sources and last verified device

- Development: `dabbl8/felucca-161-bugfixes`, installed firmware source `c34263f`, in
  `.local-baseline/felucca-161-integration`. The original dirty v022 checkout is
  preserved. The new source adds diagnostic-only read-only clock/arrangement
  samples and guarded host comparison. The later working correction changes
  guarded COMMIT watchdog servicing and diagnostic retention; no loader changes.
- Last verified device: **v0.2.3-timing**, boot2/USB1 after the interrupted original
  restoration. The authorized fixed read-only capture passed and closed all its
  ports. Audio-late and flash-failure counters are zero, but a retained prior-boot
  erase is still INFLIGHT; current saved-project contents are unverified. The
  earlier v022 exact-original-restoration evidence remains historical.
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
  Its original paired kit has282 input pins and remains `hardware_ready:false`.
  The authorized session installed these exact package bytes; its separate
  session kit is now disarmed after failure. This is not public RC firmware or
  permission for another installation.
- Eight tracks/eight shared sounding voices; all current engines, mask0x3ffd;
  USB audio off. RAM 94,812/98,304 leaves 3,492 bytes. Pool 335,696/344,064 leaves
  8,368 bytes, only 176 above its reserve. No budget or golden hash was changed.

## Latest hardware evidence and stop condition

Private session: `build/hardware-v023-20261019-111007/`; workload receipts:
`build/hardware-v023-20261019-111007-kit/hardware-run/`. Preserve the verified
fresh original archive outside Git. Do not repeat the completed upgrade/cold
checks or run another restore, workload, reboot or flash without owner direction.

- The original backup and temporary setup readback passed. Felucca's existing
  MIDI installer completed once. The60-second boot observation and exact
  A/B/C/autosave/current readback passed, including one confirmed cold restart.
- Three D8C1 samples bound TIMER4 to23,975,804..24,021,400Hz relative to the Mac
  monotonic clock; this is not independent oscillator calibration. The timing
  pass retained6,296 samples, maximum instrumented-inclusive57,015 ticks,
  zero late publications and zero taint. The resource report observed eight
  heavy slots (six FM6, one DRUM, one WHEEL), zero allocation failures, and
  user/supervisor written-since-boot watermarks of5,476/1,188 bytes. These are
  observations, not complete ISR or physical-stack bounds.
- The final command78 snapshot lost a reply during its verification scan
  (sequence3637, role11, offset31232); release was not acknowledged. All17
  captured original hashes match the accepted before-capture, but final
  verification failed, so the combined session is not an unchanged-data pass.
  The arrangement/16-scene phase was not started.
- A separately authorized health check showed the same boot1/USB1 and healthy
  stopped state. One original restore was then staged and physically confirmed;
  the companion reported `MIDI port disconnected or closed`. No retry followed.
- The subsequent owner-authorized fixed diagnostic capture confirms boot2/USB1
  and reset reason0x04 (the HAL's watchdog bit). Its retained prior-boot flash
  record is erase/address0x9D000, INFLIGHT/pending, stage/phase10 (WIP polling),
  pre-status0x02 and last-status0x03. No deliberate driver failstop was recorded.
  This identifies the interrupted operation, not why progress stopped or the
  final contents of that sector. The ordinary COMMIT/session RAM words are zero
  after startup; they do not establish successful completion.

The local correction and private package above address the reproduced servicing
and diagnostic gaps. The physical stall's cause and original restoration remain
unverified. Do not infer success from a normal interface or access the unplugged
unit; await owner availability and separate authorization for the prepared batch.

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
| New source on hardware | Exact v023 installed; boot, upgrade and cold persistence passed. Final snapshot timed out and original restoration encountered a watchdog reset. | Resolve the erase/watchdog failure, establish actual saved-data state and restore originals safely. The arrangement phase and complete batch remain unqualified. |

The [single combined batch](rc-hardware-test-plan.md) is now stopped and disarmed;
it is not an instruction to repeat the successful stages. Clock/MIDI observations
are retained, arrangement was not started and original restoration is unverified.
[Independent recovery preparation](independent-recovery-preparation.md)
records the board and source-specific gates. Do not flash or
repeat narrow workloads just to obtain another uncalibrated pass. Its instrumented
workload cannot, by itself, close the clock/ABI/independent-recovery gates.

Earlier results and failed attempts are preserved in
[the archived status](history/rc-status-through-v022-20261019.md). Those entries
are historical, not instructions to repeat a SAVE, restore or installation.
