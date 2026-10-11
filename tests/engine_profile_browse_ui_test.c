/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual EDIT layer KNOB 2 dispatch with preserved unavailable bank records. */
#include "engine_profile_host.h"
static unsigned checks;
#define R(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void seed(unsigned k,unsigned e){up_rec_t*r=up_rec(k);memset(r,0,sizeof *r);r->used=UP_USED;r->ver=UP_VER;r->engine=e;r->np=P_COUNT;memcpy(r->name,"PRESERVED",10);for(unsigned p=0;p<P_COUNT;p++)up_set_value(r,p,TSEL->p[p]);R(up_valid(r));}
int main(void){ui_power_on();R(D8ENGINE_EFFECTIVE_MASK==(D8BUILD_PROFILE==1?0x1409u:0x3411u));R(!set_engine(12));R(!apply_preset_to(TSEL,0));seed(0,7);seed(1,12);seed(2,9);favorite_set(NENGINES,0,1);favorite_set(NENGINES,2,1);__typeof__(up_bank) bank;memcpy(bank,up_bank,sizeof bank);__typeof__(favorites) fav=favorites;
 ui.ly=LAYER_EDIT;ui.ly_t0=LY_OPEN;fm1_in.buttons=ly_bit(LAYER_EDIT);R(layer_open()==LAYER_EDIT);
 unsigned np=preset_shown(12);TSEL->user=0;TSEL->preset=preset_orig(ENGINES[12],np-1);layer_knob(1,1);R(TSEL->eng_req==12&&user_of(TSEL)==1);
 TSEL->user=0;TSEL->preset=preset_orig(ENGINES[12],0);layer_knob(1,-1);R(TSEL->eng_req==12&&user_of(TSEL)==1);R(!memcmp(bank,up_bank,sizeof bank));R(!memcmp(&fav,&favorites,sizeof fav));R(up_count()==3);
 printf("profile%u actual EDIT KNOB2 checks%u PASS\n",D8BUILD_PROFILE,checks);return 0;}
