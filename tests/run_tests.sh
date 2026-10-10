#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Host tests of the Felucca sources (no hardware). Run from the repo root after ./build.sh:
#   tests/run_tests.sh
#
# Regression suite (tests/regress.c, tests/target_budget.py; details at the top of regress.c):
#   golden renders  every engine x preset, the GM map on DRUM, voice modes, FX sends, a 4-track mix: one hash
#                   each in tests/golden.txt. A change of the sound fails with the list of renders.
#   health          clipping, DC, peak level, voices free after the release, silence at the end.
#   CPU             instructions / sample per preset and mix (tests/cpu_baseline.txt, +25 %), ns printed;
#                   target: loop instructions of the render functions in the pi32v2 disassembly
#                   (tests/target_budget.txt, +10 %; exact, static).
#   voices          the budget of 8, steal fades, MONO / LEGATO / UNISON keep their note, the VOICE cap,
#                   no hanging notes on any MIDI / key routing.
# UI renders (tests/ui_render.c): every screen in every palette from the real drawing code: the layout lint (no text
#                   off the screen, cut, hidden, overlapping or spilling out of its cell / card; only free text
#                   ellipsised), GREY gray, MONO neutral, the draw cost, the text audit (build/ui_new/text_audit.tsv);
#                   PNGs of GREY MONO GREEN PAPER NIGHT in build/ui_new (tests/ui_render.py), the findings in build/ui_new/report.txt;
#                   FM6's 32 algorithm charts as drawn (no box overlapping, no route through a box or crossing another);
#                   DIGITAL's screens (its algorithm charts, OP ENV) with FELUCCA_FM4=1 too (build/ui_fm4: lint, GREY, MONO);
#                   every frame of the rolling digits (lint, GREY, MONO), their filmstrips in build/ui_slot;
#                   the alignment: every text / icon / keycap meant to be centred, or on its neighbours' line, by its
#                   ink against its box (cells, chips, buttons, rows, knobs, the roll's strip, the keycaps' pills), every
#                   screen in FLAT and LINE and every palette, over every value it can show; 1 px off or more fails
#                   (build/ui_new/align.txt). MENU > LARGE: every screen again (FLAT, LINE, every palette) and the page / value
#                   sweep with the tall cards, the same lint and alignment; sheet_LARGE_GREY.png, sheet_LARGE_MONO.png.
# UI (tests/ui_test.c): the UI sources against stub display / buttons / knobs: sound loads keep the steps and
#                   the track's ARP / SCL / SLICER, the SEQ > PATTERNS loader and its REPLACE? dialog, the
#                   one-step undo of both (SAVE held), REC on TRACKS / SEQ / ARP, STEP and ARP while recording,
#                   MIDI IN ROUT, saves refused while playing and the OVERWRITE? dialog, MUTE on TRACKS KNOB 1,
#                   the DRUM grid (keys, knobs, LEDs, pages, live recording into it, BEAT from PATTERNS).
# Audio / persistence / editor: the real C paths against simulated DMA and NOR flash: bounded overload
#                   fades, shared-voice limits, deferred settings and retries, failed-save rollback, the autosave
#                   (1.2: when it writes, the power-on restore, damaged copies, a write cut short, wear over a session),
#                   malformed transfers, transport-stop timeouts, MIDI and UART recovery; the MENU settings over the
#                   editor (MENU_DESC / MENU_SET: every item, clamping, unknown ids, saving, USB SERIAL applied later).
# CHORD (tests/chord_test.c): the chord keys (src/chord.c): diatonic triads / sevenths of several scales and roots,
#                   the fixed shapes and voicings (at most 4 notes), names, MONO plays the root, a release ends
#                   exactly what its key / MIDI note started, recording, the ARP, MIDI IN, kits ignore CHRD.
# RATCH (tests/ratchet_test.c): a step's ratchet (x1..x4): its parts in the sequencer (equal, gated, chords and drum
#                   hits whole, one chance roll, swing, no slide or tie out, STOP), FUN8 round trip and older projects x1,
#                   user preset patterns, SEQ > CHANCE KNOB 3 and the roll / grid drawing.
# MOD (tests/mod_test.c): the modulation matrix: slots that do nothing are bit-identical, every source on each
#                   kind of destination, clamping, MIDI CC1 / CC11 / aftertouch routing, the cost of 4 active
#                   slots (at most +5 %), demos in build/mod_demo/.
# PERFORM (tests/perform_test.c): the FX hold layer (src/perform.c): 1/16 starts, stereo buffer effects, the
#                   too-long REPEAT, the SLICER interplay, silent layer keys, idle bit-identical, cost; build/perform_demo/;
#                   OCT UP / DN (the harmonizer): pitch, stereo, clicks, the shimmer bounded, cost; build/fx_demo/.
# REVERB (tests/reverb_test.c): REVERB TYPE (src/fx.c): ROOM bit for bit as before, SPRING's decay against SIZE,
#                   its chirp (group delay rising with frequency), stability at the corners, level, a model change
#                   without a click, its cost against ROOM (+30 % at most); demos in build/fx_demo/.
# SLICE (tests/slice_test.c): slice tables, AUTO onsets of a user-slot loop, reverse, keys, modes, the MAN slices
#                   (SLICES page) and their store in the slot (src/slice_store.c); the presets and the loop;
#                   demos in build/slice_demo/.
# INPUT (tests/input_test.c): the key / button debounce of hal/fm1_input.h against the TIMER5 scan and bouncing
#                   contacts: a press within 2 scans (<= 2.3 ms), one note per bouncy press, no early or hanging
#                   release, stray samples ignored, fast repeats, the encoders' detents; the LED scan: lit LEDs every
#                   frame, dim ones a short pulse (the second line write) every frame, each only on its own column;
#                   the breath (#119): dark .. ~60 % of lit (DIM LO ~30 %), smooth, no dark run over ~10 ms near its peak;
#                   the power-on sweep (1.1, hal/fm1_led_anim.h): its length, every LED every frame at its level, the
#                   head left to right with its tail, the buttons, the end on the idle glow, the tick as before after.
# CLICK (tests/click_test.c, 1.1): the metronome (src/click.c) and the count-in (src/seq.c) through audio.c: each beat at
#                   the first sample of its block, with a 1/16 track's steps 1 5 9 13 (internal and external clock, DIV 1/8
#                   and SWING too), the accent, the frequencies, length, LEVEL, MASTER, OFF / REC / ON, USB audio bit for bit
#                   without it; COUNT-IN 1 / 2 BARS (step 1 exactly N bars after PLAY, STOP cancels, PLAY again nothing,
#                   never with an external clock), notes in its last eighth onto step 1; build/click_demo/.
# USB audio (tests/uac_test.c): the UAC1 descriptors as a host parses them (with and without CDC; 44100 and
#                   48000 Hz), the ring and packetiser: 44.1 frames per packet, every frame in order (bit for bit),
#                   underrun / overrun, restart; SET_CUR; the 44.1 -> 48 kHz resampler (every frame against a
#                   double-precision one, response, SNR, cost); the stream at 48 kHz (47..49 frames per packet,
#                   fast and slow I2S clocks, switching rates mid-stream).
# web (web/test_web.mjs): the editor protocol against its mock device, whose tables must equal the
#                   firmware's (tests/descdump.c -> build/host/desc.json; the MENU settings: tests/editor_test.c -> build/host/menu.json),
#                   the package builder, the updater.
# Browser emulator (web/emu, when emcc is there): the firmware in WebAssembly (build/emu) boots, plays keys and MIDI,
#                   draws, lights its LEDs, keeps a save across instances, plays a song bit for bit as the same file
#                   built with cc (web/emu/native_check.c); the cost of 1 s of a heavy song against real time.
# PHYS (tests/phys_test.c): stability over the whole parameter and pitch range, the worst-case cost against
#                   the heaviest factory preset, demos in build/phys_demo/; tests/phys_ref.cpp compares the
#                   fixed-point models with DaisySP's float originals when DaisySP is there (DAISYSP=path).
# DRUM (tests/drum_test.c): the drum voices (src/drum_voice.c): pitch, decay, centroid and level against
#                   Felucca's targets, the controls' directions, no clipping, DC, retriggers, the hat choke, the
#                   kick on a small speaker; the DRUM engine (src/eng_drum.c): its key map, the 8 lanes together,
#                   one hit per lane, the choke between lanes; the cost per voice; demos in build/drum_demo/.
# NOISE (tests/noise_test.c): the engine (src/eng_noise.c): COLR's slope (white, pink, brown), the filter and the
#                   register clock following the key, META periodic at the key, no DC, no clipping at the
#                   corners, a note from silence the same twice, the cost per voice; demos in build/noise_demo/.
# DIGITAL -> FM6 (tests/fm4_test.c, built with FELUCCA_FM4=1): the retired four-operator engine against its conversion
#                   (src/fm4_convert.c): routes and carriers per algorithm, the presets' PTCH, and the sound (pitch,
#                   centroid, RMS envelope) of its presets and algorithms; demos in build/fm4_demo/. tests/digital_test.c
#                   (FELUCCA_FM4=1 too): DIGITAL's operator envelopes. Default builds have no DIGITAL (engine 1 reserved).
#                   tests/fm4_div0_test.c (UBSan, #61): the conversion without a divide that can be 0 gives the values the
#                   guarded form gave (every INDEX x FLT ENV, random sounds); a 0.9 project with DIGITAL ORGAN converts.
# FM6 (tests/fm6_test.c): the 6-operator FM engine (src/eng_fm6.c, src/fm6_core.c): the 32 algorithms' carriers, the
#                   operator envelopes (stages, rates, the voice ending), bit-stable notes, a click-free retrigger, no DC /
#                   clipping over the factory patches, the macros' directions, PTCH, pack / unpack and the SysEx
#                   layouts, the 6-voice cap, the cost per voice; demos in build/fm6_demo/.
# ROBUST (tests/robust_test.c): damaged or crafted stored data and editor requests: a SLICE scan bounded by the slot,
#                   a slot that fails its check leaves no zone, engine numbers past the last refused, a retained older
#                   RAM project bounded, user preset patterns inside their fields, malformed requests never stop PLAY.
# Sanitizers (ASan + UBSan, when the compiler has them; SANITIZE=0 skips): the stored-data and protocol tests again
#                   (loader, M-UPGRADE entry, editor, projects, backup, ROBUST) and three short fuzz runs with fixed
#                   seeds: tests/fuzz_ed.c (editor SysEx and raw USB-MIDI packets), tests/fuzz_proj.c (mutated project
#                   stores -> import -> restore -> render), tests/fuzz_smp.c (user sample slot headers -> scan -> SAMPLE /
#                   GRAIN / SLICE). Longer runs: build/host/asan/fuzz_ed 300000 7 (iterations, seed), the same for the others.
#                   UBSan leaves out the DSP's intended wraps (signed overflow, shifts) and the XIP rebase of a user zone's
#                   offset (bounds, object-size, pointer-overflow: correct on the device's flat flash, not in C's model).
# Change baseline entries only for reviewed, intentional differences in sound or cost;
# retain every unaffected golden / CPU / target entry. VERBOSE=1: every render.
set -e
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"
CC="${CC:-cc} -O1 -Wall -Wno-unused-function"
fail=0
run() { echo "== $1"; shift; "$@" || fail=1; }

$CC -o "$OUT/storage_test" tests/storage_test.c
run "flash storage (A/B, torn writes)" "$OUT/storage_test"

$CC -o "$OUT/native_storage_gate_test" tests/native_storage_gate_test.c
run "eight-track refusal of all legacy application object writers" "$OUT/native_storage_gate_test"

$CC -o "$OUT/upreset_test" tests/upreset_test.c
run "user presets (UP_PUT parser, bank round trip, versions, PHYS DRUM and SAMPLE PERC -> DRUM, grid records, DIGITAL kept)" "$OUT/upreset_test"

$CC -o "$OUT/input_test" tests/input_test.c
run "keys and buttons: fast press, long release, bouncy contacts (one note each), glitches, encoders" "$OUT/input_test"

$CC -o "$OUT/midi_uart_test" tests/midi_uart_test.c
run "TRS MIDI parser" "$OUT/midi_uart_test"

HALF=$(sed -n 's/^#define HALF_FRAMES \([0-9]*\).*/\1/p' firmware/src/core.h)
$CC -O2 -DT_CDC=1 -DHALF_FRAMES=$HALF -o "$OUT/uac_test" tests/uac_test.c -lm
run "USB audio input: descriptors (with CDC), ring and packets, 44.1 -> 48 kHz resampler, 48 kHz stream" "$OUT/uac_test"
$CC -O2 -DT_CDC=0 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_nocdc" tests/uac_test.c -lm
run "USB audio input: descriptors (without CDC), ring and packets, resampler, 48 kHz stream" "$OUT/uac_test_nocdc"
# USB descriptor layouts (#67): CDC UAC LAYOUT CDC-presented 48K; layout 0 and the console left out = 1.0's bytes
# (+ the 48 kHz rate with 48K = 1, the default; 48K = 0: byte for byte)
for v in 1.1.0.1.1 1.1.1.1.1 1.1.2.1.1 1.1.3.1.1 1.1.0.0.1 1.1.2.0.1 0.1.0.1.1 1.1.0.1.0 1.1.0.0.0 1.1.2.1.0 0.1.0.1.0 \
         1.0.0.1.1 1.0.1.1.1 1.0.0.0.1 0.0.0.1.1; do
    IFS=. read -r t_cdc t_uac t_lay t_on t_48 <<EOF
$v
EOF
    $CC -DT_CDC="$t_cdc" -DT_UAC="$t_uac" -DT_LAYOUT="$t_lay" -DT_ON="$t_on" -DT_48K="$t_48" -o "$OUT/usb_desc_test" \
        tests/usb_desc_test.c
    run "USB descriptors: CDC $t_cdc (presented $t_on), UAC $t_uac (48 kHz $t_48), layout $t_lay" "$OUT/usb_desc_test"
done

[ -f build/felucca.fwsc ] || { echo "run ./build.sh first"; exit 1; }

$CC -DOWN_PKG=1 -o "$OUT/ota_test" tests/ota_test.c
run "M-UPGRADE entry (own loader)" "$OUT/ota_test" build/felucca.fwsc

head -c 200000 build/felucca.bin > "$OUT/old_app.bin"
python3 tools/fm1pkg_make.py "$OUT/old_app.bin" build/loader/ota.bin "$OUT/old.fwsc" >/dev/null
$CC -o "$OUT/ldr_test" tests/ldr_test.c
run "update loader: other app -> this build" "$OUT/ldr_test" "$OUT/old.fwsc" build/felucca.fwsc

# D8P1 is a standalone proposed-file codec, not a firmware storage adoption path.
run "D8P1 independent synthetic reference files" python3 tests/d8p1_fixtures_test.py
run "Dabbl8 card configurations: all subsets and bounded CLI" python3 tests/d8card_test.py
${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/d8card_test" firmware/src/d8p1.c firmware/src/d8card.c tests/d8card_test.c
run "Dabbl8 card dependencies and non-mutating project preflight" "$OUT/d8card_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
${CC%% *} -std=c11 -O1 -Wall -Wextra -Werror -o "$OUT/d8p1_test" firmware/src/d8p1.c tests/d8p1_test.c
run "D8P1 bounded byte codec and malformed input refusal" "$OUT/d8p1_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/d8store_test" firmware/src/d8p1.c firmware/src/d8store.c tests/d8store_test.c
run "D8P1 relative multi-sector records and every program-byte cut" "$OUT/d8store_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/d8pool_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/d8pool_test.c
run "D8P1 three projects, shared autosave and interrupted pool writes" "$OUT/d8pool_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/d8pool_mapped_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8pool_mapped_test.c
run "D8P1 mapped existing allocations, session policy and physical cut guards" "$OUT/d8pool_mapped_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p

${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/dabbl8_project_archive" firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_project_archive.c
run "offline native project-set archive: exact originals, sparse roles and whole-set readback" python3 tests/native_project_archive_test.py "$OUT/dabbl8_project_archive"

if [ -f build/gen/felucca_tables.h ]; then
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/dabbl8_project_convert" firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm
    run "offline D8P1 bundle: original snapshots, migration reports and verified references" python3 tests/project_conversion_test.py "$OUT/dabbl8_project_convert"
    ${CC%% *} -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src -o "$OUT/dabbl8_pool_initialize" firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_pool_initialize.c
    run "offline migration: complete originals, persisted autosave and real shared-pool proposal" python3 tests/migration_bundle_test.py "$OUT/dabbl8_project_convert" "$OUT/dabbl8_pool_initialize"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/d8p1_project_test" firmware/src/d8p1.c tests/d8p1_project_test.c -lm
    run "D8P1 native state and all frozen legacy round trips" python3 tests/d8p1_project_test.py "$OUT/d8p1_project_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/d8p1_motion_policy_test" firmware/src/d8p1.c tests/d8p1_motion_policy_test.c -lm
    run "D8P1 motion bytes and actual upstream eligibility" "$OUT/d8p1_motion_policy_test" tests/fixtures/d8p1/maximum.d8p
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/descdump" tests/descdump.c -lm
    echo "== parameter and engine tables as JSON (for the editor mock test)"
    "$OUT/descdump" > "$OUT/desc.json" || fail=1
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/hostsim" tests/hostsim.c -lm
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/quick_layer_conflicts_test" tests/quick_layer_conflicts_test.c -lm
    run "eight-track chord, DRUM, SCL and selected automation ownership" "$OUT/quick_layer_conflicts_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/banked_mixer_test" tests/banked_mixer_test.c -lm
    run "eight-track bank controls, quick-layer ownership and all screen layouts" "$OUT/banked_mixer_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/versioned_track_test" tests/versioned_track_test.c -lm
    run "versioned eight-track editor: bounds, ownership, busy errors and legacy refusal" "$OUT/versioned_track_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/track_client_bridge8" tests/track_client_bridge.c -lm
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/track_client_bridge4" tests/track_client_bridge.c -lm
    run "companion track client: real C four/eight-track negotiation, edits and stale reply guards" node web/test_d8tracks.mjs "$OUT/track_client_bridge8" "$OUT/track_client_bridge4"
    run "companion MIDI session: framing, lifecycle and actual C handlers" node web/test_d8midi.mjs "$OUT/track_client_bridge8" "$OUT/track_client_bridge4"
    run "companion editor model: musical schema and all-track actual C controls" node web/test_d8companion.mjs "$OUT/track_client_bridge8" "$OUT/track_client_bridge4"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/eight_track_runtime_test" tests/eight_track_runtime_test.c -lm
    run "eight-track recording, playback, USB/TRS routing and bounds" "$OUT/eight_track_runtime_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/voice_budget_test" tests/voice_budget_test.c -lm
    run "eight actual parts: ninth note, held/sustain/release, stealing and render budget" "$OUT/voice_budget_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/phys_pool_test" tests/phys_pool_test.c -lm
    run "PHYS eight-part shared state: ownership, model/engine changes, retriggers and full-pool refusal" "$OUT/phys_pool_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/fm6_pool_test" tests/fm6_pool_test.c -lm
    run "FM6 eight-part operator state: ownership, retriggers, release and full-pool refusal" "$OUT/fm6_pool_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/slice_pool_test" tests/slice_pool_test.c -lm
    run "SLICE eight-part reverse windows: ownership, exact decode and render isolation" "$OUT/slice_pool_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/heavy_pool_test" tests/heavy_pool_test.c -lm
    run "GRAIN/PHYS/DRUM eight-part shared state: mixed lifetimes and engine fades" "$OUT/heavy_pool_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/wheel_pool_test" tests/wheel_pool_test.c -lm
    run "WHEEL eight-part shared voice state: phase, percussion, retrigger and ownership" "$OUT/wheel_pool_test"
    $CC -O2 -w -DNPART=8 -Ibuild/gen -Ifirmware/src -o "$OUT/main_workspace_test" firmware/src/d8p1.c tests/main_workspace_test.c -lm
    run "eight-part display/project workspace: asynchronous DMA and drawing ownership" "$OUT/main_workspace_test" tests/fixtures/projects/fun9.bin
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/d8p1_runtime_test" firmware/src/d8p1.c tests/d8p1_runtime_test.c -lm
    run "D8P1 stopped runtime adoption: full state, retained metadata and late-start refusal" "$OUT/d8p1_runtime_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_signature_test" firmware/src/d8p1.c tests/native_signature_test.c -lm
    run "native coherent canonical dirty signature: arrangement, music and UI exclusions" "$OUT/native_signature_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_signature_equivalence_test" firmware/src/d8p1.c tests/native_signature_equivalence_test.c -lm
    run "native signature optimization: canonical wire and pre-normalization refusals" "$OUT/native_signature_equivalence_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_project_menu_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/native_project_menu_test.c -lm
    run "actual native three-slot project menu/cache/ownership" "$OUT/native_project_menu_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/d8p1_pool_runtime_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/d8p1_pool_runtime_test.c -lm
    run "D8P1 actual runtime shared-pool save, recall and autosave policy" "$OUT/d8p1_pool_runtime_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/d8p1_flash_runtime_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8p1_flash_runtime_test.c -lm
    run "actual runtime through guarded existing-driver adapter" "$OUT/d8p1_flash_runtime_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/eight_track_performance_test" tests/eight_track_performance_test.c -lm
    run "eight-track solo: actual banked gestures, all masks, dry/send isolation and cold gain" "$OUT/eight_track_performance_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_fx_tail_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_fx_tail_test.c -lm
    run "native resident tails: real DSP counter oracles, silent echo gaps and late writes" "$OUT/native_fx_tail_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_fx_counter_boundary_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_fx_counter_boundary_test.c -lm
    run "native FX publication: actual nested TIMER5 and complete post-block counters" "$OUT/native_fx_counter_boundary_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_output_queue_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_output_queue_test.c -lm
    run "native logical output queues: actual DMA/USB service, drain and late writes" "$OUT/native_output_queue_test"
    $CC -O2 -w -DFELUCCA_UAC_TONE=1 -Ibuild/gen -Ifirmware/src -o "$OUT/native_output_queue_tone_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_output_queue_test.c -lm
    run "native output queues: autonomous USB benchmark refuses autosave" "$OUT/native_output_queue_tone_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_autosave_integration_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_integration_test.c -lm
    run "native signature and output-queue integration: natural drain/save/restore" "$OUT/native_autosave_integration_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_quiet_autosave_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_quiet_autosave_test.c -lm
    run "native automatic write: quiet guard at every physical mutation" "$OUT/native_quiet_autosave_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_autosave_session_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_session_test.c -lm
    run "native autosave session: committed reconciliation, idle/wear/retry/boot and logical queue gates" "$OUT/native_autosave_session_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/instrument_capture_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_capture_test.c -lm
    run "read-only instrument capture: actual handler, fixed stores, current state and interruption" "$OUT/instrument_capture_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/instrument_write_quarantine_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_write_quarantine_test.c -lm
    run "pre-migration quarantine: actual boot, application writers, binding and read-only capture" "$OUT/instrument_write_quarantine_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_migration_preflight_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/native_migration_preflight_test.c -lm
    run "readonly migration staging: complete native set and exclusive LCD arena" "$OUT/native_migration_preflight_test"
    run "instrument capture collector: actual C bridge and hostile replies" python3 tests/instrument_capture_collector_test.py "$OUT/instrument_capture_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/native_autosave_combined_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_combined_test.c -lm
    run "native combined autosave: actual main-loop hold/queue/drain/save/restore" "$OUT/native_autosave_combined_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/scale_test" tests/scale_test.c -lm
    run "scales: white-key mapping and note lifecycle" "$OUT/scale_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/chord_test" tests/chord_test.c -lm
    run "chord keys: diatonic and fixed chords, voicings, MONO root, releases, recording, ARP, MIDI IN, kits" "$OUT/chord_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/speaker_test" tests/speaker_test.c -lm
    run "SPEAKER EQ: FLAT / LOWCUT / BASS+ responses, BASS+ harmonics of the bass, the sub cut, no offset after" "$OUT/speaker_test"
    run "DSP render (ANALOG preset 0)" "$OUT/hostsim" 0 0 1 "$OUT/render.wav"
    mkdir -p build/tracks_demo
    run "TRACKS: 4-track pattern, live recording (lengths, swing), voice budget, engine switch, cost" env TRACKS=build/tracks_demo "$OUT/hostsim" 0 0 1 "$OUT/tracks.wav"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/project_test" tests/project_test.c -lm
    run "project formats (FUN1..FUN5 -> FUN6, the grid and song chain; DIGITAL tracks -> FM6, SAMPLE PERC -> DRUM)" "$OUT/project_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/legacy_fixture_test" tests/legacy_fixture_test.c -lm
    run "immutable FUN1..FUN9 imports and canonical round trips" "$OUT/legacy_fixture_test" tests/fixtures/projects
    $CC -O1 -w -DLEGACY_DEST_TRACKS=8 -Ibuild/gen -Ifirmware/src -o "$OUT/legacy_fixture_test_8" tests/legacy_fixture_test.c -lm
    run "historical layouts/imports with an eight-track destination; refuse FUN9 truncation" "$OUT/legacy_fixture_test_8" tests/fixtures/projects
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/dabbl8_import_preview" tools/dabbl8_import_preview.c -lm
    run "offline legacy import preview: preserve originals, report migrations and refuse unknown data" python3 tests/offline_import_test.py "$OUT/dabbl8_import_preview"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/motion_v1_test" tests/motion_v1_test.c -lm
    run "D8M1: all 512 addresses and locks, version/range/duplicate/truncation refusal" "$OUT/motion_v1_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/motion_test" tests/motion_test.c -lm
    run "motion, whole-step chance, FUN7 migration, song restore, ARP repeat and the 1.2 ARP modes" "$OUT/motion_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/ratchet_test" tests/ratchet_test.c -lm
    run "RATCH: x1..x4 in a step (notes, chords, drum hits), gates, chance, swing, projects, user presets, CHANCE page" "$OUT/ratchet_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/midi_control_test" tests/midi_control_test.c -lm
    run "USB/TRS clock, bend, sustain, ownership and panic recovery" "$OUT/midi_control_test"
    $CC -O1 -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/digital_test" tests/digital_test.c -lm
    run "DIGITAL (retired, built here with FELUCCA_FM4=1): operator envelopes/levels" "$OUT/digital_test"
    $CC -O2 -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/fm4_test" tests/fm4_test.c -lm
    mkdir -p build/fm4_demo
    run "DIGITAL -> FM6: the conversion against DIGITAL (FELUCCA_FM4=1): pitch, centroid, RMS envelope; demos" \
        "$OUT/fm4_test" build/fm4_demo
    $CC -O2 -w -fsanitize=integer-divide-by-zero -fno-sanitize-recover=all -Ibuild/gen -Ifirmware/src \
        -o "$OUT/fm4_div0_test" tests/fm4_div0_test.c -lm
    run "DIGITAL -> FM6 without a divide by zero (#61): the same values as before, a 0.9 ORGAN project (UBSan)" \
        "$OUT/fm4_div0_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/theme_test" tests/theme_test.c -lm
    run "themes: contrast, text blending and font metrics" "$OUT/theme_test"
    $CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/text_ref_test" tests/text_ref_test.c -lm
    run "text: pens, kerning and pixels equal the reference renderer" "$OUT/text_ref_test"
    if python3 -c "import PIL" 2>/dev/null; then
        run "text spacing: glyph gaps against the font's own (16x supersampled), S M <= 0.25 px, L <= 0.5 px" \
            python3 tests/text_spacing_test.py build/gen/ui_fonts.h
    fi
    $CC -O1 -w -Ibuild/gen -o "$OUT/settings_test" tests/settings_test.c
    run "settings: PER1..PER4 migration, palette ids and preference preservation" "$OUT/settings_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/ui_test" tests/ui_test.c -lm
    run "UI: sounds keep steps, undo, recording, MIDI overflow, pending saves, panel recovery, drum grid, song chain, GREY gray, MONO neutral" "$OUT/ui_test"
    FV=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)   # (the splash, ABOUT: the real version)
    $CC -O1 -w "-DFELUCCA_VERSION=\"$FV\"" -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/ui_render" tests/ui_render.c -lm
    mkdir -p build/ui_new/ppm build/ui_slot
    run "UI renders: layout lint (every screen and palette, every page, engine and column value), GREY gray, MONO neutral, alignment by ink (1 px fails), draw cost" \
        "$OUT/ui_render" build/ui_new build/ui_slot
    if python3 -c "import PIL" 2>/dev/null; then python3 tests/ui_render.py build/ui_new build/ui_slot; fi
    $CC -w -DFELUCCA_FM4=1 -Ibuild/gen -Ifirmware/src -o "$OUT/ui_test_fm4" tests/ui_test.c -lm
    run "UI built with FELUCCA_FM4=1 (DIGITAL, kept in the tree): its OP pages, EDIT cycle, algorithm charts" "$OUT/ui_test_fm4"
    $CC -O1 -w -DFELUCCA_FM4=1 "-DFELUCCA_VERSION=\"$FV\"" -Ibuild/gen -Ifirmware/src -Itests -o "$OUT/ui_render_fm4" tests/ui_render.c -lm
    mkdir -p build/ui_fm4/ppm build/ui_fm4_slot
    run "UI renders with FELUCCA_FM4=1: DIGITAL's screens (EDIT, OP ENV, the 8 algorithm charts), lint, GREY gray, MONO neutral" \
        "$OUT/ui_render_fm4" build/ui_fm4 build/ui_fm4_slot
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/audio_test" tests/audio_test.c -lm
    run "audio: overload protection, bounded fades and DMA diagnostics" "$OUT/audio_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/click_test" tests/click_test.c -lm
    run "metronome and count-in: beats sample for sample (internal and external clock), accent, level, MASTER, not in USB, count-in timing, notes onto step 1" "$OUT/click_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/persistence_test" tests/persistence_test.c -lm
    run "persistence: deferred settings, retry and failed-save rollback" "$OUT/persistence_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/backup_test" tests/backup_test.c -lm
    run "full backup: CRC before writes, stale runtime, USB reset / timeout, malformed objects, older projects" "$OUT/backup_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/editor_test" tests/editor_test.c -lm
    run "editor: real C protocol, malformed transfers, queue recovery, MENU settings (writes build/host/menu.json)" \
        env MENU_JSON="$OUT/menu.json" D8CAPS_JSON="$OUT/d8caps.json" D8INFO_JSON="$OUT/d8info.json" "$OUT/editor_test"
    $CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/robust_test" tests/robust_test.c -lm
    run "robustness: crafted sample slots, engine numbers, retained old projects, preset patterns, malformed requests" \
        "$OUT/robust_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/mod_test" tests/mod_test.c -lm
    mkdir -p build/mod_demo
    run "modulation matrix: off = bit-identical, the math, MIDI CC1 / CC11 / aftertouch, cost, demos" "$OUT/mod_test" build/mod_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/slicer_test" tests/slicer_test.c -lm
    mkdir -p build/slicer_demo
    run "SLICER: no clicks, timing, sync with the sequencer, STUT, cost, demos" "$OUT/slicer_test" build/slicer_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/swing_test" tests/swing_test.c -lm
    run "SWING: track + global at most 100, sequencer and SLICER step lengths, the SWG display" "$OUT/swing_test"
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/perform_test" tests/perform_test.c -lm
    mkdir -p build/perform_demo build/fx_demo
    run "FX layer effects: on the 1/16, stereo, too-long REPEAT, SLICER, silent keys, idle bit-identical, clicks, OCT UP / DN, cost, demos" "$OUT/perform_test" build/perform_demo build/fx_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/reverb_test" tests/reverb_test.c -lm
    run "REVERB TYPE: ROOM bit-identical, SPRING decay / chirp / stability / level, model change, cost, demos" "$OUT/reverb_test" build/fx_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/regress" tests/regress.c -lm
    run "regression: golden renders, health, voices, CPU budget" "$OUT/regress" tests/golden.txt tests/cpu_baseline.txt
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/phys_test" tests/phys_test.c -lm
    mkdir -p build/phys_demo
    run "PHYS: stability C-1..G9 over the parameter corners, worst-case cost against PHASE WIRE, demos" "$OUT/phys_test" build/phys_demo
    D=${DAISYSP:-vendor/DaisySP}/Source
    if [ -d "$D/PhysicalModeling" ] && command -v c++ >/dev/null 2>&1; then
        $CC -O2 -w -Ibuild/gen -Ifirmware/src -Itests -c -o "$OUT/phys_fixed.o" tests/phys_fixed.c
        c++ -O2 -std=c++14 -w -I"$D" -I"$D/Utility" -o "$OUT/phys_ref" tests/phys_ref.cpp \
            "$D/PhysicalModeling/modalvoice.cpp" "$D/PhysicalModeling/resonator.cpp" "$D/PhysicalModeling/stringvoice.cpp" \
            "$D/PhysicalModeling/KarplusString.cpp" "$D/Filters/svf.cpp" "$D/Utility/dcblock.cpp" \
            "$D/Dynamics/crossfade.cpp" "$OUT/phys_fixed.o"
        run "PHYS: the fixed-point models against DaisySP's float originals (mode frequencies, decays, Svf)" "$OUT/phys_ref"
    else
        echo "== skip PHYS reference test (no DaisySP: set DAISYSP to a checkout)"
    fi
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/drum_test" tests/drum_test.c -lm
    mkdir -p build/drum_demo
    run "DRUM: voice targets, controls, no clipping, retrigger, hat choke, the kick on a small speaker, keys, 8 lanes, cost, demos" "$OUT/drum_test" build/drum_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/noise_test" tests/noise_test.c -lm
    mkdir -p build/noise_demo
    run "NOISE: colour slopes, key-tracked filter and clock, META period, DC, clipping, retrigger, cost, demos" "$OUT/noise_test" build/noise_demo
    $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fm6_test" tests/fm6_test.c -lm
    mkdir -p build/fm6_demo
    run "FM6: algorithms, envelopes, retrigger, DC, clipping, macros, patch formats, voices, cost, demos" "$OUT/fm6_test" build/fm6_demo
    # SLICE is in the standard build (firmware/src/core.h): its test always runs (after #22 by andreahaku)
    if grep -q '^#define SLC_BREAK_BPM ' build/gen/felucca_samples.h; then
        mkdir -p build/slice_demo
        python3 tests/slice_loop.py build/slice_demo/loop
        python3 tools/fm1_sample_upload.py build LOOP build/slice_demo/loop build/slice_demo/loop.wav:60 >/dev/null
        $CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/slice_test" tests/slice_test.c -lm
        run "SLICE: tables, onsets, reverse, keys, modes, MAN slices and their store, demos" "$OUT/slice_test" \
            build/slice_demo/loop build/slice_demo
    else
        echo "== SLICE: build/ was made with FELUCCA_SLICE=0 (no BREAK); run ./build.sh without it first"
        fail=1
    fi
else
    echo "== skip hostsim (run ./build.sh once)"
fi

# ASan + UBSan: the stored-data and protocol paths, and short fuzz runs (fixed seeds: the same inputs every run)
SAN="-O1 -g -fsanitize=address,undefined -fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow"
SAN="$SAN -fno-sanitize-recover=undefined -w"
san_ok() {
    printf 'int main(void){return 0;}\n' > "$OUT/san_probe.c" &&
        ${CC%% *} $SAN -o "$OUT/san_probe" "$OUT/san_probe.c" 2>/dev/null && "$OUT/san_probe"
}
if [ "${SANITIZE:-1}" = 0 ]; then
    echo "== skip the sanitizer runs (SANITIZE=0)"
elif [ ! -f build/gen/felucca_tables.h ]; then
    echo "== skip the sanitizer runs (run ./build.sh once)"
elif ! san_ok; then
    echo "== skip the sanitizer runs (the compiler has no ASan / UBSan)"
else
    mkdir -p "$OUT/asan"
    A="$OUT/asan"
    SCC="${CC%% *} $SAN -Ibuild/gen -Ifirmware/src"
    export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
    # Standalone codec uses full ASan/UBSan, without the upstream DSP exclusions.
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/d8p1_test" firmware/src/d8p1.c tests/d8p1_test.c
    run "ASan/UBSan: D8P1 strict bounded byte codec" "$A/d8p1_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/d8card_test" firmware/src/d8p1.c firmware/src/d8card.c tests/d8card_test.c
    run "ASan/UBSan: strict card dependencies and project preflight" "$A/d8card_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/d8store_test" firmware/src/d8p1.c firmware/src/d8store.c tests/d8store_test.c
    run "ASan/UBSan: strict relative multi-sector storage and exhaustive cuts" "$A/d8store_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/d8pool_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/d8pool_test.c
    run "ASan/UBSan: strict shared pool cuts, rotation and refusal" "$A/d8pool_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p tests/fixtures/d8p1/unknown-optional.d8p
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/d8pool_mapped_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8pool_mapped_test.c
    run "ASan/UBSan: mapped physical bounds, session revocation and cuts" "$A/d8pool_mapped_test" tests/fixtures/d8p1/minimal.d8p tests/fixtures/d8p1/maximum.d8p
    $SCC -o "$A/dabbl8_project_convert" firmware/src/d8p1.c tools/dabbl8_project_convert.c -lm
    run "ASan/UBSan: offline D8P1 bundle and original/reference preservation" python3 tests/project_conversion_test.py "$A/dabbl8_project_convert"
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/dabbl8_pool_initialize" firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_pool_initialize.c
    run "ASan/UBSan: original-preserving migration and real shared-pool proposal" python3 tests/migration_bundle_test.py "$A/dabbl8_project_convert" "$A/dabbl8_pool_initialize"
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -Ifirmware/src -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/dabbl8_project_archive" firmware/src/d8p1.c firmware/src/d8pool.c tools/dabbl8_project_archive.c
    run "ASan/UBSan: strict native project-set archive and whole-set readback" python3 tests/native_project_archive_test.py "$A/dabbl8_project_archive"
    $SCC -o "$A/d8p1_project_test" firmware/src/d8p1.c tests/d8p1_project_test.c -lm
    run "ASan/UBSan: D8P1 native state and all frozen legacy round trips" python3 tests/d8p1_project_test.py "$A/d8p1_project_test"
    $SCC -o "$A/d8p1_motion_policy_test" firmware/src/d8p1.c tests/d8p1_motion_policy_test.c -lm
    run "ASan/UBSan: D8P1 motion bytes and actual upstream eligibility" "$A/d8p1_motion_policy_test" tests/fixtures/d8p1/maximum.d8p
    $SCC -o "$A/ldr_test" tests/ldr_test.c
    run "ASan/UBSan: update loader (other app -> this build)" "$A/ldr_test" "$OUT/old.fwsc" build/felucca.fwsc
    $SCC -DOWN_PKG=1 -o "$A/ota_test" tests/ota_test.c
    run "ASan/UBSan: M-UPGRADE entry (own loader)" "$A/ota_test" build/felucca.fwsc
    for t in editor_test project_test backup_test robust_test; do
        $SCC -o "$A/$t" tests/$t.c -lm
        run "ASan/UBSan: $t" "$A/$t"
    done
    for t in versioned_track_test quick_layer_conflicts_test banked_mixer_test eight_track_runtime_test voice_budget_test phys_pool_test fm6_pool_test slice_pool_test heavy_pool_test wheel_pool_test; do
        $SCC -DNPART=8 -o "$A/$t" tests/$t.c -lm
        run "ASan/UBSan: actual eight parts $t" "$A/$t"
    done
    $SCC -DNPART=8 -o "$A/track_client_bridge8" tests/track_client_bridge.c -lm
    $SCC -o "$A/track_client_bridge4" tests/track_client_bridge.c -lm
    run "ASan/UBSan: companion client through actual C four/eight-track handlers" node web/test_d8tracks.mjs "$A/track_client_bridge8" "$A/track_client_bridge4"
    run "ASan/UBSan: companion MIDI session through actual C handlers" node web/test_d8midi.mjs "$A/track_client_bridge8" "$A/track_client_bridge4"
    run "ASan/UBSan: companion editor model through actual C handlers" node web/test_d8companion.mjs "$A/track_client_bridge8" "$A/track_client_bridge4"
    $SCC -o "$A/dabbl8_import_preview" tools/dabbl8_import_preview.c -lm
    run "ASan/UBSan: offline import preservation, migrations and refusal" python3 tests/offline_import_test.py "$A/dabbl8_import_preview"
    $SCC -DNPART=8 -o "$A/main_workspace_test" firmware/src/d8p1.c tests/main_workspace_test.c -lm
    run "ASan/UBSan: eight-part asynchronous display/project workspace" "$A/main_workspace_test" tests/fixtures/projects/fun9.bin
    $SCC -o "$A/d8p1_runtime_test" firmware/src/d8p1.c tests/d8p1_runtime_test.c -lm
    run "ASan/UBSan: D8P1 stopped runtime adoption and full refusal preservation" "$A/d8p1_runtime_test"
    $SCC -o "$A/native_signature_test" firmware/src/d8p1.c tests/native_signature_test.c -lm
    run "ASan/UBSan: native coherent canonical dirty signature" "$A/native_signature_test"
    $SCC -o "$A/native_signature_equivalence_test" firmware/src/d8p1.c tests/native_signature_equivalence_test.c -lm
    run "ASan/UBSan: native signature canonical wire equivalence and refusals" "$A/native_signature_equivalence_test"
    ${CC%% *} -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$A/native_storage_gate_test" tests/native_storage_gate_test.c
    run "ASan/UBSan: strict eight-track application object write refusal" "$A/native_storage_gate_test"
    $SCC -o "$A/native_project_menu_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/native_project_menu_test.c -lm
    run "ASan/UBSan: actual native project menu/cache/ownership" "$A/native_project_menu_test"
    $SCC -o "$A/d8p1_pool_runtime_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/d8p1_pool_runtime_test.c -lm
    run "ASan/UBSan: actual native runtime shared-pool persistence and refusals" "$A/d8p1_pool_runtime_test"
    $SCC -o "$A/d8p1_flash_runtime_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/d8p1_flash_runtime_test.c -lm
    run "ASan/UBSan: actual runtime through atomic physical adapter" "$A/d8p1_flash_runtime_test"
    $SCC -o "$A/eight_track_performance_test" tests/eight_track_performance_test.c -lm
    run "ASan/UBSan: eight-track solo dry/send isolation and cold first activation" "$A/eight_track_performance_test"
    $SCC -o "$A/native_fx_tail_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_fx_tail_test.c -lm
    run "ASan/UBSan: resident DSP tails and every late mutation refusal" "$A/native_fx_tail_test"
    $SCC -o "$A/native_fx_counter_boundary_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_fx_counter_boundary_test.c -lm
    run "ASan/UBSan: actual audio/TIMER5 counter publication boundaries" "$A/native_fx_counter_boundary_test"
    $SCC -o "$A/native_output_queue_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_output_queue_test.c -lm
    run "ASan/UBSan: actual logical audio/USB queues and late refusal" "$A/native_output_queue_test"
    $SCC -DFELUCCA_UAC_TONE=1 -o "$A/native_output_queue_tone_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_output_queue_test.c -lm
    run "ASan/UBSan: autonomous USB benchmark refuses autosave" "$A/native_output_queue_tone_test"
    $SCC -o "$A/native_autosave_integration_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_integration_test.c -lm
    run "ASan/UBSan: native signature and natural output-drain integration" "$A/native_autosave_integration_test"
    $SCC -o "$A/native_quiet_autosave_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_quiet_autosave_test.c -lm
    run "ASan/UBSan: automatic write quiet guard and late activity" "$A/native_quiet_autosave_test"
    $SCC -o "$A/native_autosave_session_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_session_test.c -lm
    run "ASan/UBSan: native autosave session canonical reconciliation and policy" "$A/native_autosave_session_test"
    $SCC -o "$A/instrument_capture_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_capture_test.c -lm
    run "ASan/UBSan: read-only instrument capture and actual USB/storage interruption" "$A/instrument_capture_test"
    $SCC -o "$A/instrument_write_quarantine_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/instrument_write_quarantine_test.c -lm
    run "ASan/UBSan: actual pre-migration application write quarantine" "$A/instrument_write_quarantine_test"
    $SCC -o "$A/native_migration_preflight_test" firmware/src/d8p1.c firmware/src/d8pool.c tests/native_migration_preflight_test.c -lm
    run "ASan/UBSan: complete migration preflight and exclusive LCD arena" "$A/native_migration_preflight_test"
    run "ASan/UBSan: instrument collector through actual instrument bridge" python3 tests/instrument_capture_collector_test.py "$A/instrument_capture_test"
    $SCC -o "$A/native_autosave_combined_test" firmware/src/d8p1.c firmware/src/d8pool.c firmware/src/d8pool_mapped.c tests/native_autosave_combined_test.c -lm
    run "ASan/UBSan: combined actual main-loop autosave routes" "$A/native_autosave_combined_test"
    $SCC -o "$A/fuzz_ed" tests/fuzz_ed.c -lm
    run "ASan/UBSan fuzz: editor SysEx and raw USB-MIDI packets (20000, seed 7)" "$A/fuzz_ed" 20000 7
    $SCC -o "$A/fuzz_proj" tests/fuzz_proj.c -lm
    run "ASan/UBSan fuzz: project stores -> import -> restore -> render (5000, seed 13)" "$A/fuzz_proj" 5000 13
    $SCC -o "$A/fuzz_smp" tests/fuzz_smp.c -lm
    run "ASan/UBSan fuzz: user sample slot headers -> scan -> SAMPLE / GRAIN / SLICE (1000, seed 17)" "$A/fuzz_smp" 1000 17
fi

run "regression: target cost of the render loops (pi32v2 disassembly)" python3 tests/target_budget.py \
    build/felucca.dis tests/target_budget.txt

run "installer CLI (fm1_install.py) against a simulated FM-1" python3 tests/install_test.py

if command -v node >/dev/null 2>&1; then
    run "web pages: editor protocol + samples, package builder, update protocol" node web/test_web.mjs
    run "web backup: capture, validation before writes, restore order" node web/test_backup.mjs
else
    echo "== skip web tests (no node)"
fi

if ! command -v emcc >/dev/null 2>&1; then
    echo "== skip the browser emulator (no emcc: Emscripten builds web/emu)"
elif ! command -v node >/dev/null 2>&1 || [ ! -f build/gen/felucca_tables.h ]; then
    echo "== skip the browser emulator (needs node and build/gen)"
else
    run "browser emulator: the firmware to WebAssembly (web/emu/build.sh -> build/emu)" sh web/emu/build.sh
    cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -o "$OUT/emu_native" web/emu/native_check.c -lm
    run "browser emulator: the same song from the native build" "$OUT/emu_native" "$OUT/emu_native.f32"
    run "browser emulator: boot, keys, MIDI, screen, LEDs, PLAY, a save kept across instances, = native, cost" \
        node web/emu/emu_test.mjs build/emu/felucca.wasm "$OUT/emu_native.f32"
fi

[ $fail -eq 0 ] && echo "ALL HOST TESTS PASSED" || { echo "HOST TESTS FAILED"; exit 1; }
