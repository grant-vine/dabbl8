/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* #251, MENU > LARGE: a tall card sets its value in one face over everything its knob can show (L where every value
 * fits L, else M, else S), so turning it never changes the value's size. card_span (what the knob shows: fn's text
 * of each value lo..hi) is set just before a draw_column (card_span_of / card_span_by) and taken by it; none: the
 * value's own face. The face is worked out when what the card is of changes (span_face's key), not every frame. */
typedef struct card_span {
    void (*fn)(const struct card_span *sp, int32_t v, char *val);   /* value v as the card shows it (val: 16) */
    const void *p;                                       /* fn's: a param_desc_t, a name list .. */
    int32_t lo, hi, ctx;
} card_span_t;
static card_span_t card_span __attribute__((section(".bss.dabbl8_ui")));
static void span_param(const card_span_t *sp, int32_t v, char *val)
{
    const char *u;
    uint8_t named = fmt_named;                           /* (param_format sets it: the card's own, kept) */
    param_format((const param_desc_t *)sp->p, v, val, &u);
    fmt_named = named;
}
static void span_names(const card_span_t *sp, int32_t v, char *val)
{
    str_cpy(val, ((const char *const *)sp->p)[v], 16);
}
static void span_mod_dst(const card_span_t *sp, int32_t v, char *val)   /* MOD's DST: the track's (its engine's) names */
{
    str_cpy(val, mod_dst_name((const track_t *)sp->p, v), 16);
}
static void card_span_by(void (*fn)(const card_span_t *, int32_t, char *), const void *p, int32_t lo, int32_t hi, int32_t ctx)
{
    card_span.fn = fn;
    card_span.p = p;
    card_span.lo = lo;
    card_span.hi = hi;
    card_span.ctx = ctx;
}
static void card_span_of(const param_desc_t *d)          /* a parameter: its whole range */
{
    card_span_by(span_param, d, d->min, d->max, (int32_t)d->fmt);
}
/* the face of a tall card's value s (0 L, 1 M, 2 S): L where the sparse L face has its glyphs and it fits with 3 px
 * either side, M where it fits with 2 (52 px: "THUMB"), else S (cut by cv_text_fit when even S is too wide) */
static uint32_t tall_face(const char *s)
{
    return large_face_has(s) && ink_w(&AF_L, s) <= COL_W - 6 ? 0u : ink_w(&AF_M, s) <= COL_W - 4 ? 1u : 2u;
}
static uint32_t span_face(uint32_t c, const card_span_t *sp, const char *label)
{
    static uint32_t key[4] __attribute__((section(".bss.dabbl8_ui")));
    static uint32_t face[4] __attribute__((section(".data.dabbl8_ui"))) = {3u, 3u, 3u, 3u};   /* (3: none yet) */
    uint32_t k = str_hash((uint32_t)(uintptr_t)sp->fn * 31u + (uint32_t)(uintptr_t)sp->p * 7u + (uint32_t)sp->lo * 131u +
                          (uint32_t)sp->hi * 8191u + (uint32_t)sp->ctx * 524287u + song.sel + TSEL->eng_req * 4u, label);
    int32_t v;
    char t[16];
    c &= 3u;
    if (k == key[c] && face[c] <= 2u)
        return face[c];
    key[c] = k;
    face[c] = 0;
    for (v = sp->lo; v <= sp->hi && face[c] < 2u; v++) {   /* (a few hundred values at most, once) */
        uint32_t f;
        sp->fn(sp, v, t);
        if ((f = tall_face(t)) > face[c])
            face[c] = f;
    }
    return face[c];
}

