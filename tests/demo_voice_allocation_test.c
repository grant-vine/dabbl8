/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define UI_TEST_NO_MAIN 1
static void observe_kill(void *);
#define D8_VOICE_KILL_OBSERVER(v) observe_kill(v)
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include <assert.h>
static unsigned released,held_drum,held_synth,off_budget,killed_tracks;
static unsigned checked,peak,heard_tracks;
static int generated_demo;
static d8p1_project_state demo;
static d8p1_project_state scenario;
static uint8_t wire[D8P1_LIMIT];
static void observe_kill(void *pointer)
{
 voice_t *v=pointer;
 assert(v->active);off_budget+=voices_busy()<NVOICE;
 for(unsigned t=0;t<NTRK;t++)for(unsigned i=0;i<NVOICE;i++)if(v==&trk[t].v[i]){
  killed_tracks|=1u<<t;
  if(!v->gate)released++;
  else if(trk[t].engine==ENGI_DRUM)held_drum++;
  else held_synth++;
 }
}
static void render_blocks(unsigned blocks)
{
 int32_t audio[CTL*2];
 for(unsigned b=0;b<blocks;b++){
  mix_block(audio,CTL);unsigned active=0;
  for(unsigned t=0;t<NTRK;t++)for(unsigned i=0;i<NVOICE;i++)if(trk[t].v[i].active){
   active++;heard_tracks|=1u<<t;
  }
  if(active>peak)peak=active;
  assert(active<=NVOICE);checked++;
 }
}
static void run_case(const char *name,unsigned mode)
{
 scenario=demo;
 if(mode==1)scenario.project.t[3].p[P_MUTE]=1;
 if(mode==2)scenario.project.t[3].engine=0;
 size_t n=0;assert(d8p1_project_encode(wire,sizeof wire,&n,&scenario));
 assert(!d8p1_load_runtime(wire,n,7));
 released=held_drum=held_synth=off_budget=killed_tracks=checked=peak=heard_tracks=voice_kills=0;
 seq_start();
 for(unsigned bar=0;bar<38;bar++){
  render_blocks((FS*2u+CTL-1u)/CTL);
  if(generated_demo&&!mode)assert(voice_kills==2u*(bar+1u)&&released==voice_kills);
  printf("%s bar=%u kills=%u released=%u drum=%u synth=%u off_budget=%u peak=%u\n",name,bar+1,voice_kills,released,held_drum,held_synth,off_budget,peak);
 }
 assert(voice_kills==released+held_drum+held_synth&&!off_budget);
 if(generated_demo&&!mode)assert(voice_kills==76);
 if(!mode)assert(voice_kills>0&&!held_drum&&!held_synth&&killed_tracks==8&&peak==NVOICE&&heard_tracks==255);
 else assert(!voice_kills&&peak==(mode==1?7u:8u)&&heard_tracks==(mode==1?247u:255u));
 seq_stop();unsigned stopped=voice_kills;render_blocks((FS*10u+CTL-1u)/CTL);
 assert(voice_kills==stopped&&!voices_busy());
 printf("PASS %s blocks=%u kills=%u released=%u drum=%u synth=%u tracks=%02x victims=%02x\n",name,checked,voice_kills,released,held_drum,held_synth,heard_tracks,killed_tracks);
}
int main(int argc,char **argv)
{
 assert(argc==1||argc==2);generated_demo=argc==1;ui_power_on();
 int32_t audio[CTL*2];mix_block(audio,CTL);
 if(argc==2){
  FILE*f=fopen(argv[1],"rb");assert(f);size_t n=fread(wire,1,sizeof wire,f);
  assert(!ferror(f)&&fgetc(f)==EOF);assert(!fclose(f));
  assert(d8p1_project_decode(&demo,wire,n,7));
 }else{
  project_capture(&demo.project);demo.project.g[G_BPM]=120;
  const unsigned base[8]={36,43,48,55,60,64,67,72},transpose[4]={0,0,3,5};
  for(unsigned t=0;t<NTRK;t++){
   proj_trk_t *tr=&demo.project.t[t];
   tr->p[P_SLEN]=16;tr->p[P_SDIV]=2;tr->p[P_SSWING]=0;tr->p[P_LEVEL]=72;tr->p[P_MUTE]=0;tr->p[P_VOICE]=V_MONO;
   for(unsigned i=0;i<NSTEP;i++){
    step_t*s=&tr->step[i];memset(s,0,sizeof *s);s->time=ST_REST;
    if(i>=16)continue;
    if(tr->engine==ENGI_DRUM){s->hit=(uint8_t)(1u<<(i%4==0?(i%8==0?0:1):2));s->time=ST_NOTE;s->vel=48;}
    else if(i==0||i%4==t%4){s->note[0]=(uint8_t)(base[t]+(t%2?transpose[i/4]:0));s->n=1;s->time=ST_NOTE;s->vel=48;}
   }
  }
 }
 size_t n=0;assert(d8p1_project_encode(wire,sizeof wire,&n,&demo));
 assert(!d8p1_load_runtime(wire,n,7));
 for(unsigned t=0;t<NTRK;t++)assert(trk[t].p[P_SLEN]==16&&trk[t].p[P_VOICE]==V_MONO);
 assert(trk[3].engine==ENGI_DRUM&&trk_vmode(&trk[3])==V_POLY);
 run_case("demo",0);run_case("drums-muted",1);run_case("eight-mono-synths",2);
 return 0;
}
