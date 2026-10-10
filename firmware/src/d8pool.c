/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8pool.h"
#include "d8p1.h"
_Static_assert(D8POOL_HEADER+D8P1_LIMIT==D8POOL_BLOCK,"payload capacity");
static uint32_t dp_get(const uint8_t *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void dp_put(uint8_t *p,uint32_t n)
{ for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i)); }
static int dp_valid(const d8pool *s)
{ return s&&s->read&&s->erase&&s->program&&s->stopped; }
static uint32_t dp_crc(uint32_t c,const uint8_t *p,uint32_t n)
{
    while(n--) { c^=*p++; for(unsigned bit=0;bit<8;bit++)c=(c>>1)^(0xedb88320u&(0u-(c&1u))); }
    return c;
}
static int dp_scan(const d8pool *s,unsigned block,d8pool_record *r,unsigned *object)
{
    uint8_t h[32],page[256]; uint32_t base=block*D8POOL_BLOCK,c=0xffffffffu;
    if(s->read(s->context,base,h,sizeof h))return D8POOL_IO;
    if(dp_get(h+28)!=d8p1_crc32(h,28)) {
        /* A torn write/erase may leave any magic bit between erased 1 and
         * committed 0. Unrecognisable foreign bytes require migration. */
        for(unsigned i=0;i<4;i++) {
            uint8_t expected=(uint8_t)(D8POOL_MAGIC>>(i*8));
            if((h[i]&expected)!=expected)return D8POOL_UNSUPPORTED;
        }
        return D8POOL_EMPTY;
    }
    if(dp_get(h)!=D8POOL_MAGIC||h[4]!=1||h[6]||h[7]||dp_get(h+8)>=D8POOL_OBJECTS||dp_get(h+24)!=0xffffffffu)
        return D8POOL_UNSUPPORTED;
    if(h[5]!=block||!dp_get(h+16)||dp_get(h+16)>D8P1_LIMIT)return D8POOL_EMPTY;
    for(uint32_t off=0;off<dp_get(h+16);off+=sizeof page) {
        uint32_t n=dp_get(h+16)-off; if(n>sizeof page)n=sizeof page;
        if(s->read(s->context,base+D8POOL_HEADER+off,page,n))return D8POOL_IO;
        c=dp_crc(c,page,n);
    }
    if(~c!=dp_get(h+20))return D8POOL_EMPTY;
    r->block=block;r->sequence=dp_get(h+12);r->length=dp_get(h+16);r->crc=dp_get(h+20);*object=dp_get(h+8);
    return D8POOL_OK;
}
int d8pool_inventory(const d8pool *s,d8pool_index *out)
{
    d8pool_record records[D8POOL_BLOCKS]; unsigned object[D8POOL_BLOCKS],valid=0;
    d8pool_index index={0};
    if(!dp_valid(s)||!out)return D8POOL_INVALID;
    for(unsigned i=0;i<D8POOL_BLOCKS;i++) {
        int rc=dp_scan(s,i,&records[i],&object[i]);
        if(rc==D8POOL_EMPTY)continue;
        if(rc)return rc;
        valid|=1u<<i; unsigned id=object[i];
        if(!(index.present&(1u<<id))||
           (records[i].sequence!=index.object[id].sequence&&records[i].sequence-index.object[id].sequence<0x80000000u))index.object[id]=records[i];
        index.present|=1u<<id;
    }
    /* Require one unambiguous newest record, including cyclic wrap sets and
     * equal generations in distinct blocks. Shared-spare writers never
     * create those duplicates, so refuse even matching CRC/length pairs. */
    for(unsigned i=0;i<D8POOL_BLOCKS;i++)if(valid&(1u<<i)) {
        d8pool_record *best=&index.object[object[i]];uint32_t d=best->sequence-records[i].sequence;
        if(d>=0x80000000u||(!d&&best->block!=records[i].block))return D8POOL_AMBIGUOUS;
    }
    *out=index;return D8POOL_OK;
}
int d8pool_current(const d8pool *s,unsigned object,d8pool_record *out)
{
    d8pool_index index;int rc;
    if(object>=D8POOL_OBJECTS||!out)return D8POOL_INVALID;
    rc=d8pool_inventory(s,&index);if(rc)return rc;
    if(!(index.present&(1u<<object)))return D8POOL_EMPTY;
    *out=index.object[object];return D8POOL_OK;
}
int d8pool_load(const d8pool *s,unsigned object,void *out,size_t capacity,size_t *written,unsigned mask)
{
    d8pool_record r;d8p1_view view;int rc;
    if(!out||!written||mask>7)return D8POOL_INVALID;
    rc=d8pool_current(s,object,&r);if(rc)return rc;
    if(capacity<r.length)return D8POOL_INVALID;
    /* Publishing length must not overwrite the validated wire. Use subtraction
     * to avoid endpoint overflow when checking unrelated address ranges. */
    uintptr_t a=(uintptr_t)out,b=(uintptr_t)written;
    if((a<=b&&b-a<r.length)||(b<a&&a-b<sizeof *written))return D8POOL_INVALID;
    if(s->read(s->context,r.block*D8POOL_BLOCK+D8POOL_HEADER,out,r.length))return D8POOL_IO;
    if(d8p1_crc32(out,r.length)!=r.crc||d8p1_read(&view,out,r.length)!=1||!d8p1_refs_available(&view,mask))return D8POOL_INVALID;
    *written=r.length;return D8POOL_OK;
}
int d8pool_save(const d8pool *s,unsigned object,const void *data,size_t length,unsigned mask)
{
    d8p1_view view;d8pool_index index;d8pool_record check;
    uint8_t h[32]={0};unsigned used=0,dest,check_object;uint32_t base,sequence;int rc;
    if(!dp_valid(s)||object>=D8POOL_OBJECTS||!data||!length||length>D8P1_LIMIT||mask>7||
       d8p1_read(&view,data,length)!=1||!d8p1_refs_available(&view,mask))return D8POOL_INVALID;
    if(!s->stopped(s->context))return D8POOL_BUSY;
    rc=d8pool_inventory(s,&index);if(rc)return rc;
    for(unsigned i=0;i<D8POOL_OBJECTS;i++)if(index.present&(1u<<i))used|=1u<<index.object[i].block;
    for(dest=0;dest<D8POOL_BLOCKS;dest++)if(!(used&(1u<<dest)))break;
    if(dest==D8POOL_BLOCKS)return D8POOL_INVALID;
    sequence=(index.present&(1u<<object))?index.object[object].sequence+1u:1u;base=dest*D8POOL_BLOCK;
    /* A recovered pool can contain very old generations. Refuse a write that
     * would make the new sequence ambiguous against a retained old block. */
    for(unsigned i=0;i<D8POOL_BLOCKS;i++)if(i!=dest) {
        rc=dp_scan(s,i,&check,&check_object);
        if(rc==D8POOL_EMPTY)continue;
        if(rc)return rc;
        if(check_object==object&&sequence-check.sequence>=0x80000000u)return D8POOL_AMBIGUOUS;
    }
    dp_put(h,D8POOL_MAGIC);h[4]=1;h[5]=(uint8_t)dest;dp_put(h+8,object);dp_put(h+12,sequence);
    dp_put(h+16,(uint32_t)length);dp_put(h+20,d8p1_crc32(data,length));dp_put(h+24,0xffffffffu);dp_put(h+28,d8p1_crc32(h,28));
    for(uint32_t off=0;off<D8POOL_BLOCK;off+=D8POOL_SECTOR) {
        if(!s->stopped(s->context))return D8POOL_BUSY;
        if(s->erase(s->context,base+off))return D8POOL_IO;
    }
    for(uint32_t off=0;off<length;off+=256u) {
        uint32_t n=(uint32_t)length-off;if(n>256u)n=256u;
        if(!s->stopped(s->context))return D8POOL_BUSY;
        if(s->program(s->context,base+D8POOL_HEADER+off,(const uint8_t *)data+off,n))return D8POOL_IO;
    }
    if(!s->stopped(s->context))return D8POOL_BUSY;
    if(s->program(s->context,base,h,sizeof h))return D8POOL_IO;
    rc=dp_scan(s,dest,&check,&check_object);
    if(rc||check_object!=object||check.sequence!=sequence||check.length!=length||check.crc!=dp_get(h+20))return D8POOL_IO;
    return D8POOL_OK;
}
