# Native reference arrangement: first playback slice

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
remain intact. Eight destinations may independently reference the same source
track. Automation and parameter-lock unlock use source owner/place and current
engine compatibility; destination bases and locks remain independent.

The actual master sixteenth clock supplies bar boundaries independently of
track 1's pattern length. Internal clock and configured USB/TRS realtime
Start/Continue/Stop enter the existing sequencer. Publication is block-quantized,
not a sample-exact output/timing promise. Sixteen rows/scenes, four banks and
repeats 1–16 are retained. KNOB 1 browses native rows; KNOB 4 while playing queues
the selected row for the next master bar (latest valid request wins). Native
row names and repeats come from retained metadata. This slice exposes these
rows read-only; editing and scene overlays are separate increments.

Scenes with any apply bit refuse explicitly: MUTE=1, MIX=2, TRANSPOSE=4.
Unsupported overlays are not silently ignored. Fill is not implemented here.

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

## Memory and bounded work

Three immutable source snapshots and a small typed policy occupy the existing
four-source chain extent; no full-project copy or ISR pointer remains in the
display arena. Preparation invalidates old suspension before unlocked bulk
copies. Start/Continue during copying abort preparation. A late failure may
conservatively discard prior preparation; it leaves live editable music intact
and does not publish partially usable sources. Final publication guards only
bounded policy/flags; all source reads, CRCs and bulk copies remain IRQ enabled.

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
load→Continue refusal→fresh Start, missing callback, unsupported overlays and
fourth reference, late copy-phase activity/Start/store mutation, same-pitch
panel/MIDI/sustain/multiple-channel holds and final release, All Sound Off,
no output-ring publication for 96 local releases, and no NOR writes.

Optimized/inherited sanitizer results, exact source head and target memory/
static costs must be recorded in the evidence accompanying this slice.
Inherited DSP sanitizer exclusions are not full undefined-behavior coverage.
No audible timing, peripheral drain, hardware recovery or release qualification
is inferred from host or virtual-NOR tests. Golden audio hashes are unchanged.

The next increment implements actual mute/mix/transpose scene overlays and
bounded fill behavior, preserving transient versus saved state and held-note
semantics. This first slice provides real playback rather than a dormant plan.

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
