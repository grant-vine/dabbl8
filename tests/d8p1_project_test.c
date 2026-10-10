/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include "../firmware/src/d8p1_project.h"
static uint8_t raw[D8P1_LIMIT],encoded[D8P1_LIMIT],before[D8P1_LIMIT];
static d8p1_project_state state,old,workspace,roundtrip,reference;
static project_t legacy_expected;
static unsigned checks,errors;
static void check(int yes,const char *label){checks++;if(!yes){errors++;fprintf(stderr,"FAIL: %s\n",label);}}
static size_t load(const char *path){FILE *f=fopen(path,"rb");if(!f)exit(2);size_t n=fread(raw,1,sizeof raw,f);if(fgetc(f)!=EOF)exit(2);fclose(f);return n;}
int main(int argc,char **argv){
 if(argc<4)return 2;
 for(unsigned f=1;f<=2;f++){
  size_t n=load(argv[f]),written=0;
  check(d8p1_project_decode(&state,raw,n,15),"independent file decodes to structured eight-track state");
  check(d8p1_project_encode(encoded,sizeof encoded,&written,&state)&&written==n&&!memcmp(raw,encoded,n),"structured state emits exact independent reference without payload scratch");
  check(state.project.parts==8&&state.project.phys==2&&state.project.chain.count==0,"new scene rows kept separate from historical project-slot chain");
  old=state;
  for(size_t i=0;i<n;i++)check(!d8p1_project_decode(&state,raw,i,15)&&!memcmp(&state,&old,sizeof state),"every truncation preserves complete destination state");
  memset(encoded,0xB5,sizeof encoded);memcpy(before,encoded,sizeof encoded);written=777;
  check(!d8p1_project_encode(encoded,n-1,&written,&state)&&written==777&&!memcmp(encoded,before,sizeof encoded),"short encode capacity preserves all output bytes");
  state.project.t[7].p[7]=-65;
  check(!d8p1_project_encode(encoded,sizeof encoded,&written,&state)&&written==777&&!memcmp(encoded,before,sizeof encoded),"out-of-range signed native value is refused without narrowing");state=old;
 }

 old=state;
 #define BAD_NATIVE(label,change) do { state=old;change;size_t w=777;memset(encoded,0xB5,sizeof encoded);memcpy(before,encoded,sizeof encoded);check(!d8p1_project_encode(encoded,sizeof encoded,&w,&state)&&w==777&&!memcmp(encoded,before,sizeof encoded),label); } while(0)
 BAD_NATIVE("invalid selected track",state.project.sel=8);
 BAD_NATIVE("legacy parts cannot silently become eight",state.project.parts=4);
 BAD_NATIVE("unconverted physical schema",state.project.phys=1);
 BAD_NATIVE("reserved native metadata",state.project.rsv=1);
 BAD_NATIVE("legacy slot chain cannot be silently discarded",state.project.chain.count=1);
 BAD_NATIVE("reserved legacy chain metadata cannot be silently discarded",state.project.chain.rsv[0]=1);
 BAD_NATIVE("invalid name",state.project.name[0]=31);
 BAD_NATIVE("bank count",state.arrangement.banks=5);
 BAD_NATIVE("scene count",state.arrangement.scenes=17);
 BAD_NATIVE("chain count",state.arrangement.rows=17);
 BAD_NATIVE("project reference",state.arrangement.bank[0].project[7]=4);
 BAD_NATIVE("track reference",state.arrangement.bank[0].track[7]=8);
 BAD_NATIVE("bank reference",state.arrangement.scene[15].bank=4);
 BAD_NATIVE("scene application flags",state.arrangement.scene[15].apply=8);
 BAD_NATIVE("scene level",state.arrangement.scene[15].level[7]=128);
 BAD_NATIVE("scene pan upper",state.arrangement.scene[15].pan[7]=64);
 BAD_NATIVE("scene pan lower",state.arrangement.scene[15].pan[7]=-65);
 BAD_NATIVE("scene transpose upper",state.arrangement.scene[15].transpose[7]=25);
 BAD_NATIVE("scene transpose lower",state.arrangement.scene[15].transpose[7]=-25);
 BAD_NATIVE("chain scene reference",state.arrangement.row[15].scene=16);
 BAD_NATIVE("chain zero repeat",state.arrangement.row[15].repeat=0);
 BAD_NATIVE("chain repeat upper",state.arrangement.row[15].repeat=17);
 BAD_NATIVE("engine schema",state.project.t[7].engine=14);
 BAD_NATIVE("step note",state.project.t[7].step[63].note[3]=128);
 BAD_NATIVE("step note count",state.project.t[7].step[63].n=5);
 BAD_NATIVE("step time",state.project.t[7].step[63].time=3);
 BAD_NATIVE("step reserved flags",state.project.t[7].step[63].flags=4);
 BAD_NATIVE("step velocity",state.project.t[7].step[63].vel=128);
 BAD_NATIVE("step orphan accent",state.project.t[7].step[63].hit=0;state.project.t[7].step[63].acc=1);
 BAD_NATIVE("step probability",state.project.t[7].step[63].probability=102);
 BAD_NATIVE("patch byte",state.project.fm6[7][127]=128);
 BAD_NATIVE("motion count",state.project.motion.count=65);
 BAD_NATIVE("motion value",state.project.motion.event[0].value=128);
 BAD_NATIVE("motion ID",state.project.motion.event[0].param=29);
 BAD_NATIVE("motion address",state.project.motion.event[0].place=512);
 BAD_NATIVE("motion duplicate across lock kinds",state.project.motion.event[1]=state.project.motion.event[0];state.project.motion.event[1].param^=128);
 for(unsigned t=0;t<8;t++){
  BAD_NATIVE("all tracks signed parameter lower",state.project.t[t].p[98]=-65);
  BAD_NATIVE("all tracks signed parameter upper",state.project.t[t].p[98]=128);
 }
 state=old;size_t written=777;
 check(!d8p1_project_encode((uint8_t *)&state,sizeof state,&written,&state)&&written==777&&!memcmp(&state,&old,sizeof state),"output alias preserves native source");
 check(!d8p1_project_encode(encoded,sizeof encoded,(size_t *)&state.project.magic,&state)&&!memcmp(&state,&old,sizeof state),"written count alias preserves native source");
 #undef BAD_NATIVE

 state=old;state.project.sel=7;state.project.t[7].p[7]=-32;
 step_t *last=&state.project.t[7].step[63];last->n=4;last->time=ST_NOTE;last->flags=3;last->vel=100;last->hit=255;last->acc=128;last->probability=42;
 const uint8_t notes[4]={60,64,67,72};memcpy(last->note,notes,4);step_set_ratchet(last,4);
 memset(&state.project.motion,0,sizeof state.project.motion);state.project.motion.count=2;state.project.motion.on=128;
 state.project.motion.event[0].place=511;state.project.motion.event[0].param=0x87;state.project.motion.event[0].value=-32;
 state.project.motion.event[1].place=255;state.project.motion.event[1].param=7;state.project.motion.event[1].value=127;
 state.project.sum=proj_sum(&state.project);written=0;
 check(d8p1_project_encode(encoded,sizeof encoded,&written,&state)&&d8p1_project_decode(&roundtrip,encoded,written,15)&&!memcmp(&state,&roundtrip,sizeof state),"track eight step64 full music and signed lock preserve nine-bit addresses without aliasing track4");
 state=old;
 size_t n=load(argv[2]);old=state;
 check(!d8p1_project_decode(&state,raw,n,3)&&!memcmp(&state,&old,sizeof state),"missing project slots refuse before state changes");
 n=load(argv[3]);check(!d8p1_project_decode(&state,raw,n,15)&&!memcmp(&state,&old,sizeof state),"unknown optional chunks cannot become staged native state");

 for(int f=4;f<argc;f++){
  n=load(argv[f]);d8p1_legacy_report report;size_t written=0;
  check(proj_import(&legacy_expected,raw,(int)n),"independent unchanged importer reads expected legacy chain");
  memcpy(before,raw,n);
  check(d8p1_legacy_convert(&state,&workspace,&report,raw,n,15),"real frozen legacy fixture converts with available references");
  check(!memcmp(before,raw,n),"legacy source remains byte-exact through migration");
  check(d8p1_project_encode(encoded,sizeof encoded,&written,&state)&&d8p1_project_decode(&roundtrip,encoded,written,15),"legacy state round-trips through the new file codec");
  check(!memcmp(&state,&roundtrip,sizeof state),"all native fields and new arrangement semantics survive round trip");
  check(state.project.parts==8&&state.project.phys==2&&state.project.chain.count==0,"legacy conversion normalizes metadata without using old disk layouts");
  for(unsigned t=4;t<8;t++)for(unsigned i=0;i<64;i++)check(!state.project.t[t].step[i].n&&!state.project.t[t].step[i].hit&&state.project.t[t].step[i].time==ST_REST,"upper legacy tracks initialize empty rests");
  check(state.arrangement.rows==legacy_expected.chain.count,"converted chain keeps original row count");
  unsigned expected_mask=0;
  for(unsigned i=0;i<legacy_expected.chain.count;i++)expected_mask|=1u<<legacy_expected.chain.row[i].slot;
  check(report.referenced_projects==expected_mask,"reference mask equals actual legacy row destinations");
  unsigned expected_scenes=0;
  for(unsigned slot=0;slot<4;slot++){
   if(expected_mask&(1u<<slot))check(report.scene_for_project[slot]==expected_scenes++,"used slot mapping is canonical ascending order");
   else check(report.scene_for_project[slot]==255,"unused legacy slots are not invented as scenes");
  }
  check(state.arrangement.banks==expected_scenes&&state.arrangement.scenes==expected_scenes,"only referenced slots create banks/scenes");
  for(unsigned i=0;i<state.arrangement.rows;i++){
   unsigned scene=state.arrangement.row[i].scene;const d8p1_scene_state *sc=&state.arrangement.scene[scene];
   unsigned slot=legacy_expected.chain.row[i].slot;
   check(state.arrangement.row[i].repeat==legacy_expected.chain.row[i].repeat,"repeat counts equal actual legacy rows");
   check(scene==report.scene_for_project[slot],"row order resolves each original project slot through explicit mapping");
   for(unsigned t=0;t<8;t++)check(state.arrangement.bank[sc->bank].project[t]==slot,"source project equals actual legacy slot, never the scene ordinal");
   check(sc->apply==0,"legacy chain retains pattern-only semantics with no scene mix/mute/transpose application");
   for(unsigned t=0;t<8;t++)check(state.arrangement.bank[sc->bank].track[t]==t,"legacy source track indices remain identities");
  }
  old=state;d8p1_legacy_report old_report=report;
  if(report.referenced_projects){
   unsigned missing=report.referenced_projects&(~report.referenced_projects+1u);
   check(!d8p1_legacy_convert(&state,&workspace,&report,raw,n,15u&~missing)&&!memcmp(&state,&old,sizeof state)&&!memcmp(&report,&old_report,sizeof report),"unavailable legacy project refuses without aliasing or changing published state/report");
  }
  check(!d8p1_legacy_convert(&state,&state,&report,raw,n,15)&&!memcmp(&state,&old,sizeof state),"legacy work buffer cannot alias published destination");
  check(!d8p1_legacy_convert(&state,&workspace,&report,raw+1,n-1,15)&&!memcmp(&state,&old,sizeof state),"unaligned/short legacy input cannot reach historical native casts");

  check(!d8p1_legacy_convert(&state,&workspace,(d8p1_legacy_report *)&state,raw,n,15)&&!memcmp(&state,&old,sizeof state),"report cannot overwrite published native destination");
  reference=workspace;
  check(!d8p1_legacy_convert(&state,&workspace,(d8p1_legacy_report *)&workspace,raw,n,15)&&!memcmp(&workspace,&reference,sizeof workspace)&&!memcmp(&state,&old,sizeof state),"report cannot alias staging workspace");
  check(!d8p1_legacy_convert(&state,&workspace,(d8p1_legacy_report *)raw,raw,n,15)&&!memcmp(raw,before,n)&&!memcmp(&state,&old,sizeof state),"report cannot overwrite immutable original file");
  raw[100]^=1;
  check(!d8p1_legacy_convert(&state,&workspace,&report,raw,n,15)&&!memcmp(&state,&old,sizeof state)&&!memcmp(&report,&old_report,sizeof report),"corrupted legacy input preserves published state and report");
  char canonical[1024];size_t path_n=strlen(argv[f]);if(path_n<4||path_n+32>=sizeof canonical)return 2;
  snprintf(canonical,sizeof canonical,"%.*s.expected-fun9.bin",(int)(path_n-4),argv[f]);n=load(canonical);
  check(d8p1_legacy_convert(&reference,&workspace,&report,raw,n,15)&&!memcmp(&reference,&old,sizeof old),"legacy semantics equal unchanged upstream canonical FUN9 reference");

 }
 printf("Native staged state: %zu bytes (host layout, not target fit)\n",sizeof state);
 printf("D8P1 project adapter: %u checks, %u failures; staged state only, no runtime/flash adoption\n",checks,errors);return errors?1:0;
}
