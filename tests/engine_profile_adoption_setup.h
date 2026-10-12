/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual project/native adoption with live notes; host-only mask arguments.
 * Production wrappers remain immutable full compiled-engine builds. */
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
static unsigned lcd_fences;
static void fence(void){lcd_fences++;}
#define UI_LCD_SYNC_HOOK fence
#include "engine_profile_host.h"
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
