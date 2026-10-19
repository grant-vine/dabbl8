# Engine resource limits - 18 October 2026

These are implementation limits for the full eight-track configuration, not
hardware-qualified maximum combinations. Issue #12 remains open. Eight tracks
share **eight sounding voices total**; a per-track cap never increases that
shared budget. MONO, LEGATO and UNISON can reduce or consume the available voices.

`voice.c:trk_nvoice` applies an engine's nonzero `poly` field, bounded by NVOICE;
zero means the shared eight-voice limit. The installed v0.1.8-timing configuration
has FM4 disabled, SLICE enabled and USB audio disabled.

| Engine | Stable ID | Per-track source cap | Shared heavy-state ownership |
| --- | --- | --- | --- |
| ANALOG | 0 | 8 | None |
| DIGITAL/FM4 | 1 | Disabled/reserved in this candidate | Unavailable |
| PHASE | 2 | 8 | None |
| LOFI | 3 | 8 | None |
| SAMPLE | 4 | 8 | None |
| VOICE/formant | 5 | 4 | None |
| TRIO | 6 | 8 | None |
| WHEEL | 7 | 8 | One body per sounding voice |
| GRAIN | 8 | 3 | One body for the active track |
| PHYS | 9 | 3 | One body per sounding voice |
| DRUM | 10 | 8 lanes/voices | One body for the active track, containing its lanes |
| NOISE | 11 | 8 | None |
| FM6 | 12 | 6 | One body per sounding voice |
| SLICE | 13 | 8 | One body per sounding voice using reverse playback |

Sources: stable table in `firmware/src/engines.c`; descriptor `.poly` fields in
the corresponding `eng_*.c`; FM6_POLY in `eng_fm6.c`, PHYS_POLY in `eng_phys.c`,
GR_POLY in `grain_limits.h`, DV_NLANE in `drum_voice.c`; ownership rules in
`heavy_state.h:heavy_live` and `heavy_get`. DRUM is a one-shot engine: its
envelopes/lane behavior differ from a sustained melodic voice. Lane choking and
voice reclamation can reduce simultaneous sounding voices further.

The shared pool reserves eight 7,020-byte union bodies, 56,160 bytes total in
this configuration. An FM6 state uses a body whose reservation is sized for the
largest union member; six FM6 notes do not reserve six additional pools. A cap
of eight bodies is a memory bound, not a CPU deadline guarantee. Stack writes,
effects buses and other engine arrays must be considered separately.

## Measured and unmeasured combinations

| Workload | Evidence | Limit of the result |
| --- | --- | --- |
| Existing workspace, one note per track | Physical v0.1.8 timing/resource on 18 October: 6,303 timing blocks, zero late publications; peak two heavy bodies, no allocation failures; 17 captured data roles unchanged | Current sounds, short bounded workload, uncalibrated timer; not every preset/effect or eight heavy bodies |
| Six FM6 notes on channel 2, DRUM on channel 4, ANALOG on channel 1 | Local renderer: 24,822 blocks, peak eight voices/seven bodies, full note release and canonical project equality. Physical v0.1.8: 128 pairs per stage, 6,292 timing blocks, zero late/taint, peak seven bodies, no allocation failures, 17 data roles unchanged; `build/headless-seven-20261018/RESULTS.md` | Short bounded physical workload, existing sounds, uncalibrated timer; no listening, all-presets, eight-body or complete deadline qualification |
| Eight notes on the existing single FM6 track | Earlier local proposal observed six voices/six bodies, matching FM6_POLY=6; its incorrect eight-slot assertion failed and is retained | Not a firmware defect or a physical failure; the unqualified draft capture helper must not be used |

The owner's current workspace has only one per-voice heavy engine track (FM6)
and one per-track heavy engine (DRUM). Therefore its source-defined maximum is
seven simultaneously live heavy bodies. Filling all eight bodies requires a
different sound configuration; do not change that workspace or load a project
under the current no-saved-data-change instruction.

The seven-body host proof used the saved read-only current-workspace capture,
SHA-256 `18311925ba6d069f50eecf161c2e362d05e6575cd5a8a914284e87e44601243c`.
It checks stopped/recording/routing state, exact engine assignments, FM6 POLY
and its six-voice cap, eight voices during every held burst, ownership bounds,
empty MIDI tracking and canonical project equality. Sanitizers cover address
and undefined behavior with the existing DSP signed-overflow/shift exclusions.
The unsuppressed compiler log retains existing target-pointer and included
test-harness warnings; this is not a warning-clean or all-preset claim.

Remaining #12 work includes a separately authorized eight-body configuration,
broader engine/effect and concurrent
UI/MIDI measurements, clock/observer overhead qualification, stack envelope
and final supported-combination decisions. Keep those separate from the source
caps above. These records do not authorize device access, edits or flashing.
