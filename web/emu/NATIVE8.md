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
