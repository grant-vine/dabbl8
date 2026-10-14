<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Eight-track emulator workflows

The browser wrapper compiles the actual UI, sequencer, mixer, native codecs,
first-use formatter, local save consent, presets/settings and autosave against
simulated NOR. Cold boot is read-only. The real first-use or save confirmation
can open the corresponding virtual-NOR session; no device is accessed.

Use cached `build/gen` headers and an already installed Emscripten, Node and C
compiler. The explicit mode sets eight tracks, Dabbl8 identity, FM4 off and SLICE
on. During first-use release/formatting the browser shows a progress overlay and
disables panel controls; raw competing input still revokes native formatting. It uses `dabbl8-emu-flash-v1`, separate from upstream saves, and does not
request MIDI access or expose an unavailable installer.

```sh
sh web/emu/build.sh --dabbl8 0.1.1
sh web/emu/check-eight.sh
```

The focused runner writes logs, JSON results, real first-use/named/autosave NOR
snapshots, RGB565 screen captures and interleaved audio under
`build/emu-workflows/evidence`. It never downloads dependencies, generates
headers, builds vendor firmware, changes golden hashes or invokes hardware.
`EMU_OUT_DIR` changes the build destination without changing artifact names.

The eight-track workflow boots blank and presses/releases SAVE to initialize.
It selects every track through ALGORITHM, loads ANALOG through the EDIT layer,
edits independent levels through both GLO banks and records distinct notes on
stopped STEP pages through REC and the keyboard. It exercises all three named
project slots, load, overwrite, cancellation, playback save refusal, renewed
save consent after reboot, real stopped autosave and restoration in new WASM
instances. Sector retention uses the browser's actual `storeSectors` and
`restoreSectors` with the page's base64 JSON round trip. A separate ancillary
worklet scenario checks queued SAVE, published execution/completion phases,
retained sector reboot and fail-closed competing input/restart. A separate ancillary
scenario saves/loads a T8 preset and retains a MENU palette change after reboot.

The eight-active audio fixture is explicitly manufactured with one note per
track on sixteen steps. It establishes simultaneous eight-track activity,
nonzero deterministic audio, the shared eight-voice cap, and 88,200-frame native
versus WASM equality. It is separate from the actual panel-recorded song.
Host/WASM renders do not establish FM-1 deadlines, memory limits, flash recovery
or release/hardware safety.

## Reference for subsequent fixes

Use commit `04ba2469bed14847de51adb471d74159d2ee3614` as the behavioural
baseline. Its eight-track WASM is 766,901 bytes with SHA-256
`f421d0dc336e051f9e9cff970c05c7845e1dfaee96617af5b86524b8ff5d054e`.
The local evidence is preserved under
`.local-baseline/emulator-workflows-20261013/`: `source/build/emu-workflows/evidence/`
holds scripted results and immutable pre-fix audio captures; `evidence/root-qa.md`
and `evidence/gate-review.md` record actual browser use and independent approval.
Archived consent captures are in `evidence/team-archive/artifacts/consent-colours/`.

For each fix:

1. Reproduce the affected failure with this reference or reuse its exact
   source/configuration-bound evidence. Keep reference binaries, NOR snapshots,
   audio and results intact. Write candidate outputs to a separate directory,
   for example `sh web/emu/check-eight.sh build/emu-candidate`.
2. Lock the fix with an affected regression that fails before and passes after.
   Reuse unchanged coverage; run the broader eight-track workflow runner when
   shared playback, input or persistence changes. Inspect local check selection
   first; an unmapped-path refusal requires explicit scoped coverage.
3. Preserve independent track edits and both banks, the eight shared-voice cap,
   all three named projects, deliberate save consent, cancellation/refusal,
   autosave and retained-NOR reboot. Exercise the real browser for visible changes.
4. For sound-preserving optimisations, compare candidate audio bit-for-bit with
   the retained reference using the same inputs and compiler flags. Keep
   manufactured eight-active audio separate from the panel-recorded song. Do not
   replace golden hashes, clear effects or drop engines to force a pass.
5. Check affected target cost, RAM and stack separately with cached tools. The
   target budget is a weighted instruction-growth model, not measured cycles;
   emulator speed is not an FM-1 deadline measurement. Compare matched target
   configurations and preserve failures without raising budgets.

At this reference, 42 workflow cases and seven first-use worklet cases passed.
Four target cost checks still failed: SLICE +14%, audio IRQ +49%, TIMER5 +14%
and UAC48 +12%. RAM data+BSS was 97,220/98,304 bytes; the pool used
335,672/344,064 bytes, leaving 8,392 bytes (only 200 beyond its 8,192-byte
reserve). These are starting observations, not clearance to flash.
The legacy full state-digest mismatch caused by added guard metadata remains
recorded; exact PCM equality is separate evidence, not a passing state hash.

Prioritise the audio interrupt cost investigation, then the remaining target
costs and stack bounds, while retaining this behaviour. Hardware storage/USB,
deadlines and recovery must still be qualified separately; no device write is
authorised by emulator success.

The upstream four-track defaults and `emu_test.mjs` assertions remain unchanged:

```sh
EMU_OUT_DIR=build/emu-upstream sh web/emu/build.sh
node web/emu/emu_test.mjs build/emu-upstream/felucca.wasm
node web/emu/worklet_default_test.mjs build/emu-upstream/felucca.wasm build/emu-workflows/evidence/worklet-upstream-four.json
node web/emu/build_mode_test.mjs build/emu-upstream build/emu-workflows build/emu-workflows/evidence/build-modes.json
```

The original suite assumes a four-track blank boot with no first-use page.
Running it on blank eight-track firmware preserves its failures; it is not the
eight-track workflow suite. The earlier `native_storage_check.c` synthetic
read-only boot scenarios remain available and retain their own expectations.
