# Eight-track UI transition verification

These checks strengthen the host evidence for #10. They do not qualify a firmware release or satisfy the physical MIDI and memory gates in #9/#12. No firmware was installed. The firmware runtime and audio golden files are unchanged by this PR.

`tests/quick_layer_conflicts_test.c` now exercises every one of the 64 source/destination track pairs. A melodic chord remains on the original track while selection changes, then releases without held gates on any track. A global solo retains its original bank destination until release. An FX mute retains its original owner without becoming a played note. These are 192 held-key cases, including transitions within and between both banks. The test explicitly chooses a melodic preset: the factory fourth track is a drum track and cannot supply the three-note chord fixture.

Another 64 cases cover every drum track/lane pair. A black lane key followed by a white step key writes exactly one lane on the selected track. Other tracks' step hits and all track mutes remain unchanged. Existing record-arm, chord, drum and selected upper-bank automation tests still run.

`tests/banked_mixer_test.c` retains its full palette/style/font-size/track/screen layout sweep. It additionally checks the rendered text in mixer level, mixer pan, active global and held FX screens: the selected bank label must appear exactly once and the opposite bank label must be absent. This adds 1,280 semantic checks to the 46,720 rendered frames. An absent label fails even when all text fits.

Both files already run in the optimized and sanitizer sections of `tests/run_tests.sh`; no runner insertion is required. [Pinned results and commands](evidence/2026-10-10-ui-transitions/manifest.json) record the source parent, generated-asset hashes and output hashes. Apple clang 21.0.0 passed both modes. Sanitizers use the existing upstream DSP exclusions; this is not a claim of unrestricted UBSan coverage. The expanded tests do not measure target cycles, IRQ latency, stack or engine pool high-water usage. The complete suite was not duplicated locally for this test-only change.

## Physical qualification worksheet

Run this only after the device/install/restore approval and recovery gates are satisfied. Record exact hardware revision, source commit, firmware SHA-256, sample/preset set and supported engine/effect combination before each workload. Keep original project and autosave snapshots outside Git.

1. Connect USB and TRS independently, then together. Record all eight destinations, alternating ingress ports and releasing notes on the other port. Repeat in selected-track routing while changing banks with notes and sustain held. Compare captured notes and releases with the intended track; inspect each track's recorded steps.
2. Repeat the chord/solo/FX ownership matrix and all drum lanes on the physical panel. Check selected track, bank, REC destination, engine and mute indication by eye on both banks. Include short taps, long holds and rapid ALGORITHM turns while keys are held. Capture the input sequence and any mismatch, rather than reporting a general “UI pass.”
3. During eight-track playback and recording, continuously move through mixer, drum roll, motion, FM6 algorithm, quick layers and palettes. Use the actual maximum supported engine/sample/effect combination, and incoming chords exceeding eight sounding voices to exercise steals. Record voice budget, stuck-note/ownership errors, clipping and audible discontinuities.
4. Measure audio deadline misses and maximum audio callback/IRQ duration with the target's measurement instrumentation. Record sample rate, block size, duration, cycle counter method and worst observed value; compare it with the block deadline. Host render counts and desktop elapsed time cannot substitute for these measurements.
5. Record minimum remaining stack/canary margin including nested IRQ paths, engine pool high-water, allocation failures and display/workspace ownership violations during the same workload. Linker totals and isolated function frames are separate evidence and cannot establish these bounds.
6. Repeat stopped save/load/autosave transitions after the physical persistence backend and migration are integrated. Include three projects, shared autosave, filled samples and refusal while transport is active. Document intentional skips and every failure. Do not close #9, #10 or #12 until their complete acceptance gates have independent evidence.

This worksheet defines measurements still required. It does not assert those counters or device procedures have been implemented or executed.
