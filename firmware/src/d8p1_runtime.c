/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Include after project.c in an eight-track translation unit. Main-loop API;
 * no device protocol or persistent write is enabled by these adoption APIs. */
#include "d8p1_project.h"
enum { D8RT_OK=0, D8RT_BAD=1, D8RT_BUSY=2, D8RT_MISSING_ENGINE=3 };

/* Const cache view survives drawing. Invalidated by the next successful native
 * or historical load. Main-loop readers only; never retain it across a load. */
const d8p1_arrangement *d8p1_runtime_arrangement(void)
{
    return d8p1_runtime_cache.valid ? &d8p1_runtime_cache.arrangement : 0;
}

/* Read-only schema-1 preview after the real structural/reference validator.
 * Known missing engines refuse before acquiring the display/project arena.
 * This is not a replacement for decoding, native semantic policy or the final
 * normalized project guard. IDs and every late sound target use the same
 * per-track preview as structured project adoption. */
static int d8p1_runtime_engines(const d8p1_view *view,unsigned mask)
{
    _Static_assert(NTRK==8 && P_COUNT==99 && P_E0==91 && NSTEP==64,"schema-1 preview offsets");
    for(unsigned c=0;c<view->count;c++)if((view->chunk[c].type&0x7fffu)==2u){
        const uint8_t *tracks=view->chunk[c].data;
        for(unsigned k=0;k<8;k++){
            const uint8_t *t=tracks+k*(99u+2u+64u*9u);int16_t values[8];
            if(t[99]>=NENGINES)return D8RT_BAD;
            for(unsigned i=0;i<8;i++)values[i]=(int16_t)t[91+i]-64;
            if(!proj_runtime_track_available(k,t[99],t[100],values,8,2,mask))return D8RT_MISSING_ENGINE;
        }
        return D8RT_OK;
    }
    return D8RT_BAD;
}

/* Call in the main loop with interrupts enabled.
 * Input must remain immutable until copied. It may be an external bounded
 * buffer or exactly the borrowed staging wire; other arena aliases refuse.
 * available_projects is verified context, not a promise to fetch those slots.
 * Return codes preserve active music/cache on refusal; staging may change on a
 * late busy refusal. Existing engine migrations and parameter fitting apply. */
static int d8p1_load_runtime_mask_guarded(const void *raw,size_t n,unsigned available_projects,unsigned engine_mask,int (*admit)(void))
{
    d8p1_view view;
    if (!engine_mask || (engine_mask&~D8ENGINE_EFFECTIVE_MASK) || !raw || n>D8P1_LIMIT || available_projects>15 ||
        (raw != main_workspace.d8p1.wire && d8ps_overlap(raw,n,&main_workspace,sizeof main_workspace)))
        return D8RT_BAD;
    if (cv_cpu_active || migration_owner || transport_busy() || transport_req) return D8RT_BUSY;
    if (d8p1_read(&view,raw,n)!=1 || !d8p1_refs_available(&view,available_projects)) return D8RT_BAD;
    int preview=d8p1_runtime_engines(&view,engine_mask);if(preview)return preview;
    d8p1_stage_workspace *stage=main_d8p1_workspace_try();
    if (!stage) return D8RT_BUSY;
    if (raw != stage->wire) memcpy(stage->wire,raw,n);
    if (!d8p1_project_decode(&stage->state,stage->wire,n,available_projects)) return D8RT_BAD;
    int rc=project_restore_runtime_mask_guarded(&stage->state.project,1,engine_mask,admit);
    if (rc) return rc==PROJ_RUNTIME_MISSING_ENGINE ? D8RT_MISSING_ENGINE : rc==2 ? D8RT_BUSY : D8RT_BAD;
    d8arr_invalidate();
    memcpy(&d8p1_runtime_cache.arrangement,&stage->state.arrangement,sizeof d8p1_runtime_cache.arrangement);
    d8p1_runtime_cache.valid=1;
    return D8RT_OK;
}
static int d8p1_load_runtime_mask(const void *raw,size_t n,unsigned projects,unsigned engines)
{ return d8p1_load_runtime_mask_guarded(raw,n,projects,engines,NULL); }
int d8p1_load_runtime(const void *raw,size_t n,unsigned available_projects)
{
    return d8p1_load_runtime_mask(raw,n,available_projects,D8ENGINE_EFFECTIVE_MASK);
}

/* Shared coherent structured capture. Main-loop entry with interrupts enabled;
 * borrowing fences LCD DMA, then rechecks transport under the existing IRQ guard.
 * Wire encoding/validation belongs to each caller; no snapshot survives drawing. */
static int d8p1_capture_state_runtime(void)
{
    if (cv_cpu_active || migration_owner || transport_busy() || transport_req) return D8RT_BUSY;
    d8p1_stage_workspace *stage=main_d8p1_workspace_try();
    if (!stage) return D8RT_BUSY;
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
    int rc=d8p1_capture_state_runtime();
    if (rc) return rc;
    d8p1_stage_workspace *stage=&main_workspace.d8p1;
    return d8p1_project_encode(out,capacity,written,&stage->state) ? D8RT_OK : D8RT_BAD;
}

/* A stopped coherent signature of exactly the canonical native musical state.
 * Borrow the existing staging arena; no retained cache or flash write. Selected
 * track and project-page controls deliberately do not mark music dirty. As with
 * upstream autosave, this 32-bit change detector is not an integrity proof.
 * Output must be main-loop owned and outside the borrowed arena. */
int d8p1_signature_runtime(uint32_t *signature)
{
    if (!signature || d8ps_overlap(signature,sizeof *signature,&main_workspace,sizeof main_workspace))
        return D8RT_BAD;
    d8p1_stage_workspace *stage=&main_workspace.d8p1;
    size_t n=0;
    int rc=d8p1_capture_state_runtime();
    if(rc)return rc;
    /* Validate before normalizing: an invalid selected track must still refuse.
     * This preserves capture's native-state refusals without the first wire CRC. */
    if(!d8ps_valid(&stage->state))return D8RT_BAD;
    stage->state.project.sel=0;
    stage->state.project.g[G_SLOT]=stage->state.project.g[G_NAME]=
        stage->state.project.g[G_LOAD]=stage->state.project.g[G_SAVE]=0;
    if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8RT_BAD;
    *signature=as_mix(2166136261u,stage->wire,(uint32_t)n);
    return D8RT_OK;
}
