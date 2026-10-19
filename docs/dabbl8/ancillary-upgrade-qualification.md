# Current ancillary formats after a virtual upgrade

The local upgrade fixture now checks more than unchanged raw flash ranges.
`tests/native_ancillary_upgrade_test.c` seeds valid, synthetic **current Dabbl8
records**, runs the actual Felucca loader on virtual NOR, poisons the application
caches, and reopens those records through the actual application readers.
No old-firmware migration, owner backup, vendor image or device access is used.

## Covered records and oracles

- Current PER4 settings: a nondefault palette, speaker filter, hold/LED preferences
  and favourites. `st_load`, `settings_import` and `settings_export` must recover
  the exact expected current settings. Both valid copies differ; the newer copy
  must win.
- Both current user-preset banks: 32 named FM6 presets with parameters and note
  patterns. `up_boot` must recover both banks exactly, including slot 32. Older
  committed copies have deliberately different names.
- Current user FM6 patch object: all 32 patch bindings must reopen through
  `upf_get` with the exact expected canonical patches. The older committed copy
  contains only one used patch, so selecting it cannot accidentally pass.
- All three user-sample allocations: one valid zone per slot and distinct,
  nonempty ADPCM payloads. Actual `smp_user_scan` must recover each zone. A separate
  CRC comparison verifies payload preservation; this does **not** claim that the
  sample scanner itself validates the payload CRC.
- A/B/C and shared autosave: actual native pool save/load preserves the valid
  public minimal D8P1 fixture exactly. The complete 1 MiB image is also compared
  outside the approved application and known update-record sectors.

The settings, preset and FM6 fixture commit records are manufactured with valid
headers, payload CRCs and differing sequences. This tests reader selection and
format reopening, **not ancillary writer atomicity**. The native pool uses its
actual writer. All sample pointers address the same simulated NOR as the loader.
Application read/boot caches are poisoned before both pre- and post-upgrade reads;
the application readers must perform no erase or program operation.

The actual loader fixture is kept in a separate translation unit,
`tests/ldr_ancillary_bridge.c`, to avoid substituting a simplified update loop or
colliding with the application fixture's static symbols. Both package inputs
come from cached private Dabbl8 diagnostic freezes, not vendor firmware.

## Local result — 19 October 2026

Two positive runs pass: optimized and fatal ASan/UBSan. Seven deliberate
post-upgrade header invalidations per mode also pass their expected refusal
oracles, for **14 invalidation controls**. Both committed headers are invalidated
for paired objects. Losing either preset bank must also prevent all 32 FM6 patch
bindings from reopening; the control explicitly expects that dependency. Other
unaffected groups must still reopen. Sample controls invalidate only the selected
slot header. These are intentional host controls, not device failures.

The loader, native codec and pool translation units use strict sanitizers.
Only the application fixture retains the established DSP signed-overflow/shift
exclusions; all remaining sanitizer findings are fatal. The final 16 cases pin
100 transitive source/header inputs and both package hashes before and after
execution. Private commands, compiler version, logs and results are under
`build/rc-ancillary-reopen-20261019/`.

The normal local host suite includes this fixture and its seven controls in both
modes. Its shell syntax is checked; the broad suite was not repeated for this
test-only extension. The selective runner correctly refuses these currently
unmapped paths; the focused compile/run commands were reviewed and recorded
explicitly, without a full-suite fallback or dependency download.

For a focused optimized reproduction, use existing generated headers and two
locally built Dabbl8 packages. Run from the repository root; the executable only
reads the packages and operates on in-memory NOR:

```sh
cc -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -w -DNPART=8 \
  -DFELUCCA_FM4=0 -DFELUCCA_SLICE=1 -Ibuild/gen -Ifirmware/src \
  tests/native_ancillary_upgrade_test.c tests/ldr_ancillary_bridge.c \
  firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c \
  -lm -o build/native_ancillary_upgrade_test
build/native_ancillary_upgrade_test OLD_DABBL8.fwsc NEW_DABBL8.fwsc
```

Append `--invalidate 0` through `--invalidate 6` for the seven refusal controls.
Use the strict-object sanitizer recipe in `tests/run_tests.sh` for that mode;
do not broaden the loader/codec/pool's sanitizer exclusions to match the DSP.

## Qualification limits

This covers the current settings layout, ordinary FM6 preset records and simple
sample zones. It does not qualify every historical own version, every preset
engine/grid layout, edited slice metadata, every sample-zone combination, physical
cold boot, power loss, flash-driver behaviour, timing or recovery. Existing wider
engine/effects, sample-upload and raw-range evidence remains separate and is
reused. Hardware ancillary upgrade/interruption acceptance remains open in #13
and #20. No acceptance checkbox is closed by these host checks.

The v0.2.3 diagnostic firmware, loader, application boundaries and frozen package
remain unchanged. These supplementary tests require no firmware rebuild or
additional owner SAVE/restore step. The prepared hardware batch remains unarmed.
