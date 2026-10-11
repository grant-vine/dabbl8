/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual project/native adoption with live notes; host-only mask arguments.
 * Production wrappers remain immutable full compiled-engine builds. */
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
static unsigned lcd_fences;
static void fence(void){lcd_fences++;}
#define UI_LCD_SYNC_HOOK fence
#include "ui_test.c"
#if NPART >= NVOICE
#include "../firmware/src/d8p1_runtime.c"
#endif
#include <assert.h>
static unsigned proofs;
#define REQUIRE(x) do { proofs++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static project_t input,saved_input;
static struct {
    __typeof__(trk) tracks;
    __typeof__(song) song_state;
    __typeof__(motion) motion_state;
    __typeof__(chain) chain_state;
    __typeof__(chain_config) config;
    __typeof__(fm6_patch) patches;
    __typeof__(fm6_pgen) generations;
    __typeof__(ui) ui_state;
    __typeof__(undo) undo_state;
    __typeof__(pat_sig) pattern_signatures;
    __typeof__(pat_last) pattern_last;
#if NPART >= NVOICE
    __typeof__(d8p1_runtime_cache) cache;
#endif
    __typeof__(panic_req) panic;
    __typeof__(transport_req) transport;
    __typeof__(cin_left) countin;
    __typeof__(cin_total) countin_total;
    __typeof__(dma) pending_dma;
    unsigned fences;
    uint8_t current_project,sync;
    uint32_t undo_n;
    char name[sizeof proj_name];
} before;
static void snapshot(void)
{
    memcpy(before.tracks,trk,sizeof trk);memcpy(&before.song_state,&song,sizeof song);
    memcpy(&before.motion_state,&motion,sizeof motion);memcpy(&before.chain_state,&chain,sizeof chain);
    memcpy(&before.config,&chain_config,sizeof chain_config);memcpy(before.patches,fm6_patch,sizeof fm6_patch);
    memcpy(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen);memcpy(&before.ui_state,&ui,sizeof ui);
    memcpy(&before.undo_state,&undo,sizeof undo);memcpy(before.pattern_signatures,pat_sig,sizeof pat_sig);
    memcpy(before.pattern_last,pat_last,sizeof pat_last);memcpy(before.name,proj_name,sizeof proj_name);
#if NPART >= NVOICE
    memcpy(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache);
#endif
    memcpy(&before.pending_dma,&dma,sizeof dma);before.fences=lcd_fences;
    before.panic=panic_req;before.transport=transport_req;before.countin=cin_left;before.countin_total=cin_total;
    before.current_project=proj_cur;before.sync=sync_reload;before.undo_n=undo_depth;
}
static void unchanged(void)
{
    REQUIRE(!memcmp(before.tracks,trk,sizeof trk));REQUIRE(!memcmp(&before.song_state,&song,sizeof song));
    REQUIRE(!memcmp(&before.motion_state,&motion,sizeof motion));REQUIRE(!memcmp(&before.chain_state,&chain,sizeof chain));
    REQUIRE(!memcmp(&before.config,&chain_config,sizeof chain_config));REQUIRE(!memcmp(before.patches,fm6_patch,sizeof fm6_patch));
    REQUIRE(!memcmp(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen));REQUIRE(!memcmp(&before.ui_state,&ui,sizeof ui));
    REQUIRE(!memcmp(&before.undo_state,&undo,sizeof undo));REQUIRE(!memcmp(before.pattern_signatures,pat_sig,sizeof pat_sig));
    REQUIRE(!memcmp(before.pattern_last,pat_last,sizeof pat_last));REQUIRE(!memcmp(before.name,proj_name,sizeof proj_name));
#if NPART >= NVOICE
    REQUIRE(!memcmp(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache));
#endif
    REQUIRE(!memcmp(&before.pending_dma,&dma,sizeof dma) && before.fences==lcd_fences);
    REQUIRE(before.panic==panic_req && before.transport==transport_req && before.countin==cin_left && before.countin_total==cin_total);
    REQUIRE(before.current_project==proj_cur && before.sync==sync_reload && before.undo_n==undo_depth);
}
static void fresh(void)
{
    ui_power_on();int32_t audio[2u*CTL];mix_block(audio,CTL);
    for(unsigned k=0;k<NTRK;k++){trk[k].p[P_VOICE]=V_MONO;trk_note_on(&trk[k],60+k,100);}
    REQUIRE(voices_busy()==NTRK);
    project_capture(&input);
    for(unsigned k=0;k<NTRK;k++){
        input.t[k].engine=0;input.t[k].preset=0;
        for(unsigned p=0;p<P_COUNT;p++)input.t[k].p[p]=param_desc_of(0,p)->def;
    }
    input.parts=NPART;input.phys=PROJ_PHYS;input.sum=proj_sum(&input);
#if NPART >= NVOICE
    d8p1_runtime_cache.valid=1;memset(&d8p1_runtime_cache.arrangement,0,sizeof d8p1_runtime_cache.arrangement);
    memcpy(d8p1_runtime_cache.arrangement.bank[0].name,"RETAINED",8);
#endif
}
static void refuse(unsigned mask,int expected)
{
    input.sum=proj_sum(&input);saved_input=input;snapshot();
    REQUIRE(project_restore_runtime_mask(&input,1,mask)==expected);
    unchanged();REQUIRE(!memcmp(&input,&saved_input,sizeof input));
}
static void accept(unsigned mask,unsigned engine)
{
    input.sum=proj_sum(&input);saved_input=input;
    REQUIRE(project_restore_runtime_mask(&input,1,mask)==0);
    REQUIRE(trk[0].eng_req==engine && transport_req==0 && panic_req==((1u<<NTRK)-1u));
    REQUIRE(!memcmp(&input,&saved_input,sizeof input));
}
int main(void)
{
    unsigned mask=D8ENGINE_EFFECTIVE_MASK,out=99;
    fresh();refuse(mask&~1u,PROJ_RUNTIME_MISSING_ENGINE);
    fresh();refuse(0,1);refuse(mask|0x4000u,1);
    fresh();input.t[0].engine=NENGINES;refuse(mask,1);
    fresh();input.t[0].engine=ENGI_DIGITAL;
#if FELUCCA_FM4
    refuse(mask&~2u,PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask&~(1u<<ENGI_FM6),ENGI_DIGITAL);
    REQUIRE(eng_runtime_target(&out,2,ENGI_DIGITAL,0,D8ENGINE_CANONICAL)==0 && out==ENGI_DIGITAL);
#else
    refuse(mask&~(1u<<ENGI_FM6),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask,ENGI_FM6);
    REQUIRE(eng_runtime_target(&out,mask,ENGI_DIGITAL,0,D8ENGINE_CANONICAL)==D8ENGINE_INVALID_REQUEST && out==99);
#endif
    fresh();input.t[0].engine=ENGI_SAMPLE;input.t[0].p[P_E0]=SMP_SET_PERC;
    refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask&~(1u<<ENGI_SAMPLE),ENGI_DRUM); /* normalized target alone suffices */
    fresh();input.phys=1;input.t[0].engine=ENGI_PHYS;input.t[0].p[P_E0]=4;
    refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask&~(1u<<ENGI_PHYS),ENGI_DRUM);
    fresh();input.phys=0;input.t[0].engine=ENGI_PHYS;input.t[0].p[P_E0]=2;
    refuse(mask&~(1u<<ENGI_PHYS),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask&~(1u<<ENGI_DRUM),ENGI_PHYS);
    REQUIRE(trk[0].p[P_E0]==PM_MODAL && trk[0].p[P_E0+6]==64); /* historical DUST */
    fresh();input.phys=0;input.t[0].engine=ENGI_PHYS;input.t[0].p[P_E0]=4;
    refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask&~(1u<<ENGI_PHYS),ENGI_DRUM); /* revision-0 MODEL DRUM */
#if NTRK == 8
    fresh();input.t[7].engine=ENGI_DRUM;
    refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE); /* only track 8 unavailable */
#endif
    fresh();input.parts=0;refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE);
    fresh();input.t[0].preset=PROJ_DEF_KEEP;refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE);
    fresh();input.t[0].engine=ENGI_SAMPLE;input.t[0].p[P_E0]=0;input.t[0].preset=PROJ_DEF_SOUND;
    refuse(mask&~(1u<<ENGI_DRUM),PROJ_RUNTIME_MISSING_ENGINE); /* late factory PERC alias */
    refuse(mask&~(1u<<ENGI_SAMPLE),PROJ_RUNTIME_MISSING_ENGINE); /* initial published engine */
    accept(mask,ENGI_DRUM);
    fresh();input.t[0].engine=ENGI_SAMPLE;input.t[0].p[P_E0]=0;input.t[0].preset=4;
    accept(mask&~(1u<<ENGI_DRUM),ENGI_SAMPLE); /* stored params, not factory preset number */
#if FELUCCA_SLICE
    fresh();input.t[0].engine=13;refuse(mask&~(1u<<13),PROJ_RUNTIME_MISSING_ENGINE);
    accept(mask,13);
#else
    fresh();input.t[0].engine=13;refuse(mask,1);
#endif
    /* A caller's arena input also remains byte-identical on early policy refusal. */
#if NPART >= NVOICE
    fresh();main_workspace.project=input;project_t original=main_workspace.project;snapshot();
    REQUIRE(project_restore_runtime_mask(&main_workspace.project,1,mask&~1u)==PROJ_RUNTIME_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(&original,&main_workspace.project,sizeof original));
    static d8p1_project_state native;
    static uint8_t raw[D8P1_LIMIT],saved[D8P1_LIMIT];size_t n=0;
    fresh();memset(&native,0,sizeof native);native.project=input;
    REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);
    cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);cv_blit(0,20);
    REQUIRE(dma.p!=0);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask&~1u)==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(raw,saved,n));
    REQUIRE(d8p1_load_runtime(raw,n,15)==D8RT_OK); /* actual immutable production wrapper */
    fresh();input.t[7].engine=ENGI_DRUM;input.sum=proj_sum(&input);native.project=input;n=0;
    REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask&~(1u<<ENGI_DRUM))==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(raw,saved,n)); /* wire track 8 alone unavailable */
    fresh();input.t[0].engine=ENGI_SAMPLE;input.t[0].p[P_E0]=SMP_SET_PERC;input.sum=proj_sum(&input);
    native.project=input;n=0;REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask&~(1u<<ENGI_DRUM))==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(raw,saved,n));
    fresh();input.t[0].engine=ENGI_SAMPLE;input.t[0].p[P_E0]=0;input.t[0].preset=PROJ_DEF_SOUND;
    input.sum=proj_sum(&input);native.project=input;n=0;
    REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask&~(1u<<ENGI_DRUM))==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(raw,saved,n)); /* encoded sentinel preview */
    fresh();native.project=input;n=0;REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));
    d8p1_stage_workspace *stage=main_d8p1_workspace_try();REQUIRE(stage);
    memcpy(stage->wire,raw,n);memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(stage->wire,n,15,mask&~1u)==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(stage->wire,saved,n)); /* borrowed wire unchanged */
    fresh();input.t[0].engine=ENGI_DIGITAL;input.sum=proj_sum(&input);native.project=input;n=0;
    REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask&~(FELUCCA_FM4?2u:1u<<ENGI_FM6))==D8RT_MISSING_ENGINE);
    unchanged();REQUIRE(!memcmp(raw,saved,n));
#if !FELUCCA_SLICE
    fresh();input.t[0].engine=13;input.sum=proj_sum(&input);native.project=input;n=0;
    REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
    REQUIRE(d8p1_load_runtime_mask(raw,n,15,mask)==D8RT_BAD);
    unchanged();REQUIRE(!memcmp(raw,saved,n)); /* structural ID valid, implementation absent */
#endif
#endif
    fresh();saved_input=input;REQUIRE(project_restore_runtime(&input)==0);
    REQUIRE(!memcmp(&input,&saved_input,sizeof input));
    printf("engine adoption: tracks=%u FM4=%u SLICE=%u proofs=%u PASS; live-state refusal and legacy target checks, host only\n",NTRK,FELUCCA_FM4,FELUCCA_SLICE,proofs);
    return 0;
}
