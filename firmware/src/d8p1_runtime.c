/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Include after project.c in an eight-track translation unit. Main-loop API;
 * no device protocol, persistent write or arrangement playback is enabled. */
#include "d8p1_project.h"
enum { D8RT_OK=0, D8RT_BAD=1, D8RT_BUSY=2 };

/* Const cache view survives drawing. Invalidated by the next successful native
 * or historical load. Main-loop readers only; never retain it across a load. */
const d8p1_arrangement *d8p1_runtime_arrangement(void)
{
    return d8p1_runtime_cache.valid ? &d8p1_runtime_cache.arrangement : 0;
}

/* Call in the main loop with interrupts enabled.
 * Input must remain immutable until copied. It may be an external bounded
 * buffer or exactly the borrowed staging wire; other arena aliases refuse.
 * available_projects is verified context, not a promise to fetch those slots.
 * Return codes preserve active music/cache on refusal; staging may change on a
 * late busy refusal. Existing engine migrations and parameter fitting apply. */
int d8p1_load_runtime(const void *raw, size_t n, unsigned available_projects)
{
    d8p1_view view;
    if (!raw || n>D8P1_LIMIT || available_projects>15 ||
        (raw != main_workspace.d8p1.wire && d8ps_overlap(raw,n,&main_workspace,sizeof main_workspace)))
        return D8RT_BAD;
    if (cv_cpu_active || transport_busy() || transport_req) return D8RT_BUSY;
    if (d8p1_read(&view,raw,n)!=1 || !d8p1_refs_available(&view,available_projects)) return D8RT_BAD;
    d8p1_stage_workspace *stage=main_d8p1_workspace();
    if (raw != stage->wire) memcpy(stage->wire,raw,n);
    if (!d8p1_project_decode(&stage->state,stage->wire,n,available_projects)) return D8RT_BAD;
    int rc=project_restore_runtime_mode(&stage->state.project,1);
    if (rc) return rc==2 ? D8RT_BUSY : D8RT_BAD;
    memcpy(&d8p1_runtime_cache.arrangement,&stage->state.arrangement,sizeof d8p1_runtime_cache.arrangement);
    d8p1_runtime_cache.valid=1;
    return D8RT_OK;
}

/* Stopped, coherent native snapshot. Out/written are caller-owned and may not
 * alias the arena, except out may be exactly its pre-borrowed wire member.
 * A historical nonempty slot chain refuses rather than losing its meaning.
 * No flash writes or device backup protocol are performed here. */
int d8p1_capture_runtime(uint8_t *out, size_t capacity, size_t *written)
{
    if (!out || !written || d8ps_overlap(written,sizeof *written,&main_workspace,sizeof main_workspace) ||
        (out != main_workspace.d8p1.wire && d8ps_overlap(out,capacity,&main_workspace,sizeof main_workspace)))
        return D8RT_BAD;
    if (cv_cpu_active || transport_busy() || transport_req) return D8RT_BUSY;
    d8p1_stage_workspace *stage=main_d8p1_workspace();
    fm1_irq_off();
    if (song.playing || chain_busy() || transport_req || seq_counting()) {
        fm1_irq_on();
        return D8RT_BUSY;
    }
    project_capture(&stage->state.project);
    if (d8p1_runtime_cache.valid)
        memcpy(&stage->state.arrangement,&d8p1_runtime_cache.arrangement,sizeof stage->state.arrangement);
    else
        memset(&stage->state.arrangement,0,sizeof stage->state.arrangement);
    fm1_irq_on();
    return d8p1_project_encode(out,capacity,written,&stage->state) ? D8RT_OK : D8RT_BAD;
}
