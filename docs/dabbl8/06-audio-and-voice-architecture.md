# Audio and voice architecture

Separate TrackState from VoiceSlot and EngineState. TrackState owns persistent parameters, sequence references, MIDI route and mix settings. A sounding VoiceSlot identifies its owner track, note, generation and engine state. EngineState comes from a bounded pool appropriate to the engine. Eight track identities should not require eight permanently allocated copies of every large engine.

Stage one preserves eight shared sounding voices and existing sonic behavior. Define a deterministic allocator: reclaim released voices first; consider optional unison voices next; use documented track priority or note age for remaining steals; protect a bass/lead only within a bounded policy. Drum reservation or priority is configurable, not an absolute guarantee that causes unlimited allocation. Track muting and engine switching must release or transfer ownership safely.

Stage two explores Melodee-style cost units, calibrated on this firmware and hardware. Engine cost depends on active voices, modulation, filtering, FX, USB and interrupt work. Use per-engine caps and a total budget. The first release must not advertise twelve or sixteen FM voices just because another fork does. Expanded FM polyphony is an experiment with an explicit pass/fail gate.

Observed audio blocks contain 128 frames, giving roughly 2.9 ms at the nominal 44.1 kHz engine rate. Current source sheds workload after sustained high usage. A proposed engineering target is to keep representative worst-case render time at or below 75 percent of the block deadline, leaving headroom for nested input work. This is a target, not a measured result. Capture median, p95, p99 and maximum time, late blocks, voice steals and heap/pool high-water marks.

Do not allocate, erase flash, decode large assets, draw UI or perform unbounded parsing in the audio callback. Apply control changes at block boundaries through bounded commands. Timing tests must include rapid encoder changes, MIDI bursts, USB streaming, long release tails and simultaneous drums. Host instruction counts are regression signals, not measured FM-1 CPU percentages.

Keep stereo USB audio for the first release. Multichannel stems are attractive but require descriptor, packet-bandwidth, resampling, interrupt and macOS compatibility measurements. Agree whether stems are pre-FX or post-FX before implementation. Bluetooth is not inherited: Felucca does not enable the radio, so it is a separate investigation, not a checkbox feature.
