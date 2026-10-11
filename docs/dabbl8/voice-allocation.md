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

The default four-track build retains upstream crossfades and its golden renders. There is no golden-hash rewrite. Enabling the eight-track target is separate work (#9/#12), including real target memory and workload measurements. This allocator is bounded in scan work: at most eight tracks × eight destination voices per scan. Expanded builds compact active creation ages before the 32-bit counter would overflow, preserving strict age ordering and equal-age scan ties. The historical four-track counter behaviour remains unchanged.

## Verification

The test covers eight held protected notes and a ninth, cross-track stealing, released tails, chord/bass priority, unison refusal/stealing, DRUM, real MIDI sustain ownership and pedal-up after a steal, complete releases, and 10,000 burst events. It checks active allocations and the summed return of the actual `track_render` calls, not only `voices_busy`. The standard runner includes this test. Dedicated ASan/UBSan checks run the same eight-track sources. Full four-track regression and the pinned target build are recorded separately in the evidence directory.

## Counter rollover

The code audit reproduced an eight-track ordering failure at the inherited counter boundary: ages immediately after wrap looked older than notes created before wrap. This affected numerical-age choices in shared allocation, local allocation and overload fallback. It did not itself exceed the eight-voice budget.

Before assigning a new native age when the counter equals UINT32_MAX, compute each active voice's rank from the unmodified ages, then publish all ranks together and resume above the highest rank. Equal ages remain equal. Inactive slots do not contribute. A sustained note can remain the oldest regardless of elapsed counter distance; this uses no signed half-range comparison or note-lifetime assumption. Compaction happens before marking the new voice active or replacing its age.

The temporary rank table has one byte per destination (64 bytes for eight tracks). Scans are bounded by the destination count: at most 64 × 64 comparisons, with the inner scan only for active voices. No heap, storage or wider persistent voice field is added. This rare operation still runs in the audio ISR: target stack, instruction cost and physical deadline verification remain necessary. Frequency alone does not prove it safe. The four-track compile keeps its inherited increment and allocation behaviour.

Boundary regressions cover shared protected stealing, indefinitely held notes, equal-age ties, empty pools, local held/released stealing and actual overload victim choice. Negative controls run these new regressions against unchanged allocator source to demonstrate that the tests reject the original bug. Normal and inherited DSP sanitizer scopes are recorded explicitly; they do not establish physical timing, waveform click quality or long-session hardware qualification.

Analog and Trio also use creation age to seed a newly initialised noise state. Compaction leaves all existing non-age track/voice state, including noise seeds, untouched; a newly initialised voice after compaction uses the compacted creation age. Age is therefore not exclusively allocation metadata. The test snapshots all tracks around direct compaction and verifies that only ages change. No claim of identical post-rollover noise audio is made, and no golden hashes are rewritten.

The pinned vendor compiler inlines compaction into `voice_start`: its explicit local stack reservation rises from 20 to 88 bytes, in addition to the unchanged register push. This reservation applies on normal admissions too. Persistent RAM is unchanged, but that does not imply unchanged stack use. Full interrupt call-chain and nesting high-water evidence remain outstanding; separating the rare path is an optimisation to evaluate before adoption.
