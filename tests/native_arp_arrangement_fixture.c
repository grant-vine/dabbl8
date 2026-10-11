/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual sequencer, clock and motion with explicitly seeded virtual NOR. */
#define D8POOL_RUNTIME_NO_MAIN 1
static unsigned period_queries;
#define D8ARR_PERIOD_TEST_HOOK() (++period_queries)
static void held_build_hook(void);
#define D8ARR_HELD_BUILD_TEST_HOOK() held_build_hook()
#include "d8p1_pool_runtime_test.c"
#include "../firmware/src/d8arr_prepare.c"
static unsigned build_inject;
static void held_build_hook(void)
{
 if(build_inject){build_inject=0;midi_note_event(0,111,100);midi_note_event(0,111,0);}
}
static d8p1_project_state source;
static int32_t audio[2*CTL];
static unsigned inject_at,inject_kind,inject_reads;
static int arr_read(void *c,uint32_t off,void *p,uint32_t n)
{
 int rc=rd(c,off,p,n);
 if(inject_at&&chain.native.policy.preparing&&++inject_reads==inject_at){
  if(inject_kind==1)transport_req=1;
  if(inject_kind==2)seq_start();
  if(inject_kind==3)d8_capture_store_changed();
 }
 return rc;
}
static d8pool arr_pool={NULL,arr_read,er,pg,stop};
static int cat(void*c,d8p1_project_catalog*out){(void)c;return d8p1_catalog_pool(&arr_pool,out);}
static int save_cb(void*c,unsigned o,const char*n){(void)c;return d8p1_save_as_pool(&arr_pool,o,n);}
static int load_cb(void*c,unsigned o){(void)c;return d8p1_load_pool(&arr_pool,o);}
static int rename_cb(void*c,unsigned o,const char*n){(void)c;return d8p1_rename_pool(&arr_pool,o,n);}
static int prep_cb(void*c){(void)c;return d8arr_prepare_pool(&arr_pool);}
static project_native_ops ops={NULL,cat,save_cb,load_cb,rename_cb,prep_cb};
static void arrange(void)
{
 d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;memset(a,0,sizeof *a);a->banks=3;a->scenes=3;a->rows=3;
 for(unsigned b=0;b<3;b++){
  memcpy(a->scene[b].name,"SCENE",5);a->scene[b].name[5]=(char)('1'+b);a->scene[b].bank=(uint8_t)b;
  for(unsigned t=0;t<8;t++){a->bank[b].project[t]=(uint8_t)b;a->bank[b].track[t]=(uint8_t)(7-t);}
  a->row[b].scene=(uint8_t)b;a->row[b].repeat=(uint8_t)(b==0?2:1);
 }
 /* Aliased source tracks have separate destination bases and locks. */
 a->bank[0].track[1]=a->bank[0].track[0];d8p1_runtime_cache.valid=1;
}
static void seed(void)
{
 ui_power_on();mix_block(audio,CTL);blank();size_t n=fixture("tests/fixtures/d8p1/minimal.d8p");
 proof(!d8p1_load_runtime(wire,n,0),"initial native live load");mix_block(audio,CTL);
 proof(d8p1_project_decode(&source,wire,n,0),"actual native source decode");
 for(unsigned o=0;o<3;o++){
  source.project.motion.count=3;source.project.motion.on=128;
  source.project.motion.event[0]=(motion_event_t){7u<<6,P_PAN,10};
  source.project.motion.event[1]=(motion_event_t){(7u<<6)|1,P_PAN|MOTION_LOCK,30};
  source.project.motion.event[2]=(motion_event_t){7u<<6,P_FM1_ATK,41};
  for(unsigned t=0;t<8;t++){
   proj_trk_t *q=&source.project.t[t];q->p[P_SLEN]=3;q->p[P_SDIV]=2;
   for(unsigned k=0;k<NSTEP;k++){q->step[k].time=ST_NOTE;q->step[k].n=1;q->step[k].note[0]=(uint8_t)(40+o*8+t);q->step[k].vel=99;}
  }
  proof(d8p1_project_encode(wire,sizeof wire,&n,&source),"encode distinct eight-track source");
  proof(!d8pool_save(&pool,o,wire,n,7),"seed actual native pool source");
 }
 reset();arrange();proof(!project_native_bind(&ops,1),"explicit test-only trusted native binding");
 writes=0;inject_at=inject_reads=0;
}
static int live_same(void)
{return !memcmp(before.tracks,trk,sizeof trk)&&!memcmp(&before.song_state,&song,sizeof song)&&
 !memcmp(&before.motion_state,&motion,sizeof motion)&&!memcmp(before.patches,fm6_patch,sizeof fm6_patch)&&
 !memcmp(&before.undo_state,&undo,sizeof undo)&&before.slot==proj_cur;}
static void prepare_start(void);
static void held_oracle(void)
{
 for(unsigned t=0;t<8;t++)for(unsigned n=0;n<128;n++){
  unsigned bit=(chain.native.policy.held[t][n/32]>>(n%32))&1u;
  proof(bit==(unsigned)(midi_note_held(&trk[t],n)||midi_local_held(&trk[t],n)),"held cache equals independent original ownership predicates");
 }
}
static void cache_cases(void)
{
 arrange();reset();prepare_start();song.playing=0;usb.config=0;
 for(unsigned t=0;t<8;t++){trk[t].p[P_MUTE]=0;trk[t].p[P_AMODE]=0;trk[t].p[P_CHRD]=CH_MAJ;}
 for(unsigned i=0;i<200;i++){
  unsigned ch=i%16,n=36+(i*17)%72;
  song.g[G_ROUTE]=(int16_t)(i%2);song.sel=(uint8_t)((i*5)%8);
  if(i%7==0)midi_control(ch,64,127);
  midi_note_event(ch,n,100);held_oracle();
  if(i%3==0)midi_note_event(ch,n,100); /* repeated/chord source remap */
  midi_note_event(ch,n,0);held_oracle();
  if(i%7==0){midi_control(ch,64,0);held_oracle();}
  if(i%5==0){
   unsigned k=i%27;track_t *t=&trk[(i*3)%8];
   if(kb_chn[k])key_off(k,&trk[kb_trk[k]]);
   kb_trk[k]=(uint8_t)trk_index(t);kb_note[k]=(uint8_t)n;key_on(k,t);held_oracle();
   key_off(k,t);held_oracle();
  }
  if(i%11==0){midi_control(ch,123,0);held_oracle();}
  if(i%13==0){midi_control(ch,120,0);held_oracle();}
 }
 midi_in_overflow=1;events_block(1);held_oracle();
 for(unsigned t=0;t<8;t++){midi_forget_track(t);held_oracle();}
 seq_stop();song.g[G_ROUTE]=0;
}
static void prepare_start(void)
{proof(!chain_prepare()&&chain.armed&&chain.native.policy.valid&&!writes,"actual frontend read-only preparation arms");transport_req=0;seq_start();proof(d8arr_running()&&song.playing&&chain.row==0&&chain.remaining==d8p1_runtime_cache.arrangement.row[0].repeat,"actual sequencer starts native row zero");}
static void boundary(uint32_t carry)
{uint32_t p=div_samples(2);clk_step=15;clk_pos=p-4;events_block(4+carry);}
static void legacy_boundaries(void)
{
 reset();memset(&chain,0,sizeof chain);song.g[G_CLOCK]=0;transport_req=0;
 uint8_t timing[sizeof chain.timing];
 for(unsigned t=0;t<8;t++)memcpy(chain.timing[t],&trk[t].p[P_SLEN],sizeof chain.timing[t]);
 memcpy(timing,chain.timing,sizeof timing);
 chain.config.count=2;chain.config.row[0].slot=0;chain.config.row[0].repeat=2;
 chain.config.row[1].slot=1;chain.config.row[1].repeat=1;
 for(unsigned o=0;o<2;o++)for(unsigned t=0;t<8;t++){
  memcpy(chain.source[o].timing[t],&trk[t].p[P_SLEN],sizeof chain.source[o].timing[t]);
  chain.source[o].timing[t][0]=(int16_t)(o?5:3);chain.source[o].timing[t][1]=2;
  chain.source[o].step[t][0]=(step_t){.time=ST_NOTE,.n=1,.note={60+o+t},.vel=99};
 }
 chain.armed=1;seq_start();proof(chain.running&&!chain.native_mode&&chain.row==0&&chain.remaining==2,"actual historical Start retains first row/repeat");
 track_t *t=&trk[0];t->seq_idx=2;
 uint32_t len=step_samples(t,div_samples((uint32_t)t->p[P_SDIV]),t->seq_idx);
 t->seq_pos=len-5;chain_tick(4);proof(chain.row==0&&chain.remaining==2,"historical pre-boundary fragment cannot advance");
 chain_tick(8);proof(chain.row==0&&chain.remaining==1,"historical first repeat keeps source/row");
 chain_tick(8);proof(chain.row==1&&chain.slot==1&&chain.carry==3&&chain.remaining==1,"actual historical row boundary preserves carry and source");
 for(unsigned i=0;i<8;i++)proof(trk[i].p[P_SLEN]==5&&seq_steps(&trk[i])[0].note[0]==61+i,"historical row publishes all eight source tracks/timing");
 seq_tick(t,0);proof(t->seq_idx==0&&t->seq_pos==3,"actual sequencer consumes historical transition carry");
 t->seq_idx=4;len=step_samples(t,div_samples((uint32_t)t->p[P_SDIV]),t->seq_idx);t->seq_pos=len-1;chain_tick(4);
 proof(!song.playing&&!chain.running&&!chain.armed,"actual historical final boundary stops");
 for(unsigned i=0;i<8;i++)proof(!memcmp(&trk[i].p[P_SLEN],timing+i*sizeof chain.timing[i],sizeof chain.timing[i]),"historical Stop restores editable timing");
}
static void period_cases(void)
{
 unsigned n=period_queries;events_block(1);
 proof(period_queries==n,"inactive editable playback does not request native period");
 chain.armed=1;seq_start();events_block(1);
 proof(chain.running&&!chain.native_mode&&period_queries==n,"actual historical playback does not request native period");seq_stop();
 project_native.ops.prepare_arrangement=prep_cb;arrange();reset();prepare_start();n=period_queries;events_block(1);
 proof(period_queries==n+1,"actual native playback requests one master-bar period per fragment");
 seq_stop();n=period_queries;events_block(1);
 proof(period_queries==n,"suspended native playback does not request period");
 chain.native_mode=2;chain.native.policy.valid=0;events_block(1);
 proof(period_queries==n&&!song.playing&&!chain.running,"invalid-resume native tombstone executes neither timing path");
}
static uint8_t overlay_wire[D8P1_LIMIT],overlay_after[D8P1_LIMIT];
static int nonzero(const int32_t *v,unsigned n){for(unsigned i=0;i<n;i++)if(v[i])return 1;return 0;}
static void overlay_setup(unsigned apply)
{
 seed();d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;
 for(unsigned s=0;s<a->scenes;s++){
  a->scene[s].apply=(uint8_t)apply;a->scene[s].mute=(uint8_t)(s?0x80:1);
  for(unsigned t=0;t<8;t++){a->scene[s].level[t]=(uint8_t)(20+s*20+t);a->scene[s].pan[t]=(int8_t)(-64+s*32+t);a->scene[s].transpose[t]=(int8_t)(s? -12:12);}
 }
}
static void overlay_cases(void)
{
 /* Exact codec/live capture, every apply combination, source alias and return. */
 for(unsigned apply=0;apply<8;apply++){
  overlay_setup(apply);size_t n=0,m=0;
  proof(!d8p1_capture_runtime(overlay_wire,sizeof overlay_wire,&n),"encode live scene overlays through actual native capture");
  proof(!d8p1_load_runtime(overlay_wire,n,7),"scene overlay wire adopts through actual checked runtime");
  uint32_t original=d8p1_crc32(nor,sizeof nor);prepare_start();
  uint32_t cached=d8p1_crc32(chain.native.manual,sizeof chain.native.manual);
  proof(d8arr_mix.apply==apply&&d8arr_mix.mute==1,"selected scene publishes actual effective audio layer");
  for(unsigned t=0;t<8;t++)proof(d8arr_mix.level[t]==20+t&&d8arr_mix.pan[t]==-64+(int)t,"all eight scene mix destinations cached independently");
  boundary(1);boundary(2);proof(chain.row==1&&d8arr_mix.mute==0x80&&d8arr_mix.level[7]==47,"row transition selects next effective overlay");
  proof(d8p1_crc32(chain.native.manual,sizeof chain.native.manual)==cached,"overlay/automation never alters immutable source cache");
  seq_stop();transport_req=0;
  proof(!d8arr_mix.apply,"Stop disables transient audio overlay");
  proof(!d8p1_capture_runtime(overlay_after,sizeof overlay_after,&m)&&m==n&&!memcmp(overlay_after,overlay_wire,n),"Stop preserves exact encoded editable project and scene state");
  proof(d8p1_crc32(nor,sizeof nor)==original&&!writes,"overlay playback preserves exact all saved source bytes");
 }
 /* Unsupported bits and all signed/unsigned bounds refuse before publication. */
 for(unsigned kind=0;kind<6;kind++){
  overlay_setup(7);d8p1_scene_state *s=&d8p1_runtime_cache.arrangement.scene[1];
  if(kind==0)s->apply=8;if(kind==1)s->level[7]=128;if(kind==2)s->pan[7]=-65;
  if(kind==3)s->pan[7]=64;if(kind==4)s->transpose[7]=-25;if(kind==5)s->transpose[7]=25;
  snapshot();int rc=d8arr_prepare_pool(&arr_pool);
  proof(rc==(kind?D8POOL_INVALID:D8POOL_UNSUPPORTED)&&unchanged(),"invalid overlay bit/range refuses without state publication");
 }
 /* Pitched sequence-only transpose, clamping, dedup and exact released pitch. */
 overlay_setup(7);prepare_start();track_t *t=&trk[0];t->eng_req=t->engine=0;t->p[P_MUTE]=0;t->p[P_QUANT]=0;
 step_t s={.time=ST_NOTE,.n=2,.note={120,126},.vel=99};seq_step(t,&s,1000,0);
 proof(t->seq_n==1&&t->seq_notes[0]==127,"scene transpose clamps and deduplicates sounding pitches");
 s.n=1;s.note[0]=48;s.hit=1;seq_step(t,&s,1000,0);
 proof(t->seq_n==2&&t->seq_notes[0]==60&&t->seq_notes[1]==DRUM_LANE_NOTE[0],"pitched transpose leaves drum lane hits unchanged");
 t->eng_req=ENGI_DRUM;seq_step(t,&s,1000,0);proof(t->seq_notes[0]==48,"mapped drum kit keys are not transposed");
#if FELUCCA_SLICE
 t->eng_req=13;seq_step(t,&s,1000,0);proof(t->seq_notes[0]==48,"mapped slice keys are not transposed");
#endif
 t->eng_req=t->engine=0;t->p[P_AMODE]=0;t->p[P_CHRD]=CH_OFF;song.g[G_ROUTE]=1;song.sel=0;
 midi_note_event(0,60,100);midi_control(0,64,127);midi_note_event(0,60,0);
 s.hit=0;s.note[0]=48;seq_step(t,&s,1000,0);proof(t->seq_notes[0]==60&&midi_note_held(t,60),"transposed sequence collides with actual sustained input pitch");
 seq_release(t);proof(midi_note_held(t,60),"sequence release retains sustained exact sounding pitch");
 /* The SLIDE removal path must use the same ownership cache. */
 s.flags=SF_SLIDE;seq_step(t,&s,1000,0);s.note[0]=50;seq_step(t,&s,1000,0);
 proof(midi_note_held(t,60),"transposed slide removal retains actual input ownership");
 midi_control(0,64,0);proof(!midi_note_held(t,60),"final pedal release retires retained transposed collision");
 seq_stop();song.g[G_ROUTE]=0;
 /* Actual mixer endpoints and performance ramp: no p/motion-base replacement. */
 overlay_setup(D8ARR_APPLY_MIX);prepare_start();t=&trk[0];t->p[P_MUTE]=0;t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
 t->eng_req=0;mix_block(audio,CTL);trk_all_off(t);trk_note_on(t,60,100);
 int16_t level=t->p[P_LEVEL],pan=t->p[P_PAN];
 chain.native.scene[0].level[0]=0;d8arr_scene_publish();memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);mix_part(t,CTL);
 proof(!nonzero(mix_l,CTL)&&!nonzero(mix_r,CTL),"effective zero scene level silences actual dry mixer");
 chain.native.scene[0].level[0]=127;chain.native.scene[0].pan[0]=-64;d8arr_scene_publish();memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);mix_part(t,CTL);
 proof(nonzero(mix_l,CTL)&&!nonzero(mix_r,CTL),"scene hard-left pan affects actual dry mixer");
 proof(t->p[P_LEVEL]==level&&t->p[P_PAN]==pan,"effective mix never overwrites underlying automation parameters");
 go_page(GR_TRK);song.sel=0;edit_param(0,1);int16_t edited=t->p[P_LEVEL];seq_stop();proof(t->p[P_LEVEL]==edited,"actual mixer edit under overlay survives Stop");
 overlay_setup(D8ARR_APPLY_MUTE);prepare_start();t=&trk[0];t->p[P_MUTE]=0;t->p[P_AMODE]=0;song.g[G_ROUTE]=1;song.sel=0;
 midi_note_event(0,60,100);memset(&pf,0,sizeof pf);for(unsigned k=0;k<8;k++)pf.mg[k]=32768;
 for(unsigned b=0;b<64;b++){perf_begin(CTL);memset(mix_l,0,sizeof mix_l);memset(mix_r,0,sizeof mix_r);mix_part(t,CTL);}
 proof(pf.mg[0]==0&&!nonzero(mix_l,CTL)&&!nonzero(mix_r,CTL)&&midi_note_held(t,60),"scene mute uses actual ramp while held input ownership survives");
 perf_solo=2;perf_begin(CTL);proof((pf.act&PF_MUTE)&(1u<<(PF_M1+0)),"scene mute composes with actual solo performance layer");perf_solo=0;
 seq_stop();midi_note_event(0,60,0);proof(!midi_note_held(t,60)&&!d8arr_mix.apply,"Stop and final input release clear overlay without hung ownership");song.g[G_ROUTE]=0;
}
static void fill_cases(void)
{
 overlay_setup(7);prepare_start();unsigned original=chain.row;
 proof(!d8arr_request_fill(2)&&!d8arr_request_fill(1)&&chain.native.policy.fill_pending==1,"latest valid pending fill wins");
 proof(d8arr_request_fill(16)==2&&chain.native.policy.fill_pending==1,"invalid fill preserves prior pending request");
 events_block(1);proof(chain.row==original&&!chain.native.policy.fill_active,"fill waits for actual master bar");boundary(3);
 proof(chain.row==1&&chain.native.policy.fill_active&&chain.native.policy.fill_return==1&&chain.native.policy.fill_remaining==1,"fill replaces A second bar and computes next B context");
 boundary(2);proof(chain.row==1&&!chain.native.policy.fill_active&&chain.remaining==1,"single fill completes exactly once then returns B");
 seq_stop();overlay_setup(7);prepare_start();boundary(0);proof(chain.row==0&&chain.remaining==1,"first A repeat completed before next-bar replacement");
 proof(!d8arr_request_fill(2),"queue fill replacing one-repeat B");boundary(1);
 proof(chain.native.policy.fill_active&&chain.native.policy.fill_return==2,"one-repeat B can be wholly replaced");boundary(1);proof(chain.row==2&&!chain.native.policy.fill_active,"replacement consumes B and returns C");seq_stop();
 /* Mid-fill repeated USB transport Stop/Continue retains exact phase/context. */
 overlay_setup(7);prepare_start();proof(!d8arr_request_fill(2),"arm active-fill transport case");boundary(0);events_block(17);
 for(unsigned i=0;i<3;i++){
  unsigned phase=trk[0].seq_idx,masterstep=clk_step;uint32_t pos=trk[0].seq_pos,masterpos=clk_pos;unsigned ret=chain.native.policy.fill_return,remaining=chain.native.policy.fill_remaining;
  proof(!d8arr_request_fill(0),"queue pending fill before Stop");midi_clock_transport(0xFC,fm1_ms);
  proof(!song.playing&&chain.native.policy.fill_active&&chain.native.policy.fill_pending==D8ARR_NONE&&!d8arr_mix.apply,"Stop cancels pending but preserves active fill context");
  midi_clock_transport(0xFB,fm1_ms+1);
  proof(d8arr_running()&&chain.native.policy.fill_active&&trk[0].seq_idx==phase&&trk[0].seq_pos==pos&&clk_step==masterstep&&clk_pos==masterpos&&chain.native.policy.fill_return==ret&&chain.native.policy.fill_remaining==remaining&&d8arr_mix.apply==7,"Continue resumes exact active-fill phase and return route");events_block(5);
 }
 boundary(1);proof(!chain.native.policy.fill_active&&chain.row==1&&chain.remaining==1,"resumed fill returns once to unchanged precomputed progression");seq_stop();
 /* Chained fills replace successive normal bars, never append after end. */
 overlay_setup(0);prepare_start();d8arr_request_fill(2);boundary(0);d8arr_request_fill(0);boundary(0);
 proof(chain.native.policy.fill_active&&chain.row==0&&chain.native.policy.fill_return==2,"fill requested during fill replaces next upcoming normal bar");boundary(0);proof(chain.row==2&&!chain.native.policy.fill_active,"chained fills retain bounded normal progression");
 d8arr_request_fill(0);boundary(0);proof(!song.playing&&!chain.native.policy.fill_active,"already-end progression stops rather than append unscheduled fill");
 overlay_setup(0);prepare_start();boundary(0);d8arr_request_fill(1);boundary(0);boundary(0); /* C now current */
 d8arr_request_fill(1);boundary(0);proof(!song.playing,"final row cannot append a fill");
 /* Fill occupying last scheduled bar completes before end-stop cleanup. */
 overlay_setup(0);prepare_start();boundary(0);boundary(0); /* B current */
 d8arr_request_fill(0);boundary(0);proof(chain.native.policy.fill_active&&chain.native.policy.fill_return==D8ARR_NONE,"fill may replace final scheduled C bar");boundary(0);proof(!song.playing&&!chain.running&&!chain.native.policy.fill_active,"last replacement ends with real sequencer Stop");
 /* Manual route priority and active cancellation are quantized. */
 overlay_setup(0);prepare_start();d8arr_request_fill(2);d8arr_request_row(1);proof(chain.native.policy.fill_pending==D8ARR_NONE&&d8arr_request_fill(0)==2,"normal row queue cancels and dominates pending fill");boundary(0);proof(chain.row==1&&!chain.native.policy.fill_active,"manual row wins at actual bar");seq_stop();
 overlay_setup(0);prepare_start();d8arr_request_fill(2);boundary(0);d8arr_cancel_fill();proof(chain.native.policy.fill_active,"active cancel retains phase until boundary");boundary(0);proof(chain.row==1&&!chain.native.policy.fill_active,"cancel returns bounded normal route at boundary");seq_stop();
 /* Observed source change while suspended invalidates the entire return route. */
 overlay_setup(0);prepare_start();d8arr_request_fill(2);boundary(0);seq_stop();d8_capture_store_changed();midi_clock_transport(0xFB,fm1_ms);
 proof(!song.playing&&chain.native_mode==2,"source mutation refuses stale active-fill Continue");seq_start();proof(!chain.native_mode&&!chain.running&&song.playing,"fresh Start after stale fill uses editable project");seq_stop();
 /* Successful load and uncertain save revoke a suspended fill, not its originals. */
 overlay_setup(7);prepare_start();d8arr_request_fill(2);boundary(0);seq_stop();
 proof(!d8p1_load_pool(&pool,1)&&chain.native_mode==2,"actual source adoption invalidates suspended active-fill route");
 midi_clock_transport(0xFB,fm1_ms);proof(!song.playing&&!d8arr_mix.apply,"loaded source cannot Continue a stale effective fill");
 overlay_setup(7);prepare_start();d8arr_request_fill(2);boundary(0);seq_stop();cut=0;
 proof(d8p1_save_pool(&pool,0)==D8POOL_IO&&!chain.native.policy.valid,"uncertain source save invalidates suspended active-fill route");cut=-1;
 midi_clock_transport(0xFB,fm1_ms);proof(!song.playing,"uncertain source write never resumes old fill");
 /* Fresh Start resets fill; count-in cannot publish a partially prepared overlay. */
 overlay_setup(7);prepare_start();d8arr_request_fill(2);boundary(0);seq_stop();seq_start();
 proof(song.playing&&!chain.native.policy.fill_active&&chain.native.policy.fill_pending==D8ARR_NONE&&chain.row==0,"fresh Start restarts normal row zero without retained fill");seq_stop();transport_req=0;
 cin_left=1;snapshot();proof(d8arr_prepare_pool(&arr_pool)==D8POOL_BUSY&&unchanged(),"count-in refuses overlay preparation without publication");cin_left=0;
 /* Active fill boundary and final Stop preserve a panel hold until actual key-up. */
 overlay_setup(7);prepare_start();d8arr_request_fill(2);boundary(0);track_t *held=&trk[7];held->p[P_MUTE]=0;held->p[P_AMODE]=0;held->p[P_CHRD]=CH_OFF;
 held->eng_req=0;mix_block(audio,CTL);kb_trk[0]=7;kb_note[0]=100;key_on(0,held);held->seq_n=1;held->seq_notes[0]=100;
 boundary(0);proof(midi_local_held(held,100),"fill return keeps actual same-pitch panel ownership");
 /* Other sequence voices may steal a voice from the eight shared pool. Renew
  * actual input after the boundary to test Stop gate retention independently. */
 key_off(0,held);key_on(0,held);
 held->seq_n=1;held->seq_notes[0]=100;seq_stop();unsigned gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(gated&&midi_local_held(held,100),"Stop after fill preserves panel sounding gate");key_off(0,held);gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(!gated&&!midi_local_held(held,100),"final physical panel release after fill retires sounding gate");
 /* Actual calibrated encoder routing/direction, not just direct callback tests. */
 overlay_setup(0);prepare_start();go_page(GR_SONG);ui.song_row=2;
 unsigned oldenc=panel.enc[EN_K3],otherenc=panel.enc[EN_K2];int olddir=panel.dir[EN_K3];panel.enc[EN_K2]=(uint8_t)oldenc;panel.enc[EN_K3]=(uint8_t)otherenc;panel.dir[EN_K3]=-1;
 host_enc[panel.enc[EN_K3]]=-1;frame();proof(chain.native.policy.fill_pending==2&&msg_is("FILL QUEUED"),"physical calibrated reverse K3 arms selected row fill");
 host_enc[panel.enc[EN_K3]]=1;frame();proof(chain.native.policy.fill_pending==D8ARR_NONE&&msg_is("FILL CANCELLED"),"physical calibrated reverse K3 cancels fill");panel.enc[EN_K3]=(uint8_t)oldenc;panel.enc[EN_K2]=(uint8_t)otherenc;panel.dir[EN_K3]=(int8_t)olddir;seq_stop();
 /* Real selected USB/TRS pulse and transport queues, not direct clock state. */
 for(unsigned clock=1;clock<=2;clock++){
  overlay_setup(7);song.g[G_CLOCK]=(int16_t)clock;events_block(1);proof(!chain_prepare(),"prepare active fill for actual selected realtime source");transport_req=0;
  proof(midi_enqueue(0xFAu<<8,clock),"selected-source Start enqueues for fill");events_block(1);d8arr_request_fill(2);
  unsigned pulses=0;
  while(!chain.native.policy.fill_active&&song.playing&&pulses++<130){fm1_ms+=20;midi_enqueue(0xF8u<<8,clock);events_block(1);}
  proof(chain.native.policy.fill_active&&pulses<=130,"actual selected clock pulses reach replacement fill boundary");
  for(unsigned j=0;j<10;j++){fm1_ms+=20;midi_enqueue(0xF8u<<8,clock);events_block(1);}
  unsigned idx=trk[0].seq_idx,masterstep=clk_step;uint32_t pos=trk[0].seq_pos,masterpos=clk_pos;
  midi_enqueue(0xFCu<<8,clock);events_block(1);proof(!song.playing&&chain.native.policy.fill_active,"actual selected realtime Stop preserves partial fill");
  midi_enqueue(0xFBu<<8,clock);events_block(1);proof(song.playing&&trk[0].seq_idx==idx&&trk[0].seq_pos==pos&&clk_step==masterstep&&clk_pos==masterpos,"actual selected realtime Continue preserves track/master phase");
  pulses=0;while(chain.native.policy.fill_active&&song.playing&&pulses++<130){fm1_ms+=20;midi_enqueue(0xF8u<<8,clock);events_block(1);}
  proof(song.playing&&!chain.native.policy.fill_active&&chain.row==1&&pulses<100,"remaining external fill pulses return exactly once to normal route");
  d8arr_request_fill(2);fm1_ms+=501;events_block(1);proof(!song.playing&&chain.native.policy.fill_pending==D8ARR_NONE,"clock timeout cancels pending fill and stops real sequence");song.g[G_CLOCK]=0;events_block(1);
 }
 /* Broken input cancels a pending fill immediately. */
 overlay_setup(0);prepare_start();d8arr_request_fill(2);midi_in_overflow=1;events_block(1);proof(chain.native.policy.fill_pending==D8ARR_NONE,"broken input invalidates pending fill generation");seq_stop();
 /* Maximum repeats/rows/scenes/banks use the same bounded route context. */
 overlay_setup(7);d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;a->banks=4;a->scenes=a->rows=16;a->bank[3]=a->bank[0];
 for(unsigned i=0;i<16;i++){a->scene[i]=a->scene[i%3];a->scene[i].bank=(uint8_t)(i%4);a->row[i].scene=(uint8_t)i;a->row[i].repeat=16;}
 prepare_start();d8arr_request_fill(15);boundary(0);proof(chain.native.policy.fill_return==0&&chain.native.policy.fill_remaining==14,"replacement consumes one of sixteen repeats");boundary(0);proof(chain.row==0&&chain.remaining==14&&!chain.native.policy.fill_active,"maximum-capacity fill returns exact repeat count");
 d8arr_request_row(15);boundary(0);proof(chain.row==15&&chain.native.policy.scene==15&&chain.native.policy.bank==3,"last bounded row/scene/bank remains playable with overlays");
 seq_stop();transport_req=0;
}

static int inherited_arrangement_main(void)
{
 seed();snapshot();proof(!chain_prepare()&&live_same()&&!writes,"preparation preserves editable music, undo and FM6");
 transport_req=0;seq_start();events_block(1);
 for(unsigned t=0;t<8;t++)proof(seq_steps(&trk[t])[0].note[0]==47-(t==1?0:t),"actual eight destination reference routing");
 proof(trk[0].p[P_PAN]==10&&trk[1].p[P_PAN]==10,"aliased source automation remaps to both destinations");
 trk[0].p[P_PAN]=13;trk[1].p[P_PAN]=17;motion_restore(&trk[0]);motion_restore(&trk[1]);
 motion_step(&trk[0],0,d8arr_motion(&trk[0]),1);motion_step(&trk[1],0,d8arr_motion(&trk[1]),1);
 motion_step(&trk[0],1,d8arr_motion(&trk[0]),1);motion_step(&trk[1],1,d8arr_motion(&trk[1]),1);
 proof(trk[0].p[P_PAN]==30&&trk[1].p[P_PAN]==30,"source parameter locks apply independently");
 motion_step(&trk[0],2,d8arr_motion(&trk[0]),1);motion_step(&trk[1],2,d8arr_motion(&trk[1]),1);
 proof(trk[0].p[P_PAN]==10&&trk[1].p[P_PAN]==10,"unlock resolves source automation rather than destination place");
 /* Current engine changes dynamically filter source-specific automation. */
 trk[0].eng_req=(uint8_t)((chain.native.policy.source_engine[0][7]+1)%14);int16_t atk=trk[0].p[P_FM1_ATK];
 motion_step(&trk[0],0,d8arr_motion(&trk[0]),1);proof(trk[0].p[P_FM1_ATK]==atk,"mismatched current engine refuses specific automation");
 trk[0].eng_req=chain.native.policy.source_engine[0][7];
 uint8_t editable=trk[0].step[0].note[0];boundary(3);
 proof(chain.row==0&&chain.remaining==1,"master bar repeat independent of three-step track0");
 boundary(5);proof(chain.row==1&&chain.carry==5&&trk[0].seq_idx==0&&trk[0].seq_pos==5,"master bar publishes referenced next row with exact carry");
 proof(seq_steps(&trk[0])[0].note[0]==55&&trk[0].step[0].note[0]==editable,"new sounding pattern never replaces editable steps");
 proof(!d8arr_request_row(2)&&!d8arr_request_row(0),"pending row latest valid request wins");
 proof(d8arr_request_row(3)==2&&chain.native.policy.pending==0,"invalid row preserves pending route");
 events_block(1);proof(chain.row==1,"request waits for master boundary");boundary(2);
 proof(chain.row==0&&chain.remaining==2&&chain.carry==2,"pending selection applies at bar and resets repeats");
 proof(!d8arr_request_row(2),"queue before stop");uint16_t phase=trk[0].seq_idx;uint32_t pos=trk[0].seq_pos;
 midi_clock_transport(0xFC,1);proof(!song.playing&&chain.native.policy.suspended&&chain.native.policy.pending==255,"Stop cancels pending and suspends");
 midi_clock_transport(0xFB,2);proof(d8arr_running()&&trk[0].seq_idx==phase&&trk[0].seq_pos==pos,"Continue resumes row and sequence phase");
 midi_clock_transport(0xFA,3);proof(chain.row==0&&chain.remaining==2&&clk_pos==CLK_START,"Start resets row and master phase");
 proof(!d8arr_request_row(2),"queue before broken input");midi_in_overflow=1;events_block(1);
 proof(chain.native.policy.pending==255,"broken MIDI stream cancels pending route");
 seq_stop();d8_capture_store_changed();midi_clock_transport(0xFB,4);
 proof(!song.playing&&!chain.running,"physical store epoch change refuses stale Continue");
 proof(chain.native_mode==2,"stale Continue leaves explicit invalid-resume tombstone");
 seq_start();proof(song.playing&&!chain.native_mode&&!chain.running&&clk_pos==CLK_START,"fresh Start after invalid resume plays editable project from top");seq_stop();
 /* Attempted save invalidates suspended source even with postcommit uncertainty. */
 transport_req=0;chain.native_mode=1;chain.native.policy.valid=1;cut=0;proof(d8p1_save_pool(&pool,0)==D8POOL_IO&&!chain.native.policy.valid,"uncertain attempted source writer revokes suspended preparation");reset();
 for(unsigned bit=8;bit<=32;bit*=2){arrange();d8p1_runtime_cache.arrangement.scene[0].apply=(uint8_t)bit;snapshot();proof(d8arr_prepare_pool(&arr_pool)==D8POOL_UNSUPPORTED&&unchanged(),"unsupported overlay refuses without weakening scene semantics");}
 arrange();d8p1_runtime_cache.arrangement.bank[0].project[0]=3;snapshot();proof(d8arr_prepare_pool(&arr_pool)==D8POOL_UNSUPPORTED&&unchanged(),"fourth reference refuses without autosave alias");arrange();
 /* A request during copied source reads may invalidate preparation, but must
  * leave live instruments, editable patterns, undo and saved identity intact. */
 for(unsigned kind=1;kind<=3;kind++){
  transport_req=0;chain.native.policy.valid=1;chain.native.policy.suspended=1;
  inject_reads=0;inject_at=2;inject_kind=kind;snapshot();
  int rc=d8arr_prepare_pool(&arr_pool);
  proof(rc==D8POOL_BUSY&&live_same()&&!chain.armed&&!chain.native.policy.valid,"late activity/Start/store mutation aborts partially copied preparation");
  inject_at=0;transport_req=0;
 }
 /* End-of-chain and maximum repeats through actual master-bar hook. */
 arrange();reset();prepare_start();
 chain.row=2;chain.remaining=1;chain.native.policy.bank=2;
 boundary(3);proof(!song.playing&&!chain.running&&chain.native.policy.suspended,"last native row stops at master boundary");
 arrange();d8p1_runtime_cache.arrangement.row[0].repeat=16;reset();prepare_start();
 for(unsigned i=0;i<15;i++){boundary(1);proof(chain.row==0&&chain.remaining==15-i,"all sixteen master-bar repeats retained");}
 boundary(7);proof(chain.row==1&&chain.carry==7,"sixteenth bar advances without truncation");
 chain.native.policy.generation=UINT32_MAX;proof(!d8arr_request_row(2),"request near generation wrap");
 seq_stop();proof(chain.native.policy.generation==0&&chain.native.policy.pending==255,"transport generation rollover cancels pending");
 /* Legal worst-case 96 local releases have no MIDI-output queue publication:
  * the inherited sequencer emits local voices, not keyboard MIDI packets. */
 arrange();reset();prepare_start();usb.config=1;mo_w=mo_r=0;
 for(unsigned t=0;t<8;t++){trk[t].seq_n=12;for(unsigned k=0;k<12;k++)trk[t].seq_notes[k]=(uint8_t)(30+k);}
 d8arr_apply(1,0);proof(!mo_w&&!mo_r,"96 bounded local note-offs produce no output-ring burst");
 for(unsigned t=0;t<8;t++)proof(!trk[t].seq_n,"each track local note list releases at boundary");
 /* MIDI-held and panel-held collision retains its sounding gate. */
 track_t *held=&trk[7];song.g[G_ROUTE]=0;song.playing=0;held->p[P_MUTE]=0;held->p[P_AMODE]=0;midi_note_event(7,100,100);mix_block(audio,CTL);
 held->seq_n=1;held->seq_notes[0]=100;seq_release(held);
 unsigned gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(gated&&midi_note_held(held,100),"same-pitch live MIDI hold survives native sequencer release");
 midi_note_event(7,100,0);gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(!gated&&!midi_note_held(held,100),"final live MIDI release ends retained gate without hanging");seq_stop();usb.config=0;
 arrange();reset();prepare_start();held->p[P_MUTE]=0;held->p[P_AMODE]=0;song.playing=0;
 midi_note_event(7,100,100);midi_control(7,64,127);midi_note_event(7,100,0);
 held->seq_n=1;held->seq_notes[0]=100;seq_release(held);
 proof(midi_note_held(held,100),"sustain-owned same pitch remains held after native release");
 midi_control(7,64,0);proof(!midi_note_held(held,100),"pedal-up retires sustained ownership");
 midi_note_event(7,100,100);held->seq_n=1;held->seq_notes[0]=100;seq_stop();
 gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(gated&&midi_note_held(held,100),"Stop releases sequencer before native-running cleanup, retaining live key");
 midi_control(7,120,0);gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate;
 proof(!gated&&!midi_note_held(held,100),"All Sound Off ends protected native held gates");
 arrange();reset();prepare_start();song.playing=0;held->p[P_MUTE]=0;held->p[P_AMODE]=0;
 kb_trk[0]=7;kb_note[0]=100;held->p[P_CHRD]=CH_OFF;key_on(0,held);held->seq_n=1;held->seq_notes[0]=100;seq_release(held);
 gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(gated&&midi_local_held(held,100),"same-pitch panel key retains native sounding gate");
 key_off(0,held);gated=0;for(unsigned v=0;v<NVOICE;v++)gated|=held->v[v].gate&&held->v[v].note==100;
 proof(!gated&&!midi_local_held(held,100),"panel key-up ends protected gate without hang");seq_stop();
 /* Existing count-in, external clock source and loss cancellation are actual
  * events_block routes, not a separate planner model. */
 arrange();reset();cin_left=1;snapshot();proof(d8arr_prepare_pool(&arr_pool)==D8POOL_BUSY&&unchanged(),"active count-in refuses preparation");cin_left=0;
 prepare_start();proof(!d8arr_request_row(2),"queue before clock-source change");song.g[G_CLOCK]=1;events_block(1);
 proof(!song.playing&&!chain.running&&chain.native.policy.pending==255,"clock-source change stops and cancels route");
 arrange();reset();proof(!chain_prepare(),"external Start preparation");transport_req=0;midi_clock_transport(0xFA,10);
 proof(d8arr_running()&&song.playing,"USB MIDI Start uses actual native row zero");
 proof(!d8arr_request_row(2),"queue before external cable loss");fm1_ms=511;midi_clock.have_pulse=0;midi_clock.start_ms=10;events_block(1);
 proof(!song.playing&&!chain.running&&chain.native.policy.pending==255,"external clock timeout cancels pending route");song.g[G_CLOCK]=0;events_block(1);
 /* Both actual USB/TRS realtime queues select their configured source. */
 for(unsigned clock=1;clock<=2;clock++){
  arrange();reset();song.g[G_CLOCK]=(int16_t)clock;events_block(1);
  proof(!chain_prepare(),"prepare for configured USB/TRS source");transport_req=0;fm1_ms+=1;
  proof(midi_enqueue(0xFAu<<8,3-clock),"enqueue other-source Start");events_block(1);
  proof(!song.playing&&chain.armed,"other clock source cannot activate native arrangement");
  proof(midi_enqueue(0xFAu<<8,clock),"enqueue selected-source Start");events_block(1);
  proof(song.playing&&d8arr_running()&&chain.row==0,"selected USB/TRS Start reaches real sequencer");
  proof(!d8arr_request_row(2)&&midi_enqueue(0xFCu<<8,clock),"queue selected-source Stop");events_block(1);
  proof(!song.playing&&chain.native.policy.pending==255,"selected-source Stop cancels native pending route");
  proof(midi_enqueue(0xFBu<<8,clock),"enqueue selected-source Continue");events_block(1);
  proof(song.playing&&d8arr_running(),"selected-source Continue resumes native arrangement");seq_stop();
 }
 song.g[G_CLOCK]=0;events_block(1);
 /* Every bounded bank/scene/row slot is represented, not truncated to eight. */
 arrange();d8p1_arrangement *all=&d8p1_runtime_cache.arrangement;
 all->banks=4;all->scenes=16;all->rows=16;all->bank[3]=all->bank[0];
 for(unsigned i=0;i<16;i++){all->scene[i]=all->scene[i%3];all->scene[i].bank=(uint8_t)(i%4);all->row[i].scene=(uint8_t)i;all->row[i].repeat=1;}
 reset();prepare_start();
 for(unsigned i=1;i<16;i++){boundary(i%7);proof(chain.row==i&&chain.native.policy.scene==i&&chain.native.policy.bank==i%4,"all sixteen scene/row references and four banks play");}
 boundary(0);proof(!song.playing,"sixteen-row final boundary stops");
 /* Two channels owning the same live destination pitch retain it until the
  * final owner releases; no transposed source note is used as ownership. */
 arrange();reset();prepare_start();song.playing=0;held->p[P_MUTE]=0;held->p[P_AMODE]=0;song.g[G_ROUTE]=1;song.sel=7;
 midi_note_event(2,100,100);midi_note_event(3,100,100);held->seq_n=1;held->seq_notes[0]=100;seq_release(held);
 midi_note_event(2,100,0);proof(midi_note_held(held,100),"second channel owner survives first release after native collision");
 midi_note_event(3,100,0);proof(!midi_note_held(held,100),"last channel release clears native retained ownership");seq_stop();song.g[G_ROUTE]=0;
 /* Successful adoption invalidates resume but permits ordinary fresh Start. */
 arrange();reset();prepare_start();seq_stop();proof(!d8p1_load_pool(&pool,1)&&chain.native_mode==2,"successful source load invalidates suspended native route");
 midi_clock_transport(0xFB,fm1_ms);proof(!song.playing,"load then Continue refuses invalid route");
 seq_start();proof(song.playing&&!chain.native_mode&&!chain.running,"load then HOME fresh Start plays adopted editable project");seq_stop();
 arrange();d8p1_runtime_cache.arrangement.scene[0].apply=128;transport_req=0;
 proof(chain_prepare()==1&&!transport_req&&!song.playing,"failed SONG preparation never requests editable fallback playback");events_block(1);
 proof(!song.playing,"failed SONG request remains stopped");
 cache_cases();
 arrange();reset();build_inject=1;snapshot();
 proof(d8arr_prepare_pool(&arr_pool)==D8POOL_BUSY&&!chain.armed&&!chain.native.policy.valid,"ownership mutate-and-revert during initial cache build refuses publication");
 proof(build_inject==0,"mixed cache guard case executes actual builder midpoint hook");
 /* Native row count/name/repeats are real loaded metadata, not stale legacy. */
 arrange();char name[13];unsigned repeat=0;
 proof(d8arr_ui_rows()==3&&d8arr_ui_row(2,name,&repeat)&&!strcmp(name,"SCENE3")&&repeat==1,"native UI reads actual arrangement metadata");
 go_page(GR_SONG);ui.song_row=0;edit_param(0,2);proof(ui.song_row==2,"actual SONG knob browses native row count");
 reset();prepare_start();ui.song_row=1;edit_param(3,1);proof(chain.native.policy.pending==1,"actual fourth SONG knob queues selected row");
 unsigned rows_before=d8arr_ui_rows();edit_param(1,1);proof(d8arr_ui_rows()==rows_before&&msg_is("NATIVE ROW READ ONLY"),"actual native scene editor refuses unsupported mutation");seq_stop();transport_req=0;
 project_native.ops.prepare_arrangement=NULL;proof(chain_prepare()==1,"missing optional prepare callback explicitly refuses");
 legacy_boundaries();period_cases();overlay_cases();fill_cases();
 proof(!writes,"arrangement path never writes NOR");
 printf("Native arrangement: %u checks, %u failures; actual runtime/sequencer/clock/motion, virtual NOR only\n",checks,failures);return !!failures;
}
