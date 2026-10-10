/* SPDX-License-Identifier: GPL-3.0-only */
/* Real native runtime plus shared virtual NOR; no device or migration write. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
static uint8_t nor[D8POOL_BYTES+512],baseline[sizeof nor],wire[D8P1_LIMIT],out[D8P1_LIMIT],saved[4][D8P1_LIMIT];
static size_t saved_n[4];
static unsigned checks,failures,reads,writes;
static int cut=-1,read_error=-1,backend_busy,request_after=-1;
static struct {
 __typeof__(trk) tracks;__typeof__(song) song_state;__typeof__(motion) motion_state;
 __typeof__(chain) chain_state;__typeof__(chain_config) config;__typeof__(d8p1_runtime_cache) cache;
 __typeof__(fm6_patch) patches;__typeof__(fm6_pgen) generations;__typeof__(ui) ui_state;__typeof__(undo) undo_state;
 uint32_t panic;uint8_t slot,countin;
} before;
static void proof(int yes,const char *name){checks++;if(!yes){failures++;fprintf(stderr,"FAIL: %s\n",name);}}
static void snapshot(void){
 memcpy(before.tracks,trk,sizeof trk);memcpy(&before.song_state,&song,sizeof song);memcpy(&before.motion_state,&motion,sizeof motion);
 memcpy(&before.chain_state,&chain,sizeof chain);memcpy(&before.config,&chain_config,sizeof chain_config);memcpy(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache);
 memcpy(before.patches,fm6_patch,sizeof fm6_patch);memcpy(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen);memcpy(&before.ui_state,&ui,sizeof ui);memcpy(&before.undo_state,&undo,sizeof undo);
 before.panic=panic_req;before.slot=proj_cur;before.countin=cin_left;
}
static int unchanged(void){return !memcmp(before.tracks,trk,sizeof trk)&&!memcmp(&before.song_state,&song,sizeof song)&&!memcmp(&before.motion_state,&motion,sizeof motion)&&
 !memcmp(&before.chain_state,&chain,sizeof chain)&&!memcmp(&before.config,&chain_config,sizeof chain_config)&&!memcmp(&before.cache,&d8p1_runtime_cache,sizeof d8p1_runtime_cache)&&
 !memcmp(before.patches,fm6_patch,sizeof fm6_patch)&&!memcmp(before.generations,(const void *)fm6_pgen,sizeof fm6_pgen)&&!memcmp(&before.ui_state,&ui,sizeof ui)&&!memcmp(&before.undo_state,&undo,sizeof undo)&&
 before.panic==panic_req&&before.slot==proj_cur&&before.countin==cin_left;}
static void reset(void){cut=read_error=request_after=-1;backend_busy=0;reads=writes=0;transport_req=0;}
static int rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;proof(a<=D8POOL_BYTES&&n<=D8POOL_BYTES-a,"bounded read");reads++;if(read_error==0)return -1;if(read_error>0)read_error--;memcpy(p,nor+256+a,n);return 0;}
static int mutate(void){if(cut==0)return -1;if(cut>0)cut--;writes++;if(request_after>=0&&writes>=(unsigned)request_after)transport_req=1;return 0;}
static int er(void *c,uint32_t a){(void)c;proof(a%4096==0&&a<=D8POOL_BYTES-4096,"bounded erase");if(mutate())return -1;memset(nor+256+a,255,4096);return 0;}
static int pg(void *c,uint32_t a,const void *p,uint32_t n){(void)c;const uint8_t *q=p;proof(n&&n<=256&&a<=D8POOL_BYTES-n&&(a&255)+n<=256,"bounded program");if(mutate())return -1;for(unsigned i=0;i<n;i++)nor[256+a+i]&=q[i];return 0;}
static int stop(void *c){(void)c;return !backend_busy;}
static d8pool pool={NULL,rd,er,pg,stop};
static void blank(void){memset(nor,0xa5,sizeof nor);memset(nor+256,255,D8POOL_BYTES);reset();}
static void put(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static size_t fixture(const char *name){FILE *f=fopen(name,"rb");if(!f)exit(2);size_t n=fread(wire,1,sizeof wire,f);if(fgetc(f)!=EOF||ferror(f))exit(2);fclose(f);return n;}
static int stored(unsigned o){size_t n=0;return !d8pool_load(&pool,o,out,sizeof out,&n,7)&&n==saved_n[o]&&!memcmp(out,saved[o],n);}
int main(void){
 ui_power_on();int32_t audio[2u*CTL];mix_block(audio,CTL);size_t n=fixture("tests/fixtures/d8p1/minimal.d8p");
 proof(!d8p1_load_runtime(wire,n,0),"valid initial native music");mix_block(audio,CTL);
 blank();snapshot();proof(d8p1_save_pool(&pool,0)==D8POOL_EMPTY&&unchanged()&&!writes,"blank pool never authorizes implicit migration");
 proof(d8p1_load_pool(&pool,0)==D8POOL_EMPTY&&unchanged(),"empty load preserves all music");
 memcpy(nor+256,"FELU",4);reset();proof(d8p1_save_pool(&pool,0)==D8POOL_UNSUPPORTED&&unchanged()&&!writes,"legacy bytes prohibit native write");
 blank();proof(!d8pool_save(&pool,0,wire,n,7),"test-only explicit native seed; not controller migration");
 reset();snapshot();proof(d8p1_load_pool(&pool,1)==D8POOL_EMPTY&&unchanged()&&!writes,"missing project never aliases another slot");
 for(unsigned o=0;o<4;o++){
  trk[7].p[P_LEVEL]=(int16_t)(40+o*10);trk[7].p[P_PAN]=(int16_t)(o*7-12);trk[7].step[63].n=1;trk[7].step[63].note[0]=(uint8_t)(60+o);trk[7].step[63].time=0;song.sel=7;
  proof(!d8p1_capture_runtime(saved[o],sizeof saved[o],&saved_n[o]),"coherent full eight-track baseline");uint8_t slot=proj_cur;
  proof(!d8p1_save_pool(&pool,o)&&stored(o),"actual runtime save matches complete snapshot");
  proof(proj_cur==(o<3?o:slot),"manual slot publishes after commit; autosave retains current slot");
 }
 for(unsigned o=0;o<4;o++){
  trk[7].p[P_LEVEL]=3;trk[7].step[63].n=1;trk[7].step[63].note[0]=12;proof(!d8p1_load_pool(&pool,o),"actual runtime recalls selected stored object");
  n=0;proof(!d8p1_capture_runtime(out,sizeof out,&n)&&n==saved_n[o]&&!memcmp(out,saved[o],n),"entire live project round trips, including track eight");
  proof(proj_cur==(o<3?o:PROJ_NO_SLOT),"autosave restore has no saved-project identity");mix_block(audio,CTL);
 }
 reset();snapshot();proof(d8p1_restore_pool_autosave(NULL,0)==D8POOL_EMPTY&&unchanged()&&!reads&&!writes,"disabled boot policy performs no access or adoption");
 trk[7].p[P_LEVEL]=1;proof(!d8p1_restore_pool_autosave(&pool,1)&&trk[7].p[P_LEVEL]==70,"allowed autosave restores real music");mix_block(audio,CTL);
 cv_begin(240,124,T_BG);cv_rect(1,1,7,9,T_ACCENT);uint32_t pixels=pixels_hash(cv_px,CV_MAX);cv_blit(0,20);unsigned consumed=dma_consumed;
 proof(!d8p1_load_pool(&pool,2)&&dma_consumed==consumed+1&&!dma.p&&!dma_errors&&pixels_hash(host_screen+20*240,CV_MAX)==pixels,"pool preflight fences pending LCD pixels before staging");mix_block(audio,CTL);
 /* Native arrangement survives storage and drawing without enabling playback. */
 d8p1_project_state native;proof(d8p1_project_decode(&native,saved[2],saved_n[2],7),"decode saved state for native metadata");
 native.arrangement.banks=1;native.arrangement.scenes=1;native.arrangement.rows=1;memcpy(native.arrangement.bank[0].name,"NATIVE",6);memcpy(native.arrangement.scene[0].name,"NATIVE",6);
 for(unsigned t=0;t<8;t++){native.arrangement.bank[0].project[t]=(uint8_t)(t%3);native.arrangement.bank[0].track[t]=(uint8_t)t;}
 native.arrangement.scene[0].bank=0;native.arrangement.row[0].scene=0;native.arrangement.row[0].repeat=2;
 n=0;proof(d8p1_project_encode(wire,sizeof wire,&n,&native)&&!d8p1_load_runtime(wire,n,7),"native three-project arrangement loads");
 proof(!d8p1_save_pool(&pool,2)&&!d8p1_load_pool(&pool,2)&&d8p1_runtime_arrangement()&&!memcmp(d8p1_runtime_arrangement(),&native.arrangement,sizeof native.arrangement)&&!chain_busy(),"pool preserves complete arrangement without starting playback");
 cv_begin(8,8,T_BG);cv_blit(0,0);proof(d8p1_runtime_arrangement()&&!memcmp(d8p1_runtime_arrangement(),&native.arrangement,sizeof native.arrangement),"drawing retains recalled native metadata");mix_block(audio,CTL);
 /* Faults after each mutation leave all previous saves and full live music. */
 memcpy(baseline,nor,sizeof nor);d8pool_index index;proof(!d8pool_inventory(&pool,&index),"complete current pool");
 for(unsigned o=0;o<4;o++){saved_n[o]=0;proof(!d8pool_load(&pool,o,saved[o],sizeof saved[o],&saved_n[o],7),"retain current wires for cut proofs");}
 trk[7].p[P_LEVEL]=97;n=0;proof(!d8p1_capture_runtime(wire,sizeof wire,&n),"replacement snapshot");unsigned ops=2+(unsigned)((n+255)/256)+1;
 for(unsigned c=0;c<ops;c++){
  memcpy(nor,baseline,sizeof nor);reset();snapshot();cut=(int)c;
  proof(d8p1_save_pool(&pool,0)==D8POOL_IO&&unchanged(),"each interrupted runtime save preserves full live state and slot");reset();for(unsigned o=0;o<4;o++)proof(stored(o),"each interrupted runtime save retains every current stored object");
 }
 for(unsigned c=1;c<ops;c++){
  memcpy(nor,baseline,sizeof nor);reset();snapshot();request_after=(int)c;
  proof(d8p1_save_pool(&pool,0)==D8POOL_BUSY&&transport_req==1&&unchanged(),"late transport request gates every subsequent pool mutation");reset();for(unsigned o=0;o<4;o++)proof(stored(o),"late request retains all committed objects");
 }
 memcpy(nor,baseline,sizeof nor);reset();snapshot();read_error=0;proof(d8p1_load_pool(&pool,0)==D8POOL_IO&&unchanged()&&!writes,"read error cannot adopt or write");reset();
 for(unsigned b=0;b<8;b++){
  if(b==0)song.playing=1;if(b==1)chain.armed=1;if(b==2)chain.running=1;if(b==3)cin_left=1;if(b==4)transport_req=1;if(b==5)backend_busy=1;if(b==6)cv_begin(8,8,T_BG);if(b==7)transport_req=3;
  reads=writes=0;snapshot();proof(d8p1_save_pool(&pool,0)==D8POOL_BUSY&&d8p1_load_pool(&pool,0)==D8POOL_BUSY&&unchanged()&&!reads&&!writes,"busy transport/backend/drawing refuses before I/O");
  song.playing=chain.armed=chain.running=cin_left=transport_req=0;backend_busy=0;if(b==6)cv_blit(0,0);
 }
 reset();snapshot();proof(d8p1_save_pool(NULL,0)==D8POOL_INVALID&&d8p1_load_pool(&pool,4)==D8POOL_INVALID&&unchanged()&&!reads&&!writes,"invalid backend/object refused without alias");
 /* A historical fourth identity may exist in offline/native wire, but must
  * never be claimed by the selected three-project persistent controller. */
 memcpy(nor,baseline,sizeof nor);reset();n=fixture("tests/fixtures/d8p1/maximum.d8p");proof(!d8p1_load_runtime(wire,n,15),"legacy fourth-reference wire remains representable for preservation");mix_block(audio,CTL);reset();snapshot();
 proof(d8p1_save_pool(&pool,0)==D8POOL_INVALID&&unchanged()&&!writes,"fourth-project reference refuses persistent capture without remapping");
 /* Unknown optional payload remains committed and protected, even in a
  * different object. CRC-valid wire is not automatically writable/adoptable. */
 memcpy(nor,baseline,sizeof nor);reset();proof(!d8pool_inventory(&pool,&index),"locate unknown payload target");n=fixture("tests/fixtures/d8p1/unknown-optional.d8p");uint8_t *unknown=nor+256+index.object[1].block*8192;
 memcpy(unknown+256,wire,n);put(unknown+16,(uint32_t)n);put(unknown+20,d8p1_crc32(wire,n));put(unknown+28,d8p1_crc32(unknown,28));reset();snapshot();
 proof(d8p1_save_pool(&pool,0)==D8POOL_INVALID&&d8p1_load_pool(&pool,0)==D8POOL_INVALID&&unchanged()&&!writes,"unknown optional current object protects all persistence operations");
 memcpy(nor,baseline,sizeof nor);reset();proof(!d8pool_inventory(&pool,&index),"locate required project target");nor[256+index.object[1].block*8192+256+300]^=1;reset();snapshot();
 proof(d8p1_save_pool(&pool,0)==D8POOL_INVALID&&unchanged()&&!writes,"missing damaged project reference is not counted as available");
 /* A future current object must not be overwritten or used as a reference. */
 memcpy(nor,baseline,sizeof nor);reset();proof(!d8pool_inventory(&pool,&index),"future test locates current block");uint8_t *h=nor+256+index.object[1].block*8192;h[4]=2;put(h+28,d8p1_crc32(h,28));reset();snapshot();
 proof(d8p1_save_pool(&pool,0)==D8POOL_UNSUPPORTED&&d8p1_load_pool(&pool,0)==D8POOL_UNSUPPORTED&&unchanged()&&!writes,"future metadata anywhere protects the pool and live music");
 for(unsigned i=0;i<256;i++)proof(nor[i]==0xa5&&nor[256+D8POOL_BYTES+i]==0xa5,"NOR guards retained");
 printf("D8P1 runtime pool: %u checks, %u failures; %u save cuts and %u late-start cuts; actual eight-track runtime, virtual NOR only\n",checks,failures,ops,ops-1);return !!failures;
}
