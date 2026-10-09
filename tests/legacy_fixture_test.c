/* SPDX-License-Identifier: GPL-3.0-only */
/* Immutable byte captures from unchanged v1.1.5, not private device backups. */
#define main hostsim_main
#include "hostsim.c"
#undef main
/* Enlarge only the import destination, not the engine/runtime. This detects
 * historical disk types/loops that accidentally follow the live track count. */
#ifdef LEGACY_DEST_TRACKS
#undef NTRK
#define NTRK LEGACY_DEST_TRACKS
#endif
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include <stddef.h>
_Static_assert(offsetof(project_v1_t, sum)==684, "FUN1 checksum");
_Static_assert(offsetof(project_v2_t, t)==66 && offsetof(project_v2_t, sum)==2548, "FUN2 offsets");
_Static_assert(offsetof(project_v3_t, t)==66 && offsetof(project_v3_t, sum)==2580, "FUN3 offsets");
_Static_assert(offsetof(project_v4_t, t)==66 && offsetof(project_v4_t, sum)==2676, "FUN4 offsets");
_Static_assert(offsetof(project_v5_t, t)==66 && offsetof(project_v5_t, sum)==3348, "FUN5 offsets");
_Static_assert(offsetof(project_v6_t, chain)==3346 && offsetof(project_v6_t, sum)==3384, "FUN6 offsets");
_Static_assert(PROJ_FM6_OFF==3120 && PROJ_NAME_OFF==3632, "FUN9 offsets");
static const struct {const char *name;unsigned size;} cases[]={
 {"fun1",688},{"fun1-digital",688},{"fun2",2552},{"fun3",2584},
 {"fun4",2680},{"fun4-phys-drum",2680},{"fun5",3352},{"fun6",3388},
 {"fun7",3388},{"fun8",3584},{"fun8-perc",3584},{"fun9",3648},{"fun9-digital",3648}};
static unsigned readfile(const char *dir,const char *name,const char *suffix,void *p) {
 char path[1024];snprintf(path,sizeof path,"%s/%s%s.bin",dir,name,suffix);
 FILE *f=fopen(path,"rb");if(!f){perror(path);exit(2);}unsigned n=fread(p,1,3648,f);
 if(ferror(f)||fgetc(f)!=EOF){fclose(f);exit(2);}fclose(f);return n;
}
int main(int argc,char **argv) {
 if(argc!=2)return 2;int bad=0;
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++) {
  union{uint32_t align;uint8_t raw[3648];} input,expected,corrupt;
  unsigned n=readfile(argv[1],cases[i].name,"",input.raw);
  unsigned en=readfile(argv[1],cases[i].name,".expected-fun9",expected.raw);
  project_t q,ref,round;project_store_t st;
  int ok=n==cases[i].size && en==3648 && proj_import(&q,input.raw,n) && proj_import(&ref,expected.raw,en);
  if(ok) ok=!memcmp(q.g,ref.g,sizeof q.g) && q.sel==ref.sel &&
   !memcmp(q.t,ref.t,4*sizeof q.t[0]) && !memcmp(q.fm6,ref.fm6,4*FM6_PACKED) &&
   !memcmp(&q.chain,&ref.chain,sizeof q.chain) && !memcmp(&q.motion,&ref.motion,sizeof q.motion) &&
   !memcmp(q.name,ref.name,sizeof q.name);
  if(NTRK==4) {
   ok=ok && proj_pack(&st,&q) && !memcmp(st.raw,expected.raw,3648) &&
    proj_import(&round,&st,sizeof st) && !memcmp(&round,&q,sizeof q);
  } else {
   memset(&st,0xa5,sizeof st);project_store_t before=st;
   ok=ok && !proj_pack(&st,&q) && !memcmp(&before,&st,sizeof st);
  }
  corrupt=input;corrupt.raw[100]^=1;
  ok=ok && !proj_import(&round,corrupt.raw,n) && !proj_import(&round,input.raw,n-1);
  printf("%s: %u B, destination %u tracks, baseline import/round-trip/corruption %s\n",cases[i].name,n,(unsigned)NTRK,ok?"ok":"FAIL");bad+=!ok;
 }
 return bad?1:0;
}
