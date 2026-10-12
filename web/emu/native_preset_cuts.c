/* SPDX-License-Identifier: GPL-3.0-only */
/* Joint record/patch transaction cuts, synthetic NOR only. */
#define PRESET_MATRIX 1
#include "native_preset_check.c"
static uint8_t baseline[sizeof nor];
static int save_slot(unsigned slot,unsigned engine)
{ host_preset(&trk[7],engine,0);song.sel=7;name_open(NK_USER_SAVE,slot);tap(B_OCTUP);return up_used(slot); }
static void request_new(void)
{ host_preset(&trk[7],ENGI_FM6,0);song.sel=7;trk[7].p[P_LEVEL]=17;fm6_patch[7][FP_ALG]=(uint8_t)((fm6_patch[7][FP_ALG]+1)%32);name_open(NK_USER_SAVE,0);tap(B_OCTUP); }
static int reboot_fixture(void)
{ d8up_ui_end();name_close();ui.confirm=0;web_buttons(0);up_boot();return d8up_recover_banks(); }
int main(int argc,char **argv)
{
 web_nor_erase();felucca_init();static uint8_t wire[D8P1_LIMIT];size_t n=0;
 CHECK(d8p1_capture_runtime(wire,sizeof wire,&n)==D8RT_OK);d8pool pool={NULL,pr,pe,pp,ps};CHECK(d8pool_save(&pool,0,wire,n,1)==D8POOL_OK);if(argc==3&&!strcmp(argv[1],"first-initialized")){FILE*f=fopen(argv[2],"rb");CHECK(f);CHECK(fread(nor,1,sizeof nor,f)==sizeof nor);CHECK(fclose(f)==0);}
 web_boot();elapsed(500);
 if((argc==2&&!strcmp(argv[1],"first-prototype"))||(argc==3&&!strcmp(argv[1],"first-initialized"))){
  memcpy(baseline,nor,sizeof nor);primitive_count=0;request_new();unsigned steps=primitive_count;up_rec_t newer=*up_rec(0);uint8_t newpatch[FM6_PACKED];CHECK(!upf_get(0,newpatch));
  for(unsigned after=0;after<2;after++)for(unsigned cut=1;cut<=steps;cut++){
   memcpy(nor,baseline,sizeof nor);CHECK(!reboot_fixture());cut_at=cut;cut_after=after;primitive_count=0;request_new();cut_at=0;int recovered=reboot_fixture();
   if(argc==3)CHECK(!recovered);
   if(up_used(0)){uint8_t loaded[FM6_PACKED];CHECK(!recovered);CHECK(!upf_get(0,loaded));CHECK(!memcmp(up_rec(0),&newer,sizeof newer));CHECK(!memcmp(loaded,newpatch,sizeof loaded));}
   else {CHECK(!up_used(0));if(recovered){CHECK(!memcmp(ui.msg,"PRESET RECOVERY",15));static uint8_t refused[sizeof nor];memcpy(refused,nor,sizeof nor);unsigned writes=web_flash_writes_count();request_new();CHECK(web_flash_writes_count()==writes);CHECK(!memcmp(nor,refused,sizeof nor));}}
   for(unsigned b=0;b<5;b++)CHECK(!memcmp(nor+d8pool_mapped_address(b),baseline+d8pool_mapped_address(b),D8POOL_BLOCK));
   printf("first prototype cut %u %s: %s\n",cut,after?"after":"before",up_used(0)?"complete new":"empty/refused, no init fallback");
  }
  printf("first %s preset cuts: %u boundaries x2; %u checks PASS\n",argc==3?"initialized":"prototype",steps,checks);return 0;
 }
 CHECK(save_slot(0,ENGI_FM6));CHECK(save_slot(1,0));CHECK(save_slot(16,ENGI_FM6));
 up_rec_t old=*up_rec(0),other=*up_rec(1),otherbank=*up_rec(16);uint8_t oldpatch[FM6_PACKED],otherpatch[FM6_PACKED];CHECK(!upf_get(0,oldpatch));CHECK(!upf_get(16,otherpatch));memcpy(baseline,nor,sizeof nor);
 primitive_count=0;request_new();CHECK(primitive_count>0);unsigned steps=primitive_count;up_rec_t newer=*up_rec(0);uint8_t newpatch[FM6_PACKED];CHECK(!upf_get(0,newpatch));CHECK(memcmp(&old,&newer,sizeof old));CHECK(memcmp(oldpatch,newpatch,sizeof oldpatch));
 for(unsigned after=0;after<2;after++)for(unsigned cut=1;cut<=steps;cut++){
  memcpy(nor,baseline,sizeof nor);CHECK(!reboot_fixture());cut_at=cut;cut_after=after;primitive_count=0;request_new();cut_at=0;
  CHECK(!reboot_fixture());CHECK(up_used(0));uint8_t loaded[FM6_PACKED],kept[FM6_PACKED];CHECK(!upf_get(0,loaded));CHECK(!upf_get(16,kept));
  int wasold=!memcmp(up_rec(0),&old,sizeof old)&&!memcmp(loaded,oldpatch,sizeof loaded);
  int wasnew=!memcmp(up_rec(0),&newer,sizeof newer)&&!memcmp(loaded,newpatch,sizeof loaded);
  CHECK(wasold||wasnew);CHECK(!memcmp(up_rec(1),&other,sizeof other));CHECK(!memcmp(up_rec(16),&otherbank,sizeof otherbank));CHECK(!memcmp(kept,otherpatch,sizeof kept));
  for(unsigned b=0;b<5;b++)CHECK(!memcmp(nor+d8pool_mapped_address(b),baseline+d8pool_mapped_address(b),D8POOL_BLOCK));
  // A retry must retain its sole coherent predecessor through another cut.
  uint8_t recovered[sizeof up_bank[0]];memcpy(recovered,&up_bank[0],sizeof recovered);
  cut_at=2;cut_after=1;primitive_count=0;request_new();cut_at=0;CHECK(!reboot_fixture());CHECK(!memcmp(&up_bank[0],recovered,sizeof recovered));CHECK(!upf_get(0,loaded));CHECK(!memcmp(loaded,wasold?oldpatch:newpatch,sizeof loaded));
  printf("joint cut %u %s: coherent %s, retry retains predecessor\n",cut,after?"after":"before",wasold?"old":"new");
 }
 printf("joint preset cuts: %u boundaries x2; %u checks PASS\n",steps,checks);return 0;
}
