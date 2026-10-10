/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual signature + output producer/service + native automatic adapter. */
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include "native_output_queue_test.c"
static unsigned cases;
static uint32_t sig(void){uint32_t v=0;CHECK(d8p1_signature_runtime(&v)==D8RT_OK);return v;}
static void setup(void)
{
 cold();FILE *f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)exit(2);
 size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(!d8p1_load_runtime(wire,n,0));int32_t audio[CTL*2];mix_block(audio,CTL);
 for(unsigned t=0;t<8;t++)trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=trk[t].p[P_DIST]=0;
 CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));blank();flash_ok=1;
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();
 stream(1);drain();CHECK(df_automatic_quiet());proj_cur=2;
}
int main(void)
{
 for(queue_kind=0;queue_kind<7;queue_kind++){
  setup();uint32_t base=sig();CHECK(!reads&&!writes);
  int16_t level=trk[7].p[P_LEVEL];trk[7].p[P_LEVEL]=(int16_t)(level==127?126:level+1);
  d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;memset(a,0,sizeof *a);
  a->banks=1;memcpy(a->bank[0].name,"BANK",4);a->scenes=1;memcpy(a->scene[0].name,"SCENE",5);
  a->scene[0].apply=1;a->scene[0].level[7]=99;a->rows=1;a->row[0].repeat=2;d8p1_runtime_cache.valid=1;
  uint32_t dirty=sig();CHECK(dirty!=base&&!reads&&!writes);queue_disturb();oracle();
  CHECK(!df_automatic_quiet());
  char name[sizeof proj_name];memcpy(name,proj_name,sizeof name);
  uint32_t packet=uac.last,ring=uq.ring,history=uq.history;uint8_t pending=uq.packet,uncertain=uq.uncertain,h0=audio_nonzero[0],h1=audio_nonzero[1];
  /* Computing dirtiness never authorizes storage or drains queued audio. */
  CHECK(sig()==dirty&&!reads&&!writes&&proj_cur==2&&!memcmp(name,proj_name,sizeof name));
  CHECK(packet==uac.last&&ring==uq.ring&&history==uq.history&&pending==uq.packet&&uncertain==uq.uncertain&&h0==audio_nonzero[0]&&h1==audio_nonzero[1]);oracle();
  CHECK(d8p1_autosave_flash(0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
  CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes&&!irq_disabled);
  /* Only existing producers/service drain ordinary queues; sticky uncertainty
   * remains ineligible, even after all actual buffer oracles become zero. */
  drain();for(unsigned h=0;h<2;h++){qa_half=h;fm1_alnk0_irq();}drain();oracle();
  CHECK(sig()==dirty&&!reads&&!writes);
  if(queue_kind==4){CHECK(!df_automatic_quiet()&&d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes);cases++;continue;}
  CHECK(df_automatic_quiet());CHECK(!d8p1_autosave_flash(1)&&writes&&proj_cur==2&&!memcmp(name,proj_name,sizeof name)&&!irq_disabled);
  CHECK(sig()==dirty);trk[7].p[P_LEVEL]=level;a->scene[0].level[7]=1;CHECK(sig()!=dirty);
  CHECK(!d8p1_restore_flash_autosave(1,1)&&proj_cur==PROJ_NO_SLOT);int32_t audio[CTL*2];mix_block(audio,CTL);
  CHECK(sig()==dirty&&d8p1_runtime_cache.arrangement.scene[0].level[7]==99&&d8p1_runtime_cache.arrangement.row[0].repeat==2);cases++;
 }
 printf("Native autosave integration: %u checks, %u failures; %u queue states with canonical track8/arrangement edits, natural drain/save/restore and sticky uncertainty; simulated storage, no scheduler or physical qualification\n",checks,failures,cases);
 return failures!=0;
}
