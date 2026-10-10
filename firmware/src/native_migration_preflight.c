/* SPDX-License-Identifier: GPL-3.0-only */
/* Native-only readonly full-plan validator. No flash callbacks, runtime adoption,
 * binding or permission. Main loop only; no competing arena borrow/reentry. */
#include "native_migration_preflight.h"
static struct { uint32_t generation, received; uint8_t phase; } d8mp __attribute__((section(".pool")));
static int d8mp_metadata_alias(const void *p,size_t n)
{ return d8ps_overlap(p,n,&cv_cpu_active,sizeof cv_cpu_active)||d8ps_overlap(p,n,&cv_canvas_valid,sizeof cv_canvas_valid)||d8ps_overlap(p,n,&d8mp,sizeof d8mp)||d8ps_overlap(p,n,&migration_owner,sizeof migration_owner)||d8ps_overlap(p,n,&migration_generation,sizeof migration_generation); }
static int d8mp_begin(uint32_t *generation)
{
    if(!generation||d8mp_metadata_alias(generation,sizeof *generation))return D8MP_BAD;
    int rc=main_migration_begin(generation);if(rc)return rc;
    d8mp.generation=*generation;d8mp.received=0;d8mp.phase=1;return D8MP_OK;
}
static int d8mp_receive(uint32_t generation,uint32_t offset,const void *bytes,uint32_t length)
{
    d8mp_workspace *w=main_migration_workspace(generation);
    if(!w||generation!=d8mp.generation)return D8MP_STALE;
    if(d8mp.phase!=1)return D8MP_BUSY;
    if(!bytes||!length||length>256u||offset!=d8mp.received||offset>D8POOL_BYTES||
       length>D8POOL_BYTES-offset||length>256u-(offset&255u)||
       migration_alias(bytes,length)||d8mp_metadata_alias(bytes,length))return D8MP_BAD;
    memcpy(w->plan+offset,bytes,length);d8mp.received+=length;return D8MP_OK;
}
static int d8mp_read(void *context,uint32_t offset,void *out,uint32_t length)
{
    uint32_t generation=*(const uint32_t *)context;
    d8mp_workspace *w=main_migration_workspace(generation);
    if(!w||d8mp.generation!=generation||d8mp.phase!=2||!out||offset>D8POOL_BYTES||length>D8POOL_BYTES-offset)return -1;
    memcpy(out,w->plan+offset,length);return 0;
}
static int d8mp_noerase(void *c,uint32_t a){(void)c;(void)a;return -1;}
static int d8mp_noprogram(void *c,uint32_t a,const void *p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static int d8mp_stopped(void *c)
{ uint32_t g=*(const uint32_t *)c;return main_migration_workspace(g)&&d8mp.generation==g&&d8mp.phase==2; }
static int d8mp_erased(const uint8_t *p,uint32_t n)
{ while(n--)if(*p++!=255)return 0;return 1; }
static int d8mp_validate(uint32_t generation,d8mp_result *result)
{
    d8mp_workspace *w=main_migration_workspace(generation);
    if(!w||generation!=d8mp.generation)return D8MP_STALE;
    if(!result||migration_alias(result,sizeof *result)||d8mp_metadata_alias(result,sizeof *result))return D8MP_BAD;
    if(d8mp.phase!=1||d8mp.received!=D8POOL_BYTES)return D8MP_BUSY;
    d8mp.phase=2;
    d8pool reader={&generation,d8mp_read,d8mp_noerase,d8mp_noprogram,d8mp_stopped};
    d8mp_result checked={0};int rc=d8pool_inventory(&reader,&checked.index);
    if(rc||!(checked.index.present&8u))goto invalid;
    unsigned used=0;
    for(unsigned object=0;object<D8POOL_OBJECTS;object++)if(checked.index.present&(1u<<object)){
        const d8pool_record *r=&checked.index.object[object];
        const uint8_t *block=w->plan+r->block*D8POOL_BLOCK;
        size_t n=0,encoded=0;
        used|=1u<<r->block;
        if(!d8mp_erased(block+32,D8POOL_HEADER-32)||!d8mp_erased(block+D8POOL_HEADER+r->length,D8POOL_BLOCK-D8POOL_HEADER-r->length)||
           d8pool_load(&reader,object,w->stage.wire,sizeof w->stage.wire,&n,checked.index.present&7u)||
           !d8p1_project_decode(&w->stage.state,w->stage.wire,n,checked.index.present&7u)||
           !d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&encoded,&w->stage.state)||
           encoded!=n||memcmp(w->stage.wire,block+D8POOL_HEADER,n))goto invalid;
    }
    /* Recognized old/torn records are legitimate recovery inputs, but not a
     * canonical proposed set. Spare and unused allocations must be erased. */
    for(unsigned b=0;b<D8POOL_BLOCKS;b++)if(!(used&(1u<<b))&&!d8mp_erased(w->plan+b*D8POOL_BLOCK,D8POOL_BLOCK))goto invalid;
    checked.generation=generation;checked.crc=d8p1_crc32(w->plan,D8POOL_BYTES);
    d8mp.phase=4;*result=checked;return D8MP_OK;
invalid:
    d8mp.phase=3;return D8MP_INVALID;
}
static int d8mp_end(uint32_t generation)
{
    if(generation!=d8mp.generation)return D8MP_STALE;
    int rc=main_migration_end(generation);if(!rc){d8mp.phase=0;d8mp.received=0;}return rc;
}
