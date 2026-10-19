# Historical RC qualification preparation — 19 October 2026

Read the [current handoff](local-work-handoff.md) and [RC status](release-candidate-status.md)
for installed/prepared identities, data state, permissions and the next action.
The incidents, methods and proposed sequences below record earlier v020–v022
work. They are not pending instructions, evidence of the current device's data
state or authorization to access it. The current [hardware plan](rc-hardware-test-plan.md)
remains unarmed; do not repeat an old restore/upgrade sequence from this record.

## Historical v022 preparation

At that stage the device was v0.2.1-timing with originals restored, and the
private v0.2.2-timing package was frozen at `build/rc-v022-20261019-r2/`, with
local review evidence in `build/rc-package-v022r2-20261019/`. Preserve those
sealed inputs and their scope. Later device events supersede that observation;
it does not prove that originals are restored on the currently installed image.

## Earlier v020/v021 qualification record

The v020 original-restoration and closed-port observations below apply only to
their recorded session. No device action or flash is authorized by this document.
Preserve the sealed v020 review kit and subsequent correction evidence.

## Upgrade preservation blocker found locally

`tests/ldr_native_pool_preservation_test.c` runs the actual existing Felucca
loader against virtual NOR using the frozen v019 and v020 packages. No firmware
or loader source was changed. The normal loader completion scan excludes
`[0x97000,0xE0000)` from update-record cleanup, while Dabbl8's fifth 8 KiB pool
block occupies `[0xE5000,0xE7000)`. The latter is scanned.

The initial synthetic-byte reproduction erased both sectors. The stronger
regression now constructs a **known writable, structurally valid D8P1 project**:
wire offset3584 is track6's last two parameter bytes, engine/preset and notes.
A bounded choice of legal note/parameter bytes makes its first80 bytes resemble
an update record with a valid CRC. The actual D8P1 parser accepts it, and the
actual pool writer saves it as the newest autosave in block4. The loader then
erases E5000, losing that newest autosave and falling back to the older one.
The extra-block preservation and exact latest-autosave assertions both fail.
The protected Felucca area and real update-record controls behave correctly.

Changing only the marker checksum, with native chunk/file checksums repaired and
the actual pool writer used again, makes all preservation assertions pass.
ASan/UBSan found no fault. Evidence is under
`build/rc-remaining-local-20261018/loader-valid-project-{regression,control}.log`.
This establishes valid native-project storage reachability and a real upgrade
preservation defect in the simulated loader/storage paths. It does not prove
that the owner's current data was damaged, or explain earlier COMMIT/USB failures.
The retained pre-fix run remains red. The owner then said Continue after the
narrow loader exception was explained. On 19 October the cleanup-only fix was
applied, leaving application, staging, calibration and flash limits unchanged.
The same regression now passes, including all five blocks and exact newest
autosave; both sectors are covered. Existing Felucca loader negative/range/record
checks pass. Independent source/test review APPROVE is preserved in
`build/rc-upgrade-fix-20261019/gate-review-source.md`.

Reproduction with existing local inputs only:

```sh
clang -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
  tests/ldr_native_pool_preservation_test.c firmware/src/d8p1.c \
  firmware/src/d8pool.c firmware/src/d8pool_mapped.c \
  -o build/rc-remaining-local-20261018/loader-pool-regression
build/rc-remaining-local-20261018/loader-pool-regression \
  build/rc-v019-20261018/dabbl8-0.1.9-timing-diagnostic.fwsc \
  build/rc-v020-20261018/dabbl8-0.2.0-timing-diagnostic.fwsc
# Both modes pass with the fix. Retained before-source run fails (see evidence).
```

The owner-authorized exception is limited to excluding the existing extra native
block from record cleanup; it is not permission for additional loader changes,
data relocation, different boundaries or another flashing mechanism. The private
v0.2.1-timing candidate contains this fix; nothing has been installed. The frozen
package and corresponding-source evidence are under `build/rc-v021-20261019/`.
Felucca OTA stages the incoming package's loader before installation; review
that exact first-hop path and resulting loader bytes rather than assuming the
installed application alone determines the loader that performs the upgrade.

## Prepared own-upgrade acceptance sequence — local fix ready, not queued

1. Use the reviewed, frozen v0.2.1-timing successor to v020 only after separate
   owner installation approval. Its source/package/loader delta is recorded;
   storage format, full engine mask and UAC-off profile stay unchanged. This is
   a real version upgrade, not an identical reinstall or a downgrade.
2. Reuse the passing v020 SAVE/recall/reboot results and 84 source-bound matrix
   scenarios. Run only checks affected by the successor. Actual boot/virtual NOR
   tests prove software paths, not installed physical upgrade durability.
3. Before hardware, verify the Felucca OTA staging, application installation and
   record cleanup write sets against every user-data range. Reuse the audited
   package decoder; app-only write bounds alone are insufficient for this check.
4. Prepare one session, with explicit installation approval: fresh original
   logical backup; temporary all-three-slot/nonempty-autosave setup and SAVE;
   verify logical readback; close connections; perform the approved upgrade once.
   Existing private original99bd and temporary c779 archives remain unchanged.
5. After restart, require the normal interface without a first-use erase prompt,
   stable boot/USB state and healthy flash, then compare A/B/C/autosave exactly.
   Compare current against the saved autosave (not an unrelated pre-upgrade live
   edit). Physical pool generations need not equal the archive container hash.
6. Restore originals with one SAVE and exact logical readback, close all ports.
   Stop on any failure; no automatic retry, reset or further restore after failure.

This qualifies projects/autosave only. Settings, user presets and samples need
their own exact pre/post observations if the public upgrade claim includes them.
Command78 full raw capture remains refused after its earlier missing reply;
repair/qualify that path before relying on it for those wider preservation claims.
Stock rollback is a recovery procedure, not a promise to preserve Dabbl8 projects.

## Prepared target timing and memory checks

The exact source uses 128-frame audio halves at 44,100 Hz: nominal deadline
2,902.494 microseconds, or 69,659.864 ticks at the nominal 24 MHz TIMER4 rate.
The half deadline must be measured against actual audio cadence. TIMER4's source
comment reports an upstream ROM-environment measurement; that is not a measured
calibration of this installed application and clock configuration.

The render interval is quantized to whole microseconds by `shed_check`; the
inclusive interval spans two counter snapshots in `fm1_alnk0_irq`. It excludes
assembly entry before the first snapshot, histogram/window recording after the
second snapshot, the final `in_audio` clear and interrupt return. Nested work
and interrupt arrival latency require separate accounting. Do not rename the
inclusive histogram into a full interrupt/deadline measurement.

Before release performance claims:

- Bind every capture to the exact frozen image/configuration and original
  workload contract. Existing helper version strings do not attest binary bytes.
- Establish timer/audio-cadence bounds using an independent reference, retaining
  raw observations and uncertainty. Host command round trips alone are too loose
  to prove microsecond timer accuracy; nominal FS is not an independent reference.
- Bound omitted entry/exit/recording and observer work using exact linked code
  plus measured target evidence. Do not subtract an average observer cost from a
  worst-case sample or compare host instructions directly with target cycles.
- Reuse the successful idle/demo/heavy cases at their recorded version scope.
  Add only uncovered concurrent control/transport/effect/routing cases, qualified
  first through the actual renderer and existing bounded MIDI cleanup helpers.
  No preset/parameter change is implicitly no-write: autosave may persist it.
- Stop for any late-audio, reset, flash failure, tainted/saturated/invalid frame,
  allocation failure or uncertain note cleanup. Percentiles cannot excuse a
  failed maximum/deadline. Use upper percentile bin bounds, not midpoints.
- Keep UAC off for the current candidate; UAC-on is a different qualification.
- Retain current static RAM/pool figures. The 8,376-byte pool margin is only
  184 bytes above its reserve. Stack pattern scans are lower bounds on observed
  writes, not maximum stack depth. Audit full linked call paths, indirect calls,
  dynamic frames and permitted interrupt nesting against both stack allocations.
  Do not claim complete stack bounds from a clean watermark alone.

No hardware timing script is armed. The next hardware session should be requested
only after current owner readiness and the relevant approval; the timing method
and complete stack/observer bounds remain unqualified. Independent installation/restore/rollback and public-scope signoff
remain separate gates.
