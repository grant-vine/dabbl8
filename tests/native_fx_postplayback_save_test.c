/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual panel/audio path; independent full resident closed-range oracle. */
#include "../web/emu/felucca_web.c"
static unsigned checks, failures, require_zero;
static uint32_t trace=2166136261u;
static FILE *capture;
#define CHECK(x) do { checks++; if (!(x)) { failures++; printf("FAIL line %d: %s\n",__LINE__,#x); } } while (0)
static void advance(unsigned ms)
{
 unsigned frames=ms*FS/1000;
 while(frames){unsigned n=frames>128?128:frames;web_render(n);frames-=n;
  for(unsigned i=0;i<n;i++){
   uint32_t l,r;memcpy(&l,&out_l[i],4);memcpy(&r,&out_r[i],4);
   trace=(trace^l)*16777619u;trace=(trace^r)*16777619u;
   if(require_zero)CHECK(out_l[i]==0&&out_r[i]==0);
   if(capture){CHECK(fwrite(&out_l[i],4,1,capture)==1);CHECK(fwrite(&out_r[i],4,1,capture)==1);}
  }
 }
}
static void tap(unsigned b){web_buttons(1u<<b);advance(60);web_buttons(0);advance(60);}
static void key(unsigned k){web_keys(1u<<k);advance(60);web_keys(0);advance(60);}
static uint32_t state_hash(const int16_t *buffer,unsigned n)
{
 uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^(uint16_t)buffer[i])*16777619u;return h;
}
static void resident_oracle(void)
{
 /* Scan every active cell independently of the audio-owned certificates. */
 int32_t sum=0;unsigned pos=0;
 for(unsigned k=0;k<4;k++){
  int32_t c=rev_comb[pos],lp=fx.comb_lp[k];
  CHECK(lp==c+mulq15(lp-c,song.g[G_RDAMP]*200));
  CHECK(mulq15(lp,25000+song.g[G_RSIZE]*50)==c);
  for(unsigned i=0;i<REV_COMB[k];i++)CHECK(rev_comb[pos+i]==c);
  sum+=c;pos+=REV_COMB[k];
 }
 int32_t d=dly_buf[(fx.dly_w-1)&(DLY_LEN-1)],dw=mulq15(d*2,song.g[G_DMIX]*258);
 CHECK(mulq15(d-fx.dly_lp,2000+song.g[G_DCOLOR]*240)==0);
 CHECK(mulq15(fx.dly_lp,song.g[G_DFDBK]*230)==d);
 for(unsigned i=1;i<=delay_samples();i++)CHECK(dly_buf[(fx.dly_w-i)&(DLY_LEN-1)]==d);
 for(unsigned i=0;i<CHO_LEN;i++)CHECK(cho_buf[i]==0);
 unsigned upper=0,lower=0;
 for(unsigned i=0;i<556;i++){
  int32_t cell=rev_ap[i];CHECK(sum+(cell>>1)==cell);
  lower+=cell==2*sum-1;upper+=cell==2*sum;
 }
 CHECK(lower&&upper);
 for(unsigned i=0;i<441;i++)CHECK(rev_ap[556+i]>=2*sum-2&&rev_ap[556+i]<=2*sum-1);
 /* All556x441 phase pairs: next AP2 cell remains in the same pair, and
  * every current/future pair gives exactly the DC blocker's constant input. */
 for(unsigned i=0;i<556;i++)for(unsigned j=0;j<441;j++){
  int32_t a=rev_ap[i]-sum,o=rev_ap[556+j],next=a+(o>>1);
  CHECK(next>=2*sum-2&&next<=2*sum-1);
 }
 for(int32_t a=sum-1;a<=sum;a++)for(int32_t o=2*sum-2;o<=2*sum-1;o++){
  int32_t next=a+(o>>1),input=((((o-a+dw)>>2)*(int32_t)song.master_q12)>>10);
  CHECK(next>=2*sum-2&&next<=2*sum-1);
  CHECK(dc_l==input*64&&dc_r==input*64);
 }
 CHECK(dce_l>=0&&dce_l<4096&&dce_r>=0&&dce_r<4096);
 printf("ORACLE full comb=%u AP1=556 AP2=441 phasepairs=245196 sum=%d delay=%d DC=%d/%d input=%d mixed=%u/%u\n",pos,sum,d,dc_l,dc_r,dc_l/64,lower,upper);
}
static void turn(unsigned role,int steps)
{
 for(unsigned i=0;i<(unsigned)(steps<0?-steps:steps);i++){web_enc(role,steps<0?-1:1);advance(40);}
}
static void propose_save(void)
{
 tap(B_HOME);tap(B_SAVE);tap(B_SAVE);
 CHECK(!ui.home&&cur_page()->graph==GR_SLOTS);
 turn(EN_K1,-3);turn(EN_K1,1);turn(EN_K4,1);tap(B_OCTUP);
 if(ui.confirm==CF_OVR_PROJ)tap(B_OCTUP);
 CHECK(name_on()&&nm.kind==NK_PROJ_SAVE&&nm.slot==1);
 unsigned writes=web_flash_writes;
 tap(B_OCTUP);
 if(d8ls.phase==LS_CONSENT){
  web_buttons(1u<<B_SAVE);advance(60);CHECK(d8ls.phase==LS_RELEASE&&web_flash_writes==writes);
  web_buttons(0);advance(120);
 }
 CHECK(d8ls.phase==LS_IDLE&&d8ls.intent&&web_flash_writes>writes);
 CHECK(project_used(1)==1&&proj_cur==1&&!strcmp(ui.msg,"SAVED"));
 printf("SAVE after C4: writes=%u->%u slot=%u message=%s\n",writes,web_flash_writes,proj_cur,ui.msg);
}
static int32_t bus_sample(void)
{
 int32_t zero=0,wet=0;fx_buses(&zero,&zero,&zero,&wet,1);
 int32_t l=((wet>>2)*(int32_t)song.master_q12)>>10,r=l;master_out(&l,&r);
 CHECK(l==r);return l;
}
static void refusal_controls(void)
{
 /* The exact range must still collapse at the current parameters. */
 uint16_t master=song.master_q12;song.master_q12=4096;
 CHECK(!fx_resident_quiet());song.master_q12=master;CHECK(fx_resident_quiet());
 int16_t size=song.g[G_RSIZE];song.g[G_RSIZE]=127;
 CHECK(!fx_resident_quiet());song.g[G_RSIZE]=size;CHECK(fx_resident_quiet());
 int16_t model=song.g[G_RTYPE];song.g[G_RTYPE]=1;
 CHECK(!fx_resident_quiet());song.g[G_RTYPE]=model;CHECK(fx_resident_quiet());
 /* A hidden late cell immediately revokes proof, before its audible return. */
 unsigned at=(fx.ap_i[1]+300u)%441u;
 FT16(ap,&rev_ap[556+at],16384);CHECK(!fx_resident_quiet());
 unsigned audible=0;
 for(unsigned i=0;i<1000;i++){audible|=bus_sample()!=0;CHECK(!fx_resident_quiet());}
 CHECK(audible);printf("CONTROLS master/size/model revoke; tracked hidden AP2 impulse audible=%u and refused\n",audible);
}
int main(int argc,char **argv)
{
 if(argc>1){capture=fopen(argv[1],"wb");CHECK(capture!=NULL);if(!capture)return 2;}
 /* Given a blank native image initialized through the actual SAVE panel. */
 web_nor_erase();web_boot();advance(1000);CHECK(d8fi.phase==FI_REQUIRED);
 tap(B_SAVE);advance(30000);CHECK(d8fi.phase==FI_IDLE);
 /* When a C4 note is STEP-recorded through EDIT/SEQ/REC and released. */
 tap(B_HOME);web_buttons(1u<<B_EDIT);advance(400);key(0);web_buttons(0);advance(400);
 tap(B_SEQ);tap(B_REC);key(7);tap(B_REC);
 CHECK(step_on(&trk[0].step[0])&&trk[0].step[0].note[0]==60);
 advance(20000);resident_oracle();
 uint32_t ap1=state_hash(rev_ap,556),ap2=state_hash(rev_ap+556,441);
 require_zero=1;advance(6000);resident_oracle();
 CHECK(state_hash(rev_ap,556)==ap1);CHECK(state_hash(rev_ap+556,441)!=ap2);
 printf("AUDIO zeroframes=264600 trace=%08x AP1=%08x AP2=%08x->%08x quiet=%d\n",trace,ap1,ap2,state_hash(rev_ap+556,441),fx_resident_quiet());
 if(capture){CHECK(!fclose(capture));capture=NULL;}
 /* Then indefinite zero output is admitted and the real deliberate SAVE
  * commits without resetting, zeroing or disabling any effects resident state. */
 CHECK(fx_resident_quiet());CHECK(ls_quiet());
 if(!failures){propose_save();if(!failures)refusal_controls();}
 printf("FX postnote save: %u checks %u failures\n",checks,failures);return failures!=0;
}
