# Native reference arrangement playback, overlays and fills

This software slice executes loaded D8P1 rows through the actual sequencer. It
is not production storage activation, hardware qualification, or completion of
issue #15. Issue #14 remains the hardware gate. The explicit trusted native
frontend binding must already represent completed migration; recognizing a
record, loading wire, or preparing playback grants no storage authority. The
production boot path remains unbound and refuses SONG preparation.

## Implemented behavior

Stopped SONG PLAY validates the whole current native pool and all live
arrangement references, then copies patterns, four timing parameters and sparse
motion from at most three manual sources into the existing chain cache.
Project reference 3 refuses; it never aliases shared autosave or the historical
fourth project. The editable project, instruments, FM6 patches, mixer and undo
remain intact beneath transient scene overlays. Eight destinations may independently reference the same source
track. Automation and parameter-lock unlock use source owner/place and current
engine compatibility; destination bases and locks remain independent.

The actual master sixteenth clock supplies bar boundaries independently of
track 1's pattern length. Internal clock and configured USB/TRS realtime
Start/Continue/Stop enter the existing sequencer. Publication is block-quantized,
not a sample-exact output/timing promise. Sixteen rows/scenes, four banks and
repeats 1–16 are retained. KNOB 1 browses native rows; KNOB 4 while playing queues
the selected row for the next master bar (latest valid request wins). Native
row names and repeats come from retained metadata. This slice exposes these
rows read-only; metadata editing remains a separate increment.

Known apply bits are MUTE=1, MIX=2 and TRANSPOSE=4; unknown bits refuse.
Mute is an additional performance mute using the existing gain ramp, composing
with manual mute and solo while retaining input ownership. Mix supplies an
absolute effective level/pan at the mixer; editable parameters and motion bases
remain underneath. Transpose shifts only pitched sequence notes before scale
quantization, clamps to MIDI range and deduplicates sounding pitches. Drum hits
and engines with mapped kit/slice keys remain unchanged, as do panel/MIDI notes.
Stop removes the effective layer; valid Continue restores the same scene.

While playing, positive KNOB 3 queues the selected row as a one-master-bar fill;
negative KNOB 3 cancels it. Pending fills show ARM, active fills RUN, and active
cancellation RET. Calibrated encoder mapping/direction applies. Latest valid fill
wins; KNOB 4 normal row selection cancels a fill and has priority. Cancellation
returns to normal progression at the next bar, without cutting the active bar.
A fill replaces, rather than inserts, the upcoming scheduled bar and consumes
its repeat/progression. A repeat2 → fill replacing its second bar → B is one
example; replacing one-repeat B returns directly to C. No fill is appended when
normal progression has already ended. A fill can replace the last scheduled bar
and Stop follows that bar. During an active fill, another fill replaces the next
upcoming normal bar using a bounded precomputed return row/repeat context.

Stop cancels pending fills, but preserves active-fill phase and return context.
Valid Continue finishes the remaining fill once and returns to that context;
repeated Stop/Continue does not consume extra bars. Fresh Start/reprepare resets
fill state. Load, attempted save including uncertain commit, store epoch changes
and existing preparation guards invalidate a suspended fill just as they do an
ordinary suspended route. There is no fill-specific coherence exemption.

Stop cancels pending requests, restores editable timing/recording and suspends
row/phase. Continue resumes only a valid suspended preparation. Successful
project adoption, attempted source save/rename (including uncertain commit),
rebind, and observed physical store-epoch changes invalidate that resume.
An invalid-resume tombstone refuses Continue; a fresh HOME/MIDI Start clears
it and plays the current editable project from its beginning. SONG requests
always prepare first and never silently start editable music after refusal.
Start while valid native suspension restarts row zero. Count-in preparation,
clock-source change, timeout and broken MIDI input are explicitly gated or
cancelled through the existing transport. A physical epoch is 32 bits; complete
wrap between checks is not qualified as a new authority/coherence proof.

## Engine-card compatibility policy

References select patterns, not sounds. A structurally valid source can use an
engine absent from a destination build: preparation copies only its steps,
LEN/DIV/SWING/GATE and motion, and retains its exact stored engine ID as
provenance. It never adopts source instruments, presets, FM6 patches or sample
assets. Generic eligible motion transfers to the current destination sound;
FM envelope/level, DRUM lane and E0..E7 automation and locks require exact
source/destination engine identity, including when a lock unlocks. DIGITAL or
PERC provenance is not silently relabelled. Historical conversion must perform
its real parameter/motion migration before producing canonical source records.

Preparation checks every destination's canonical requested engine before
borrowing the arena or changing prior preparation, and rechecks under the
final publication guard. An unavailable destination returns
`D8POOL_UNSUPPORTED`; invalid mask arguments return `D8POOL_INVALID`. It never
chooses another sound. Initial refusal preserves prior live/cache/canvas/DMA
state. Late refusal uses the existing invalid/disarmed preparation path:
editable music stays intact, partial sources cannot run, and Continue or fresh
Start cannot use that invalid preparation until preparation succeeds again.
Source integrity, reference, ownership, transport and generation guards remain.

The mask argument is an internal synchronous test seam. Production always
passes the immutable full compiled engine mask, including actual FM4/SLICE
compatibility options; reduced-profile build guards remain closed. This slice
does not enable downloadable engine cards, negotiate assets, or grant writes.
`tests/arrangement_engine_compatibility_test.c` exercises real pool preparation,
pattern notes/timing, matching and mismatched automation/lock/unlock, source
byte preservation, complete initial refusal and late discard. Unchanged-parent
and deliberately source-restricting negative controls are recorded separately.
No timing or hardware qualification follows from these host checks.

## Memory and bounded work

Three immutable source snapshots and a small typed policy occupy the existing
four-source chain extent, including a 416-byte immutable scene cache and small
fill return context; no full-project copy or ISR pointer remains in the
display arena. Preparation invalidates old suspension before unlocked bulk
copies. Start/Continue during copying abort preparation. A late failure may
conservatively discard prior preparation; it leaves live editable music intact
and does not publish partially usable sources. Final publication guards only
bounded policy/flags; all source reads, CRCs and bulk copies remain IRQ enabled.

An overlay boundary also publishes 18 bytes of effective audio mix/mute state.
A bar change includes up to 96 local note-off operations, eight P_COUNT=99
motion restoration scans, 64 bytes of timing and eight phase resets; source
step-zero events may then invoke up to 96 local note-ons. This is not O(1) audio
work. Same-pitch MIDI/panel held notes are protected using actual ownership
helpers, including sustain; final key/channel release and All Sound Off retire
them. A 128-bit mask per track lives inside the existing unused source extent.
Each sequence release uses one cached pitch bit. Actual MIDI/chord/panel
ownership changes refresh only affected pitches with the original predicates;
sustain, repeated notes and channel remaps follow those same paths. Aggregate
panic/forget and initial stopped preparation rebuild bounded ownership data.
These rebuilds are real work (16×128 raw ownership entries with chord-source
checks, 24 four-note chords and 27 four-note panel chords for complete build;
rare single-track forget checks 128 pitches). They are not hidden deadline
qualification. An input-generation guard rejects a mixed initial cache rebuild.
The cache is tested independently against original ownership predicates after
actual input transitions, rather than merely against its own builder.
It retains eight shared voices and existing stealing/release envelopes.
The sequencer's local note path does not emit MIDI packets: this boundary does
not itself produce a 96-packet output burst. The inherited keyboard output ring
is 64 packets and silently declines enqueue when full; physical output remains
unqualified. No engine or patch arrays are copied at a bar boundary.

## Verification scope

`tests/native_arrangement_test.c` uses the real native runtime/pool frontend,
sequencer, master clock, USB/TRS realtime queue, motion, held ownership and
explicit virtual NOR. It covers all bank/scene/row capacities, 16 repeats,
track-length-independent boundaries/carry, alias motion/unlock, pending latest
wins/invalid requests, generation wrap/cancellation, count-in, clock source/loss,
load→Continue refusal→fresh Start, missing callback, unknown overlay bits and
fourth reference, late copy-phase activity/Start/store mutation, same-pitch
panel/MIDI/sustain/multiple-channel holds and final release, All Sound Off,
no output-ring publication for 96 local releases, and no NOR writes.

Optimized/inherited sanitizer results, exact source head and target memory/
static costs must be recorded in the evidence accompanying this slice.
Inherited DSP sanitizer exclusions are not full undefined-behavior coverage.
No audible timing, peripheral drain, hardware recovery or release qualification
is inferred from host or virtual-NOR tests. Golden audio hashes are unchanged.

Overlay/fill tests additionally exercise every apply combination, exact encoded
editable-state and saved-source preservation, mixer/ramp endpoints, sequence
transpose clamp/dedup/mapped-key exclusions, calibrated KNOB 3, replacement
repeat/end semantics, active-fill Stop/Continue through real USB/TRS queues,
source adoption/uncertain-save refusal and final held-input releases. Scene
editing, microtiming depth and hardware timing qualification remain separate
increments. A passing host test does not establish audible device behavior.

## Cold historical transition experiment

This candidate separates only the eight-track historical row-advance boundary
from the inlined audio-fragment path. The four-track source path remains
unchanged. The hot check still computes track-zero step length, checks phase
and decrements repeats; only an actual next-row/final-row transition calls
`chain_advance_legacy`. Its work is not removed: final stop or row selection,
up to 96 sequence releases, eight motion restorations and timing/phase resets
still execute synchronously. Calls, direct helper costs/frames and the reachable
path must be reported alongside the static IRQ metric; a lower IRQ proxy alone
is not a device deadline or stack qualification. Actual historical boundary,
repeat, carry, source routing and final-stop tests supplement the native tests.
The failed dd20173 and 110c1ae target reports remain retained.

## Explicit timing dispatch experiment

Native-running playback now queries its master-bar period and uses the native
clock hook; historical mode0 uses its original track-zero timing. Inactive or
invalid-resume native modes use neither path. This removes the real eager
period computation outside native playback, without moving an entire tick
behind a noinline boundary. A test-only query-site counter (absent from target)
covers editable/historical/native/suspended/tombstone states, alongside actual
clock/carry/transport tests. Target metrics and hardware timing remain separate
qualification gates; all earlier failed evidence is retained.

## Automatic native row-boundary experiment

Only the implicit next-row/final-row action uses a narrow cold helper. Native
validity, period/phase/bar computation, explicit pending row requests and repeat
checks remain inlined; this is not a whole tick hidden from the interrupt cost
check. Existing all-row, master-carry, end-stop and held-ownership tests execute
the real helper. Cleanup/restore work remains synchronous and must be included
through its tail callees and frames, even if the IRQ proxy improves. Earlier
failed candidate reports remain retained; device timing is still unqualified.
