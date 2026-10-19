# Boot timing correction and evidence

The v024 installation completed through Felucca's unchanged updater, but its
first read-only status reported lifetime `audio_late2`. The batch stopped before
power-cycle, SAVE or restoration. This is preserved as a failed health gate;
it does not establish audible playback failure or the time of either event.

## What the investigation established

The upstream-derived late counter lives in `.noinit`. Its address and76-byte
container are identical in the installed v023/v024 images, and normal startup
preserves it while resetting the maximum render-time statistic. Updater staging
silences samples but leaves DMA running across masked flash operations. Events
there can survive into the new image. A low new maximum therefore cannot locate
an old late event. This is not proof of an upstream playback defect.

There is also a real startup integration defect. Audio DMA starts before the
native saved-project adoption. Adoption masks interrupts while publishing the
eight-track runtime, including parameters, steps and FM6 state. The former audio
startup fixture substituted a zero-cost adoption, while the actual native fixture
used no-op IRQ hooks. Each tested its component, missing their interaction.

The corrected startup enables TIMER5/input service, completes read-only adoption
with audio DMA stopped, then starts audio and opens external MIDI. The admission
checks, exact project decoding, storage map, consent, busy waits, watchdog fix,
engine/effect selection and updater/loader remain unchanged.

## Observability and acceptance

Timing diagnostics retain `audio_late` and add `audio_late_entry`, captured before
any audio/IRQ starts, plus `audio_late_boot`, unsigned lifetime-minus-entry. All
three status values use one lifetime snapshot. No counter is reset or forgiven.
The strict pure host parser rejects missing, duplicate, out-of-range or inconsistent
values. New-candidate checks require zero current-boot events, stable lifetime/entry
and boot/USB counters, and all other health conditions. Any new current-boot event
fails, even if later rendering is fast. The old v024 failure is not reclassified
as a pass. Its exact physical cause remains unproven without a new authorized test.

## Local evidence

All paths below are private ignored evidence under `build/`:

- `rc-boot-timing-20261019/red-startup/`: strengthened owner fixture fails32/32
  simulated phases on the old order; `green-startup/` and `green-startup4/` pass
  all32 each with retained history intact and no DMA blackout.
- `rc-boot-timing-20261019/adoption-repro/`: actual native adoption/codec/NOR,
  shipping order1615 IRQ masks with simulated DMA running; reordered0, exact
  restored music and no NOR writes. The promoted existing native boot fixture
  passes12254 checks; its old-main negative control fails as expected.
- `local-checks/run-bmr7ej_5/`: focused startup/counter/native/TIMER5/console-host
  selection passes. Counter producer tests compile actual shipping baseline and
  status code; cover10 coherence/wrap cases plus12 pure parser controls.
- `rc-boot-timing-20261019/`: actual ISR/DSP off/paused/collect parity passes
  optimized/fatal sanitizer4/8-track cases; remaining console/parser/isolation
  checks pass with existing Python3.14. The initial Python3.9 clock-parser import
  failure and a UAC-on configuration budget refusal are retained explicitly;
  neither is represented as a passing candidate run.45 selector tests pass.
- `app-only/full/run-zte1a6vf/`: corrected v025 full/UAC-off target passes link
  and unchanged static budgets. Application528908B; RAM94796/98304,3508 free;
  pool335696/344064,8368 free (176 above reserve). User/supervisor reservations
  remain24320/7936. RAM flash text is byte-identical to v024,3933 instructions
  with no calls. Linked IR preserves baseline-before-audio and adoption-before-DMA.
- `rc-package-v025-20261019/`: exact package decode/corruption, virtual OTA/loader/
  native-pool preservation and28 fake-console reader controls. The five-role
  comparison helper and codec inputs remain unchanged; the four prior comparison
  cases and original validation are reused with explicit hash binding.

Reuse the84 engine/effect scenarios and42 emulator workflows for unchanged DSP
and workflow inputs. These simulations do not measure physical deadlines or
interrupt latency, establish complete stack bounds, prove corrected physical
COMMIT completion or qualify independent recovery. No FM-1 access occurred.

## Avoiding another test loop

Keep one current handoff, exact frozen inputs and a pass/fail evidence ledger.
Run affected checks once; keep historical failures separate from current gates.
Test the actual integration boundary before another owner visit. Do not add a
new snapshot/restore simply to re-prove the same music. Hardware is for the
remaining physical questions, not rediscovering source-order or host-parser bugs.

## Host collector follow-up

The general timing, resource, clock and arrangement collectors now enforce the
same boot protocol. For an explicitly expected v0.2.5 or later identity, the
coherent three-field tuple is mandatory and every current-boot event fails.
Lifetime/entry are retained in the observation epoch, so changing both cannot
hide a reset or counter change. Older collectors keep their prior semantics;
the v0.1.8-only MIDI workload is still version-pinned and must not be reused on
v0.2.5. There is no fallback from a missing new tuple to a legacy zero counter.

The shared package reader also accepts the actual schema-1 private diagnostic
manifest (`package_name` plus its named `artifacts` entry), as well as historical
`candidates` manifests. Mixed formats, mismatched version, bad names and missing
artifacts refuse. Both actual v025 clock/resource CLI plans pass without opening
ports. This is a host-tools follow-up: it neither changes nor rebuilds the frozen
firmware/kit and does not authorize device access.

The host follow-up evidence is `build/rc-host-audio-epoch-20261019/`:67 host tests
and seven clock test groups pass, including strict new-protocol history/refusal
and owned-cleanup cases. The actual whole resource-console fixture passes full
profile optimized/fatal sanitizer runs with its missing diagnostic baseline
fixture restored. Preserve the initial counter/manifest/fixture failures and the
corrected receipts. The84 engine/effects cases and42 emulator workflows were not
repeated. All181 pinned firmware paths and the unchanged package/loader remain
identical. Four host collector files intentionally differ from the conservative
275-file build-input receipt; do not claim that entire source receipt unchanged
or replace the frozen corresponding-source archive with the working directory.
