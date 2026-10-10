# D8-006: eight shared sounding voices

The eight-track allocator has one shared budget of eight active engine voices. Per-track voice arrays remain allocation destinations; their size does not grant eight voices to each track. The target remains four tracks until the runtime and memory gates pass. `tests/voice_budget_test.c` compiles the real core with `NPART=8`, including eight real tracks and engine state arrays, rather than simulating a track count in a four-track policy function.

## Upstream invariant and no-room paths

Upstream protects each MONO/LEGATO/UNISON lead and the lowest held POLY note. With four tracks and eight voices, a full budget always contains a released voice, extra unison voice or non-bass chord tone. At eight tracks, one held protected note on every track exhausts that argument.

- `voice_room`: its previous hard-no-victim return admitted a new voice without reclaiming a slot. It now refuses if no victim exists.
- `voice_alloc`: a free local slot previously remained eligible when the global search found no victim. It now returns null; the POLY note-on caller checks that result before dereferencing it.
- `voice_reuse`: reactivating a fading/inactive reused voice requires admission. Failure propagates to the same null check, including DRUM lane reuse.
- `mono_play`: its primary lead needs hard admission; extra UNISON voices use soft admission and skip on refusal. Existing active lead/legato changes consume no additional slot.
- Local POLY cap exhaustion restarts a selected local voice; it cannot allocate a ninth voice. Same-note and same-drum-lane retriggers reuse a slot. Engine-switch queued notes return through `trk_note_on` after the old voices end.
- `voice_kill` originally marked stage 4, excluded that voice from `voices_busy`, but rendered its one-block ramp. Thus busy count alone was not a strict engine-render budget. Expanded builds retire the victim before admission: active, stage and envelope/output amplitude are zeroed.

## Musical priority

For a primary note, select the lowest category, then oldest creation age, then the existing ascending track/voice scan order:

1. Released tails (gate off).
2. Extra UNISON voices and stale extra voices after a non-POLY mode change.
3. Held POLY chord tones other than that track's lowest held note.
4. Protected lead/bass notes, only when the earlier categories have no victim.

A pedal-held note still has its gate on; it competes as held, not released. DRUM is a POLY one-shot engine: it uses the same priorities without a special unlimited privilege. Bass protection is a preference, not a permanent reservation. A ninth primary event replaces the oldest protected lead/bass if all eight are protected. The owning track's existing local voice restarts in place when selected; another track's victim is retired first.

Extra UNISON voices use only categories 1/2 on other tracks and may be refused. They cannot displace chord tones or protected leads to complete a unison stack. A new primary lead can displace extra unison voices. There is no voice reservation by track number, instrument label or external MIDI channel.

## Sound and cost limits

For `NPART >= NVOICE`, stolen voices do not run their engine for an overlapping fade block. This bounds both active state and actual engine render calls to eight. Immediate retirement may produce an audible waveform discontinuity; listening/click-quality checks and target ISR deadlines remain release gates. This PR does not claim seamless stealing or target performance. A future smoothing design must fit the same eight-voice budget and memory limits rather than relabel a ninth rendering voice as a tail.

The default four-track build retains upstream crossfades and its golden renders. There is no golden-hash rewrite. Enabling the eight-track target is separate work (#9/#12), including real target memory and workload measurements. This allocator is bounded in scan work: at most eight tracks × eight destination voices per scan. Shared ages use the inherited 32-bit counter; long-run age wrap behavior is not newly specified here and must be audited during long-session testing.

## Verification

The test covers eight held protected notes and a ninth, cross-track stealing, released tails, chord/bass priority, unison refusal/stealing, DRUM, real MIDI sustain ownership and pedal-up after a steal, complete releases, and 10,000 burst events. It checks active allocations and the summed return of the actual `track_render` calls, not only `voices_busy`. The standard runner includes this test. Dedicated ASan/UBSan checks run the same eight-track sources. Full four-track regression and the pinned target build are recorded separately in the evidence directory.
