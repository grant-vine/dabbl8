# Selective Felucca 1.6.1 backports — 19 October 2026

This development integration starts from the actual v0.2.2 working source,
preserved in commit `34c3b21`, rather than the older top-level checkout.
The original dirty checkout and private frozen v022 package/hardware evidence
remain unchanged. The changed source is **not hardware qualified**. No device
was accessed, flashed, reset or written during this integration.

Upstream is pinned to [Felucca 1.6.1](https://github.com/hugelton/Felucca/commit/a53ecf428c29c780108fd70a31b71a32a6b5c313).
Its earlier release commits contain the prerequisite fixes below; this is a
selective source port, not a merge of upstream's four-track firmware.

| Fix | Upstream origin | Dabbl8 adaptation |
| --- | --- | --- |
| Wrong-board package rejected before erase | `a53ecf4`, ota.c / ldr_core.c | Require existing FM-1 flash.bin length `0x93000`; refuse with -15/-14. Pre-staging refusal does not erase staging. Preserve the v021 native E5000–E7000 cleanup exclusion. No boundaries change. |
| Stable LARGE value font | `a53ecf4`, #251 | Cache the smallest face needed by the knob's complete formatted range. Keep current geometry, fonts, track-bank semantics and native scene-name rendering. Only existing pages are ported; no new FM6 operator pages. Keep new UI state out of merged DSP globals. |
| Stable automation rows | `9f6a885`, #199 | Sort by step with stable insertion order within each step. Keep 64 records, 16-bit native addresses, eight-track ownership and the existing stored format. |
| Bipolar endpoints | `9f6a885`, #199 | Display -64..63 as -100..+100 percent in parameter cards and the legacy mixer. Stored values and DSP remain unchanged. |
| Coalesced favourite/filter settings writes | `a4d7ca2`, #197 | Queue detents until the view changes or ten seconds of panel inactivity. Retain the actual native quiet/transport/session-gated settings writer and its retry/error handling. |
| No held LED row during erase | `1727537`, #236 | Blank the exact GPIO row bits at the shared app erase seam, covering native writers too. Expansion stays call-free in RAM; preserve all other GPIO bits. No scanner timing change. |
| Divider / legacy mixer redraw | `88f1030`, #181 | Fill around divider lines rather than clearing them first. Eight-track mixer already uses a composed canvas; retain that implementation. |
| BASS+ headroom | `88f1030`, #180 | Use upstream x1.5 harmonic gain instead of x3. Intentional audible change only in BASS+ mode; no golden hashes rewritten. Retain the DIST stationary-tail fix. |

Already present and retained: older-macOS audio-first USB descriptors
(`76b3e3c`), locked-GLO PLAY/STOP (`ebe1574`) and the native held-ARP mapping
work (`3623857`, `a403f2a`, `f1ac23d` lineage). The ARP port remains aware of
native adoption/arrangement transitions and absolute MIDI pitch.

## Exclusions and dependencies

- No wholesale `project.c`, `core.h`, `storage.c`, flash map or editor protocol
  replacement. Upstream's FUN10 projects, four slots, A/B autosave and FM6 bank
  at E7000 conflict with Dabbl8's D8P1 formats, three slots/shared autosave and
  native pool. Only Dabbl8's own upgrade semantics remain supported.
- No new INSERT/HALL/FLANGER/PHASER/THROW effects, drum kits, conditional steps,
  per-track MIDI routing, MIDI Learn/Program Change, custom palette, FM6 bank,
  expanded automation or other new musical/UI features. MOMENTARY's mixer fix
  depends on the absent MOMENTARY feature, so it is not imported.
- No second-core beta, mailbox, core stacks or linker partition changes.
  Eight sequencer tracks still share eight sounding voices and the current
  heavy-state pool; full engine mask remains `0x3ffd`, USB audio off.
- Upstream's new `firmware/engines` submodule is pinned at
  `c46e0086f69bdc26df1cd144dca9e683cc4531dd` (Felucca-Engines, MIT).
  It is not needed for these fixes: retain the existing in-tree engines,
  build inputs and GPL-3.0-only/dependency notices. No dependency downloads.
- Later upstream `5738617968a22b5d4c0d3470cfcc95559052f446` changes documentation
  and attribution, including on-screen credit text; those are recorded separately
  and not silently folded into the stable bugfix port or used to relicense
  Dabbl8's existing engine copies.

## Local verification

Reproduction evidence before fixes is retained privately in
`build/upstream-161/red/`. Wrong-board OTA/loader, BASS+ headroom, bipolar text,
automation row order, font-size changes, divider clearing, per-detent settings
writes and erase blanking all have failing controls. Imported speaker testing
also exposed a partial-last-block array overrun in its fixture; the sample copy
is now bounded. No sanitizer exclusion was added to hide it.

Focused runner (existing package is fixture data, never installed):

```sh
sh tests/check_upstream_fixes.sh /absolute/path/to/existing-v022.fwsc build/upstream-fixes
```

It covers four/eight-track UI, strict OTA/loader, eight-track speaker/settings
and native pool preservation, optimized and with fatal ASan/UBSan. DSP tests
retain only the existing signed-overflow/shift exclusions. Existing tests also
cover exact row-zero GPIO mapping and normal scan/debounce behavior.

The affected-check planner explicitly exposes otherwise-unmapped paths.
Explicit local scopes cover flash-driver, own-project-transfer, read-only boot,
save-consent, startup-order, engine-fx and performance-fx. The 84 engine/effects
cases pass; final HAL/startup checks pass separately after the target correction.

`web/emu/check-eight.sh build/upstream-161/emu-final` passes 42 eight-track
workflow cases, worklet first-use/cancellation, preset/settings persistence,
and exact native/WASM parity over 88,200 frames. The actual browser was exercised
through initialization, bank 5–8, mixer and retained-storage reload. Twelve
dark/light normal/LARGE HOME/mixer/roll images were inspected; the native brand
renderer passes 576 frames/1,344 checks with zero DMA errors.

The generic `ui_render.c` native-mode harness retains 48 incompatible legacy
splash-square/text findings and renders no tall scenes in that mode. Baseline
and candidate reports match byte-for-byte. It is not counted as a passing native
UI gate; the current native brand renderer, focused tests and real emulator
provide the relevant evidence. Failed harness/link attempts remain in the logs.

Using the existing pinned JieLi toolchain and cached amd64 Docker image, the
non-installable v0.2.3-timing application passes link/MMIO/RAM-code/static-budget
checks: image 527,328 B, RAM 94,812/98,304 B (3,492 free), pool
335,696/344,064 B (8,368 free). RAM code has no calls; budgets are unchanged.
The loader independently compiles and passes its RAM-only section check;
wrapped loader size is 6,950 B. No installable package or release was created.

Evidence, manifests, commands, failures and screenshots are under
`build/upstream-161/` and `build/app-only/full/`. Prior physical timing and
persistence results still apply only to their original v022 source/package.
Calibrated timing, complete stack bounds, independent recovery and new hardware
qualification remain open; a local pass does not make this an RC or authorize a
future flash.
