/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual five consumers over independently allocated, source-local slot data. */
#include <stdint.h>
#include <stdlib.h>
static uint8_t *range_slots[3];
static uint8_t range_read(const uint8_t *data, uint32_t off);
#define SMP_USER_XIP(k) range_slots[k]
#define SMP_READ_BYTE(data, off) range_read(data, off)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
static unsigned range_checks, range_bad, range_reads, range_invalid;
static gr_part_t range_grain;
static const uint8_t *range_lo;
static uint32_t range_len;
static void range_expect(int yes) { range_checks++;range_bad+=!yes; }
static uint8_t range_read(const uint8_t *data, uint32_t off) {
    uintptr_t p=(uintptr_t)data;
    range_reads++;
    if(off>UINTPTR_MAX-p || p+off<(uintptr_t)range_lo || p+off-(uintptr_t)range_lo>=range_len) {
        range_invalid++;return 0;
    }
    return data[off];
}
static void range_allow(const uint8_t *p,uint32_t n) { range_lo=p;range_len=n;range_reads=range_invalid=0; }
static int32_t range_reference(const uint8_t *p,uint32_t pos,int32_t *pred,int32_t *idx) {
    uint32_t c=(p[pos/2]>>(4*(pos%2)))&15;int32_t step=IMA_STEP[*idx];
    int32_t delta=step/8 + ((c&4)?step:0) + ((c&2)?step/2:0) + ((c&1)?step/4:0);
    *pred+=c&8?-delta:delta;*pred=clamp(*pred,-32768,32767);
    *idx=clamp(*idx+IMA_IDX[c&7],0,88);return *pred;
}
static void range_case(unsigned slot,unsigned maximum) {
    smp_user_hdr_t *h=(smp_user_hdr_t *)range_slots[slot];
    memset(h,0,SMP_USER_SIZE);h->magic=SMP_USER_MAGIC;h->version=1;h->nz=maximum?1:2;
    h->data_len=maximum?SMP_USER_SIZE-SMP_USER_DATA:550;
    for(unsigned j=0;j<h->nz;j++) {
        smp_zone_t *z=&h->zone[j];z->off=maximum?0:j?293:17;
        z->n=maximum?2*h->data_len-1:j?513:501;z->le=z->n-1;z->rate=65536;
        z->root16=60*16;z->lo=j?64:0;z->hi=j?127:63;
    }
    uint8_t *payload=range_slots[slot]+SMP_USER_DATA;
    for(unsigned i=0;i<h->data_len;i++)payload[i]=(uint8_t)(17*i+73*slot+29*maximum);
    uint32_t crc=UINT32_MAX;
    for(unsigned i=0;i<h->data_len;i++){crc^=payload[i];for(unsigned b=0;b<8;b++)crc=(crc>>1)^((crc&1)?0xedb88320u:0);}
    h->crc=~crc;
    range_allow(payload,h->data_len);smp_user_scan(slot);
    range_expect(usr_nz[slot]==h->nz && !range_invalid && range_reads>0);
    range_expect(slc_data(&slc_usr[slot])==payload && slc_usr[slot].len>0);
    range_allow(payload,h->data_len);gr_reset(&range_grain,SMP_NSETS+slot,smp_user_gen);
    for(unsigned tries=0;tries<GR_NIDX;tries++) {
        unsigned pending=0;for(unsigned j=0;j<range_grain.nz;j++)pending+=range_grain.zdone[j]<range_grain.zcnt[j];
        if(!pending)break;gr_build(&range_grain,&trk[0]);
    }
    range_expect(!range_invalid);
    for(unsigned j=0;j<h->nz;j++) {
        const smp_zone_t *z=&usr_zone[slot][j];uint32_t zi=0x8000u|slot<<5|j;
        const uint8_t *data=smp_zone_data(zi,z);voice_t probe={0};int32_t pred=0,idx=0;
        range_expect(z->off==h->zone[j].off && data==payload+z->off);
        range_allow(payload+h->zone[j].off,(h->zone[j].n+1)/2);
        for(unsigned i=0;i<z->n;i++)range_expect(sample_next(data,z,&probe,0)==range_reference(payload+h->zone[j].off,i,&pred,&idx));
        range_expect(!range_invalid && range_reads==z->n);
        pred=idx=0;int32_t reference_pred=0,reference_idx=0;
        const uint8_t *grain_data=smp_set_data(SMP_NSETS+slot)+z->off;
        range_allow(payload+h->zone[j].off,(h->zone[j].n+1)/2);
        for(unsigned i=0;i<z->n;i++)range_expect(gr_dec(grain_data,i,&pred,&idx)==range_reference(payload+h->zone[j].off,i,&reference_pred,&reference_idx));
        range_expect(!range_invalid && range_reads==z->n);
        int32_t seek_pred,seek_idx;reference_pred=reference_idx=0;
        for(unsigned i=0;i+1<z->n;i++)range_reference(payload+h->zone[j].off,i,&reference_pred,&reference_idx);
        range_allow(payload+h->zone[j].off,(h->zone[j].n+1)/2);gr_seek(&range_grain,z,j,z->n-1,&seek_pred,&seek_idx);
        range_expect(!range_invalid && seek_pred==reference_pred && seek_idx==reference_idx);
        gr_grain_t grain={0};grain.z=z;int16_t reverse[GR_RB];
        range_allow(payload+h->zone[j].off,(h->zone[j].n+1)/2);gr_fill(&range_grain,&grain,reverse,j,z->n-1);
        range_expect(!range_invalid);
        reference_pred=reference_idx=0;
        for(unsigned i=0;i<z->n;i++) {
            int32_t value=range_reference(payload+h->zone[j].off,i,&reference_pred,&reference_idx);
            if(i>=grain.rlo)range_expect(reverse[i-grain.rlo]==value);
        }
    }
    range_allow(payload,h->data_len);slc_dec_t d;slc_dec_at(&d,0,0);
    unsigned segment=0,relative=0;int32_t pred=0,idx=0;
    for(unsigned i=0;i<slc_usr[slot].len;i++) {
        if(relative==h->zone[segment].n){segment++;relative=0;pred=idx=0;}
        int32_t expected=range_reference(payload+h->zone[segment].off,relative++,&pred,&idx);
        range_expect(slc_dec_next(&slc_usr[slot],slc_data(&slc_usr[slot]),&d)==expected);
    }
    range_expect(!range_invalid && range_reads==slc_usr[slot].len);
    for(unsigned j=0;j<h->nz;j++) {
        range_allow(payload+h->zone[j].off,(h->zone[j].n+1)/2);
        memset(&sample_wave,0,sizeof sample_wave);memset(&trk[0],0,sizeof trk[0]);
        trk[0].p[P_E0]=SMP_NSETS+slot;last_note=j?72:60;
        for(unsigned i=0;i<400 && !sample_wave.ready;i++)sample_wave_tick(&trk[0]);
        range_expect(sample_wave.ready && sample_wave.pos==h->zone[j].n && !range_invalid && range_reads==h->zone[j].n);
    }
    range_allow(payload,h->data_len);memset(&sp,0,sizeof sp);song.sel=0;
    trk[0].p[P_E0]=slot+1;trk[0].p[P_E1]=0;
    slice_env(0,slc_usr[slot].len);
    range_expect(!range_invalid && range_reads==slc_usr[slot].len && sp.env_len==slc_usr[slot].len);
    /* A header past the admitted data must refuse before any decoder read. */
    h->zone[0].off=h->data_len+1;range_allow(payload,h->data_len);smp_user_scan(slot);
    range_expect(!usr_nz[slot] && !slc_usr[slot].len && !range_reads);
}
#ifndef SAMPLE_SOURCE_RANGE_NO_MAIN
int main(void) {
    unsigned distant=0;
    for(unsigned k=0;k<3;k++) {
        range_slots[k]=malloc(SMP_USER_SIZE);if(!range_slots[k])return 2;
        uintptr_t a=(uintptr_t)range_slots[k],b=(uintptr_t)SMP_DATA;
        distant+=(a>b?a-b:b-a)>UINT32_MAX;
    }
    for(unsigned k=0;k<3;k++)for(unsigned maximum=0;maximum<2;maximum++)range_case(k,maximum);
    for(unsigned k=0;k<3;k++)free(range_slots[k]);
    printf("Source-local sample ranges: %u checks, %u failures; disjoint three slots, distant=%u, tracks=%u; five consumer paths; decoder/helper/waveform scope\n",range_checks,range_bad,distant,NTRK);
    return range_bad!=0;
}
#endif
