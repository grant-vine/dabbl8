/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after d8pool.c and d8p1_runtime.c in an eight-track unit. */
#include "d8p1_pool_runtime.h"
_Static_assert(NTRK==8,"native persistence requires eight tracks");
static int d8pr_ready(const d8pool *s)
{ return s&&s->read&&s->erase&&s->program&&s->stopped; }
static int d8pr_stopped(void *context)
{
    const d8pool *s=context;
    return !cv_cpu_active&&!transport_busy()&&!transport_req&&s->stopped(s->context);
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
static int d8pr_preflight(const d8pool *s,d8pool_index *index,d8p1_stage_workspace **stage,d8p1_project_catalog *catalog)
{
    int rc;
    if(!d8pr_ready(s))return D8POOL_INVALID;
    if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    rc=d8pool_inventory(s,index);if(rc)return rc;
    if(!index->present)return D8POOL_EMPTY;
    *stage=main_d8p1_workspace();
    if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    for(unsigned o=0;o<D8POOL_OBJECTS;o++)if(index->present&(1u<<o)) {
        size_t n=0;
        rc=d8pool_load(s,o,(*stage)->wire,sizeof (*stage)->wire,&n,index->present&7u);
        if(rc)return rc;
        if(!d8p1_project_decode(&(*stage)->state,(*stage)->wire,n,index->present&7u))return D8POOL_INVALID;
        if(catalog&&o<3) {
            memcpy(catalog->name[o],(*stage)->state.project.name,12);
            catalog->name[o][12]=0;catalog->present|=(uint8_t)(1u<<o);
        }
        if(!d8pr_stopped((void *)s))return D8POOL_BUSY;
    }
    return D8POOL_OK;
}
static int d8pr_runtime_status(int rc)
{ return rc==D8RT_OK?D8POOL_OK:rc==D8RT_BUSY?D8POOL_BUSY:D8POOL_INVALID; }
int d8p1_save_pool(const d8pool *s,unsigned object)
{
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;int rc;
    if(object>=D8POOL_OBJECTS)return D8POOL_INVALID;
    rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    rc=d8p1_capture_runtime(stage->wire,sizeof stage->wire,&n);
    if(rc)return d8pr_runtime_status(rc);
    /* A newly committed project makes its own identity available; other
     * references still require the validated preexisting stored set. */
    unsigned mask=(index.present&7u)|(object<3?1u<<object:0u);
    d8pool guarded=d8pr_guard(s);
    rc=d8pool_save(&guarded,object,stage->wire,n,mask);
    if(!rc&&object<3)proj_cur=(uint8_t)object;
    return rc;
}
int d8p1_load_pool(const d8pool *s,unsigned object)
{
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;int rc;
    if(object>=D8POOL_OBJECTS)return D8POOL_INVALID;
    rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(!(index.present&(1u<<object)))return D8POOL_EMPTY;
    rc=d8pool_load(s,object,stage->wire,sizeof stage->wire,&n,index.present&7u);
    if(rc)return rc;
    rc=d8pr_runtime_status(d8p1_load_runtime(stage->wire,n,index.present&7u));
    if(!rc)proj_cur=object<3?(uint8_t)object:PROJ_NO_SLOT;
    return rc;
}
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
    char renamed[13];memset(renamed,0,sizeof renamed);
    unsigned i=0;
    for(;i<12&&name[i];i++) {
        if((uint8_t)name[i]<32||(uint8_t)name[i]>126)return D8POOL_INVALID;
        renamed[i]=name[i];
    }
    if(name[i])return D8POOL_INVALID;
    d8pool_index index;d8p1_stage_workspace *stage;size_t n=0;
    int rc=d8pr_preflight(s,&index,&stage,NULL);if(rc)return rc;
    if(!(index.present&(1u<<object)))return D8POOL_EMPTY;
    rc=d8pool_load(s,object,stage->wire,sizeof stage->wire,&n,index.present&7u);if(rc)return rc;
    if(!d8p1_project_decode(&stage->state,stage->wire,n,index.present&7u))return D8POOL_INVALID;
    memcpy(stage->state.project.name,renamed,12);
    if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8POOL_INVALID;
    d8pool guarded=d8pr_guard(s);
    rc=d8pool_save(&guarded,object,stage->wire,n,index.present&7u);
    if(!rc&&proj_cur==object)memcpy(proj_name,renamed,sizeof renamed);
    return rc;
}
