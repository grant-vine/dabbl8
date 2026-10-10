/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8store.h"
#include "d8p1.h"

_Static_assert(D8STORE_PAYLOAD+D8P1_LIMIT==D8STORE_COPY,"two-sector payload capacity");
#define DS_MAGIC 0x42413844u /* D8AB, little endian */
static uint32_t ds_get(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static void ds_put(uint8_t *p, uint32_t n)
{ p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8); p[2]=(uint8_t)(n>>16); p[3]=(uint8_t)(n>>24); }
static int ds_valid(const d8store *s)
{ return s && s->read && s->erase && s->program && s->stopped; }
/* Streaming CRC keeps the storage scan independent of the 59,520-byte arena. */
static uint32_t ds_crc(uint32_t c, const uint8_t *p, uint32_t n)
{
    while (n--) {
        unsigned bit;
        c ^= *p++;
        for (bit=0; bit<8; ++bit) c=(c>>1) ^ (0xedb88320u & (0u-(c&1u)));
    }
    return c;
}
static int ds_scan(const d8store *s, unsigned copy, d8store_record *r)
{
    uint8_t h[32], page[256];
    uint32_t base=copy*D8STORE_COPY, off, crc=0xffffffffu;
    if (s->read(s->context,base,h,sizeof h)) return D8STORE_IO;
    if (ds_get(h)!=DS_MAGIC || ds_get(h+28)!=d8p1_crc32(h,28)) return D8STORE_EMPTY;
    if (h[4]!=1) return D8STORE_UNSUPPORTED;
    if (h[5]!=copy || h[6] || h[7] ||
        ds_get(h+8)!=s->object || !ds_get(h+16) || ds_get(h+16)>D8P1_LIMIT ||
        ds_get(h+24)!=0xffffffffu || ds_get(h+28)!=d8p1_crc32(h,28)) return D8STORE_EMPTY;
    for (off=0; off<ds_get(h+16); off+=sizeof page) {
        uint32_t n=ds_get(h+16)-off;
        if (n>sizeof page) n=sizeof page;
        if (s->read(s->context,base+D8STORE_PAYLOAD+off,page,n)) return D8STORE_IO;
        crc=ds_crc(crc,page,n);
    }
    if (~crc!=ds_get(h+20)) return D8STORE_EMPTY;
    r->copy=copy; r->sequence=ds_get(h+12); r->length=ds_get(h+16); r->crc=ds_get(h+20);
    return D8STORE_OK;
}
int d8store_current(const d8store *s, d8store_record *record)
{
    d8store_record a,b;
    int va,vb;
    if (!ds_valid(s) || !record) return D8STORE_INVALID;
    va=ds_scan(s,0,&a); vb=ds_scan(s,1,&b);
    if (va==D8STORE_IO || vb==D8STORE_IO) return D8STORE_IO;
    if (va==D8STORE_UNSUPPORTED || vb==D8STORE_UNSUPPORTED) return D8STORE_UNSUPPORTED;
    if (va && vb) return D8STORE_EMPTY;
    if (!vb && (va || (b.sequence!=a.sequence && b.sequence-a.sequence<0x80000000u))) *record=b;
    else *record=a;
    return D8STORE_OK;
}
int d8store_load(const d8store *s, void *out, size_t capacity, size_t *written, unsigned mask)
{
    d8store_record r;
    d8p1_view view;
    int rc;
    if (!ds_valid(s) || !out || !written || mask>15) return D8STORE_INVALID;
    rc=d8store_current(s,&r);
    if (rc) return rc;
    if (capacity<r.length) return D8STORE_INVALID;
    if (s->read(s->context,r.copy*D8STORE_COPY+D8STORE_PAYLOAD,out,r.length)) return D8STORE_IO;
    /* Also detect a backend changing the payload between scan and copy. */
    if (d8p1_crc32(out,r.length)!=r.crc || d8p1_read(&view,out,r.length)!=1 ||
        !d8p1_refs_available(&view,mask)) return D8STORE_INVALID;
    *written=r.length;
    return D8STORE_OK;
}
int d8store_save(const d8store *s, const void *data, size_t length, unsigned mask)
{
    d8store_record old,check;
    d8p1_view view;
    uint8_t h[32];
    uint32_t base,off,sequence;
    unsigned copy;
    int rc;
    if (!ds_valid(s) || !data || !length || length>D8P1_LIMIT || mask>15 ||
        d8p1_read(&view,data,length)!=1 || !d8p1_refs_available(&view,mask)) return D8STORE_INVALID;
    if (!s->stopped(s->context)) return D8STORE_BUSY;
    rc=d8store_current(s,&old);
    if (rc!=D8STORE_OK && rc!=D8STORE_EMPTY) return rc;
    copy=rc==D8STORE_EMPTY ? 0u : old.copy^1u;
    sequence=rc==D8STORE_EMPTY ? 1u : old.sequence+1u;
    base=copy*D8STORE_COPY;
    for (unsigned i=0;i<sizeof h;++i) h[i]=0;
    ds_put(h,DS_MAGIC); h[4]=1; h[5]=(uint8_t)copy;
    ds_put(h+8,s->object); ds_put(h+12,sequence); ds_put(h+16,(uint32_t)length);
    ds_put(h+20,d8p1_crc32(data,length)); ds_put(h+24,0xffffffffu); ds_put(h+28,d8p1_crc32(h,28));
    for (off=0; off<D8STORE_COPY; off+=D8STORE_SECTOR) {
        if (!s->stopped(s->context)) return D8STORE_BUSY;
        if (s->erase(s->context,base+off)) return D8STORE_IO;
    }
    for (off=0; off<length; off+=256u) {
        uint32_t n=(uint32_t)length-off;
        if (n>256u) n=256u;
        if (!s->stopped(s->context)) return D8STORE_BUSY;
        if (s->program(s->context,base+D8STORE_PAYLOAD+off,(const uint8_t *)data+off,n)) return D8STORE_IO;
    }
    if (!s->stopped(s->context)) return D8STORE_BUSY;
    if (s->program(s->context,base,h,sizeof h)) return D8STORE_IO;
    rc=ds_scan(s,copy,&check);
    if (rc || check.sequence!=sequence || check.length!=length || check.crc!=ds_get(h+20)) return D8STORE_IO;
    return D8STORE_OK;
}
