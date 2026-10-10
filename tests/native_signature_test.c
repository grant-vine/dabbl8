/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual stopped native capture, canonical signature and LCD ownership. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
static unsigned late,irq_ons;
static void inject_start(void);
static void count_irq(void);
#define UI_LCD_SYNC_HOOK inject_start
#define UI_IRQ_ON_HOOK count_irq
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
static void inject_start(void){if(late){late=0;transport_req=1;}}
static void count_irq(void){irq_ons++;}
static unsigned checks,failures,mutations;
static uint8_t raw[D8P1_LIMIT],canonical[D8P1_LIMIT],arena_before[sizeof main_workspace];
static d8p1_project_state decoded;
static uint32_t baseline;
static __typeof__(motion) saved_motion;
#define CHECK(x) do{checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line %u: %s\n",__LINE__,#x);}}while(0)
static uint32_t signature(void){uint32_t v=0;CHECK(d8p1_signature_runtime(&v)==D8RT_OK);return v;}
static void changed(void){CHECK(signature()!=baseline);mutations++;}
static void byte_change(uint8_t *p){uint8_t old=*p;*p^=1;changed();*p=old;CHECK(signature()==baseline);}
static void word_change(int16_t *p){int16_t old=*p;*p=(int16_t)(old==127?126:old+1);changed();*p=old;CHECK(signature()==baseline);}
static uint32_t oracle(void)
{
 size_t n=0;CHECK(!d8p1_capture_runtime(raw,sizeof raw,&n));
 CHECK(d8p1_project_decode(&decoded,raw,n,15));
 decoded.project.sel=0;decoded.project.g[G_SLOT]=decoded.project.g[G_NAME]=decoded.project.g[G_LOAD]=decoded.project.g[G_SAVE]=0;
 CHECK(d8p1_project_encode(canonical,sizeof canonical,&n,&decoded));
 uint32_t h=2166136261u;for(size_t i=0;i<n;i++){h^=canonical[i];h*=16777619u;}return h;
}
static void refuses(int expected)
{
 uint32_t out=0xa5a5a5a5;CHECK(d8p1_signature_runtime(&out)==expected&&out==0xa5a5a5a5);
}
int main(void)
{
 ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);
 FILE *f=fopen("tests/fixtures/d8p1/maximum.d8p","rb");if(!f)return 2;
 size_t n=fread(raw,1,sizeof raw,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(!d8p1_load_runtime(raw,n,15));mix_block(audio,CTL);
 baseline=signature();CHECK(baseline==oracle());
 /* UI selection/project-page controls are deliberately outside dirty policy. */
 uint8_t sel=song.sel;for(unsigned i=0;i<8;i++){song.sel=(uint8_t)i;CHECK(signature()==baseline);}song.sel=sel;
 unsigned ignored[]={G_SLOT,G_NAME,G_LOAD,G_SAVE};
 for(unsigned i=0;i<4;i++){int16_t old=song.g[ignored[i]];song.g[ignored[i]]=3;CHECK(signature()==baseline);song.g[ignored[i]]=old;}
 for(unsigned i=0;i<G_COUNT;i++)if(i!=G_SLOT&&i!=G_NAME&&i!=G_LOAD&&i!=G_SAVE)word_change(&song.g[i]);
 char old_name=proj_name[0];proj_name[0]=old_name=='Z'?'Y':'Z';changed();proj_name[0]=old_name;CHECK(signature()==baseline);
 /* Every track parameter and stored note position, including track8/step64. */
 for(unsigned t=0;t<8;t++){
  for(unsigned p=0;p<P_COUNT;p++)word_change(&trk[t].p[p]);
  for(unsigned s=0;s<64;s++)for(unsigned k=0;k<4;k++)byte_change(&trk[t].step[s].note[k]);
  byte_change(&trk[t].preset);
  uint8_t eng=trk[t].eng_req;trk[t].eng_req=(uint8_t)((eng+1)%14);changed();trk[t].eng_req=eng;CHECK(signature()==baseline);
  uint8_t old=fm6_patch[t][0];fm6_patch[t][0]^=1;changed();fm6_patch[t][0]=old;CHECK(signature()==baseline);
 }
 /* Every active bank/scene field and chain row, not struct padding. */
 d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;CHECK(a->banks==4&&a->scenes==16&&a->rows==16);
 for(unsigned b=0;b<a->banks;b++){
  for(unsigned i=0;i<12;i++){char old=a->bank[b].name[i];if(i&&!a->bank[b].name[i-1])break;a->bank[b].name[i]=old=='Z'?'Y':'Z';changed();a->bank[b].name[i]=old;}
  for(unsigned t=0;t<8;t++){byte_change(&a->bank[b].project[t]);byte_change(&a->bank[b].track[t]);}
 }
 for(unsigned s=0;s<a->scenes;s++){
  d8p1_scene_state *v=&a->scene[s];
  for(unsigned i=0;i<12;i++){char old=v->name[i];if(i&&!v->name[i-1])break;v->name[i]=old=='Z'?'Y':'Z';changed();v->name[i]=old;}
  byte_change(&v->bank);byte_change(&v->apply);byte_change(&v->mute);
  for(unsigned t=0;t<8;t++){
   byte_change(&v->level[t]);byte_change((uint8_t *)&v->pan[t]);
   int8_t old=v->transpose[t];v->transpose[t]=(int8_t)(old==24?23:old+1);changed();v->transpose[t]=old;
  }
 }
 for(unsigned i=0;i<a->rows;i++){byte_change(&a->row[i].scene);uint8_t old=a->row[i].repeat;a->row[i].repeat=old==1?2:1;changed();a->row[i].repeat=old;}
 CHECK(signature()==baseline&&signature()==oracle());
 /* Active automation content changes; stale inactive arrangement/motion does not. */
 if(motion.count){byte_change(&motion.event[0].value);}
 saved_motion=motion;motion.count=0;uint32_t no_motion=signature();
 motion.event[0].place^=1;CHECK(signature()==no_motion);motion=saved_motion;
 /* Active modulation must hash its stored base, not transient sounding value. */
 uint32_t mask=motion_active[7][P_LEVEL>>5];int16_t old_base=motion_base[7][P_LEVEL],old_level=trk[7].p[P_LEVEL];uint8_t was_valid=motion_base_valid;
 motion_base_valid=1;motion_active[7][P_LEVEL>>5]|=1u<<(P_LEVEL&31);motion_base[7][P_LEVEL]=old_level;
 uint32_t base_signature=signature();trk[7].p[P_LEVEL]=old_level==127?126:old_level+1;CHECK(signature()==base_signature);
 motion_base[7][P_LEVEL]=trk[7].p[P_LEVEL];CHECK(signature()!=base_signature);
 trk[7].p[P_LEVEL]=old_level;motion_base[7][P_LEVEL]=old_base;motion_base_valid=was_valid;motion_active[7][P_LEVEL>>5]=mask;CHECK(signature()==baseline);
 d8p1_arrangement saved=*a;a->rows=a->scenes=a->banks=0;uint32_t empty=signature();CHECK(empty!=baseline);
 memset(a->bank,0xa5,sizeof a->bank);memset(a->scene,0xa5,sizeof a->scene);memset(a->row,0xa5,sizeof a->row);CHECK(signature()==empty);
 d8p1_runtime_cache.valid=0;CHECK(signature()==empty);*a=saved;d8p1_runtime_cache.valid=1;CHECK(signature()==baseline);
 /* Refusals preserve caller output and active canvas before borrowing. */
 CHECK(d8p1_signature_runtime(0)==D8RT_BAD);
 CHECK(d8p1_signature_runtime((uint32_t *)main_workspace.d8p1.wire)==D8RT_BAD);
 cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);memcpy(arena_before,&main_workspace,sizeof main_workspace);
 refuses(D8RT_BUSY);CHECK(!memcmp(arena_before,&main_workspace,sizeof main_workspace));cv_blit(0,20);
 unsigned consumed=dma_consumed;uint32_t px=pixels_hash(cv_px,CV_MAX);CHECK(signature()==baseline);CHECK(dma_consumed==consumed+1&&!dma.p&&!dma_errors&&pixels_hash(host_screen+20*240,CV_MAX)==px);
 song.playing=1;refuses(D8RT_BUSY);song.playing=0;transport_req=1;refuses(D8RT_BUSY);transport_req=0;
 cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);cv_blit(0,20);late=1;refuses(D8RT_BUSY);CHECK(transport_req==1);transport_req=0;
 chain_config.count=1;refuses(D8RT_BAD);chain_config.count=0;CHECK(signature()==baseline);
 printf("Native signature: %u checks, %u failures; %u musical mutations, coherent canonical oracle, UI exclusions and refusal output preservation; no scheduler/flash writes\n",checks,failures,mutations);
 return failures!=0;
}
