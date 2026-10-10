/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual eight-track runtime adoption, with asynchronous LCD ownership. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
static void late_start(void);
static void observed_irq_on(void);
#define UI_IRQ_ON_HOOK observed_irq_on
#define UI_LCD_SYNC_HOOK late_start
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
static unsigned checks,failures,inject,irq_ons;
static void observed_irq_on(void) { irq_ons++; }
static void late_start(void) { if(inject){inject=0;transport_req=1;} }
static void proof(int value,const char *name){checks++;if(!value){failures++;fprintf(stderr,"FAIL: %s\n",name);}}
static uint8_t raw[D8P1_LIMIT+1],original[D8P1_LIMIT];
static d8p1_project_state source;
static struct {
    __typeof__(trk) tracks;
    __typeof__(song) song_state;
    __typeof__(motion) motion_state;
    __typeof__(chain) chain_state;
    __typeof__(chain_config) chain_config_state;
    __typeof__(d8p1_runtime_cache) cache;
    __typeof__(fm6_patch) patches;
    __typeof__(fm6_pgen) patch_generations;
    __typeof__(ui) ui_state;
    __typeof__(undo) undo_state;
    uint32_t panic;
    uint8_t countin;
} before;
static void snapshot(void){
    memcpy(before.tracks,trk,sizeof trk);memcpy(&before.song_state,&song,sizeof song);
    memcpy(&before.motion_state,&motion,sizeof motion);memcpy(&before.chain_state,&chain,sizeof chain);
    memcpy(&before.chain_config_state,&chain_config,sizeof chain_config);memcpy(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache);
    memcpy(before.patches,fm6_patch,sizeof fm6_patch);memcpy(before.patch_generations,(const void *)fm6_pgen,sizeof fm6_pgen);
    memcpy(&before.ui_state,&ui,sizeof ui);memcpy(&before.undo_state,&undo,sizeof undo);before.panic=panic_req;before.countin=cin_left;
}
static int unchanged(void){return !memcmp(before.tracks,trk,sizeof trk)&&!memcmp(&before.song_state,&song,sizeof song)&&
 !memcmp(&before.motion_state,&motion,sizeof motion)&&!memcmp(&before.chain_state,&chain,sizeof chain)&&
 !memcmp(&before.chain_config_state,&chain_config,sizeof chain_config)&&!memcmp(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache)&&
 !memcmp(before.patches,fm6_patch,sizeof fm6_patch)&&!memcmp(before.patch_generations,(const void *)fm6_pgen,sizeof fm6_pgen)&&
 !memcmp(&before.ui_state,&ui,sizeof ui)&&!memcmp(&before.undo_state,&undo,sizeof undo)&&before.panic==panic_req&&before.countin==cin_left;}
static size_t load(const char *path){FILE *f=fopen(path,"rb");if(!f)exit(2);size_t n=fread(raw,1,sizeof raw,f);if(fgetc(f)!=EOF)exit(2);fclose(f);return n;}
static d8p1_project_state convert_workspace;
static project_t expected_capture,actual_capture;
static uint8_t capture_wire[D8P1_LIMIT],capture_before[D8P1_LIMIT];
static d8p1_project_state captured;
static uint8_t expected_patches[8][FP_SIZE+1];
static void policy_roundtrip(size_t n){
 source.project.sum=proj_sum(&source.project); /* same integrity header supplied by native decode */
 proof(project_restore_runtime(&source.project)==0,"upstream policy reference adopts structured project");
 project_capture(&expected_capture);memcpy(expected_patches,fm6_patch,sizeof expected_patches);
 trk[7].p[P_LEVEL]=3;trk[7].step[63].n=0;song.sel=0;motion.count=0;
 irq_ons=0;proof(d8p1_load_runtime(raw,n,15)==D8RT_OK,"native API restores actual source after deliberate runtime edits");
 proof(irq_ons==2,"native commit never enables interrupts during nested default sound adoption");
 project_capture(&actual_capture);
 proof(!memcmp(&actual_capture,&expected_capture,sizeof actual_capture)&&!memcmp(fm6_patch,expected_patches,sizeof expected_patches),"complete runtime music and patches match existing explicit fitting/migration policy");
 proof(d8p1_runtime_arrangement()&&!memcmp(d8p1_runtime_arrangement(),&source.arrangement,sizeof source.arrangement),"policy adoption preserves separate proposed arrangement");
 size_t captured_n=777;irq_ons=0;
 proof(d8p1_capture_runtime(capture_wire,sizeof capture_wire,&captured_n)==D8RT_OK&&irq_ons==2,"coherent stopped snapshot uses one complete capture guard");
 proof(d8p1_project_decode(&captured,capture_wire,captured_n,15)&&!memcmp(&captured.project,&actual_capture,sizeof actual_capture)&&!memcmp(&captured.arrangement,&source.arrangement,sizeof source.arrangement),"native capture round trip retains full live music and banks scenes rows");
}
int main(void){
 ui_power_on();int32_t audio[2u*CTL];mix_block(audio,CTL);
 for(unsigned t=0;t<8;t++){trk[t].p[P_VOICE]=V_MONO;trk_note_on(&trk[t],t==3?36:60+t,100);}
 proof(voices_busy()==8,"pre-load live notes occupy exactly the shared eight-voice budget");
 size_t n=load("tests/fixtures/d8p1/maximum.d8p");
 proof(d8p1_project_decode(&source,raw,n,15),"independent maximum fixture decodes");
 for(unsigned i=0;i<G_COUNT;i++)source.project.g[i]=GP[i].def;
 source.project.g[G_BPM]++;source.project.sel=7;
 for(unsigned t=0;t<8;t++){
  proj_trk_t *q=&source.project.t[t];q->engine=(uint8_t)(t==1?ENGI_FM6:t);q->preset=0;
  for(unsigned i=0;i<P_COUNT;i++)q->p[i]=param_desc_of(q->engine,i)->def;
  q->p[P_LEVEL]=20+t;q->p[P_PAN]=(int16_t)t*3-12;q->p[P_MUTE]=t&1;q->p[P_SLEN]=64;
  memcpy(source.project.fm6[t],FM6_INIT,128);memcpy(source.project.fm6[t]+118,"D8RUNTIME!",10);
 }
 memset(&source.project.motion,0,sizeof source.project.motion);source.project.motion.count=2;source.project.motion.on=255;
 source.project.motion.event[0].place=511;source.project.motion.event[0].param=P_LEVEL;source.project.motion.event[0].value=90;
 source.project.motion.event[1].place=255;source.project.motion.event[1].param=P_PAN|128;source.project.motion.event[1].value=-32;
 source.project.sum=proj_sum(&source.project);n=0;
 proof(d8p1_project_encode(raw,sizeof raw,&n,&source),"native eight-track source encodes");memcpy(original,raw,n);
 cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);uint32_t pixels=pixels_hash(cv_px,CV_MAX);cv_blit(0,20);unsigned consumed=dma_consumed;
 irq_ons=0;proof(d8p1_load_runtime(raw,n,15)==D8RT_OK,"full native project adopts while stopped");
 proof(irq_ons==2,"complete native music adoption uses one uninterrupted publish guard");
 proof(dma_consumed==consumed+1&&!dma.p&&!dma_errors&&pixels_hash(host_screen+20*240,CV_MAX)==pixels,"adoption consumes unchanged pending LCD pixels");
 proof(d8p1_runtime_arrangement()&&!memcmp(d8p1_runtime_arrangement(),&source.arrangement,sizeof source.arrangement),"all banks scenes rows retained outside canvas");
 proof(song.sel==7&&!song.playing&&!chain_config.count&&!chain_busy()&&transport_req==0&&panic_req==255,"selected track eight and stopped transport publish without old-chain playback");
 for(unsigned i=0;i<G_COUNT;i++)if(i!=G_SLOT&&i!=G_LOAD&&i!=G_SAVE)proof(song.g[i]==source.project.g[i],"song globals adopt actual policy");
 for(unsigned t=0;t<8;t++){
  proof(trk[t].eng_req==source.project.t[t].engine&&trk[t].preset==0,"all engine owners adopt independently");
  for(unsigned i=0;i<P_COUNT;i++)proof(trk[t].p[i]==(t==1&&i==P_E7?FM6_OWN:source.project.t[t].p[i]),"all parameters retain native meaning with explicit FM6 OWN adoption");
  proof(!memcmp(trk[t].step,source.project.t[t].step,sizeof trk[t].step),"all 64 full steps preserved per track");
  uint8_t patch[FP_SIZE+1];fm6_unpack(source.project.fm6[t],patch);fm6_sanitize(patch);proof(!memcmp(patch,fm6_patch[t],sizeof patch),"owned patch unpacking and sanitation match actual policy");
 }
 proof(!memcmp(&motion,&source.project.motion,sizeof motion)&&motion.event[0].place==511&&motion.event[1].place==255,"track eight and track four signed automation remain distinct");
 for(unsigned t=0;t<8;t++)trk[t].p[P_MUTE]=0; /* exercise even the source's muted engines */
 for(unsigned block=0;block<256;block++){mix_block(audio,CTL);proof(voices_busy()<=8,"post-load actual mixer preserves shared voice budget");}
 for(unsigned t=0;t<8;t++)proof(!trk[t].nheld&&trk[t].engine==trk[t].eng_req&&!trk[t].xf_on,"actual mixer retires old live ownership and adopts every requested engine");
 proof(!panic_req&&!song.playing&&!chain_busy(),"actual event loop consumes the load panic without starting arrangement playback");
 cv_begin(240,124,T_BG);cv_blit(0,0);project_capture(&proj_scratch);
 proof(d8p1_runtime_arrangement()&&!memcmp(d8p1_runtime_arrangement(),&source.arrangement,sizeof source.arrangement),"drawing and capture cannot erase live arrangement metadata");
 snapshot();for(size_t cut=0;cut<n;cut++)proof(d8p1_load_runtime(raw,cut,15)==D8RT_BAD&&unchanged(),"every truncation preserves full active runtime and cache");
 for(size_t at=0;at<n;at+=37){raw[at]^=1;proof(d8p1_load_runtime(raw,n,15)==D8RT_BAD&&unchanged(),"corrupt input preserves full active runtime and cache");raw[at]^=1;}
 for(unsigned mask=0;mask<15;mask++)proof(d8p1_load_runtime(raw,n,mask)==D8RT_BAD&&unchanged(),"missing referenced project refuses without adoption");
 proof(d8p1_load_runtime(raw,n,16)==D8RT_BAD&&unchanged(),"invalid context mask refuses");
 proof(d8p1_load_runtime(raw,sizeof raw,15)==D8RT_BAD&&unchanged(),"oversized payload refuses");
 proof(d8p1_load_runtime(0,n,15)==D8RT_BAD&&unchanged(),"null input refuses");
 proof(d8p1_load_runtime(main_workspace.d8p1.wire+1,n,15)==D8RT_BAD&&unchanged(),"overlapping arena input refuses");
 for(unsigned busy=0;busy<7;busy++){
  if(busy==0)song.playing=1;if(busy==1)chain.armed=1;if(busy==2)chain.running=1;
  if(busy>=3&&busy<=5)transport_req=(uint8_t)(busy-2);if(busy==6)cin_left=1;
  snapshot();proof(d8p1_load_runtime(raw,n,15)==D8RT_BUSY&&unchanged(),"playing armed running pending transport and count-in refuse without cancellation");
  song.playing=0;chain.armed=chain.running=transport_req=cin_left=0;
 }
 snapshot();inject=1;proof(d8p1_load_runtime(raw,n,15)==D8RT_BUSY&&transport_req==1&&unchanged(),"start arriving after initial check survives final atomic refusal");transport_req=0;
 cv_begin(8,8,T_BG);snapshot();proof(d8p1_load_runtime(raw,n,15)==D8RT_BUSY&&cv_cpu_active&&unchanged(),"active drawing refuses before touching arena");cv_blit(0,0);
 d8p1_stage_workspace *stage=main_d8p1_workspace();memcpy(stage->wire,raw,n);
 proof(d8p1_load_runtime(stage->wire,n,15)==D8RT_OK,"exact pre-borrowed bounded wire adopts without overlapping copy");
 snapshot();size_t unknown=load("tests/fixtures/d8p1/unknown-optional.d8p");proof(d8p1_load_runtime(raw,unknown,15)==D8RT_BAD&&unchanged(),"unknown optional content cannot be silently dropped");
 memcpy(raw,original,n);source.project.magic^=1;proof(project_restore_runtime(&source.project)==1&&unchanged(),"failed historical restore preserves native cache");
 FILE *f=fopen("tests/fixtures/projects/fun9.bin","rb");if(!f)return 2;if(fread(&proj_slot[0],1,sizeof proj_slot[0],f)!=sizeof proj_slot[0])return 2;fclose(f);
 project_load(0);proof(proj_cur==0&&!d8p1_runtime_arrangement(),"successful historical load invalidates native metadata");
 n=load("tests/fixtures/d8p1/maximum.d8p");proof(d8p1_project_decode(&source,raw,n,15),"full independent 7705-byte project stages for runtime policy");policy_roundtrip(n);
 const char *legacy[]={"fun1.bin","fun1-digital.bin","fun2.bin","fun3.bin","fun4.bin","fun4-phys-drum.bin","fun5.bin","fun6.bin","fun7.bin","fun8.bin","fun8-perc.bin","fun9.bin","fun9-digital.bin"};
 for(unsigned fidx=0;fidx<13;fidx++){
  char path[128];snprintf(path,sizeof path,"tests/fixtures/projects/%s",legacy[fidx]);size_t legacy_n=load(path);d8p1_legacy_report report;
  proof(d8p1_legacy_convert(&source,&convert_workspace,&report,raw,legacy_n,15),"all thirteen frozen legacy fixtures stage through actual converter");
  n=0;proof(d8p1_project_encode(raw,sizeof raw,&n,&source),"converted legacy state becomes known native bytes");policy_roundtrip(n);
 }
 for(unsigned migration=0;migration<2;migration++){
  n=load("tests/fixtures/d8p1/minimal.d8p");proof(d8p1_project_decode(&source,raw,n,0),"independent input for retired-engine policy");
  source.project.t[7].preset=0; /* isolate retained automation from the fixture's legacy power-on/reset sentinel */
  proj_trk_t *q=&source.project.t[2];q->engine=(uint8_t)(migration?ENGI_SAMPLE:ENGI_DIGITAL);q->preset=0;
  for(unsigned i=0;i<P_COUNT;i++)q->p[i]=param_desc_of(q->engine,i)->def;
  if(migration)q->p[P_E0]=4;
  memset(&source.project.motion,0,sizeof source.project.motion);source.project.motion.count=2;source.project.motion.on=255;
  source.project.motion.event[0].place=128;source.project.motion.event[0].param=P_E0;source.project.motion.event[0].value=4;
  source.project.motion.event[1].place=511;source.project.motion.event[1].param=P_PAN;source.project.motion.event[1].value=-24;
  source.project.g[G_BPM]=32767;q->p[P_LEVEL]=-64;
  n=0;proof(d8p1_project_encode(raw,sizeof raw,&n,&source),"structurally valid broad native values encode without silent wire normalization");policy_roundtrip(n);
  proof(trk[2].eng_req==(migration?ENGI_DRUM:ENGI_FM6)&&trk[2].p[P_LEVEL]==0&&song.g[G_BPM]==GP[G_BPM].max,"retired engines and out-of-domain values follow explicit upstream runtime policy");
  if(motion.count!=1||motion.event[0].place!=511||motion.event[0].value!=-24)fprintf(stderr,"policy %u motion count=%u place=%u value=%d\n",migration,motion.count,motion.event[0].place,motion.event[0].value);
  proof(motion.count==1&&motion.event[0].place==511&&motion.event[0].value==-24,"incompatible retired-engine automation drops while unrelated track-eight signed automation survives");
 }
 n=load("tests/fixtures/d8p1/minimal.d8p");proof(d8p1_project_decode(&source,raw,n,0),"input for nested SAMPLE power-on fallback");
 source.project.t[0].engine=ENGI_SAMPLE;source.project.t[0].preset=PROJ_DEF_SOUND;
 for(unsigned i=0;i<P_COUNT;i++)source.project.t[0].p[i]=param_desc_of(ENGI_SAMPLE,i)->def;
 n=0;proof(d8p1_project_encode(raw,sizeof raw,&n,&source),"legacy power-on sound sentinel remains representable");policy_roundtrip(n);
 proof(trk[0].eng_req==ENGI_DRUM,"nested SAMPLE preset-four fallback follows upstream DRUM policy");
 snapshot();memset(capture_wire,0xA5,sizeof capture_wire);memcpy(capture_before,capture_wire,sizeof capture_wire);size_t captured_n=777;
 proof(d8p1_capture_runtime(capture_wire,1,&captured_n)==D8RT_BAD&&captured_n==777&&!memcmp(capture_wire,capture_before,sizeof capture_wire)&&unchanged(),"short snapshot output preserves complete caller bytes count and runtime");
 proof(d8p1_capture_runtime(main_workspace.d8p1.wire+1,sizeof capture_wire,&captured_n)==D8RT_BAD&&captured_n==777&&unchanged(),"partial arena output alias refuses");
 proof(d8p1_capture_runtime(capture_wire,sizeof capture_wire,(size_t *)main_workspace.d8p1.wire)==D8RT_BAD&&unchanged(),"written-count arena alias refuses");
 for(unsigned busy=0;busy<7;busy++){
  if(busy==0)song.playing=1;if(busy==1)chain.armed=1;if(busy==2)chain.running=1;
  if(busy>=3&&busy<=5)transport_req=(uint8_t)(busy-2);if(busy==6)cin_left=1;
  snapshot();proof(d8p1_capture_runtime(capture_wire,sizeof capture_wire,&captured_n)==D8RT_BUSY&&captured_n==777&&!memcmp(capture_wire,capture_before,sizeof capture_wire)&&unchanged(),"busy snapshot preserves bytes and active state");
  song.playing=0;chain.armed=chain.running=transport_req=cin_left=0;
 }
 snapshot();inject=1;proof(d8p1_capture_runtime(capture_wire,sizeof capture_wire,&captured_n)==D8RT_BUSY&&transport_req==1&&captured_n==777&&!memcmp(capture_wire,capture_before,sizeof capture_wire)&&unchanged(),"late-start snapshot refusal preserves pending request and output");transport_req=0;
 chain_config.count=1;chain_config.row[0].slot=0;chain_config.row[0].repeat=1;snapshot();
 proof(d8p1_capture_runtime(capture_wire,sizeof capture_wire,&captured_n)==D8RT_BAD&&captured_n==777&&!memcmp(capture_wire,capture_before,sizeof capture_wire)&&unchanged(),"unconverted historical slot chain cannot be silently discarded by native capture");chain_config.count=0;
 printf("D8P1 runtime: %u checks, %u failures; stopped in-memory adoption only, no flash/playback/physical qualification\n",checks,failures);return !!failures;
}
