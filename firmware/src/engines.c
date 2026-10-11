/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Engine table (order = PRESETS browsing order and the engine numbers of the editor protocol), the
 * factory patterns (SEQ > PATTERNS) and the parts' sounds at power-on. */
#include "dsp.c"
#if D8BUILD_PROFILE != 0
#include "engine_unavailable.h"
#endif
#include "eng_analog.c"
#if D8BUILD_HAS(2)
#include "eng_phase.c"
#else
#include "engine_metadata_phase.h"
#endif
#if D8BUILD_HAS(3)
#include "eng_lofi.c"
#else
#include "engine_metadata_lofi.h"
#endif
#include "eng_sample.c"
#if D8BUILD_HAS(5)
#include "eng_formant.c"
#else
#include "engine_metadata_formant.h"
#endif
#if D8BUILD_HAS(6)
#include "eng_trio.c"
#else
#include "engine_metadata_trio.h"
#endif
#if D8BUILD_HAS(7)
#include "eng_wheel.c"
#else
#include "engine_metadata_wheel.h"
#endif
#if D8BUILD_HAS(8)
#include "eng_grain.c"
#else
#if NPART >= NVOICE
#include "heavy_state.h"
#endif
#include "engine_metadata_grain.h"
#endif
#if D8BUILD_HAS(9)
#include "eng_phys.c"
#else
#include "phys_legacy.h"
#include "engine_metadata_phys.h"
#endif           /* PHYS: DaisySP physical models (phys_dsp.c, MIT) */
#include "eng_drum.c"           /* DRUM: the 8-lane kit (drum_voice.c) */
#if D8BUILD_HAS(11)
#include "eng_noise.c"
#else
#include "engine_metadata_noise.h"
#endif
#include "eng_fm6.c"            /* FM6: 6-operator FM, msfa ported (fm6_core.c, Apache-2.0) */
#include "fm4_convert.c"        /* DIGITAL's tables, and its sounds -> FM6 */
#if FELUCCA_FM4
#include "eng_digital.c"        /* DIGITAL: four-operator FM (retired; FELUCCA_FM4=1 builds it) */
#endif
#if FELUCCA_SLICE
#if D8BUILD_HAS(13)
#include "eng_slice.c"
#else
#include "engine_metadata_slice.h"
#endif
#endif

/* the editor protocol, user presets and projects store these indices: append, never reorder */
static const engine_t *const ENGINES[NENGINES] = {
    &ENG_ANALOG,                 /* 0 */
#if FELUCCA_FM4
    &ENG_DIGITAL,                /* 1 (ENGI_DIGITAL) */
#else
    &ENG_FM4_GONE,               /* 1: reserved (DIGITAL, retired: its sounds convert to FM6, fm4_convert.c) */
#endif
    &ENG_PHASE,                  /* 2 */
    &ENG_LOFI,                   /* 3 */
    &ENG_SAMPLE,                 /* 4 */
    &ENG_FORMANT,                /* 5 VOICE (eng_formant.c: "voice" is a sounding note in voice.c) */
    &ENG_TRIO,                   /* 6 */
    &ENG_WHEEL,                  /* 7 */
    &ENG_GRAIN,                  /* 8 */
    &ENG_PHYS,                   /* 9 (ENGI_PHYS) */
    &ENG_DRUM,                   /* 10 (ENGI_DRUM) */
    &ENG_NOISE,                  /* 11 */
    &ENG_FM6,                    /* 12 (ENGI_FM6) */
#if FELUCCA_SLICE
    &ENG_SLICE,                  /* 13 (FELUCCA_SLICE=0 builds without it) */
#endif
};

/* a track's engine number as an index (the audio paths: a compare, cheaper than % NENGINES; a bad number: 0) */
static inline uint32_t eng_idx(uint32_t e) { return e < NENGINES ? e : 0u; }

/* the sample source a track's sound plays that has no data (SAMPLE's SET, GRAIN's SRC, SLICE's SRC: an empty or
 * invalid user slot, a set or PIANO this build lacks): its name (it plays a sine: eng_sample.c smp_sine), 0 = none;
 * *code a small number for it (1.., per engine and source) so the UI says it once (ui_input.c sample_notice) */
static uint8_t snd_said[NTRK];   /* per track: the code last said, 0 = none (a project load clears them: said again) */
static const char *snd_missing(const track_t *t, uint32_t *code)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t si = (uint32_t)t->p[P_E0] % SMP_NALL;
#if FELUCCA_SLICE && D8BUILD_HAS(13)
    if (e == &ENG_SLICE) {
        si = slc_src_of(t->p);
        *code = 64u + si;
        return slc_get(si) ? 0 : N_SLC_SRC[si];
    }
#endif
    if (e != &ENG_SAMPLE && e != &ENG_GRAIN)
        return 0;
    *code = (e == &ENG_GRAIN ? 32u : 1u) + si % 32u;
    return smp_set_missing(si) ? SMP_ALL_NAMES[si] : 0;
}

/* the order the engines are shown in (PRESETS browsing and its ENG knob, the EDIT layer's keys, the editor's list):
 * engine indices, never DIGITAL's reserved 1 (with FELUCCA_FM4 it follows FM6). The indices stay as they are (the
 * stores and the protocol hold them); only this table orders them */
static const uint8_t ENGINE_ORDER[NENG_SHOWN] = {
    0,                           /* ANALOG */
    12,                          /* FM6 */
#if FELUCCA_FM4
    1,                           /* DIGITAL */
#endif
    2, 3, 4, 5, 6, 7, 8, 9,      /* PHASE LOFI SAMPLE VOICE TRIO WHEEL GRAIN PHYS */
    11,                          /* NOISE */
#if FELUCCA_SLICE
    13,                          /* SLICE */
#endif
    10,                          /* DRUM */
};

/* Availability policy is now used by project adoption. Compatibility builds
 * retain their actual compiled DIGITAL/SLICE meanings; card profiles stay
 * disabled until every UI/editor/publication path is covered. */
#include "d8engine_policy.c"
static int eng_runtime_target(unsigned *out,unsigned mask,unsigned requested,unsigned preset,unsigned source)
{
    if(!mask || (mask&~D8ENGINE_EFFECTIVE_MASK))return D8ENGINE_INVALID_MASK;
    if(!out || requested>=NENGINES || source>D8ENGINE_LEGACY_FACTORY)return D8ENGINE_INVALID_REQUEST;
#if FELUCCA_FM4
    /* This is an implemented DIGITAL engine, not a retired archive alias. */
    if(requested==ENGI_DIGITAL){
        if(!(mask&2u))return D8ENGINE_UNAVAILABLE;
        *out=ENGI_DIGITAL;return D8ENGINE_OK;
    }
    if(!(mask&D8CARD_ALL))return D8ENGINE_UNAVAILABLE;
#endif
    return d8engine_target(out,mask&D8CARD_ALL,requested,preset,source);
}

/* the engines one can pick (engine 1 only with FELUCCA_FM4), in ENGINE_ORDER: eng_ok(e), the n-th of them
 * eng_vis(n), e's place among them eng_rank(e), the next / previous one eng_step(e, dir) (wraps) */
static int eng_mask_ok(unsigned mask) { return mask && !(mask & ~D8ENGINE_EFFECTIVE_MASK); }
static int eng_available(unsigned e, unsigned mask) { return eng_mask_ok(mask) && e < NENGINES && (mask & (1u << e)); }
static unsigned eng_count_mask(unsigned mask)
{
    unsigned n = 0;
    if (eng_mask_ok(mask)) for (unsigned i = 0; i < NENG_SHOWN; i++) n += !!(mask & (1u << ENGINE_ORDER[i]));
    return n;
}
/* Exact positions preserve stable IDs. Invalid/empty masks have no selectable entry. */
static unsigned eng_vis_mask(unsigned n, unsigned mask)
{
    if (!eng_mask_ok(mask)) return NENGINES;
    for (unsigned i = 0; i < NENG_SHOWN; i++) if (mask & (1u << ENGINE_ORDER[i])) {
        if (!n--) return ENGINE_ORDER[i];
    }
    return NENGINES;
}
static unsigned eng_rank_mask(unsigned e, unsigned mask)
{
    unsigned target, n = 0;
    if (eng_runtime_target(&target, mask, e, 0, D8ENGINE_LEGACY_SELECT)) return eng_count_mask(mask);
    for (unsigned i = 0; i < NENG_SHOWN; i++) if (mask & (1u << ENGINE_ORDER[i])) {
        if (ENGINE_ORDER[i] == target) return n;
        n++;
    }
    return n;
}
static unsigned eng_step_mask(unsigned e, int32_t dir, unsigned mask)
{
    unsigned n = eng_count_mask(mask);
    if (!n || e >= NENGINES || !dir) return NENGINES;
    unsigned rank = eng_rank_mask(e, mask);
    if (rank >= n) return eng_vis_mask(0, mask);
    return eng_vis_mask((rank + (dir > 0 ? 1u : n - 1u)) % n, mask);
}
static int eng_ok(uint32_t e) { return eng_available(e, D8ENGINE_EFFECTIVE_MASK); }
static unsigned eng_count(void) { return eng_count_mask(D8ENGINE_EFFECTIVE_MASK); }
static uint32_t eng_vis(uint32_t n) { unsigned count = eng_count(); return count ? eng_vis_mask(n % count, D8ENGINE_EFFECTIVE_MASK) : NENGINES; }
static uint32_t eng_rank(uint32_t e) { unsigned rank = eng_rank_mask(e, D8ENGINE_EFFECTIVE_MASK); return rank < eng_count() ? rank : 0u; }
static uint32_t eng_step(uint32_t e, int32_t dir) { return eng_step_mask(e, dir, D8ENGINE_EFFECTIVE_MASK); }
/* Fallback is restricted to startup/default insertion. Sound adoption never uses it.
 * Retain the raw DIGITAL request on success so the caller performs its actual patch conversion. */
static int eng_startup_sound(d8engine_sound *out, unsigned mask, unsigned requested, unsigned preset)
{
    if (!out) return D8ENGINE_INVALID_REQUEST;
    unsigned target; int rc = eng_runtime_target(&target, mask, requested, preset, D8ENGINE_LEGACY_FACTORY);
    d8engine_sound result = {requested, preset};
    if (rc == D8ENGINE_UNAVAILABLE) {
        result.engine = eng_vis_mask(0, mask); result.preset = 0;
        if (result.engine == NENGINES) return D8ENGINE_INVALID_MASK;
    } else if (rc) return rc;
    else if (requested == ENGI_SAMPLE && preset == D8ENGINE_PERC_PRESET) { result.engine = target; result.preset = 0; }
    *out = result; return D8ENGINE_OK;
}

/* factory sequence patterns: SEQ > PATTERNS loads one into the selected track (ui.c pat_load); a preset
 * only suggests one with PAT(n), loading a sound never touches the steps. Absolute notes, loaded as they
 * are (DRUM plays them as GM drums, SLICE as slices; SCL TRANS and OCT transpose): 0 = rest;
 * flags 1 = accent, 2 = slide, 4 = tie (holds the previous note). Names: at most 8 characters */
#define T_ 4
static const struct {
    const char *name;
    uint8_t note[16], flags[16];
} PATTERNS[] = {
    {"ACID", {45, 45, 57, 45, 0, 48, 45, 55, 45, 0, 57, 52, 45, 48, 0, 50},          /* 1 */
     {1, 0, 2, 0, 0, 0, 1, 2, 0, 0, 1, 0, 0, 2, 0, 1}},
    {"OFFBEAT", {0, 36, 0, 36, 0, 36, 0, 48, 0, 36, 0, 36, 0, 39, 0, 43},          /* 2 bass */
     {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
    {"MELODY", {60, 0, 67, 0, 72, 67, 0, 64, 62, 0, 69, 0, 74, 69, 0, 67},         /* 3 pluck */
     {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
    {"LEAD", {72, 0, 0, 74, 0, 0, 76, 0, 79, 0, 76, 0, 74, 0, 0, 0},               /* 4 */
     {1, T_, 0, 2, T_, 0, 0, 0, 1, 0, 2, 0, 0, T_, T_, 0}},
    {"PAD", {60, 0, 0, 0, 0, 0, 0, 0, 57, 0, 0, 0, 55, 0, 0, 0},                   /* 5 long notes */
     {0, T_, T_, T_, T_, T_, T_, 0, 0, T_, T_, 0, 0, T_, T_, 0}},
    {"KEYS", {0, 0, 60, 0, 0, 63, 0, 0, 0, 0, 60, 0, 0, 65, 0, 63},                /* 6 offbeat stabs */
     {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}},
    {"BELL", {72, 0, 0, 79, 0, 0, 84, 0, 0, 0, 76, 0, 0, 0, 0, 0},                 /* 7 sparse */
     {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {"SUB", {36, 0, 0, 0, 0, 0, 0, 36, 0, 0, 34, 0, 0, 0, 0, 0},                   /* 8 low, held */
     {1, T_, T_, T_, 0, 0, 0, 0, 0, 0, 0, T_, T_, T_, 0, 0}},
    /* SLICE (eng_slice.c): note = C4 + slice */
    {"CHOP", {60, 61, 62, 67, 64, 65, 60, 69, 68, 70, 62, 67, 72, 72, 74, 64},     /* 9 16 slices re-ordered */
     {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
    {"STUTTER", {60, 60, 61, 61, 62, 0, 63, 63, 64, 65, 65, 0, 66, 66, 66, 67},    /* 10 8 slices, repeats */
     {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0}},
    {"SLICES", {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75},   /* 11 in order */
     {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
    /* DRUM (General MIDI: 36 kick, 38 snare, 42 closed / 46 open hi-hat) */
    {"BEAT", {36, 42, 42, 42, 38, 42, 36, 42, 36, 42, 42, 36, 38, 42, 46, 42},     /* 12 */
     {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}},
    /* 16ths up a C minor arpeggio, twice: the line the ARP presets (RAVE, ARP 8BIT, ARP LEAD) suggest
     * now that the arpeggiator is the track's and a preset no longer switches it on */
    {"ARP", {48, 51, 55, 60, 63, 67, 72, 75, 48, 51, 55, 60, 63, 67, 72, 75},       /* 13 */
     {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}},
};
#undef T_
#define NPATTERNS (sizeof PATTERNS / sizeof PATTERNS[0])

/* the parts at power-on (engine, preset, PATTERNS[n - 1] in the sequencer, 0 = empty: all are): bass, pad, lead, drums */
static const uint8_t TRK_DEF[NPART][3] = {{0, 4, 0}, {ENGI_FM6, 4, 0}, {3, 0, 0}, {ENGI_DRUM, 0, 0}}; /* ANALOG ACID,
                                                                       * FM6 PAD (was DIGITAL PAD), LOFI PULSE LD, DRUM KIT */
static uint32_t trk_def_engine(uint32_t i) { return TRK_DEF[i % NPART][0]; }
