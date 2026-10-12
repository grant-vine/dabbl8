/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after d8pool.c and d8p1_runtime.c in an eight-track unit. */
#include "d8p1_pool_runtime.h"
_Static_assert(NTRK==8,"native persistence requires eight tracks");
static int d8pr_ready(const d8pool *s)
{ return s&&s->read&&s->erase&&s->program&&s->stopped; }
static int d8pr_stopped(void *context)
{
    const d8pool *s=context;
    return !cv_cpu_active&&!migration_owner&&!transport_busy()&&!transport_req&&s->stopped(s->context);
}
static int d8pr_read(void *context,uint32_t off,void *out,uint32_t n)
{ const d8pool *s=context;return s->read(s->context,off,out,n); }
static int d8pr_erase(void *context,uint32_t off)
{ const d8pool *s=context;return s->erase(s->context,off); }
static int d8pr_program(void *context,uint32_t off,const void *in,uint32_t n)
{ const d8pool *s=context;return s->program(s->context,off,in,n); }
static d8pool d8pr_guard(const d8pool *s)
{ d8pool guarded={(void *)s,d8pr_read,d8pr_erase,d8pr_program,d8pr_stopped};return guarded; }
/* Check the entire native set before making any stored identity available.
 * Unknown/invalid metadata in even a different current object refuses use.
 * No missing object is claimed to exist and no blank pool authorizes erase. */
/* Private synchronous preflight ownership, never a cached storage receipt.
 * Recursive pool runtime preflight entrants from a backend callback refuse before
 * borrowing or replacing the shared arena. */
static uint8_t d8pr_preflight_active;
static int d8pr_same_index(const d8pool_index *a,const d8pool_index *b)
{
    if(a->present!=b->present)return 0;
    for(unsigned o=0;o<D8POOL_OBJECTS;o++)if(a->present&(1u<<o)) {
        const d8pool_record *x=&a->object[o],*y=&b->object[o];
        if(x->block!=y->block||x->sequence!=y->sequence||x->length!=y->length||x->crc!=y->crc)return 0;
    }
    return 1;
}
/* Only records from this call's complete inventory enter here. A copied body
 * is checked against that inventory's CRC and actual codec/reference policy.
 * A final complete inventory below checks every current identity and foreign
 * allocation before the staged catalog or write preflight can be published. */
static int d8pr_copy_record(const d8pool *s,const d8pool_record *r,uint8_t *wire,unsigned mask)
{
    d8p1_view view;
    if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    if(s->read(s->context,r->block*D8POOL_BLOCK+D8POOL_HEADER,wire,r->length))return D8POOL_IO;
    if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    if(d8p1_crc32(wire,r->length)!=r->crc||d8p1_read(&view,wire,r->length)!=1||
       !d8p1_refs_available(&view,mask))return D8POOL_INVALID;
    return D8POOL_OK;
}
static int d8pr_preflight_owned(const d8pool *s,d8pool_index *index,d8p1_stage_workspace **stage,d8p1_project_catalog *catalog)
{
    int rc;uint32_t epoch=d8_capture_change.epoch;
    if(!d8pr_ready(s))return D8POOL_INVALID;
    if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    rc=d8pool_inventory(s,index);if(rc)return rc;
    if(!index->present)return D8POOL_EMPTY;
    *stage=main_d8p1_workspace_try();
    if(!*stage)return D8POOL_BUSY;
    if(!d8pr_stopped((void *)s)||epoch!=d8_capture_change.epoch)return D8POOL_BUSY;
    for(unsigned o=0;o<D8POOL_OBJECTS;o++)if(index->present&(1u<<o)) {
        const d8pool_record *r=&index->object[o];
        rc=d8pr_copy_record(s,r,(*stage)->wire,index->present&7u);
        if(rc)return rc;
        if(!d8p1_project_decode(&(*stage)->state,(*stage)->wire,r->length,index->present&7u))return D8POOL_INVALID;
        if(catalog&&o<3) {
            memcpy(catalog->name[o],(*stage)->state.project.name,12);
            catalog->name[o][12]=0;catalog->present|=(uint8_t)(1u<<o);
        }
        if(!d8pr_stopped((void *)s)||epoch!=d8_capture_change.epoch)return D8POOL_BUSY;
    }
    d8pool_index after;
    rc=d8pool_inventory(s,&after);if(rc)return rc;
    if(!d8pr_stopped((void *)s)||epoch!=d8_capture_change.epoch||!d8pr_same_index(index,&after))return D8POOL_BUSY;
    return D8POOL_OK;
}
static int d8pr_preflight(const d8pool *s,d8pool_index *index,d8p1_stage_workspace **stage,d8p1_project_catalog *catalog)
{
    if(d8pr_preflight_active)return D8POOL_BUSY;
    d8pr_preflight_active=1;
    int rc=d8pr_preflight_owned(s,index,stage,catalog);
    d8pr_preflight_active=0;
    return rc;
}
static int d8pr_runtime_status(int rc)
{ return rc==D8RT_OK?D8POOL_OK:rc==D8RT_BUSY?D8POOL_BUSY:D8POOL_INVALID; }
/* Copy UI/editor names before any arena borrow. NULL is handled separately
 * by save-as as "keep the current name"; rename always requires a name. */
static int d8pr_name_copy(char out[13],const char *name)
{
    if(!name)return 0;
    memset(out,0,13);unsigned i=0;
    for(;i<12&&name[i];i++) {
        if((uint8_t)name[i]<32||(uint8_t)name[i]>126)return 0;
        out[i]=name[i];
    }
    return !name[i];
}
static int d8pr_save(const d8pool *s,unsigned object,const char *name)
{
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;int rc;
    char named[13];
    if(object>=D8POOL_OBJECTS||(name&&!d8pr_name_copy(named,name)))return D8POOL_INVALID;
    rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    rc=d8p1_capture_runtime(stage->wire,sizeof stage->wire,&n);
    if(rc)return d8pr_runtime_status(rc);
    if(name) {
        memcpy(stage->state.project.name,named,12);
        if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8POOL_INVALID;
    }
    /* A newly committed project makes its own identity available; other
     * references still require the validated preexisting stored set. */
    unsigned mask=(index.present&7u)|(object<3?1u<<object:0u);
    d8pool guarded=d8pr_guard(s);
    d8arr_invalidate(); /* even postcommit uncertainty revokes suspended playback */
    rc=d8pool_save(&guarded,object,stage->wire,n,mask);
    if(!rc&&object<3) {
        proj_cur=(uint8_t)object;
        if(name)memcpy(proj_name,named,sizeof named);
    }
    return rc;
}
int d8p1_save_pool(const d8pool *s,unsigned object)
{ return d8pr_save(s,object,NULL); }
int d8p1_save_as_pool(const d8pool *s,unsigned object,const char *name)
{ return object<3?d8pr_save(s,object,name):D8POOL_INVALID; }
int d8p1_load_pool_guarded(const d8pool *s,unsigned object,int (*admit)(void))
{
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;int rc;
    if(object>=D8POOL_OBJECTS)return D8POOL_INVALID;
    rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(!(index.present&(1u<<object)))return D8POOL_EMPTY;
    rc=d8pool_load(s,object,stage->wire,sizeof stage->wire,&n,index.present&7u);
    if(rc)return rc;
    rc=d8pr_runtime_status(d8p1_load_runtime_mask_guarded(stage->wire,n,index.present&7u,D8ENGINE_EFFECTIVE_MASK,admit));
    if(!rc)proj_cur=object<3?(uint8_t)object:PROJ_NO_SLOT;
    return rc;
}
int d8p1_load_pool(const d8pool *s,unsigned object)
{ return d8p1_load_pool_guarded(s,object,NULL); }
int d8p1_restore_pool_autosave(const d8pool *s,int allowed)
{ return allowed?d8p1_load_pool(s,3):D8POOL_EMPTY; }

int d8p1_catalog_pool(const d8pool *s,d8p1_project_catalog *out)
{
    if(!out||d8ps_overlap(out,sizeof *out,&main_workspace,sizeof main_workspace))return D8POOL_INVALID;
    d8p1_project_catalog catalog;memset(&catalog,0,sizeof catalog);
    d8pool_index index;d8p1_stage_workspace *stage;
    int rc=d8pr_preflight(s,&index,&stage,&catalog);
    if(!rc)*out=catalog;
    return rc;
}
int d8p1_rename_pool(const d8pool *s,unsigned object,const char *name)
{
    if(object>=3||!name)return D8POOL_INVALID;
    /* Copy before borrowing the shared display/project arena. The caller may
     * provide a name from that arena, but it must remain stable while read. */
    char renamed[13];if(!d8pr_name_copy(renamed,name))return D8POOL_INVALID;
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;
    int rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(!(index.present&(1u<<object)))return D8POOL_EMPTY;
    rc=d8pool_load(s,object,stage->wire,sizeof stage->wire,&n,index.present&7u);if(rc)return rc;
    if(!d8p1_project_decode(&stage->state,stage->wire,n,index.present&7u))return D8POOL_INVALID;
    memcpy(stage->state.project.name,renamed,12);
    if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8POOL_INVALID;
    d8pool guarded=d8pr_guard(s);
    d8arr_invalidate();
    rc=d8pool_save(&guarded,object,stage->wire,n,index.present&7u);
    if(!rc&&proj_cur==object)memcpy(proj_name,renamed,sizeof renamed);
    return rc;
}

int d8p1_autosave_snapshot_pool(const d8pool *s,uint32_t *signature,d8pool_record *record)
{
    if(!signature||!record||d8ps_overlap(signature,sizeof *signature,&main_workspace,sizeof main_workspace)||
       d8ps_overlap(record,sizeof *record,&main_workspace,sizeof main_workspace)||
       d8ps_overlap(signature,sizeof *signature,record,sizeof *record))return D8POOL_INVALID;
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;
    int rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(!(index.present&8u))return D8POOL_EMPTY;
    rc=d8pool_load(s,3,stage->wire,sizeof stage->wire,&n,index.present&7u);if(rc)return rc;
    if(!d8p1_project_decode(&stage->state,stage->wire,n,index.present&7u))return D8POOL_INVALID;
    stage->state.project.sel=0;
    stage->state.project.g[G_SLOT]=stage->state.project.g[G_NAME]=
        stage->state.project.g[G_LOAD]=stage->state.project.g[G_SAVE]=0;
    if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8POOL_INVALID;
    *signature=as_mix(2166136261u,stage->wire,(uint32_t)n);*record=index.object[3];
    return D8POOL_OK;
}
