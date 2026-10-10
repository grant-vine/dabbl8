# Eight track source blockers

The central change is not simply NPART = 8. core.h defines NPART = 4, NTRK = NPART, NVOICE = 8 and NSTEP = 64. Each track contains an eight-slot voice-state array, and several engines have separate per-track state. Doubling tracks therefore increases dormant RAM even while the global sounding voice budget remains eight.

Automation addressing is a hard blocker. motion_event_t stores an eight-bit place using (track << 6) | step. Four tracks times 64 steps fit exactly. The fifth track’s first step produces 256, truncates to zero and aliases the first track. Introduce an explicit track/step address or a wider address and version the serialized format and editor protocol. Test all 512 addresses, including tracks 5–8, save/restore, locks and corrupted input.

Historical project structures are another blocker. Several supposedly frozen compatibility structs use t[NTRK]. Changing NTRK changes old layouts and defeats historical imports. Freeze legacy track counts at four and preserve each historical format’s exact sizes, offsets and checksums. Add committed fixtures, including old engine conversions, before enabling eight tracks.

Voice allocation contains an assumption that NPART < NVOICE: one protected voice per track cannot fill the whole shared pool. With eight tracks and eight voices that assumption no longer holds. Specify what happens when all tracks hold a protected note and a ninth note arrives. Never rely on the existing “cannot happen” path. Test mono bass, drum hits, chords, sustain, release tails and cross-track steals.

Audit every track-index consumer: MIDI channel mapping; TRS and USB paths; engine request application; UI track selection; mixer layouts; mute and recording masks; quick-layer key maps; pattern/chain logic; serializers; SysEx bounds; browser emulator exports; backup tools; editor arrays. An eight-bit track mask fits eight tracks but deserves signedness, complement and bounds tests. Do not replace every literal four: four knobs and four project slots are different concepts.

Parameter IDs and engine IDs must stay stable. P_COUNT is 99 in this baseline; the high bit of a motion parameter is used for locks, imposing another format ceiling. A retired engine ID is reserved. Add capabilities and schema negotiation rather than silently reassigning IDs or making old editors guess the track count.

Felucca builds the app through a single translation unit that includes source files in an intentional order. Introduce track abstraction with small reviewable changes inside that structure first. A complete build-system rewrite would make eight-track regression debugging harder.
