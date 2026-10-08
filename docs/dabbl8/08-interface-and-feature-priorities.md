# Interface and feature priorities

The proposed on-device view separates eight tracks from four knobs. A dedicated mixer shows tracks 1–8 with selected-track highlighting, mute state, engine label and voice activity. Banked groups 1–4 and 5–8 keep four physical knobs useful, but a clear bank indicator is mandatory. Test a quick track-select layer using keys against existing chord, drum and FX shortcuts before choosing gestures.

Keep predictable controls: one gesture selects a track; one enters its engine parameters; one reaches its sequence; record targets are always visible. Add per-track keyboard octave only if it does not make project recall confusing. Separate performance mute, temporary solo and destructive sequence clear. Require confirmation and undo for destructive musical actions.

Priority zero is reliability: capability protocol, format migration, voice policy, diagnostics, storage bounds and recovery. Priority one is the eight-track workflow: routing, mixer, mutes, automation and compatible editor. Priority two is arrangement: bar-quantized pattern changes, limited scenes, fills, microtiming and focused undo. Scenes should store references plus mute/mix/transpose state, not unbounded duplicated songs.

Priority three is sonic enrichment: optional acid voice from X0X’s licensed dependencies; selected Jangada textures; FoMni-style chord/strum event generation; FM6 import robustness from SLOOP. Each candidate needs a source commit, exact files, license record, behavioral tests and target timing measurement. Ideas can be independently implemented where source reuse is unavailable or inappropriate.

Existing Felucca chord keys, drum synthesis, locks, ratchets, scales and performance FX should not be rebuilt simply because another project advertises them. Compare the intended user action against the baseline, identify the actual gap, then choose a small patch or new implementation. Avoid whole-fork merges because architecture, IDs and storage layouts have diverged.

Benchmark against Groove OS with original reference songs: an FM chord arrangement, a bass-and-drum groove, a dense mixed-engine arrangement and a live mute/fill performance. Record the same output conditions, levels and host settings where possible. Judge timing, arrangement speed, voice-steal audibility, patch independence, recall and recovery. Dabbl8’s potential differentiator is open, extensible eight-track multi-engine operation—not a claim that it already sounds better.
