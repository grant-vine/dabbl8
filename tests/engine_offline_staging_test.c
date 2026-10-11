/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual offline conversion; independent identities and real full-profile
 * factory loads as the parameter/patch oracle. No target qualification. */
#define D8_SOUND_UI_TEST 1
#define ENGINE_SOUND_TEST_NO_MAIN 1
#include "engine_sound_selection_test.c"
#include "../firmware/src/d8p1_project.h"
static project_store_t source,pristine;
static d8p1_project_state result,old,work,normal;
static d8p1_legacy_report report,oldreport,normalreport;
static void checksum(void){uint32_t sum=proj_hash(source.raw,PROJ_STORE_SIZE-4);memcpy(source.raw+PROJ_STORE_SIZE-4,&sum,4);}
static unsigned offset(unsigned t){return 68+t*(P_COUNT+2+NSTEP*9);}
static void originals(unsigned e,unsigned preset){
 for(unsigned t=0;t<4;t++){
  unsigned pos=offset(t);for(unsigned i=0;i<P_COUNT;i++)source.raw[pos+i]=(uint8_t)(param_desc_of(e,i)->def+64);
  for(unsigned i=0;i<8;i++)source.raw[pos+P_E0+i]=ENGINES[e]->presets[0].e[i]+64;
  source.raw[pos+P_COUNT]=e;source.raw[pos+P_COUNT+1]=preset;
 }checksum();
}
static void preserved(void){REQUIRE(!memcmp(&source,&pristine,sizeof source));REQUIRE(!memcmp(&result,&old,sizeof result));REQUIRE(!memcmp(&report,&oldreport,sizeof report));unchanged();}
static void refusal(unsigned mask,int rc,size_t n){
 memset(&result,0xA5,sizeof result);old=result;memset(&report,0xA6,sizeof report);oldreport=report;pristine=source;snapshot();
 REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,n,15,mask)==rc);preserved();
}
int main(int argc,char **argv){
 REQUIRE(argc>=2);FILE *f=fopen(argv[1],"rb");REQUIRE(f);REQUIRE(fread(&source,1,sizeof source,f)==sizeof source);REQUIRE(fgetc(f)==EOF);fclose(f);
 seed();project_store_t initial=source;
 const unsigned ids[]={0,12,
#if FELUCCA_FM4
 1,
#endif
 2,3,4,5,6,7,8,9,11,
#if FELUCCA_SLICE
 13,
#endif
 10};
 for(unsigned j=0;j<sizeof ids/sizeof ids[0];j++){
  unsigned e=ids[j];source=initial;originals(e,0);pristine=source;
  REQUIRE(d8p1_legacy_convert(&normal,&work,&normalreport,&source,sizeof source,15));
  snapshot();REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,sizeof source,15,1u<<e)==0);unchanged();
  REQUIRE(!memcmp(&source,&pristine,sizeof source));REQUIRE(!memcmp(&report,&normalreport,sizeof report));
  for(unsigned t=0;t<4;t++){REQUIRE(!memcmp(&result.project.t[t],&normal.project.t[t],sizeof(proj_trk_t)));REQUIRE(!memcmp(result.project.fm6[t],normal.project.fm6[t],FM6_PACKED));}
  REQUIRE(!memcmp(&result.arrangement,&normal.arrangement,sizeof result.arrangement));
  if(e==0)REQUIRE(!memcmp(&result,&normal,sizeof result));
  else {
   track_t *tr=&trk[4];track_defaults(tr);REQUIRE(!set_engine_of(tr,e));REQUIRE(!apply_preset_to(tr,0));
   for(unsigned t=4;t<8;t++){
    REQUIRE(result.project.t[t].engine==e&&result.project.t[t].preset==0);
    REQUIRE(!memcmp(result.project.t[t].p,tr->p,sizeof tr->p));
    uint8_t patch[FM6_PACKED];if(e==12)fm6_pack(fm6_patch[4],patch);else memcpy(patch,FM6_INIT,sizeof patch);
    REQUIRE(!memcmp(result.project.fm6[t],patch,sizeof patch));
    for(unsigned s=0;s<64;s++)REQUIRE(result.project.t[t].step[s].time==ST_REST);
   }
  }
  REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,sizeof source,15,D8ENGINE_EFFECTIVE_MASK)==0);
  REQUIRE(!memcmp(&result,&normal,sizeof result));
 }
 seed();source=initial;originals(0,0);
 refusal(1u<<12,D8PS_PROFILE_UNAVAILABLE,sizeof source);refusal(0,D8PS_PROFILE_INVALID_MASK,sizeof source);refusal(1u<<30,D8PS_PROFILE_INVALID_MASK,sizeof source);
 for(size_t n=0;n<sizeof source;n++)refusal(D8ENGINE_EFFECTIVE_MASK,D8PS_PROFILE_BAD_DATA,n);
 source=initial;originals(4,PROJ_DEF_SOUND);refusal(1u<<4,D8PS_PROFILE_UNAVAILABLE,sizeof source); /* track 1 late PERC preset 4 needs DRUM */
 source=initial;originals(0,PROJ_DEF_KEEP);refusal(1u<<0,D8PS_PROFILE_UNAVAILABLE,sizeof source);
 source=initial;originals(4,PROJ_DEF_SOUND);
 REQUIRE(d8p1_legacy_convert(&normal,&work,&normalreport,&source,sizeof source,15));pristine=source;snapshot();
 REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,sizeof source,15,(1u<<4)|(1u<<10))==0);unchanged();
 REQUIRE(!memcmp(&source,&pristine,sizeof source)&&!memcmp(&report,&normalreport,sizeof report));
 for(unsigned t=0;t<4;t++){REQUIRE(!memcmp(&result.project.t[t],&normal.project.t[t],sizeof(proj_trk_t)));REQUIRE(!memcmp(result.project.fm6[t],normal.project.fm6[t],FM6_PACKED));}
 for(unsigned t=4;t<8;t++)REQUIRE(result.project.t[t].engine==4&&result.project.t[t].preset==0);
 source=initial;originals(0,PROJ_DEF_KEEP);
 REQUIRE(d8p1_legacy_convert(&normal,&work,&normalreport,&source,sizeof source,15));pristine=source;snapshot();
 REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,sizeof source,15,(1u<<0)|(1u<<10))==0);unchanged();
 REQUIRE(!memcmp(&source,&pristine,sizeof source)&&!memcmp(&report,&normalreport,sizeof report)&&!memcmp(&result,&normal,sizeof result));

 source=initial;originals(0,0);source.raw[offset(0)+P_COUNT]=14;checksum();refusal(D8ENGINE_EFFECTIVE_MASK,D8PS_PROFILE_UNSUPPORTED_SOURCE,sizeof source);
 source.raw[100]^=1;refusal(D8ENGINE_EFFECTIVE_MASK,D8PS_PROFILE_BAD_DATA,sizeof source);
#if !FELUCCA_SLICE
 source=initial;originals(0,0);source.raw[offset(0)+P_COUNT]=13;checksum();refusal(D8ENGINE_EFFECTIVE_MASK,D8PS_PROFILE_UNSUPPORTED_SOURCE,sizeof source);
#endif
 source=initial;originals(0,0);source.raw[offset(0)+P_COUNT]=1;checksum();
 REQUIRE(d8p1_legacy_convert(&normal,&work,&normalreport,&source,sizeof source,15));
 unsigned expected_digital=
#if FELUCCA_FM4
 1;
#else
 12;
#endif
 REQUIRE(normal.project.t[0].engine==expected_digital);
 REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,sizeof source,15,D8ENGINE_EFFECTIVE_MASK)==0&& !memcmp(&result,&normal,sizeof result));
 refusal(D8ENGINE_EFFECTIVE_MASK&~(1u<<expected_digital),D8PS_PROFILE_UNAVAILABLE,sizeof source);
 for(int a=2;a<argc;a++){
  source=initial;FILE *g=fopen(argv[a],"rb");REQUIRE(g);size_t n=fread(source.raw,1,sizeof source,g);REQUIRE(fgetc(g)==EOF);fclose(g);pristine=source;
  REQUIRE(d8p1_legacy_convert(&normal,&work,&normalreport,&source,n,15));snapshot();
  REQUIRE(d8p1_legacy_convert_profile_v1(&result,&work,&report,&source,n,15,D8ENGINE_EFFECTIVE_MASK)==0);unchanged();
  REQUIRE(!memcmp(&result,&normal,sizeof result)&&!memcmp(&report,&normalreport,sizeof report)&&!memcmp(&source,&pristine,sizeof source));
 }
 source=initial;originals(0,0);old=result;oldreport=report;pristine=source;snapshot();
 REQUIRE(d8p1_legacy_convert_profile_v1(&result,&result,&report,&source,sizeof source,15,D8ENGINE_EFFECTIVE_MASK)==D8PS_PROFILE_BAD_ARGUMENT);preserved();
 REQUIRE(d8p1_legacy_stage_profile_v1(&work,&report,&source,sizeof source,15,1u<<12)==D8PS_PROFILE_UNAVAILABLE);preserved();
 printf("Offline profile v1: %u checks PASS; FM4=%d SLICE=%d; original/sounding runtime preserved, no device proof\n",proofs,FELUCCA_FM4,FELUCCA_SLICE);
}
