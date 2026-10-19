<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Combined arrangement coverage — local only

The four combinations below run the actual C audio ISR, sequencer, native
arrangement, MIDI parsers, effects and UI. They add coverage to the retained
84 engine/effects scenarios and42 emulator workflows. Production firmware,
golden audio hashes, memory limits and the frozen package are unchanged.

| Scenario | Playback and interaction | Result in optimized and fatal ASan/UBSan builds |
| --- | --- | --- |
| Internal, dry | 16 scenes/four banks, live USB notes, metronome and HOME/SONG/STEP/MIXER drawing | PASS; all16 scenes reached naturally |
| Internal, ROOM | Distortion, chorus, delay and ROOM; actual SONG fill knob, return and queued row knob; live USB notes and drawing | PASS; fill runs/returns and queued row applies |
| USB clock, SPRING | 16 scenes/four banks, distortion/chorus/delay/SPRING, live USB notes and drawing | PASS; all16 scenes reached naturally |
| TRS clock loss and rollover | ROOM combination, live TRS notes and drawing; stop incoming clock at eight simulated seconds with the millisecond counter crossing zero | PASS; transport stops before ten simulated seconds |

Every run reaches eight shared voices and all eight heavy-state slots using
six FM6 tracks, DRUM and WHEEL. All eight tracks gate, the mixed output is
audible, requested sends/wet buses become nonzero, live channel8 notes reach
track8, and no allocation failure is recorded. Matching note-off cleanup and
effect decay reach zero on both DAC halves within twelve simulated seconds.
Asynchronous LCD transfers complete without source-buffer overwrite.

Sources are prepared before playback. The test checks that no NOR callback is
called from the audio ISR and that all source storage bytes remain unchanged.
After Stop, the exact editable project matches its initial capture after undoing
only the track selection deliberately changed by the display exercise. It does
not normalize tempo, sound, pattern, automation or arrangement differences.

The first capture comparison exposed that deliberate selection change rather
than a firmware defect: only the selected-track byte and its two CRCs differed.
That failed run and the earlier fixture/recipe setup errors remain in private
`build/rc-arrangement-stress-20261019/`. Three isolated fixture-copy controls
drop live input, drawing or the fill request; each is rejected by its expected
assertion. No production source was altered for these controls.

The selective recipe is `ci/local_checks.py --check arrangement-stress` (inspect
the plan before `--run`). It uses the tracked minimal native fixture and existing
generated headers, without private backups, device adapters or downloads. Each
scenario starts in a new process. The sanitizer mode keeps the existing DSP
signed-integer-overflow/shift exclusions; bounds, lifetime and other undefined
behavior checks remain fatal.

Clocks, input arrivals, DMA and NOR are simulated. UI calls run between audio
halves; real interrupt preemption is not simulated. The fake timer cannot
measure renderer cost, ISR deadlines or arrival latency. This is neither
physical routing nor a complete stack bound or visual-quality review.

## Consequences for the next hardware batch

`arrangement-proposal` now prepares the missing synthetic C source, with all
four banks referencing C exclusively and16 scenes. It passes actual native C
recall, the physical-input SEQ-hold/SONG/PLAY path, natural automatic completion,
eight voices/eight heavy slots, exact project/storage preservation and bounded
silence in optimized and fatal sanitizer modes. Both export the same7,385 bytes,
SHA-256 `25ce16e8ae7b6fab1d5b4020df95e41fd0b1df6525c735adb698ed6ce54dbe61`.
There is no new production command or automatic remote source preparation.

The private combined archive preserves A/B from the previously verified original
set, retains the existing heavy MIDI fixture in live/shared autosave, and puts
this arrangement in C. Native codec/virtual-pool rebuild/readback validation
passes. It is not a fresh backup or an armed restore. Keep private A/B and the
archive out of Git, and compare a fresh original capture before any future use.
Reuse receipts under `build/rc-arrangement-proposal-20261019/`; do not repeat the
earlier eight combined stress runs for this new scope.

Retain this host evidence and the completed v022 persistence/upgrade results.
Do not add another stopped MIDI-only run as proof of arrangement playback.
The native arrangement requires main-loop source preparation before Start;
merely sending MIDI Start after an ordinary restore does not prepare it.
The current guarded hardware helper admits matched notes, not a new remote
arrangement-control protocol. Keep its allowlist unchanged.

The proposal's owner action is to load C, hold SEQ to open SONG, and press PLAY
once; it stops itself. Group this with the existing SAVE/restart visit. Validate
the exact capture/Stop/readback orchestration before arming it. C recall changes
the current workspace and may update shared autosave: compare those to the new
C context, while A/B/C and ancillary data remain protected. After the final
original restore, require exact original logical readback.

Exact-candidate timing, clock/cadence uncertainty, omitted ISR work,
SP/SSP/interrupt nesting and independent recovery remain open. No device access,
saved-data write, restore staging or installation occurred for this test.
