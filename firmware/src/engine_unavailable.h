/* SPDX-License-Identifier: GPL-3.0-only
 * Felucca-derived metadata/state, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. Unavailable metadata slots; sound admission refuses before callbacks. */
static void d8_unavailable_note_on(track_t *t, voice_t *v) { (void)t; (void)v; }
static void d8_unavailable_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{ (void)t; (void)out; (void)n; (void)m; v->active=0; v->gate=0; v->stage=0; }
