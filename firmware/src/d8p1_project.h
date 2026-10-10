/* SPDX-License-Identifier: GPL-3.0-only
 * Packed musical conventions derive from Felucca:
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. */
/* Include after project.c in an eight-track translation unit. These adapters
 * stage structured state only; they never adopt it into engines or write flash.
 * Caller-owned state/workspace avoids hidden project-sized stack allocations. */
#ifndef DABBL8_D8P1_PROJECT_H
#define DABBL8_D8P1_PROJECT_H
#include "d8p1.h"
typedef char d8ps_schema_check[(NTRK==8 && P_COUNT==99 && G_COUNT==27 && NSTEP==64) ? 1 : -1];

typedef struct {
    char name[12];
    uint8_t project[8], track[8];
} d8p1_bank_state;
typedef struct {
    char name[12];
    uint8_t bank, apply, mute, level[8];
    int8_t pan[8], transpose[8];
} d8p1_scene_state;
typedef struct {
    uint8_t banks, scenes, rows;
    d8p1_bank_state bank[4];
    d8p1_scene_state scene[16];
    struct { uint8_t scene, repeat; } row[16];
} d8p1_arrangement;
typedef struct {
    project_t project;             /* chain is empty; arrangement owns new rows */
    d8p1_arrangement arrangement;
} d8p1_project_state;

static unsigned d8ps_u16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static void d8ps_w16(uint8_t *p,unsigned x) { p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8); }
static void d8ps_w32(uint8_t *p,uint32_t x) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i)); }
static int d8ps_signed(const uint8_t *p) { int n=(int)d8ps_u16(p);return n>=32768?n-65536:n; }
static int d8ps_overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return na&&nb&&(x<=y?y-x<na:x-y<nb);
}
static int d8ps_name(const char *p) {
    unsigned zero=0;for(unsigned i=0;i<12;i++){
        unsigned c=(uint8_t)p[i];if(!c)zero=1;else if(zero||c<32||c>126)return 0;
    }return 1;
}
/* Structural format validation; engine/global policy and adoption are separate. */
static int d8ps_valid(const d8p1_project_state *s) {
    if(!s)return 0;
    const project_t *q=&s->project;const d8p1_arrangement *a=&s->arrangement;
    if(q->parts!=8||q->phys!=2||q->sel>=8||q->rsv||q->chain.count||q->chain.rsv[0]||q->chain.rsv[1]||q->chain.rsv[2]||
       !d8ps_name(q->name)||a->banks>4||a->scenes>16||a->rows>16||!motion_v1_valid(&q->motion))return 0;
    for(unsigned t=0;t<8;t++){
        const proj_trk_t *tr=&q->t[t];if(tr->engine>=14)return 0;
        for(unsigned i=0;i<99;i++)if(tr->p[i]<-64||tr->p[i]>127)return 0;
        for(unsigned i=0;i<128;i++)if(q->fm6[t][i]>127)return 0;
        for(unsigned i=0;i<64;i++){
            const step_t *v=&tr->step[i];
            if(v->n>4||v->time>ST_REST||(v->flags&~(3u|SF_RATCH))||v->vel>127||
               (v->acc&~v->hit)||v->probability>101)return 0;
            for(unsigned n=0;n<4;n++)if(v->note[n]>127)return 0;
        }
    }
    for(unsigned i=0;i<a->banks;i++){
        const d8p1_bank_state *b=&a->bank[i];if(!d8ps_name(b->name))return 0;
        for(unsigned t=0;t<8;t++)if(b->project[t]>=4||b->track[t]>=8)return 0;
    }
    for(unsigned i=0;i<a->scenes;i++){
        const d8p1_scene_state *v=&a->scene[i];if(!d8ps_name(v->name)||v->bank>=a->banks||(v->apply&~7u))return 0;
        for(unsigned t=0;t<8;t++)if(v->level[t]>127||v->pan[t]<-64||v->pan[t]>63||v->transpose[t]<-24||v->transpose[t]>24)return 0;
    }
    for(unsigned i=0;i<a->rows;i++)if(a->row[i].scene>=a->scenes||!a->row[i].repeat||a->row[i].repeat>16)return 0;
    return 1;
}
static uint8_t *d8ps_chunk(uint8_t *out,size_t *pos,unsigned id,unsigned n) {
    uint8_t *p=out+*pos;d8ps_w16(p,id);d8ps_w16(p+2,n);*pos+=8u+n;return p+8;
}
static void d8ps_finish_chunk(uint8_t *p,unsigned n) { d8ps_w32(p-4,d8p1_crc32(p,n)); }
/* No full payload scratch: validate native state, then emit directly to output.
 * Refusal changes neither output nor written count. Input must remain immutable. */
static int d8p1_project_encode(uint8_t *out,size_t cap,size_t *written,const d8p1_project_state *s) {
    if(!out||!written||!d8ps_valid(s))return 0;
    const project_t *q=&s->project;const d8p1_arrangement *a=&s->arrangement;
    unsigned motion_n=8u+q->motion.count*5u,chain_n=1u+a->rows*2u;
    unsigned bank_n=1u+a->banks*28u,scene_n=1u+a->scenes*39u;
    unsigned optional=(a->banks!=0)+(a->scenes!=0);
    size_t total=32u+6u*8u+54u+5416u+1024u+motion_n+chain_n+16u;
    if(a->banks)total+=8u+bank_n;if(a->scenes)total+=8u+scene_n;
    if(cap<total||d8ps_overlap(out,total,s,sizeof *s)||d8ps_overlap(written,sizeof *written,s,sizeof *s)||d8ps_overlap(out,total,written,sizeof *written))return 0;
    memset(out,0,32);memcpy(out,"D8P1",4);d8ps_w16(out+4,1);d8ps_w16(out+6,32);d8ps_w32(out+8,(uint32_t)total);
    out[16]=8;out[17]=64;out[18]=99;out[19]=27;out[20]=(uint8_t)(6+optional);out[21]=out[22]=1;
    size_t pos=32;uint8_t *p=d8ps_chunk(out,&pos,0x8001,54);
    for(unsigned i=0;i<27;i++)d8ps_w16(p+i*2,(uint16_t)q->g[i]);d8ps_finish_chunk(p,54);
    p=d8ps_chunk(out,&pos,0x8002,5416);
    for(unsigned t=0;t<8;t++){
        uint8_t *b=p+t*677u;const proj_trk_t *tr=&q->t[t];
        for(unsigned i=0;i<99;i++)b[i]=(uint8_t)(tr->p[i]+64);b[99]=tr->engine;b[100]=tr->preset;
        for(unsigned i=0;i<64;i++){
            const step_t *v=&tr->step[i];uint8_t *d=b+101+i*9;unsigned r=step_ratchet(v)-1;
            memcpy(d,v->note,4);d[4]=(uint8_t)(v->n|v->time<<3|(v->flags&3u)<<5);
            d[5]=(uint8_t)(v->vel|(r&1u)<<7);d[6]=v->hit;d[7]=v->acc;d[8]=(uint8_t)(v->probability|(r>>1)<<7);
        }
    }d8ps_finish_chunk(p,5416);
    p=d8ps_chunk(out,&pos,0x8003,1024);
    for(unsigned t=0;t<8;t++)memcpy(p+t*128u,q->fm6[t],128);d8ps_finish_chunk(p,1024);
    p=d8ps_chunk(out,&pos,0x8004,motion_n);
    /* motion_v1_valid already succeeded; exact capacity makes encode infallible. */
    motion_v1_encode(p,motion_n,&q->motion);d8ps_finish_chunk(p,motion_n);
    p=d8ps_chunk(out,&pos,0x8005,chain_n);p[0]=a->rows;
    for(unsigned i=0;i<a->rows;i++){p[1+i*2]=a->row[i].scene;p[2+i*2]=a->row[i].repeat;}d8ps_finish_chunk(p,chain_n);
    p=d8ps_chunk(out,&pos,0x8006,16);memcpy(p,q->name,12);p[12]=q->sel;p[13]=2;p[14]=p[15]=0;d8ps_finish_chunk(p,16);
    if(a->banks){
        p=d8ps_chunk(out,&pos,7,bank_n);p[0]=a->banks;
        for(unsigned i=0;i<a->banks;i++){
            uint8_t *b=p+1+i*28;const d8p1_bank_state *v=&a->bank[i];memcpy(b,v->name,12);
            for(unsigned t=0;t<8;t++){b[12+t*2]=v->project[t];b[13+t*2]=v->track[t];}
        }d8ps_finish_chunk(p,bank_n);
    }
    if(a->scenes){
        p=d8ps_chunk(out,&pos,8,scene_n);p[0]=a->scenes;
        for(unsigned i=0;i<a->scenes;i++){
            uint8_t *b=p+1+i*39;const d8p1_scene_state *v=&a->scene[i];memcpy(b,v->name,12);b[12]=v->bank;b[13]=v->apply;b[14]=v->mute;
            for(unsigned t=0;t<8;t++){b[15+t*3]=v->level[t];b[16+t*3]=(uint8_t)(v->pan[t]+64);b[17+t*3]=(uint8_t)(v->transpose[t]+24);}
        }d8ps_finish_chunk(p,scene_n);
    }
    d8ps_w32(out+12,d8p1_crc32(out,total));*written=total;return 1;
}
/* CRC/fields/availability/aliases validate before touching staged state. */
static int d8p1_project_decode(d8p1_project_state *out,const void *data,size_t n,unsigned available_projects) {
    d8p1_view v;if(!out||d8ps_overlap(out,sizeof *out,data,n)||d8p1_read(&v,data,n)!=1||!d8p1_refs_available(&v,available_projects))return 0;
    motion_store_t motion;int found=0;
    for(unsigned i=0;i<v.count;i++)if((v.chunk[i].type&32767u)==4){
        if(!motion_v1_decode(&motion,v.chunk[i].data,v.chunk[i].length))return 0;found=1;
    }
    if(!found)return 0;
    memset(out,0,sizeof *out);project_t *q=&out->project;d8p1_arrangement *a=&out->arrangement;
    q->magic=PROJ_MAGIC;q->size=sizeof *q;q->parts=8;q->phys=2;chain_defaults(&q->chain);
    for(unsigned c=0;c<v.count;c++){
        const uint8_t *p=v.chunk[c].data;
        switch(v.chunk[c].type&32767u){
        case 1:for(unsigned i=0;i<27;i++)q->g[i]=(int16_t)d8ps_signed(p+i*2);break;
        case 2:
            for(unsigned t=0;t<8;t++){
                const uint8_t *b=p+t*677u;proj_trk_t *tr=&q->t[t];
                for(unsigned i=0;i<99;i++)tr->p[i]=(int16_t)b[i]-64;tr->engine=b[99];tr->preset=b[100];
                for(unsigned i=0;i<64;i++){
                    const uint8_t *d=b+101+i*9;step_t *s=&tr->step[i];memcpy(s->note,d,4);
                    s->n=d[4]&7;s->time=(d[4]>>3)&3;s->flags=(d[4]>>5)&3;s->vel=d[5]&127;s->hit=d[6];s->acc=d[7];s->probability=d[8]&127;
                    step_set_ratchet(s,((d[5]>>7)|((d[8]>>7)<<1))+1);
                }
            }break;
        case 3:for(unsigned t=0;t<8;t++)memcpy(q->fm6[t],p+t*128u,128);break;
        case 4:q->motion=motion;break;
        case 5:a->rows=p[0];for(unsigned i=0;i<a->rows;i++){a->row[i].scene=p[1+i*2];a->row[i].repeat=p[2+i*2];}break;
        case 6:memcpy(q->name,p,12);q->sel=p[12];break;
        case 7:
            a->banks=p[0];for(unsigned i=0;i<a->banks;i++){
                const uint8_t *b=p+1+i*28;d8p1_bank_state *d=&a->bank[i];memcpy(d->name,b,12);
                for(unsigned t=0;t<8;t++){d->project[t]=b[12+t*2];d->track[t]=b[13+t*2];}
            }break;
        case 8:
            a->scenes=p[0];for(unsigned i=0;i<a->scenes;i++){
                const uint8_t *b=p+1+i*39;d8p1_scene_state *d=&a->scene[i];memcpy(d->name,b,12);d->bank=b[12];d->apply=b[13];d->mute=b[14];
                for(unsigned t=0;t<8;t++){d->level[t]=b[15+t*3];d->pan[t]=(int8_t)((int)b[16+t*3]-64);d->transpose[t]=(int8_t)((int)b[17+t*3]-24);}
            }break;
        }
    }
    q->sum=proj_sum(q);return 1;
}
/* Conversion report is metadata; caller still preserves the complete original. */
typedef struct {
    uint8_t format, engine_before[4], engine_after[4];
    uint8_t referenced_projects, scene_for_project[4], chain_converted;
    uint16_t removed_motion_records;
} d8p1_legacy_report;
static uint8_t d8ps_source_engine(const uint8_t *b,unsigned version,unsigned t) {
    size_t base=66,stride=0,off=0;
    switch(version){
    case 1:base=offsetof(project_v1_t,t);stride=sizeof(proj_trk_v2_t);off=offsetof(proj_trk_v2_t,engine);break;
    case 2:stride=sizeof(proj_trk_v2_t);off=offsetof(proj_trk_v2_t,engine);break;
    case 3:stride=sizeof(proj_trk_v3_t);off=offsetof(proj_trk_v3_t,engine);break;
    case 4:stride=sizeof(proj_trk_v4_t);off=offsetof(proj_trk_v4_t,engine);break;
    case 5:case 6:stride=sizeof(proj_trk_v5_t);off=offsetof(proj_trk_v5_t,engine);break;
    default:base=68;stride=b[66]+2u+NSTEP*9u;off=b[66];break;
    }return b[base+t*stride+off];
}
/* Real FUN1..FUN9 importer, with caller-owned staging workspace. Legacy C
 * layouts require aligned input; refuse unaligned bytes rather than casting.
 * Exact standalone records only. No retained-slot tails or old magic upgrade.
 * out/report are published only after conversion and structural validation.
 * workspace may change on refusal; never use it as active state. */
static int d8p1_legacy_convert(d8p1_project_state *out,d8p1_project_state *workspace,
        d8p1_legacy_report *report,const void *data,size_t n,unsigned available_projects) {
    if(!out||!workspace||!report||!data||n<8||n>PROJ_STORE_SIZE||available_projects>15||((uintptr_t)data&3u)||
       d8ps_overlap(out,sizeof *out,workspace,sizeof *workspace)||d8ps_overlap(out,sizeof *out,data,n)||
       d8ps_overlap(workspace,sizeof *workspace,data,n)||d8ps_overlap(report,sizeof *report,out,sizeof *out)||
       d8ps_overlap(report,sizeof *report,workspace,sizeof *workspace)||d8ps_overlap(report,sizeof *report,data,n))return 0;
    const uint8_t *b=data;uint32_t magic=(uint32_t)d8ps_u16(b)|((uint32_t)d8ps_u16(b+2)<<16);
    uint32_t length=(uint32_t)d8ps_u16(b+4)|((uint32_t)d8ps_u16(b+6)<<16);
    if(length!=n||magic<PROJ_MAGIC_V1||magic>PROJ_MAGIC)return 0;
    /* Canonicalize the whole object, including trailing padding, even when
     * caller workspace previously held unrelated data. */
    memset(workspace,0,sizeof *workspace);
    project_t *q=&workspace->project;d8p1_arrangement *a=&workspace->arrangement;
    if(!proj_import_any(q,data,(int)n)||!proj_engines_ok(q))return 0;
    d8p1_legacy_report result;memset(&result,0,sizeof result);memset(result.scene_for_project,255,4);
    result.format=(uint8_t)(magic-PROJ_MAGIC_V1+1);unsigned original_tracks=result.format==1?1:4;
    for(unsigned t=0;t<original_tracks;t++)result.engine_before[t]=d8ps_source_engine(b,result.format,t);
    unsigned count=q->motion.count;proj_fm4(q);proj_perc(q);result.removed_motion_records=(uint16_t)(count-q->motion.count);
    for(unsigned t=0;t<4;t++)result.engine_after[t]=q->t[t].engine;
    for(unsigned t=4;t<8;t++){
        proj_trk_t *tr=&q->t[t];memset(tr,0,sizeof *tr);tr->engine=(uint8_t)trk_def_engine(t);tr->preset=PROJ_DEF_SOUND;
        for(unsigned i=0;i<99;i++)tr->p[i]=param_desc_of(tr->engine,i)->def;
        for(unsigned i=0;i<64;i++)tr->step[i].time=ST_REST;memcpy(q->fm6[t],FM6_INIT,128);
    }
    memset(a,0,sizeof *a);
    for(unsigned i=0;i<q->chain.count;i++)result.referenced_projects|=(uint8_t)(1u<<q->chain.row[i].slot);
    if(result.referenced_projects&~available_projects)return 0;
    for(unsigned slot=0;slot<4;slot++)if(result.referenced_projects&(1u<<slot)){
        unsigned i=a->banks++;a->scenes++;result.scene_for_project[slot]=(uint8_t)i;
        d8p1_bank_state *bank=&a->bank[i];d8p1_scene_state *scene=&a->scene[i];
        memcpy(bank->name,"LEGACY ",7);bank->name[7]=(char)('1'+slot);memcpy(scene->name,bank->name,12);scene->bank=(uint8_t)i;
        /* Legacy chains change patterns/timing, not current sounds or mix:
         * apply=0 deliberately preserves that meaning. Playback is separate. */
        for(unsigned t=0;t<8;t++){bank->project[t]=(uint8_t)slot;bank->track[t]=(uint8_t)t;}
    }
    a->rows=q->chain.count;result.chain_converted=(uint8_t)(a->rows!=0);
    for(unsigned i=0;i<a->rows;i++){a->row[i].scene=result.scene_for_project[q->chain.row[i].slot];a->row[i].repeat=q->chain.row[i].repeat;}
    chain_defaults(&q->chain);q->parts=8;q->phys=2;q->rsv=0;q->magic=PROJ_MAGIC;q->size=sizeof *q;q->sum=proj_sum(q);
    if(!d8ps_valid(workspace))return 0;
    memcpy(out,workspace,sizeof *out);*report=result;return 1;
}

#endif
