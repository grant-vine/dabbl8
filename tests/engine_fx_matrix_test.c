/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

enum { DRY, CHORUS, DELAY, ROOM, SPRING, COMBINED, EFFECT_CASES };
static unsigned peak_voices, peak_pool, nonzero, sent[3], wet_nonzero;
static unsigned live_pool(void)
{
    unsigned count=0;
    for(unsigned i=0;i<NVOICE;i++)if(heavy_owner[i]){
        unsigned key=heavy_owner[i]-1u;
        unsigned engine=7u+key/(NPART*NVOICE),part=(key/NVOICE)%NPART,voice=key%NVOICE;
        assert(engine<NENGINES&&part<NPART&&voice<NVOICE);
        if(heavy_live(engine,part,voice)){
            for(unsigned j=0;j<i;j++)assert(heavy_owner[j]!=heavy_owner[i]);
            count++;
        }
    }
    return count;
}
static void render(unsigned samples)
{
    int32_t out[CTL*2];
    while(samples){
        unsigned n=samples>CTL?CTL:samples;
        mix_block(out,n);
        unsigned voices=voices_busy(),pool=live_pool();
        assert(voices<=NVOICE&&pool<=NVOICE);
        if(voices>peak_voices)peak_voices=voices;
        if(pool>peak_pool)peak_pool=pool;
        for(unsigned i=0;i<n;i++){
            nonzero+=out[2*i]!=0||out[2*i+1]!=0;
            sent[0]+=send_c[i]!=0;sent[1]+=send_d[i]!=0;sent[2]+=send_r[i]!=0;
            wet_nonzero+=wet[i]!=0;
        }
        samples-=n;
    }
}
static void scenario(unsigned engine,unsigned effect)
{
    /* Given: eight tracks in a fresh process, real presets and no storage adapter. */
    assert(NPART==8&&NVOICE==8&&D8ENGINE_EFFECTIVE_MASK==0x3ffd);
    host_tracks_init();
    song.g[G_RTYPE]=effect==SPRING;
    song.g[G_DFDBK]=GP[G_DFDBK].max;
    song.g[G_RSIZE]=GP[G_RSIZE].max;
    song.g[G_CDEPTH]=GP[G_CDEPTH].max;
    unsigned notes[NPART];
    for(unsigned p=0;p<NPART;p++){
        unsigned e=engine==NENGINES?(unsigned[]){7,8,9,10,12,13,12,9}[p]:engine;
        host_preset(&trk[p],e,0);
        trk[p].p[P_LEVEL]=64;trk[p].p[P_VOICE]=V_POLY;
        trk[p].p[P_REL]=0;trk[p].p[P_AMODE]=0;
        trk[p].p[P_DIST]=effect==COMBINED?127:0;
        trk[p].p[P_CHOR]=effect==CHORUS||effect==COMBINED?127:0;
        trk[p].p[P_DLY]=effect==DELAY||effect==COMBINED?127:0;
        trk[p].p[P_REV]=effect==ROOM||effect==SPRING||effect==COMBINED?127:0;
        if(e==ENGI_SLICE){trk[p].p[P_E0]=0;trk[p].p[P_E4]=SLC_LOOP;trk[p].p[P_E5]=1;}
        notes[p]=e==ENGI_DRUM?36:60+p;
    }
    /* When: notes overlap on all eight tracks, retrigger, then release. */
    for(unsigned round=0;round<4;round++){
        for(unsigned p=0;p<NPART;p++)trk_note_on(&trk[p],notes[p],64);
        assert(voices_busy()==8);peak_voices=8;
        render(FS/4);
        for(unsigned p=0;p<NPART;p++)trk_note_off(&trk[p],notes[p]);
        render(FS/16);
    }
    for(unsigned p=0;p<NPART;p++)trk_all_off(&trk[p]);
    render(FS*2);
    /* Then: sound and requested sends occurred, ownership stayed bounded,
     * and no held notes or sounding voices remain. FX tails may remain. */
    assert(nonzero&&peak_voices==8&&!voices_busy());
    for(unsigned p=0;p<NPART;p++){
        assert(!trk[p].nmono&&!trk[p].xp_n);
        for(unsigned v=0;v<NVOICE;v++)assert(!trk[p].v[v].gate);
    }
    if(engine==7||engine==8||engine==9||engine==10||engine==12||engine==13||engine==NENGINES)
        assert(peak_pool==8);
    else assert(!peak_pool);
    assert((sent[0]!=0)==(effect==CHORUS||effect==COMBINED));
    assert((sent[1]!=0)==(effect==DELAY||effect==COMBINED));
    assert((sent[2]!=0)==(effect==ROOM||effect==SPRING||effect==COMBINED));
    assert((wet_nonzero!=0)==(effect!=DRY));
    printf("PASS engine=%u effect=%u voices=%u pool=%u audio_frames=%u wet_frames=%u cleanup=1\n",
           engine,effect,peak_voices,peak_pool,nonzero,wet_nonzero);
}
int main(void)
{
    unsigned cases=0;
    for(unsigned engine=0;engine<=NENGINES;engine++){
        if(engine<NENGINES&&!(D8ENGINE_EFFECTIVE_MASK&(1u<<engine)))continue;
        for(unsigned effect=0;effect<EFFECT_CASES;effect++){
            fflush(NULL);
            pid_t child=fork();assert(child>=0);
            if(!child){scenario(engine,effect);exit(0);}
            int status=0;assert(waitpid(child,&status,0)==child);
            if(!WIFEXITED(status)||WEXITSTATUS(status)){
                fprintf(stderr,"STOP engine=%u effect=%u child_status=%d completed=%u\n",engine,effect,status,cases);
                return 1;
            }
            cases++;
        }
    }
    printf("PASS matrix cases=%u enabled_engines=13 mixed_layouts=1 effects=6; host behavior only, no hardware timing proof\n",cases);
    return 0;
}
