/* SPDX-License-Identifier: GPL-3.0-only
 * Real eight-part shared organ voice state and retained track rotors. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define main hostsim_main
#include "hostsim.c"
#undef main
static int bad;
static uint32_t audible;
static void check(const char *s,int ok){printf("WHEEL pool: %s %s\n",s,ok?"PASS":"FAIL");bad+=!ok;}
static void fresh(void)
{
    memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
    memset(heavy_owner,0,sizeof heavy_owner);memset(heavy_pool,0,sizeof heavy_pool);memset(drw_t,0,sizeof drw_t);
    host_tracks_init();audible=0;
}
static void fill(void)
{
    for(uint32_t p=0;p<NPART;p++){
        host_preset(&trk[p],7,0);trk[p].p[P_VOICE]=V_POLY;trk_note_on(&trk[p],60+p,100);
    }
}
static uint32_t render(void)
{
    int32_t out[CTL];uint32_t n=0;
    for(uint32_t p=0;p<NPART;p++)engine_block(&trk[p]);
    for(uint32_t p=0;p<NPART;p++){
        n+=track_render(&trk[p],out,CTL);
        for(uint32_t i=0;i<CTL;i++)if(out[i])audible|=1u<<p;
    }
    return n;
}
static int ownership(void)
{
    heavy_state_t *seen[NVOICE];uint32_t n=0;
    for(uint32_t p=0;p<NPART;p++){
        uint32_t e=trk[p].engine;if(e<7||e>10)continue;
        for(uint32_t i=0;i<NVOICE;i++)if(trk[p].v[i].active){
            heavy_state_t *s=heavy_get(e,p,(e==7||e==9)?i:0);
            if(!s||n==NVOICE)return 0;
            for(uint32_t j=0;j<n;j++)if(seen[j]==s)return 0;
            seen[n++]=s;if(e==8||e==10)break;
        }
    }
    return 1;
}
int main(void)
{
    fresh();fill();for(uint32_t i=0;i<100;i++)render();
    check("eight organ tracks have unique sounding state and audible output",ownership()&&voices_busy()==8&&audible==255);
    check("per-track rotor/bar caches remain independent",sizeof drw_t==NPART*sizeof(drw_trk_t));
    drw_vc_t *v=wheel_state_of(&trk[7],&trk[7].v[0]);uint32_t phase[DRW_NP];int32_t gains[DRW_NP];
    memcpy(phase,v->ph,sizeof phase);memcpy(gains,v->gp,sizeof gains);
    trk_note_on(&trk[7],67,100);
    check("same-owner retrigger retains running phases and gain ramps",v==wheel_state_of(&trk[7],&trk[7].v[0])&&!memcmp(phase,v->ph,sizeof phase)&&!memcmp(gains,v->gp,sizeof gains)&&phase[0]);
    drw_vc_t snapshot=*wheel_state_of(&trk[3],&trk[3].v[0]);drw_trk_t rotor=drw_t[3];
    trk_note_on(&trk[7],80,100);
    check("a ninth note preserves other live organ state and rotor",ownership()&&voices_busy()==8&&!memcmp(&snapshot,wheel_state_of(&trk[3],&trk[3].v[0]),sizeof snapshot)&&!memcmp(&rotor,&drw_t[3],sizeof rotor));
    fresh();fill();trk[0].v[1].active=1;
    check("an unadmitted ninth body refuses without aliasing",wheel_state_of(&trk[0],&trk[0].v[1])==0);trk[0].v[1].active=0;
    fresh();fill();trk[0].engine=trk[0].eng_req=0;trk[7].v[1].active=1;
    v=wheel_state_of(&trk[7],&trk[7].v[1]);drw_vc_t blank={0};
    check("new owner starts with cleared organ state",v&&!memcmp(v,&blank,sizeof blank));
    fresh();host_preset(&trk[0],7,0);trk[0].p[P_VOICE]=V_UNISON;trk_note_on(&trk[0],60,100);
    check("unison retains eight sounding organ bodies",voices_busy()==8&&ownership()&&render()==8);
    uint32_t percussion=0;for(uint32_t i=0;i<NVOICE;i++)percussion+=wheel_state_of(&trk[0],&trk[0].v[i])->perc!=0;
    check("single-trigger percussion stays on the first unison body",percussion==1);
    fresh();host_preset(&trk[0],7,0);trk[0].p[P_VOICE]=V_POLY;
    for(uint32_t i=0;i<8;i++)trk_note_on(&trk[0],48+i,100);
    check("one organ track keeps its original eight-voice cap",voices_busy()==8&&ownership()&&render()==8);
    fresh();fill();uint32_t seed=23,errors=0;
    for(uint32_t i=0;i<5000;i++){
        seed=seed*1664525u+1013904223u;uint32_t p=seed>>29,e=7+(seed>>16)%4;
        if(trk[p].engine!=e){
            trk[p].eng_req=e;for(uint32_t b=0;b<300&&trk[p].engine!=e;b++)render();
            if(trk[p].engine!=e){errors++;break;}
            host_preset(&trk[p],e,0);trk[p].p[P_VOICE]=V_POLY;
        }
        trk_note_on(&trk[p],e==10?36:48+seed%40,100);
        if(voices_busy()>8||render()>8||!ownership()){errors++;break;}
    }
    check("5000 mixed notes and engine fades preserve unique state",!errors);
    fresh();fill();for(uint32_t p=0;p<NPART;p++)trk_all_off(&trk[p]);
    for(uint32_t i=0;i<30000&&voices_busy();i++)render();
    check("all organ release tails finish",!voices_busy());
    printf("WHEEL pool result: %d failures, %zu host bytes in existing shared pool\n",bad,sizeof heavy_pool);return !!bad;
}
