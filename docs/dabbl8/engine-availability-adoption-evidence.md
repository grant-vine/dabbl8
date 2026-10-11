# Engine availability: used project/native adoption evidence

Source commit: `6cfef34a7b1960caa8adf8f3394bcb3c16b2ac48`, based on
`48254c2c4c2e7fdf6a9d679fea32322568b0c7ce`. Local host checks on 2026-10-11:
Apple clang 21.0.0 (`clang-2100.1.1.101`), arm64 macOS; Python 3.14.8.
This is a completed project/native adoption slice of issue #53 stage 2,
**not complete engine-card runtime integration or a qualified firmware profile**.

## Implemented behaviour

Production project and D8P1 adoption use the immutable compiled-engine mask.
The compatibility adapter retains implemented DIGITAL with FM4 enabled and
handles the existing SLICE switch without compacting IDs. Historical conversions
check their effective sounding targets. Missing engines refuse before structured
workspace borrowing/normalization or native-wire LCD fencing, and before
runtime/cache/patch/undo/transport mutation. Normalized projects are checked again
before publication. Both initial engines and late DEF_SOUND/KEEP aliases must be
available. Native previews follow real structural/reference validation and do not
replace subsequent semantic policy. A known missing-engine result does not claim
that all other semantic/resource/asset checks would pass.

Internal static mask-argument functions exercise refusal on the host. Production
wrappers always use the full compiled mask. Candidate/effective-mask override
build guards remain closed. There is no reduced firmware, engine/state exclusion,
new device capability or changed exact 15-byte schema-1 response.

## Executed checks

Normal builds use `-O2`; adoption sanitizers use existing `tests/run_tests.sh`
ASan/UBSan flags at `-O1`, including existing DSP exclusions for signed overflow,
shift, bounds, object size and pointer overflow. These exclusions were unchanged.
The standalone policy ran strict ASan/UBSan without those exclusions.

| Tracks | FM4 | SLICE | Proofs per run | Normal | ASan/UBSan |
| --- | --- | --- | --- | --- | --- |
| 4 | 0 | 0 | 277 | PASS | PASS |
| 4 | 0 | 1 | 280 | PASS | PASS |
| 4 | 1 | 0 | 277 | PASS | PASS |
| 4 | 1 | 1 | 280 | PASS | PASS |
| 8 | 0 | 0 | 472 | PASS | PASS |
| 8 | 0 | 1 | 455 | PASS | PASS |
| 8 | 1 | 0 | 472 | PASS | PASS |
| 8 | 1 | 1 | 455 | PASS | PASS |

Every adoption fixture snapshots actual sounding tracks/voices, song, motion,
chain and native cache, FM6 patches/generations, undo/UI, pattern/project metadata,
panic/transport/count-in and LCD DMA/fences. Cases include missing engines,
invalid masks/IDs, DIGITAL and PHYS/GM/PERC migration, late default/drum aliases,
normal stored SAMPLE parameters, original structured and borrowed wire inputs,
and encoded sentinel/DIGITAL/absent-SLICE policy. The ordinary immutable production
wrappers also succeed. Initial, expanded and final follow-up matrices passed (5,936 assertions in the
final 16 runs). The follow-up explicitly checks track eight alone on both
structured/native wire paths and PHYS revision-0 DUST-to-MODAL-bowed/DRUM
normalization. Two private modified-candidate negative controls compiled and
failed as expected: moving project guards after publication tripped the track
snapshot; removing the early native-wire preview tripped pending LCD DMA/fence
preservation. These are modified-candidate controls, not pristine-parent tests
(the parent lacks the mask seam). No unexpected compiler or fixture failures
occurred; two failed follow-up orchestration attempts (receipt key and interleaved
command pairing) were corrected without source/threshold changes and retained
in the private orchestration note.

Selected existing regressions passed: historical project tests, native stopped
runtime adoption and late-start preservation, native pool runtime, four-track
editor and versioned eight-track editor. Strict policy: 2,662,146 assertions PASS.
Build guard: 36 checks PASS. Bash syntax and whitespace checks PASS. The final
matrix reran the follow-up adoption fixture; all other source hashes were verified
unchanged from the passing regression run, so no repeat of those regressions is
claimed.

## Tested source and generated-input hashes

| Source | SHA-256 |
| --- | --- |
| `firmware/src/engines.c` | `154deb6dc6f1f3ab3f4c25da88d96be936d66df3ef451e07df017003027e5868` |
| `firmware/src/project.c` | `da93d328d5adec2cbcbf6c785e2e62806a103726d52b7b7e17761f1e7fcdb4df` |
| `firmware/src/d8p1_runtime.c` | `e5d3f90c18a4e05aa5d00d6d599bef61a4d85e4d081ef6e06adc3d3153df4c05` |
| `firmware/src/d8engine_policy.c` | `faea936acbe145a0f75dc1acaf3c8809b7de57ef87df3eb9e78d11a596a68160` |
| `firmware/src/d8engine_policy.h` | `d79f0161fbdff29404e0e67e29bc4536831a5eff5a7549af63dbc21e076971e5` |
| `firmware/src/d8p1.c` | `ade527da0f804e695b01b1fb7c3296e3c6e04f0a0d12e78b989818da301ac3ec` |
| `tests/engine_adoption_test.c` | `0b50ec457fd4f951907bd487c6ac92d80bd9cc3e79369545e66c2719fc89d920` |
| `tests/run_tests.sh` | `2a2a8218241e96be04f4e9b95a4076ee4ee5a85b8c4c5d0fa0303f0e9a87bc8e` |

Generated host inputs were borrowed read-only from the existing
`uac-array-bounds/build/gen`, with these exact hashes; no generation/download was
performed for this check.

| Generated input | SHA-256 |
| --- | --- |
| `ui_keycaps.h` | `7194c54438deeeb77d91fcbe5bae361155e2b883ffefff359910d883441a9008` |
| `felucca_tables.h` | `a975289289c52b9f9fd2ff985a147b199f8a3afa9fb416e95f99cb83131993cb` |
| `ui_fonts.h` | `ccec7fcead654ccd6a279f56db4d74e4e62f275e92a13714006788568b43ed54` |
| `felucca_fm6.h` | `d1859b8194443ecbe099505909d6ea4db866d919100c81d738bb06d56004f791` |
| `felucca_samples.h` | `d72405f100494dce4c3bff53de584423b0ab96cf9b196a85487b13206ae032bc` |
| `ui_icons.h` | `ff211885beaa45b6dc7046b32d018d71aa444e9d8a75300f64aac6f7979d6882` |
| `ui_palettes.h` | `e87e29243c6129e8bd238cae5b185ca0357483f6a1787f78f63ecf54ce8dad05` |

Full source/log/binary manifests and raw logs are retained locally at
`/private/tmp/d8-engine-availability-adoption/followup-v18otfj1/receipt.json`
(with raw expected-failure logs and mutated control sources), the historical
`/private/tmp/d8-engine-availability-adoption/final-1obe632t/receipt.json`,
and the referenced earlier
`/private/tmp/d8-engine-availability-adoption/run-sh1qdn8a/receipt.json`.
These local paths are evidence locations, not firmware downloads.

## Remaining gates and limits

UI factory/user/favourite adoption, undo invariants, filtered UI/editor selection,
safe startup/added-track defaults and separate negotiated availability capability
remain incomplete. Referenced-project compatibility during arrangement preparation
also needs explicit policy and preservation semantics. Keep the compile-time
profile guard closed until every publication path is integrated and proven.

No target app/ELF/package, target memory/stack report, golden audio suite, physical
audio/MIDI timing or hardware/recovery qualification was executed here. Native
cold-load checks add code and bounded preview work; target image/stack/budget and
unchanged-default/golden verification remain required before release. No loader,
flash boundary, budget, golden, device, release or website changes were made.

## Root integration and target verification

The initial agent lane above did not execute target builds. Root separately
preserved pinned target builds of production source `6cfef34` against foundation
`48254c2`: four-track app size increases by 700 bytes and eight-track by 740
bytes; static RAM and audio pool remain unchanged (92,116/331,332 bytes for four
tracks; 96,068/335,240 for eight). Four-track static budgets pass; eight-track
static budgets still fail. These are costs of the adoption guards, not memory
savings or hardware timing qualification.

After integrating foundation `54a7874` and its current-main USB correction, root
reran all sixteen host adoption configurations at exact union `47da5e6`: all
5,936 assertions passed. Raw logs, source hashes, target receipts and independent
static review are committed under
`docs/dabbl8/evidence/2026-10-11-engine-availability-adoption/`.
The target measurements above predate this USB integration; they do not qualify
the final union target image, package, stack, golden suite or device timing.
Full final-source CI and hardware/recovery gates remain pending. Higher-level
load wrappers still report generic invalid-project errors for a missing engine;
the distinct internal refusal reason is not a negotiated editor capability.
