/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual editor structural metadata and curated EDIT sound browsing regressions. */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
static unsigned checks;
#define R(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned string_at(unsigned pos,const char*expected){size_t len=strlen(expected);R(pos+len<host_wire_n);R(!memcmp(host_wire+pos,expected,len)&&host_wire[pos+len]==0);return pos+len+1;}
static unsigned mask_oracle(void){return D8BUILD_PROFILE==1?0x1409u:D8BUILD_PROFILE==2?0x3411u:0x3ffdu;}
static void record(unsigned k,unsigned e){up_rec_t*r=up_rec(k);memset(r,0,sizeof *r);r->used=UP_USED;r->ver=UP_VER;r->engine=e;r->np=P_COUNT;memcpy(r->name,"PRESERVED",10);for(unsigned p=0;p<P_COUNT;p++)up_set_value(r,p,TSEL->p[p]);R(up_valid(r));}
int main(void){
 reset();R(NENGINES==14);R(D8ENGINE_EFFECTIVE_MASK==mask_oracle());
 uint8_t a[4]={1,G_ENGSEL};unsigned n=request(ED_DESC,a,2);
 R(GP[G_ENGSEL].max==13);R(host_wire[4]==ED_DESC&&host_wire[5]==1&&host_wire[6]==G_ENGSEL);R(ed_rv(host_wire+8)==0&&ed_rv(host_wire+10)==13);
 unsigned pos=string_at(14,GP[G_ENGSEL].label);pos=string_at(pos,GP[G_ENGSEL].unit?GP[G_ENGSEL].unit:"");for(unsigned e=0;e<14;e++)pos=string_at(pos,N_ENGNAME[e]);R(pos+1==n&&host_wire[pos]==0xf7);R(!strcmp(N_ENGNAME[13],"SLICE"));
 n=request(ED_INFO,a,0);pos=5;while(pos<n&&host_wire[pos])pos++;R(++pos<n&&host_wire[pos]==14);pos+=5;for(unsigned e=0;e<14;e++)pos=string_at(pos,ENGINES[e]->name);
 a[0]=13;n=request(ED_NAMES,a,1);R(host_wire[5]==13&&host_wire[6]==ENGINES[13]->npresets);pos=7;for(unsigned k=0;k<ENGINES[13]->npresets;k++)pos=string_at(pos,ENGINES[13]->presets[k].name);pos=string_at(pos,ENGINES[13]->page_title[0]);pos=string_at(pos,ENGINES[13]->page_title[1]);R(pos+1==n&&host_wire[pos]==0xf7);
 a[0]=2;n=request(ED_D8_CAPS,a,1);R(n==14&&host_wire[9]==14);R((host_wire[10]|host_wire[11]<<7)==mask_oracle());R(host_wire[12]==0);
 if(D8BUILD_PROFILE==1){track_t old=*TSEL;R(set_engine_of(TSEL,13)==D8ENGINE_UNAVAILABLE);R(!memcmp(&old,TSEL,sizeof old));}
#if D8BUILD_PROFILE != 0
 R(!set_engine(ENGI_FM6));R(!apply_preset_to(TSEL,0));record(0,7);record(1,12);record(2,9);favorite_set(NENGINES,0,1);favorite_set(NENGINES,2,1);
 up_bank_t bank[sizeof up_bank/sizeof up_bank[0]];memcpy(bank,up_bank,sizeof bank);__typeof__(favorites) fav=favorites;
 R(up_engine(0)==7&&up_engine(2)==9);char label[16];up_auto_name(label,7,0);R(!strcmp(label,"WHEEL 01"));up_auto_name(label,9,2);R(!strcmp(label,"PHYS 03"));up_auto_name(label,1,3);R(!strcmp(label,"FM6 04"));
 unsigned total,np=preset_shown(12);TSEL->preset=preset_orig(ENGINES[12],np-1);TSEL->user=0;R(eng_list_pos(&total)==np-1&&total==np+1);
 R(!eng_list_step_mask(1,D8ENGINE_EFFECTIVE_MASK,1));R(user_of(TSEL)==1&&TSEL->eng_req==12);
 TSEL->user=0;TSEL->preset=preset_orig(ENGINES[12],0);R(!eng_list_step_mask(-1,D8ENGINE_EFFECTIVE_MASK,1));R(user_of(TSEL)==1&&TSEL->eng_req==12);
 R(!memcmp(bank,up_bank,sizeof bank));R(!memcmp(&fav,&favorites,sizeof fav));R(up_count()==3);R(up_ready(0,D8ENGINE_EFFECTIVE_MASK)==D8ENGINE_UNAVAILABLE&&up_ready(2,D8ENGINE_EFFECTIVE_MASK)==D8ENGINE_UNAVAILABLE);
 record(3,1);R(up_engine(3)==12);R(!up_ready(3,D8ENGINE_EFFECTIVE_MASK));
#endif
 printf("profile%u metadata/browse checks%u PASS\n",D8BUILD_PROFILE,checks);return 0;
}
