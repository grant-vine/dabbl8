# Native held ARP key mapping: bounded correctness evidence

This is a correctness backport proposal, not a release or performance optimization.
Functional production source is `362385768a4173f4133c592a1849622cc798971d`, based on
combined control `89a489da5581f0442cd587024c8ba65070434094`. The test-only descendant
`a403f2a308487170445c115f9c7c845dedeeb92f` trims the arrangement seed and adds actual
panel chord coverage and guarded runner entries. Firmware and build tools are byte-identical
between those two candidate commits. These reports do not borrow the parent's full-suite pass.

## Behavior

Eight-track ARP currently retains the absolute pitch captured on key-down. Changing
ROOT, SCALE, QNT or TRN therefore leaves held and HOLD-latched keyboard pitches stale.
The proposed native-only fix retains physical key, captured octave and captured chord
interval, then maps each surviving key again when its ARP step fires. MIDI notes,
DRUM-grid notes and engine-specific key maps retain absolute pitch semantics.

Original ORD/AORDER sorting and mapped duplicate rhythm slots remain unchanged.
REPEAT chooses the last surviving mapped key; a wholly silent remapped list releases
its previously sounding ARP note before indexing the list. The captured chord interval
remains as pressed: this does not re-harmonize the chord to a newly selected scale.
Physical octave buttons change the next key pressed, while already-held keys keep
their captured octave. No saved project layout or native storage authority changes.

## Executed host scope

- Optimized and inherited DSP ASan/UBSan: **23,864 checks, zero failures each**.
- Trimmed fixture alone preserves the earlier **22,584 checks**, including the original
  **512** mapping and lifetime assertions. The copied seed is now 50 lines.
- Sixteen native combinations (all eight tracks, HOLD off/on) execute actual native
  capture/adoption, panic processing, MIDI stale-history replacement, native pool
  preparation and two real master-bar boundaries. Row changes retain logical track
  history; full project adoption invalidates the live held count.
- Another 128 actual panel CHRD combinations add 1,280 checks: MAJ versus DIA3,
  HOLD off/on, ROOT/SCALE/QNT/TRN and all eight tracks. Poly/CLOSE triads exercise
  captured signed intervals, minor DIA3 versus major shape, actual ARP voice output,
  wholly silent QNT mapping and final release. Other voicings, sevenths and every
  high/low clamp combination are outside this new chord matrix.
- Adjacent actual native arrangement/held fixture: **625,904 checks** in both modes.
- Adjacent actual resident FX-tail fixture: **1,257,832 checks** in both modes, with
  600 real DSP/oracle blocks and 42 state kinds at 29 late mutation boundaries.
- Default four-track regression: **92 golden renders unchanged**, no health, voice,
  routing, CPU-budget failures or crashes. Two faster CPU notes are retained.

The DSP sanitizer scope inherits exclusions for signed-integer-overflow, shift,
bounds, object-size and pointer-overflow. It is not strict whole-firmware UB proof.
No new full suite, CI run, firmware installation or physical MIDI/keyboard test is claimed.

## Actual target and cost scope

The fresh pinned target builds were performed for functional source `3623857`.
No repeated target build is claimed for the test-only descendant. The environment
and 29 generated-input pins are retained here; loader and package were not built.

| Target | Image | RAM data + BSS | Pool | Result |
|---|---:|---:|---:|---|
| Default four | 447,952 bytes | 92,116 bytes | 331,332 bytes | Exact combined control image; static gates pass |
| Native eight | 480,220 bytes | 96,068 bytes | 335,240 bytes | +224 image bytes, +256 RAM bytes; timing gates remain failed |

Default four SHA-256 is
`a3a87f6fe62d427bfb0f7ebc8c3f5905717e8fa636c641bd94886240331ae096`, exactly the
fresh `89a489` control. This is preservation of the current combined four-track
source, not a claim of identity to pristine Felucca v1.1.5.
Native eight SHA-256 is
`0726552f3a3272a8a85a3aa3eb468946f3748cb1d541a4a82ad6523da09d7c60`.
Its parent image is 479,996 bytes (`997777974d234bb94847094373e5194e7a97e94f113b3ab142dea1dd466360e1`).
The logical history is 257 bytes (two 128-byte arrays and one byte); the actual
linked RAM section grows by 256 bytes because existing padding/layout changes.
RAM headroom is 2,236 bytes within the unchanged 98,304-byte section limit.

The same-source parent IRQ static proxy is **54,302**, and the candidate is
**55,722**: **+1,420 (+2.615%)**. Both fail the original 37,502 budget with its
unchanged 10% allowance. An initial private report incorrectly used the older
value 55,487 from another source; its exact receipt is preserved privately,
and `measurement-correction.json` records the corrected retained-disassembly audit.
Only `seq.c` differs among compiled firmware/build inputs; all 29 generated inputs
match the same-source control. The UAC48 proxy remains **555/504**, above the exact
554.4 limit; no rounding is used to claim a pass. ROOM 233/218 and SPRING 135/126
pass their original 10% allowances.

The new reachable `kb_pitch` has 71 linked instructions, a 20-byte fixed frame and
two divides; it can run for up to 16 mapped keys per track on an ARP firing. It was
not hidden with `noinline` or omitted from a performance claim. The standard IRQ
proxy is not a complete cycle model of these reachable helpers.

The IRQ's own fixed frame remains 492 bytes. The named direct-call path analysis
is 936 versus 924 bytes, and the keyboard-map direct path is 48 versus 36 bytes.
Those figures cover **only linked named direct calls**, not all indirect engine,
ROM, tail-call or nested TIMER5 paths. They are not a physical stack bound or a
high-water measurement. Actual audio deadlines, queued DAC/USB completion and
hardware behavior remain unqualified; issue #12's timing gates remain open.

## Failures and limits retained

Initial private setup logs preserve a nested-main inclusion error, missing pool
link inputs, and incorrect reference filenames before any target compiler ran.
The first added scale-key fixture left one track's default drum engine selected,
causing five expected-mapping failures; both raw failure logs are retained. The
fixture now explicitly selects the intended synth engine rather than accepting
engine-specific drum mappings as scale-key output.

The previous 312-check negative fixture has 64 real mapping failures on source
`9803388`. Its sequencer source is byte-identical to the `89a489` control, but its
whole build is not presented as a combined-control build. The new 23,864-check
fixture is a positive regression, not a matching negative run.

The native target also emits two unused-local warnings for the original `arp_step`
`i`/`o` declarations; cleanup was deliberately omitted to keep production source
frozen. An existing volatile-alias warning in capture binding is inherited.

All virtual NOR setup is explicit host test setup, never device ownership proof.
No physical flashing, loader change, boundary change, golden rewrite, release,
or installation claim is included. Four guarded runner lines are additive; the
full runner has not yet been executed for this proposal.
