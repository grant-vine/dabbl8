/* SPDX-License-Identifier: GPL-3.0-only
 * FUN9 packed-step / D8M1 field conventions derive from Felucca:
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments.
 * Bounded D8P1 proposal-1 byte codec. No runtime adoption, map or flash calls. */
#include "d8p1.h"
/* Byte helpers keep this standalone module usable with the freestanding target.
 * Compiler-generated struct copies may still use Felucca's existing memcpy. */
static void copy_bytes(void *out,const void *in,size_t n) {
    uint8_t *d=out;const uint8_t *s=in;for(size_t i=0;i<n;i++)d[i]=s[i];
}
static void zero_bytes(void *out,size_t n) {
    uint8_t *d=out;for(size_t i=0;i<n;i++)d[i]=0;
}
static int equal_bytes(const void *a,const void *b,size_t n) {
    const uint8_t *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return 0;return 1;
}
static unsigned u16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static uint32_t u32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void w16(uint8_t *p, unsigned v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i)); }
static uint32_t crc_byte(uint32_t c,unsigned b) {
    c^=b;for(unsigned i=0;i<8;i++)c=(c>>1)^((c&1)?0xEDB88320u:0);return c;
}
uint32_t d8p1_crc32(const void *data,size_t n) {
    const uint8_t *p=data;uint32_t c=0xFFFFFFFFu;
    if(!p&&n)return 0;for(size_t i=0;i<n;i++)c=crc_byte(c,p[i]);return c^0xFFFFFFFFu;
}
static uint32_t file_crc(const uint8_t *p,size_t n) {
    uint32_t c=0xFFFFFFFFu;for(size_t i=0;i<n;i++)c=crc_byte(c,i>=12&&i<16?0:p[i]);return c^0xFFFFFFFFu;
}
static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int name_ok(const uint8_t *p) {
    unsigned zero=0;for(unsigned i=0;i<12;i++){if(!p[i])zero=1;else if(zero||p[i]<32||p[i]>126)return 0;}return 1;
}
/* Pinned schema-1 sound-only policy, checked against real motion_param in tests. */
static int motion_id(unsigned id) {
    return id<99&&(id<=16||(id>=33&&id<=36)||id==38||id==39||id==44||(id>=61&&id<=80)||id>=83);
}
static int chunk_ok(const d8p1_chunk *c) {
    const uint8_t *p=c->data;size_t n=c->length;unsigned id=c->type&0x7FFFu;
    if(!p||!id)return 0;
    switch(id){
    case 1:return n==54; /* Explicit signed LE16 globals, never native structs. */
    case 2:
        if(n!=5416)return 0;
        for(unsigned t=0;t<8;t++){
            const uint8_t *track=p+t*677u;
            for(unsigned i=0;i<99;i++)if(track[i]>191)return 0;
            if(track[99]>=14)return 0; /* Preset byte retains legacy FE/FF sentinels. */
            for(unsigned i=0;i<64;i++){
                const uint8_t *s=track+101+i*9;
                for(unsigned j=0;j<4;j++)if(s[j]>127)return 0;
                if((s[4]&7)>4||((s[4]>>3)&3)>2||(s[4]&128)||(s[7]&~s[6])||(s[8]&127)>101)return 0;
            }
        }return 1;
    case 3:
        if(n!=1024)return 0;for(size_t i=0;i<n;i++)if(p[i]>127)return 0;return 1;
    case 4:
        if(n<8||!equal_bytes(p,"D8M1",4)||p[4]>64||p[6]||p[7]||n!=8u+p[4]*5u)return 0;
        for(unsigned i=0;i<p[4];i++){
            const uint8_t *a=p+8+i*5;int value=(int)u16(a+3);if(value>=32768)value-=65536;
            if(a[0]>=8||a[1]>=64||!motion_id(a[2]&127)||value< -64||value>127)return 0;
            for(unsigned j=0;j<i;j++){const uint8_t *b=p+8+j*5;if(a[0]==b[0]&&a[1]==b[1]&&(a[2]&127)==(b[2]&127))return 0;}
        }return 1;
    case 5:
        if(n<1||p[0]>16||n!=1u+p[0]*2u)return 0;
        for(unsigned i=0;i<p[0];i++)if(p[1+i*2]>=16||!p[2+i*2]||p[2+i*2]>16)return 0;return 1;
    case 6:return n==16&&name_ok(p)&&p[12]<8&&p[13]==2&&!p[14]&&!p[15];
    case 7:
        if(n<1||p[0]>4||n!=1u+p[0]*28u)return 0;
        for(unsigned i=0;i<p[0];i++){
            const uint8_t *b=p+1+i*28;if(!name_ok(b))return 0;
            for(unsigned t=0;t<8;t++)if(b[12+t*2]>=4||b[13+t*2]>=8)return 0;
        }return 1;
    case 8:
        if(n<1||p[0]>16||n!=1u+p[0]*39u)return 0;
        for(unsigned i=0;i<p[0];i++){
            const uint8_t *b=p+1+i*39;if(!name_ok(b)||b[12]>=4||(b[13]&~7u))return 0;
            for(unsigned t=0;t<8;t++)if(b[15+t*3]>127||b[16+t*3]>127||b[17+t*3]>48)return 0;
        }return 1;
    default:return !(c->type&D8P1_REQUIRED); /* Bounded, CRC-checked inspect-only. */
    }
}
static const d8p1_chunk *find(const d8p1_view *v,unsigned id) {
    for(unsigned i=0;i<v->count;i++)if((v->chunk[i].type&0x7FFFu)==id)return &v->chunk[i];return NULL;
}
static int fields_ok(const d8p1_view *v,int unknown_ok) {
    unsigned mandatory=0;
    if(!v||v->count>8||v->count<6)return 0;
    for(unsigned i=0;i<v->count;i++){
        const d8p1_chunk *c=&v->chunk[i];unsigned id=c->type&0x7FFFu;
        if(!chunk_ok(c)||(!unknown_ok&&id>8))return 0;
        for(unsigned j=0;j<i;j++)if((v->chunk[j].type&0x7FFFu)==id)return 0;
        if(id<=6)mandatory|=1u<<id;
    }
    if(mandatory!=126u)return 0;
    const d8p1_chunk *banks=find(v,7),*scenes=find(v,8),*chain=find(v,5);
    unsigned nb=banks?banks->data[0]:0,ns=scenes?scenes->data[0]:0;
    for(unsigned i=0;i<chain->data[0];i++)if(chain->data[1+i*2]>=ns)return 0;
    for(unsigned i=0;i<ns;i++)if(scenes->data[1+i*39+12]>=nb)return 0;
    return 1;
}
int d8p1_read(d8p1_view *out,const void *data,size_t n) {
    const uint8_t *p=data;d8p1_view v;size_t pos=32;
    if(!out||!p||n<32||n>D8P1_LIMIT||overlaps(out,sizeof *out,p,n))return 0;
    if(!equal_bytes(p,"D8P1",4)||u16(p+4)!=1||u16(p+6)!=32||u32(p+8)!=n||
       p[16]!=8||p[17]!=64||p[18]!=99||p[19]!=27||p[20]>8||p[21]!=1||p[22]!=1)return 0;
    for(unsigned i=23;i<32;i++)if(p[i])return 0;
    if(u32(p+12)!=file_crc(p,n))return 0;
    zero_bytes(&v,sizeof v);v.original=p;v.length=n;v.count=p[20];
    for(unsigned i=0;i<v.count;i++){
        if(pos>n||n-pos<8)return 0;
        d8p1_chunk *c=&v.chunk[i];c->type=(uint16_t)u16(p+pos);c->length=(uint16_t)u16(p+pos+2);
        uint32_t crc=u32(p+pos+4);pos+=8;
        if(c->length>n-pos)return 0;c->data=p+pos;
        if(crc!=d8p1_crc32(c->data,c->length))return 0;
        if((c->type&0x7FFFu)>8)v.readonly=1;
        pos+=c->length;
    }
    if(pos!=n||!fields_ok(&v,1))return 0;
    *out=v;return v.readonly?2:1;
}
int d8p1_write(uint8_t *out,size_t cap,size_t *written,const d8p1_view *v) {
    size_t total=32,pos=32;
    if(!out||!written||!v||v->readonly||!fields_ok(v,0))return 0;
    if(v->original){d8p1_view original;if(d8p1_read(&original,v->original,v->length)!=1)return 0;}
    for(unsigned i=0;i<v->count;i++){
        if(total>D8P1_LIMIT-8||v->chunk[i].length>D8P1_LIMIT-total-8)return 0;
        total+=8u+v->chunk[i].length;
    }
    if(cap<total||(v->original&&(v->length>D8P1_LIMIT||overlaps(out,total,v->original,v->length)||overlaps(written,sizeof *written,v->original,v->length)))||overlaps(out,total,v,sizeof *v)||overlaps(out,total,written,sizeof *written)||overlaps(written,sizeof *written,v,sizeof *v))return 0;
    for(unsigned i=0;i<v->count;i++)if(overlaps(out,total,v->chunk[i].data,v->chunk[i].length)||overlaps(written,sizeof *written,v->chunk[i].data,v->chunk[i].length))return 0;
    zero_bytes(out,32);copy_bytes(out,"D8P1",4);w16(out+4,1);w16(out+6,32);w32(out+8,(uint32_t)total);
    out[16]=8;out[17]=64;out[18]=99;out[19]=27;out[20]=v->count;out[21]=out[22]=1;
    for(unsigned id=1;id<=8;id++){
        const d8p1_chunk *c=find(v,id);if(!c)continue;
        w16(out+pos,c->type);w16(out+pos+2,c->length);w32(out+pos+4,d8p1_crc32(c->data,c->length));
        copy_bytes(out+pos+8,c->data,c->length);pos+=8u+c->length;
    }
    w32(out+12,file_crc(out,total));*written=total;return 1;
}
int d8p1_refs_available(const d8p1_view *v,unsigned mask) {
    if(mask>15||!v||v->readonly||!fields_ok(v,0))return 0;
    if(v->original){d8p1_view original;if(d8p1_read(&original,v->original,v->length)!=1)return 0;}
    const d8p1_chunk *c=find(v,7);if(!c)return 1;
    for(unsigned i=0;i<c->data[0];i++)for(unsigned t=0;t<8;t++)
        if(!(mask&(1u<<c->data[1+i*28+12+t*2])))return 0;
    return 1;
}
