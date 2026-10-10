/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8pool_mapped.h"
uint32_t d8pool_mapped_address(unsigned block)
{ return block<4?0x97000u+block*D8POOL_BLOCK:block==4?0xe5000u:0u; }
static int dm_range(uint32_t off,uint32_t n)
{ return off<=D8POOL_BYTES&&n<=D8POOL_BYTES-off; }
static int dm_stopped(void *context)
{
    d8pool_mapped *m=context;
    return m&&m->state&&m->physical.stopped&&m->physical.stopped(m->physical.context);
}
static int dm_read(void *context,uint32_t off,void *out,uint32_t n)
{
    d8pool_mapped *m=context;uint8_t *p=out;
    if(!dm_stopped(m)||!m->physical.read||(!out&&n)||!dm_range(off,n))return -1;
    while(n) {
        uint32_t block=off/D8POOL_BLOCK,within=off%D8POOL_BLOCK,k=D8POOL_BLOCK-within;
        if(k>n)k=n;
        if(!dm_stopped(m)||m->physical.read(m->physical.context,d8pool_mapped_address(block)+within,p,k))return -1;
        off+=k;p+=k;n-=k;
    }
    return 0;
}
static int dm_erase(void *context,uint32_t off)
{
    d8pool_mapped *m=context;
    if(!m||m->state!=2||!dm_stopped(m)||!m->physical.erase||off%D8POOL_SECTOR||!dm_range(off,D8POOL_SECTOR))return -1;
    return m->physical.erase(m->physical.context,d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK);
}
static int dm_program(void *context,uint32_t off,const void *in,uint32_t n)
{
    d8pool_mapped *m=context;
    if(!m||m->state!=2||!dm_stopped(m)||!m->physical.program||!in||!n||n>256||
       (off&255u)+n>256||!dm_range(off,n))return -1;
    return m->physical.program(m->physical.context,d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK,in,n);
}
void d8pool_mapped_close(d8pool_mapped *m)
{ if(m)m->state=0; }
int d8pool_mapped_open(d8pool_mapped *m,const d8pool_physical *physical,int authorized)
{
    if(!m)return D8POOL_INVALID;
    /* Copy first: callers may reuse &m->physical for a new session. */
    d8pool_physical io={0};if(physical)io=*physical;
    m->state=0;m->physical=io;
    m->pool=(d8pool){m,dm_read,dm_erase,dm_program,dm_stopped};
    if(!io.read||!io.erase||!io.program||!io.stopped)return D8POOL_INVALID;
    if(!authorized)return D8POOL_UNSUPPORTED;
    m->state=1;
    if(!dm_stopped(m)){m->state=0;return D8POOL_BUSY;}
    d8pool_index index;int rc=d8pool_inventory(&m->pool,&index);
    if(!rc&&!index.present)rc=D8POOL_EMPTY;
    if(!rc&&!dm_stopped(m))rc=D8POOL_BUSY;
    if(rc){m->state=0;return rc;}
    m->state=2;return D8POOL_OK;
}
