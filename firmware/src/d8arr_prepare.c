/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual native playback preparation. Existing trusted pool binding only;
 * read-only, stopped main loop. No ownership receipt or production boot grant. */
#ifndef DABBL8_ARR_PREPARE_C
#define DABBL8_ARR_PREPARE_C
static int d8arr_prepare_pool(const d8pool *s)
{
    d8pool_index index,after;d8p1_stage_workspace *stage;d8arr_state next;
    if(cv_cpu_active||migration_owner||!d8p1_runtime_cache.valid)return D8POOL_BUSY;
    const d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;
    if(!a->banks||a->banks>4||!a->scenes||a->scenes>16||!a->rows||a->rows>16)return D8POOL_INVALID;
    memset(&next,0,sizeof next);next.pending=D8ARR_NONE;
    next.banks=a->banks;next.scenes=a->scenes;next.rows=a->rows;
    unsigned used=0;
    for(unsigned b=0;b<a->banks;b++)for(unsigned t=0;t<8;t++){
        if(a->bank[b].project[t]>=3||a->bank[b].track[t]>=8)return D8POOL_UNSUPPORTED;
        next.project[b][t]=a->bank[b].project[t];next.track[b][t]=a->bank[b].track[t];used|=1u<<a->bank[b].project[t];
    }
    for(unsigned i=0;i<a->scenes;i++){
        if(a->scene[i].bank>=a->banks)return D8POOL_INVALID;
        if(a->scene[i].apply)return D8POOL_UNSUPPORTED; /* overlays implemented separately */
        next.scene_bank[i]=a->scene[i].bank;
    }
    for(unsigned i=0;i<a->rows;i++){
        if(a->row[i].scene>=a->scenes||!a->row[i].repeat||a->row[i].repeat>16)return D8POOL_INVALID;
        next.row_scene[i]=a->row[i].scene;next.row_repeat[i]=a->row[i].repeat;
    }
    uint32_t metadata=d8p1_crc32(a,sizeof *a),epoch=d8_capture_change.epoch;
    int rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(used&~index.present)return D8POOL_EMPTY;
    uint32_t f=motion_guard();
    if(!d8pr_stopped((void *)s)||migration_owner||cv_cpu_active){motion_unguard(f);return D8POOL_BUSY;}
    /* Invalid before unlocked bulk copy: Start/Continue abort preparation. A
     * late failure may discard an old suspended cache, never publish half data. */
    chain.native_mode=1;chain.armed=0;chain.native.policy.valid=0;
    chain.native.policy.suspended=0;chain.native.policy.preparing=1;chain.native.policy.input_generation=0;
    motion_unguard(f);
    for(unsigned o=0;o<3;o++)if(used&(1u<<o)){
        size_t n=0;
        if(!chain.native.policy.preparing||!d8pr_stopped((void *)s)){rc=D8POOL_BUSY;goto failed;}
        rc=d8pool_load(s,o,stage->wire,sizeof stage->wire,&n,index.present&7u);if(rc)goto failed;
        if(!d8p1_project_decode(&stage->state,stage->wire,n,index.present&7u)){rc=D8POOL_INVALID;goto failed;}
        const project_t *p=&stage->state.project;
        chain.source[o].motion=p->motion;
        for(unsigned t=0;t<8;t++){
            next.source_engine[o][t]=p->t[t].engine;
            memcpy(chain.source[o].step[t],p->t[t].step,sizeof p->t[t].step);proj_steps(chain.source[o].step[t]);
            for(unsigned j=0;j<4;j++)chain.source[o].timing[t][j]=(int16_t)clamp(p->t[t].p[P_SLEN+j],TP[P_SLEN+j].min,TP[P_SLEN+j].max);
        }
        next.sequence[o]=index.object[o].sequence;next.length[o]=index.object[o].length;next.crc[o]=index.object[o].crc;
    }
    rc=d8pool_inventory(s,&after);if(rc)goto failed;
    if(index.present!=after.present){rc=D8POOL_BUSY;goto failed;}
    for(unsigned o=0;o<4;o++)if(index.present&(1u<<o)) {
        if(index.object[o].sequence!=after.object[o].sequence||index.object[o].length!=after.object[o].length||
           index.object[o].crc!=after.object[o].crc||index.object[o].block!=after.object[o].block){rc=D8POOL_BUSY;goto failed;}
    }
    next.input_generation=chain.native.policy.input_generation;
    d8arr_held_build(next.held);
    int metadata_same=d8p1_runtime_cache.valid&&metadata==d8p1_crc32(a,sizeof *a);
    f=motion_guard();
    if(!chain.native.policy.preparing||!d8pr_stopped((void *)s)||migration_owner||cv_cpu_active||
       !metadata_same||epoch!=d8_capture_change.epoch||next.input_generation!=chain.native.policy.input_generation){
        motion_unguard(f);rc=D8POOL_BUSY;goto failed;
    }
    next.valid=1;next.store_epoch=epoch;
    memcpy(&chain.native.policy,&next,sizeof next);RING_PUBLISH();chain.armed=1;transport_req=1;
    motion_unguard(f);return D8POOL_OK;
failed:
    chain.native.policy.valid=0;chain.native.policy.preparing=0;chain.armed=0;return rc;
}
#endif
