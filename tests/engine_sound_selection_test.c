/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI and editor sound publication; parameterized masks are internal only. */
#ifdef D8_SOUND_UI_TEST
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#define fresh ui_power_on
#else
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
#define fresh reset
#endif
#include <assert.h>
static unsigned proofs;
#define REQUIRE(x) do { proofs++; if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
#define STATE(X) __typeof__(X) X
static struct {
    STATE(trk); STATE(song); STATE(motion); STATE(chain); STATE(chain_config);
    STATE(fm6_patch); STATE(fm6_slot); uint8_t generations[sizeof fm6_pgen];
    STATE(ui); STATE(undo); STATE(up_bank); STATE(upf); STATE(favorites);
    STATE(pat_sig); STATE(pat_last);
#ifdef D8_SOUND_UI_TEST
    STATE(lys);
#else
    STATE(ed_w);
#endif
    uint8_t panic,sync,depth;
} before;
#define COPY(X) memcpy(&before.X, &(X), sizeof(X))
#define SAME(X) REQUIRE(!memcmp(&before.X, &(X), sizeof(X)))
static void snapshot(void)
{
    COPY(trk); COPY(song); COPY(motion); COPY(chain); COPY(chain_config);
    COPY(fm6_patch); COPY(fm6_slot); memcpy(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen);
    COPY(ui); COPY(undo); COPY(up_bank); COPY(upf); COPY(favorites); COPY(pat_sig); COPY(pat_last);
#ifdef D8_SOUND_UI_TEST
    COPY(lys);
#else
    COPY(ed_w);
#endif
    before.panic=panic_req;before.sync=sync_reload;before.depth=undo_depth;
}
static void unchanged(void)
{
    SAME(trk); SAME(song); SAME(motion); SAME(chain); SAME(chain_config);
    SAME(fm6_patch); SAME(fm6_slot); REQUIRE(!memcmp(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen));
    SAME(ui); SAME(undo); SAME(up_bank); SAME(upf); SAME(favorites); SAME(pat_sig); SAME(pat_last);
#ifdef D8_SOUND_UI_TEST
    SAME(lys);
#else
    SAME(ed_w);
#endif
    REQUIRE(before.panic==panic_req && before.sync==sync_reload && before.depth==undo_depth);
}
static void seed(void)
{
    fresh();
    undo.keep=1;
    REQUIRE(!set_engine(ENGI_FM6));
    int rc=up_store(0,"RETAINED"); REQUIRE(rc==0 || rc==3); REQUIRE(up_used(0));
    REQUIRE(favorite_set(NENGINES,0,1)); REQUIRE(favorite_set(ENGI_FM6,0,1));
    REQUIRE(!set_engine(0));
    int32_t audio[2u*CTL];
    mix_block(audio,CTL); /* consume engine/panic requests before seeding actual shared sounding voices */
    for(unsigned t=0;t<NTRK;t++) { trk[t].p[P_VOICE]=V_MONO; trk_note_on(&trk[t],60u+t,100u); }
    mix_block(audio,CTL);
    REQUIRE(voices_busy()==NTRK);
    panic_req=0;sync_reload=0;undo.keep=1;
#ifdef D8_SOUND_UI_TEST
    memset(&lys,0,sizeof lys);
#endif
}
#ifndef D8_SOUND_UI_TEST
static void masked_request(unsigned cmd,const uint8_t *a,unsigned n,unsigned mask)
{
    uint8_t f[32]={ED_HDR0,ED_HDR1,ED_HDR2}; REQUIRE(n+4<=sizeof f);
    f[3]=cmd;memcpy(f+4,a,n);host_wire_n=0;
    ed_handle_mask(f,n+4,mask);host_drain();
}
static void error_reply(unsigned cmd)
{
    REQUIRE(host_wire_n==9 && host_wire[4]==ED_D8_ERROR && host_wire[5]==1 && host_wire[6]==cmd && host_wire[7]==5);
}
#endif
int main(void)
{
    const unsigned full=D8ENGINE_EFFECTIVE_MASK, nofm=full&~(1u<<ENGI_FM6), nodrum=full&~(1u<<ENGI_DRUM);
    for(unsigned only=0;only<NENGINES;only++) {
        unsigned one=1u<<only; if(!(one&full)) continue;
        seed(); snapshot();
        for(unsigned e=0;e<NENGINES;e++) {
            /* Independent legacy identity oracle: DIGITAL alone converts when FM4 is absent. */
            unsigned expected = e;
#if !FELUCCA_FM4
            if(e==ENGI_DIGITAL) expected=ENGI_FM6;
#endif
            if(expected!=only) { REQUIRE(set_engine_of_mask(TSEL,e,one)==D8ENGINE_UNAVAILABLE); unchanged(); }
            else { REQUIRE(!set_engine_of_mask(TSEL,e,one)); REQUIRE(TSEL->eng_req==expected); snapshot(); }
        }
    }
    seed(); snapshot();REQUIRE(set_engine_of_mask(TSEL,NENGINES,full)==D8ENGINE_INVALID_REQUEST);unchanged();
    REQUIRE(set_engine_of_mask(TSEL,UINT32_MAX,full)==D8ENGINE_INVALID_REQUEST);unchanged();
    REQUIRE(select_engine_mask(ENGI_FM6,nofm)==D8ENGINE_UNAVAILABLE);unchanged();
    REQUIRE(set_engine_of_mask(TSEL,0,0)==D8ENGINE_INVALID_MASK);unchanged();
    REQUIRE(factory_load_mask(ENGI_SAMPLE,SMP_SET_PERC,nodrum)==D8ENGINE_UNAVAILABLE);unchanged();
    REQUIRE(up_load_mask(0,nofm)==D8ENGINE_UNAVAILABLE);unchanged();
    up_ui_named_mask(0,0,0,nofm);unchanged();
    favorites.filter=1;snapshot();
    unsigned total, k; preset_pos(&total); REQUIRE(total==2);
    for(unsigned n=0;n<total;n++){unsigned e=preset_at(n,&k);REQUIRE(e==ENGI_FM6 || e==NENGINES);REQUIRE(preset_go_mask(n,nofm)==D8ENGINE_UNAVAILABLE);unchanged();}
    favorites.filter=0;
    REQUIRE(!set_engine(ENGI_SAMPLE));snapshot();
    REQUIRE(apply_preset_to_mask(TSEL,SMP_SET_PERC,nodrum)==D8ENGINE_UNAVAILABLE);unchanged();
    TSEL->eng_req=NENGINES;snapshot(); REQUIRE(apply_preset_to_mask(TSEL,0,full)==D8ENGINE_INVALID_REQUEST);unchanged();
    REQUIRE(!set_engine(0));snapshot();
#if FELUCCA_FM4
    REQUIRE(factory_load_mask(ENGI_DIGITAL,2,full&~2u)==D8ENGINE_UNAVAILABLE);unchanged();
    REQUIRE(!factory_load_mask(ENGI_DIGITAL,2,nofm)); REQUIRE(TSEL->eng_req==ENGI_DIGITAL);
#else
    REQUIRE(factory_load_mask(ENGI_DIGITAL,2,nofm)==D8ENGINE_UNAVAILABLE);unchanged();
    REQUIRE(!factory_load_mask(ENGI_DIGITAL,2,full)); REQUIRE(TSEL->eng_req==ENGI_FM6);
#endif
    /* Structural records remain intact, DIGITAL user sounds use SELECT rather than factory PERC conversion. */
    up_rec(0)->engine=ENGI_DIGITAL; REQUIRE(up_valid(up_rec(0))); snapshot();
    unsigned nodig=full&~(1u<<(FELUCCA_FM4?ENGI_DIGITAL:ENGI_FM6));
    REQUIRE(up_load_mask(0,nodig)==D8ENGINE_UNAVAILABLE);unchanged();
    up_rec(0)->engine=ENGI_SAMPLE; REQUIRE(!up_load_mask(0,nodrum)); REQUIRE(TSEL->eng_req==ENGI_SAMPLE);
#ifdef D8_SOUND_UI_TEST
    seed();snapshot(); REQUIRE(edit_load_mask(ENGI_FM6,1,nofm)==D8ENGINE_UNAVAILABLE);unchanged();
    REQUIRE(edit_load_mask(NENGINES+1,1,full)==D8ENGINE_INVALID_REQUEST);unchanged();
    REQUIRE(!set_engine(ENGI_FM6));snapshot();
    REQUIRE(edit_load_mask(NENGINES,1,nofm)==D8ENGINE_UNAVAILABLE);unchanged();
    undo.trk=1;undo.what=UNDO_SOUND;undo.eng=ENGI_FM6;snapshot();undo_swap_mask(nofm);unchanged();
    undo.eng=NENGINES;snapshot();undo_swap_mask(full);unchanged();
    REQUIRE(!edit_load_mask(ENGI_DRUM,1,full));REQUIRE(TSEL->eng_req==ENGI_DRUM && lys.loaded);
#else
#if NTRK <= 4
    seed(); uint8_t a[4]={ENGI_FM6,1};snapshot();masked_request(ED_PRESET,a,2,nofm);error_reply(ED_PRESET);unchanged();
    a[0]=ENGI_SAMPLE;a[1]=SMP_SET_PERC;masked_request(ED_PRESET,a,2,nodrum);error_reply(ED_PRESET);unchanged();
    a[0]=NENGINES;masked_request(ED_PRESET,a,2,full);error_reply(ED_PRESET);unchanged();
    a[0]=1;a[1]=G_ENGSEL;
    for(int v=-1;v<=(int)NENGINES;v+=(int)NENGINES+1){unsigned u=v+8192;a[2]=u&127;a[3]=u>>7;masked_request(ED_SET,a,4,full);error_reply(ED_SET);unchanged();}
    unsigned u=ENGI_FM6+8192;a[2]=u&127;a[3]=u>>7;masked_request(ED_SET,a,4,nofm);error_reply(ED_SET);unchanged();
    a[0]=0;masked_request(ED_UP_LOAD,a,1,nofm);REQUIRE(host_wire_n==8 && host_wire[4]==ED_UP_LOAD && host_wire[6]==1);unchanged();
    a[0]=ENGI_SAMPLE;a[1]=SMP_SET_PERC;request(ED_PRESET,a,2);REQUIRE(host_wire[4]==ED_PRESET && TSEL->eng_req==ENGI_DRUM);
    a[0]=ENGI_DIGITAL;a[1]=2;request(ED_PRESET,a,2);REQUIRE(host_wire[4]==ED_PRESET && TSEL->eng_req==(FELUCCA_FM4?ENGI_DIGITAL:ENGI_FM6));
#else
    seed();uint8_t a[2]={ENGI_FM6,1};snapshot();masked_request(ED_PRESET,a,2,nofm);REQUIRE(host_wire[4]==ED_D8_ERROR && host_wire[7]==1);unchanged();
#endif
#endif
    seed(); REQUIRE(!up_load(0)); REQUIRE(TSEL->eng_req==ENGI_FM6 && TSEL->user==1);
    REQUIRE(!factory_load_mask(ENGI_SAMPLE,SMP_SET_PERC,nodrum|(1u<<ENGI_DRUM)));REQUIRE(TSEL->eng_req==ENGI_DRUM);
    printf("sound selection: %u proofs tracks=%u FM4=%u SLICE=%u %s\n",proofs,NTRK,FELUCCA_FM4,FELUCCA_SLICE,
#ifdef D8_SOUND_UI_TEST
    "actual UI"
#else
    "actual editor"
#endif
    );return 0;
}
