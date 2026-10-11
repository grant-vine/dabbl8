/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual filtered engine/preset lists, knobs, EDIT keys/LEDs/drawing and startup.
 * Expectations derive from literal stable identities/order, not policy helpers. */
#include <string.h>
static unsigned capture_text;
static char seen[8192];
static void text_seen(int y0,int y1,const char *s) { (void)y1; if(capture_text && y0<104 && strlen(seen)+strlen(s)+2<sizeof seen){strcat(seen,s);strcat(seen,"|");} }
#define GFX_HOOK_BEGIN() ((void)0)
#define GFX_HOOK_BLIT(x,y,r) ((void)0)
#define GFX_HOOK_PIXELS(n) ((void)0)
#define GFX_HOOK_TEXT(x0,y0,x1,y1,s,flags) text_seen(y0,y1,s)
#define D8_SOUND_UI_TEST 1
#define ENGINE_SOUND_TEST_NO_MAIN 1
#include "engine_sound_selection_test.c"
static const unsigned identity_order[]={0,12,
#if FELUCCA_FM4
1,
#endif
2,3,4,5,6,7,8,9,11,
#if FELUCCA_SLICE
13,
#endif
10};
static unsigned expected_order[14],expected_count;
static void expected(unsigned mask)
{
    expected_count=0;
    for(unsigned i=0;i<sizeof identity_order/sizeof identity_order[0];i++)if(mask&(1u<<identity_order[i]))expected_order[expected_count++]=identity_order[i];
}
static unsigned effective(unsigned raw) {
#if !FELUCCA_FM4
    if(raw==1) return 12;
#endif
    return raw;
}
static unsigned white_key(unsigned p) { for(unsigned k=0;k<27;k++)if(!key_black(k)&&key_place(k)==p)return k; return 99; }
static void check_lists(unsigned mask)
{
    expected(mask); seed();
    REQUIRE(eng_count_mask(mask)==expected_count);
    for(unsigned n=0;n<expected_count;n++){
        unsigned e=expected_order[n]; REQUIRE(eng_vis_mask(n,mask)==e); REQUIRE(eng_rank_mask(e,mask)==n);
        REQUIRE(eng_step_mask(e,1,mask)==expected_order[(n+1)%expected_count]);
        REQUIRE(eng_step_mask(e,-1,mask)==expected_order[(n+expected_count-1)%expected_count]);
    }
    REQUIRE(eng_vis_mask(expected_count,mask)==NENGINES);
    REQUIRE(eng_step_mask(NENGINES,1,mask)==NENGINES);
    /* Include structurally valid records for every compiled stable ID, including legacy DIGITAL. */
    for(unsigned e=0;e<NENGINES;e++) {
        REQUIRE(!set_engine(e)); int rc=up_store(e,"KEEP");REQUIRE(rc==0||rc==3);
        up_rec(e)->engine=e; REQUIRE(up_valid(up_rec(e)));
        favorite_set(NENGINES,e,1); favorite_set(e,0,1);
    }
    unsigned user_n=0; for(unsigned e=0;e<NENGINES;e++)user_n+=!!(mask&(1u<<effective(e)));
    REQUIRE(up_count()==NENGINES && up_sound_count(mask)==user_n);
    unsigned k,user_index=0;
    for(unsigned e=0;e<NENGINES;e++)if(mask&(1u<<effective(e))){REQUIRE(up_sound_nth(user_index++,mask)==e);}
    REQUIRE(up_sound_nth(user_index,mask)==UP_SLOTS);
    unsigned want=0;for(unsigned i=0;i<expected_count;i++){unsigned e=expected_order[i];want+=ENGINES[e]->npresets-(e==4?SMP_NALIAS:0);}
    unsigned total; favorites.filter=0; snapshot();preset_pos_mask(&total,mask);REQUIRE(total==want+user_n);
    unsigned index=0;
    for(unsigned i=0;i<expected_count;i++){
        unsigned e=expected_order[i];
        for(unsigned p=0;p<ENGINES[e]->npresets;p++) {
            if(e==4 && SMP_SET_ORIG[p]!=p)continue;
            REQUIRE(preset_at_mask(index++,&k,mask)==e && k==p);
        }
    }
    for(unsigned e=0;e<NENGINES;e++)if(mask&(1u<<effective(e)))REQUIRE(preset_at_mask(index++,&k,mask)==NENGINES && k==e);
    REQUIRE(index==total);REQUIRE(preset_at_mask(index,&k,mask)==NENGINES && k==UP_SLOTS);unchanged();
    favorites.filter=1;snapshot();preset_pos_mask(&total,mask);REQUIRE(total==expected_count+user_n);
    for(unsigned n=0;n<total;n++){unsigned e=preset_at_mask(n,&k,mask);REQUIRE(e==NENGINES?!!(mask&(1u<<effective(up_rec(k)->engine))):!!(mask&(1u<<e)));}
    unchanged(); favorites.filter=0;
    /* Real preset knob selects an available sound when its previous sound is absent. */
    REQUIRE(!set_engine(0));preset_step_mask(1,mask);REQUIRE(mask&(1u<<TSEL->eng_req));
    ui.home=0;for(unsigned pg=0;pg<NPAGES;pg++)if(PAGES[pg].graph==GR_BROWSE){ui.page=pg;break;}edit_param_mask(1,1,mask);REQUIRE(mask&(1u<<TSEL->eng_req));
    /* Refused exact requests do not clear records, favourites or sounding runtime. */
    seed(); unsigned missing=0;while(missing<NENGINES && (mask&(1u<<effective(missing))))missing++;
    if(missing<NENGINES){snapshot();REQUIRE(select_engine_mask(missing,mask)==D8ENGINE_UNAVAILABLE);unchanged();}
}
static void check_startup(unsigned mask)
{
    expected(mask); seed();
    for(unsigned t=0;t<NTRK;t++) {
        unsigned requested=t==0?0:t==1?12:t==2?3:t==3?10:0;
        unsigned engine=mask&(1u<<requested)?requested:expected_order[0];
        unsigned preset=(engine==requested)?(t<2?4:0):0;
        REQUIRE(!track_startup_mask(&trk[t],t,mask));REQUIRE(trk[t].eng_req==engine && trk[t].engine==engine && trk[t].preset==preset);
        REQUIRE(seq_is_empty(&trk[t]) && !trk[t].user);
        const preset_t *p=&ENGINES[engine]->presets[preset];
        for(unsigned j=0;j<8;j++)REQUIRE(trk[t].p[P_E0+j]==p->e[j]);
    }
    d8engine_sound out={99,99};REQUIRE(!eng_startup_sound(&out,mask,4,4));
    REQUIRE(out.engine==((mask&(1u<<10))?10:expected_order[0]) && !out.preset);
    seed();snapshot();REQUIRE(track_startup_mask(&trk[0],0,0)==D8ENGINE_INVALID_MASK);unchanged();
    REQUIRE(track_startup_mask(&trk[0],NTRK,mask)==D8ENGINE_INVALID_REQUEST);unchanged();
    out=(d8engine_sound){99,99};REQUIRE(eng_startup_sound(&out,mask,NENGINES,0)==D8ENGINE_INVALID_REQUEST);REQUIRE(out.engine==99 && out.preset==99);
}
static void check_layer(unsigned mask)
{
    expected(mask);seed();ui.layer=LAYER_EDIT;unsigned br;
    unsigned lit=layer_leds_mask(&br,mask);
    for(unsigned p=0;p<16;p++) {
        unsigned key=white_key(p);if(key==99)continue;
        REQUIRE(((lit|br)>>key&1u)==(p<=expected_count));
        if(p<expected_count) {
            ui.confirm=0;layer_key_mask(LAYER_EDIT,key,mask);REQUIRE(TSEL->eng_req==expected_order[p] && !ui.confirm);
        } else if(p==expected_count) {
            ui.confirm=0;layer_key_mask(LAYER_EDIT,key,mask);REQUIRE(ui.confirm==CF_INIT_SOUND);ui.confirm=0;
        } else {snapshot();layer_key_mask(LAYER_EDIT,key,mask);unchanged();}
    }
    ui.layer=LAYER_EDIT;ui.ly=LAYER_EDIT;ui.ly_t0=LY_OPEN;ui.lock=LAYER_EDIT;
    layer_knob_mask(0,1,mask);REQUIRE(mask&(1u<<TSEL->eng_req));
    seen[0]=0;capture_text=1;cv_begin(240,124,T_BG);layer_edit_mask(mask);capture_text=0;
    REQUIRE(strstr(seen,"INIT|"));
    if(expected_count==1)REQUIRE(strstr(seen,ENGINES[expected_order[0]]->name));
    cv_blit(0,0); engine_columns_mask(mask);
    cv_begin(240,124,T_BG); graph_browse_mask(mask); cv_blit(0,0);
}
int main(void)
{
    unsigned full=D8ENGINE_EFFECTIVE_MASK;
    check_lists(full);check_startup(full);check_layer(full);
    for(unsigned e=0;e<NENGINES;e++)if(full&(1u<<e)){unsigned one=1u<<e;check_lists(one);check_startup(one);check_layer(one);}
    for(unsigned e=0;e<NENGINES;e++)if(full&(1u<<e))check_lists(full&~(1u<<e));
    seed();snapshot();REQUIRE(!eng_count_mask(0) && eng_vis_mask(0,0)==NENGINES);REQUIRE(!eng_count_mask(full|0x4000));unchanged();
    printf("engine UI availability: %u proofs tracks=%u FM4=%u SLICE=%u; actual lists/knobs/keys/LEDs/drawing/startup, immutable records\n",proofs,NTRK,FELUCCA_FM4,FELUCCA_SLICE);return 0;
}
