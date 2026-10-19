/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include <assert.h>

static uint8_t wire[D8P1_LIMIT], before[D8P1_LIMIT], after[D8P1_LIMIT];
static unsigned peak, heavy_peak, blocks;
static const unsigned notes[8]={36,38,39,42,46,45,37,56};

static void render(unsigned count)
{
 int32_t out[CTL*2];
 for(unsigned b=0;b<count;b++){
  mix_block(out,CTL);unsigned active=voices_busy(),owned=0;
  assert(active<=NVOICE);
  for(unsigned i=0;i<NVOICE;i++)if(heavy_owner[i]){
   unsigned k=heavy_owner[i]-1u;
   owned+=heavy_live(7u+k/(NPART*NVOICE),(k/NVOICE)%NPART,k%NVOICE);
  }
  if(active>peak)peak=active;
  if(owned>heavy_peak)heavy_peak=owned;
  assert(owned<=NVOICE);blocks++;
 }
}
static void note(unsigned channel,unsigned key,unsigned velocity)
{
 unsigned status=(velocity?0x90u:0x80u)|channel;
 midi_in_event((status>>4)|(status<<8)|(key<<16)|(velocity<<24));
}
int main(int argc,char **argv)
{
 assert(argc==2||argc==3);ui_power_on();render(1);
 FILE *file=fopen(argv[1],"rb");assert(file);
 size_t size=fread(wire,1,sizeof wire,file);
 assert(!ferror(file)&&fgetc(file)==EOF);assert(!fclose(file));
 assert(!d8p1_load_runtime(wire,size,7));render(1);
 assert(!song.playing&&!song.rec&&!song.g[G_ROUTE]);
 assert(trk[3].engine==ENGI_DRUM&&trk_vmode(&trk[3])==V_POLY);
 unsigned lanes=0;
 for(unsigned i=0;i<8;i++)lanes|=1u<<drum_lane(notes[i]);
 assert(lanes==255);
 size_t n_before=0,n_after=0;
 assert(!d8p1_capture_runtime(before,sizeof before,&n_before));
 peak=heavy_peak=blocks=0;
 for(unsigned burst=0;burst<16;burst++){
  for(unsigned i=0;i<8;i++)note(3,notes[i],32);
  render((FS/8u+CTL-1u)/CTL);
  for(unsigned i=0;i<8;i++){
   assert(midi_sel_on[3][notes[i]]==4);
   note(3,notes[i],0);
  }
  render((FS*3u/8u+CTL-1u)/CTL);
  for(unsigned i=0;i<8;i++)assert(!midi_sel_on[3][notes[i]]);
 }
 render((FS*10u+CTL-1u)/CTL);
 assert(peak==8&&heavy_peak==1&&!voices_busy());
 assert(!song.playing&&!song.rec&&mi_r==mi_w);
 for(unsigned channel=0;channel<16;channel++)for(unsigned key=0;key<128;key++)
  assert(!midi_sel_on[channel][key]);
 assert(!d8p1_capture_runtime(after,sizeof after,&n_after));
 assert(n_after==n_before&&!memcmp(before,after,n_before));
 if(argc==3){
  FILE *contract=fopen(argv[2],"w");assert(contract);
  fprintf(contract,"{\"channel\":3,\"notes\":[36,38,39,42,46,45,37,56],\"velocity\":32,\"bursts\":16,\"on_seconds\":0.125,\"interval_seconds\":0.5,\"drain_seconds\":10,\"track_dump\":[3,%u,%u",trk[3].engine,trk[3].preset);
  for(unsigned i=0;i<P_COUNT;i++){
   unsigned value=(unsigned)(trk[3].p[i]+8192);
   fprintf(contract,",%u,%u",value&127u,(value>>7)&127u);
  }
  assert(fputs("]}\n",contract)>=0);assert(!fclose(contract));
 }
 printf("PASS USB channel4 eight-lane DRUM bursts=16 note_ons=128 velocity=32 peak_voices=%u heavy_slots=%u blocks=%u no_holds=1 project_unchanged=1\n",peak,heavy_peak,blocks);
 return 0;
}
