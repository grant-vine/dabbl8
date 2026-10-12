# Source-local sample addresses

Dabbl8 keeps a sample zone's byte offset relative to the buffer that owns its data. SAMPLE, GRAIN and SLICE resolve the built-in or user-slot buffer before reading; both waveform displays use that same source-local address rule. The three user slots, flash boundaries, saved formats, generated tables and shared eight sounding voices remain unchanged.

Previously, user-zone scanning rebased an offset against the built-in sample array. The later readers indexed outside that declared array even when the flat target flash map made the resulting numeric address meaningful. Host UBSan reported this at `eng_slice.c`; treating that as safe C or disabling bounds checks would hide the defect. The correction carries the owning data base explicitly. SLICE's audio path chooses it from the already validated source number, outside the decoder loops; generic waveform/decoder helpers use equality against the known zone arrays.

## Verified scope

The independent range fixture allocates three disjoint user buffers and checks accessed ranges for five consumers: SAMPLE, GRAIN, SLICE decoding and both waveform readers. The actual SLICE render fixture covers built-in sources0/4, all three user sources1..3, forward/reverse playback and unsupported packed source5/6 refusal. These checks retain bounds diagnostics. Range/editor sanitizer checks retain all categories; render/reverse/fuzz exclude only inherited DSP signed-integer-overflow and shift diagnostics.

On the current-main d264 composition, AppleClang21 completed34 positive host runs; eight PCM pairs, nine user/built-in SLICE demos and both92golden runs matched. The initial35th control compile failed because its executable name collided with its source directory. That failed result is preserved. Only the two controls were repeated with distinct names: wrong user-slot and wrong built-in routing produced6 and4 range assertions, respectively. They demonstrate that the fixture observes source-routing errors.

The final delivery additionally includes the merged historical-four-track UI fixture repair from main0356604. Focused Linux18 checks run the actual new range/render runner commands, two complete strict four-track editor configurations and the ordinary four-track golden suite. The compact [qualification record](evidence/2026-10-12-sample-source-correctness/qualification.json) records their outcomes.

The existing deterministic sample-header fuzz covers SAMPLE/GRAIN and adjacent controls. Its SLICE preset argument does not select the three user sources; user SLICE coverage comes from the separate actual-render fixture. Complete native-eight-track UI, all engine profiles, the entire host suite and physical-device operation are outside this qualification.

## Fresh vendor application measurements

Four isolated FULL application builds compared actual main d26458a against the source-address correction a1ea40bc. Both use FM4off/SLICEon, the same pinned compiler/image and seven retained generated headers. Main0356604 adds only tests/docs, leaving these production inputs unchanged. No generation, SDK, loader or installable package was used.

| Build | Parent app | Corrected app | RAM data+bss | Pool | Static budget result |
| --- | ---: | ---: | ---: | ---: | --- |
| Four tracks |449240B|449616B (+376)|92116B unchanged|331332B unchanged|Pass on both|
| Eight tracks |482968B|483304B (+336)|96068B unchanged|335240B unchanged|Fail on both; corrected SLICE adds a failure|

Native-eight-track SLICE score rises494→517 against baseline453 with the unchanged10% tolerance (maximum498.3). The existing audio ISR55063 against37502 and UAC562 against504 failures remain unchanged. These are static emitted-code estimates, not measured hardware cycles or deadlines. They remain open performance evidence; the address correction does not claim a timing recovery or whole-eight-track budget pass.

A separate decoder-local-state experiment is excluded: its prior score516 did not recover the budget and increased the out-of-line decoder's own saved-register frame. Future optimisation must preserve accessed ranges, segment/decoder state and audio output while demonstrating a real emitted hot-path improvement. No budget, audio golden, compiler model or layout-padding change may manufacture acceptance.

This is a source correctness change, not an installable or hardware-qualified release. Native write/recovery gates, device testing, complete stack high-water evidence and release acceptance remain open. Dependency and GPL-3.0-only notices remain preserved.
