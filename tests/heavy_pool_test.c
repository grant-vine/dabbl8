/* SPDX-License-Identifier: GPL-3.0-only
 * Mixed real eight-part GRAIN/PHYS/DRUM state admission and lifetimes. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static uint32_t audible_mask;
static void check(const char *s, int ok) { printf("heavy pool: %s %s\n",s,ok?"PASS":"FAIL");bad+=!ok; }
static void fresh(void)
{
    memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
    memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);
    host_tracks_init();audible_mask=0;
}
static uint32_t occupied(void)
{
    uint32_t n=0;
    for(uint32_t i=0;i<NVOICE;i++)if(heavy_owner[i]){
        uint32_t k=heavy_owner[i]-1u;
        n+=heavy_live(7u+k/(NPART*NVOICE),(k/NVOICE)%NPART,k%NVOICE);
    }
    return n;
}
static int ownership(void)
{
    heavy_state_t *seen[NVOICE];uint32_t n=0;
    for(uint32_t p=0;p<NPART;p++){
        uint32_t engine=trk[p].engine;
        if(engine<8||engine>10)continue;
        for(uint32_t v=0;v<NVOICE;v++)if(trk[p].v[v].active){
            heavy_state_t *s=heavy_get(engine,p,engine==9?v:0);
            if(!s||n==NVOICE)return 0;
            for(uint32_t j=0;j<n;j++)if(seen[j]==s)return 0;
            seen[n++]=s;
            if(engine!=9)break;
        }
    }
    return occupied()==n;
}
static uint32_t render(void)
{
    int32_t out[CTL];uint32_t n=0;
    for(uint32_t p=0;p<NPART;p++)engine_block(&trk[p]);
    for(uint32_t p=0;p<NPART;p++){
        n+=track_render(&trk[p],out,CTL);
        for(uint32_t i=0;i<CTL;i++)if(out[i])audible_mask|=1u<<p;
    }
    return n;
}
static void fill(uint32_t engine)
{
    for(uint32_t p=0;p<NPART;p++){
        uint32_t e=engine?engine:8+p%3;
        host_preset(&trk[p],e,0);trk[p].p[P_VOICE]=V_POLY;
        trk_note_on(&trk[p],e==10?36:60+p,100);
    }
}
int main(void)
{
    fresh();for(uint32_t p=0;p<NPART;p++)host_preset(&trk[p],8,0);
    for(uint32_t b=0;b<8;b++)for(uint32_t p=0;p<NPART;p++)engine_block(&trk[p]);
    check("inactive GRAIN blocks reserve no shared bodies",occupied()==0);
    check("all inactive destination requests refuse allocation",heavy_get(8,0,0)==0&&heavy_get(9,0,0)==0&&heavy_get(10,0,0)==0);
    fresh();fill(0);
    check("track-wide engines reject noncanonical duplicate voice owners",!heavy_get(8,0,1)&&!heavy_get(10,2,1));
    check("mixed engines own eight distinct bodies",voices_busy()==8&&occupied()==8&&ownership());
    check("mixed engines render within eight sounding voices",render()<=8&&ownership());
    heavy_state_t *first=heavy_get(8,3,0);heavy_state_t snapshot=*first;
    trk_note_on(&trk[4],75,100);
    check("a PHYS steal preserves another track's GRAIN state",!memcmp(first,&snapshot,sizeof snapshot)&&ownership());
    for(uint32_t e=8;e<=10;e++){
        fresh();fill(e);int ok=ownership()&&voices_busy()==8;
        for(uint32_t b=0;b<1000;b++)ok&=render()<=8&&ownership();
        ok&=audible_mask==((1u<<NPART)-1u);
        check(e==8?"eight GRAIN tracks retain their complete state":e==9?"eight PHYS tracks retain distinct bodies":"eight DRUM tracks retain complete kits",ok);
    }
    fresh();host_preset(&trk[0],8,0);trk_note_on(&trk[0],60,100);
    first=heavy_get(8,0,0);memset(&first->grain,0x5a,sizeof first->grain);
    trk[0].v[0].active=0;host_preset(&trk[1],9,0);trk[1].v[0].active=1;
    heavy_state_t *claimed=heavy_get(9,1,0);phys_slot_t blank={0};
    check("new engine reuses only retired state and clears its complete member",claimed==first&&!memcmp(&claimed->phys,&blank,sizeof blank));
    trk[1].v[0].active=0;host_preset(&trk[2],10,0);trk[2].v[0].active=1;
    claimed=heavy_get(10,2,0);drum_lane_t kit[DV_NLANE]={0};
    check("DRUM cannot inherit prior physical or grain body fields",claimed==first&&!memcmp(claimed->drum,kit,sizeof kit));
    fresh();fill(9);trk[0].v[1].active=1;
    check("an unadmitted ninth body fails closed",heavy_get(9,0,1)==0);
    trk[0].v[1].active=0;
    fresh();host_preset(&trk[0],8,0);trk[0].p[P_VOICE]=V_POLY;
    for(uint32_t i=0;i<10;i++)trk_note_on(&trk[0],60+i,100);
    check("three GRAIN voices share one complete twelve-grain state",voices_busy()==GR_POLY&&occupied()==1&&GR_NG==12&&ownership());
    fresh();host_preset(&trk[0],9,0);trk[0].p[P_VOICE]=V_POLY;
    for(uint32_t i=0;i<10;i++)trk_note_on(&trk[0],60+i,100);
    check("PHYS keeps its three-body per-track cap",voices_busy()==PHYS_POLY&&occupied()==PHYS_POLY&&ownership());
    fresh();host_preset(&trk[0],10,0);
    uint32_t notes[]={36,38,39,42,46,45,37,51};
    for(uint32_t i=0;i<8;i++)trk_note_on(&trk[0],notes[i],100);
    check("eight drum lanes share one complete kit",voices_busy()==8&&occupied()==1&&ownership()&&render()<=8);
    fresh();fill(0);uint32_t seed=17,errors=0;
    for(uint32_t i=0;i<10000;i++){
        seed=seed*1664525u+1013904223u;uint32_t p=seed>>29,e=8+(seed>>16)%3;
        if(trk[p].engine!=e){
            trk[p].eng_req=e;
            for(uint32_t b=0;b<300&&trk[p].engine!=e;b++)render();
            if(trk[p].engine!=e){printf("fade timeout i=%u p=%u old=%u wanted=%u xf=%u voices=%u\n",i,p,trk[p].engine,e,trk[p].xf,voices_busy());errors++;break;}
            host_preset(&trk[p],e,0);trk[p].p[P_VOICE]=V_POLY;
        }
        trk_note_on(&trk[p],e==10?notes[seed%8]:48+seed%40,100);
        uint32_t busy=voices_busy(), rendered=render(); int owned=ownership();
        if(busy>8||rendered>8||!owned){printf("stress failure i=%u p=%u e=%u busy=%u rendered=%u owned=%d slots=%u\n",i,p,e,busy,rendered,owned,occupied());errors++;break;}
    }
    check("10000 mixed notes and actual engine fades preserve bounded ownership",!errors);
    printf("heavy pool result: %d failures; %zu host bytes, %u shared bodies\n",bad,sizeof heavy_pool,NVOICE);
    return !!bad;
}
