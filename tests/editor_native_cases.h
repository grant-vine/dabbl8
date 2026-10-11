/* SPDX-License-Identifier: GPL-3.0-only */
/* Native expectations for the complete editor harness. No production policy seam. */
#define NS_FIELD(x) __typeof__(x) x
static struct {
 NS_FIELD(trk); NS_FIELD(song); NS_FIELD(motion); NS_FIELD(chain); NS_FIELD(chain_config);
 NS_FIELD(up_bank); NS_FIELD(upf); NS_FIELD(proj_slot); NS_FIELD(settings); NS_FIELD(favorites);
 NS_FIELD(fm6_patch); NS_FIELD(fm6_slot); NS_FIELD(fm6_own); NS_FIELD(usr_zone); NS_FIELD(usr_nz);
 NS_FIELD(ui); NS_FIELD(ed_w); NS_FIELD(project_native);
 uint8_t pgen[sizeof fm6_pgen], samples[sizeof host_samples];
 uint32_t up_generation, erases, writes, pending, panic, bk_pos, bk_put, serial_at, sync;
 uint32_t controls[13];
} ns;
#undef NS_FIELD
#define NS_COPY(x) memcpy(&ns.x,&x,sizeof(x))
#define NS_SAME(x) (!memcmp(&ns.x,&x,sizeof(x)))
static __typeof__(ed_w) native_watch_expected;
static uint32_t native_reload_expected;
static uint8_t native_service_wire[sizeof host_wire];
static uint32_t native_service_wire_n;
static void native_service_control(void);
static void native_snapshot(void) {
 NS_COPY(trk);NS_COPY(song);NS_COPY(motion);NS_COPY(chain);NS_COPY(chain_config);
 NS_COPY(up_bank);NS_COPY(upf);NS_COPY(proj_slot);NS_COPY(settings);NS_COPY(favorites);
 NS_COPY(fm6_patch);NS_COPY(fm6_slot);NS_COPY(fm6_own);NS_COPY(usr_zone);NS_COPY(usr_nz);
 NS_COPY(ui);NS_COPY(ed_w);NS_COPY(project_native);
 memcpy(ns.pgen,(const void*)fm6_pgen,sizeof ns.pgen);memcpy(ns.samples,host_samples,sizeof ns.samples);
 ns.up_generation=up_gen;ns.erases=host_erases;ns.writes=host_writes;ns.pending=transport_req;ns.panic=panic_req;
 ns.bk_pos=ed_bk_pos;ns.bk_put=ed_bk_put;ns.serial_at=menu_serial_at;ns.sync=sync_reload;
 const uint32_t controls[]={ui_prefs,ui_style,ui_rec_prefs,settings_leds,settings_hold,fx_lowcut,fx_usb_fixed,click_mode,click_lvl,cin_bars,ui_scr,usb_cdc_on,fm6_own_ok};memcpy(ns.controls,controls,sizeof controls);
 native_service_control();
}
static int native_same(void) {
 const uint32_t controls[]={ui_prefs,ui_style,ui_rec_prefs,settings_leds,settings_hold,fx_lowcut,fx_usb_fixed,click_mode,click_lvl,cin_bars,ui_scr,usb_cdc_on,fm6_own_ok};
 return !memcmp(&ed_w,&native_watch_expected,sizeof ed_w)&&sync_reload==native_reload_expected&&!memcmp(controls,ns.controls,sizeof controls)&&menu_serial_at==ns.serial_at&& NS_SAME(trk)&&NS_SAME(song)&&NS_SAME(motion)&&NS_SAME(chain)&&NS_SAME(chain_config)&&
 NS_SAME(up_bank)&&NS_SAME(upf)&&NS_SAME(proj_slot)&&NS_SAME(settings)&&NS_SAME(favorites)&&
 NS_SAME(fm6_patch)&&NS_SAME(fm6_slot)&&NS_SAME(fm6_own)&&NS_SAME(usr_zone)&&NS_SAME(usr_nz)&&
 NS_SAME(ui)&&NS_SAME(project_native)&&
 !memcmp(ns.pgen,(const void*)fm6_pgen,sizeof ns.pgen)&&!memcmp(ns.samples,host_samples,sizeof ns.samples)&&
 ns.up_generation==up_gen&&ns.erases==host_erases&&ns.writes==host_writes&&ns.pending==transport_req&&ns.panic==panic_req&&
 ns.bk_pos==ed_bk_pos&&ns.bk_put==ed_bk_put;
}
#undef NS_COPY
#undef NS_SAME
static unsigned native_refusals, native_reads;
/* Matched control: run the actual service with no request on the same music,
 * WATCH and queue prestate. Restore its communication state before the denied
 * request. All expected shadows/reload consumption come from this execution,
 * not broad normalization. A real request additionally updates last_ms. */
static void native_service_control(void) {
 static __typeof__(sx_out_q) queue_before;
 uint32_t r=so_r,w=so_w,wire_n=host_wire_n;
 memcpy(queue_before,sx_out_q,sizeof queue_before);
 ota_frame_done();host_wire_n=0;ed_service();host_drain();
 native_watch_expected=ed_w;native_watch_expected.last_ms=fm1_ms;
 native_reload_expected=sync_reload;native_service_wire_n=host_wire_n;
 memcpy(native_service_wire,host_wire,host_wire_n);
 ed_w=ns.ed_w;sync_reload=ns.sync;so_r=r;so_w=w;
 memcpy(sx_out_q,queue_before,sizeof queue_before);host_wire_n=wire_n;
}
static int native_error(uint32_t cmd,uint32_t reason) {
 const uint8_t error[]={0xf0,ED_HDR0,ED_HDR1,ED_HDR2,ED_D8_ERROR,1,(uint8_t)cmd,(uint8_t)reason,0xf7};
 return host_wire_n==native_service_wire_n+sizeof error &&
 !memcmp(host_wire,native_service_wire,native_service_wire_n) &&
 !memcmp(host_wire+native_service_wire_n,error,sizeof error);
}
static int native_refuse(uint32_t cmd,const uint8_t*a,uint32_t n) {
 char label[96];native_snapshot();request(cmd,a,n);native_refusals++;
 snprintf(label,sizeof label,"native command %u length %u: quarantine reason 1, music/mirrors/flash unchanged",cmd,n);
 return check(label,native_error(cmd,1)&&native_same());
}
/* Seeding is independent of the refused editor write: parse and validate a
 * record into local test storage, then install the already validated RAM fixture.
 * No editor permission override, flash call or failed-write continuation. */
static int native_seed_up(const uint8_t*a,uint32_t n) {
 up_rec_t record;uint32_t slot=UP_SLOTS;int16_t v[P_COUNT];
 if(up_parse(a,n,&record,&slot)||slot>=UP_SLOTS||!up_valid(&record))return check("independent preset fixture validates before reads",0);
 up_values(&record,v);
 for(unsigned i=0;i<P_COUNT;i++)up_set_value(&record,i,v[i]);
 if(!up_valid(&record))return check("canonical fixture remains valid",0);
 up_bank_t *bank=&up_bank[slot/UP_PER_BANK];bank->magic=UP_BANK_MAGIC;bank->rsize=sizeof record;bank->nslot=UP_PER_BANK;
 *up_rec(slot)=record;
 return check("independent valid preset fixture installed only after refusal proof",up_used(slot));
}
static unsigned native_preset_bytes(uint8_t*a,unsigned slot,unsigned engine,const char*name,int16_t*want) {
 unsigned k=0;a[k++]=slot;a[k++]=engine;while(*name)a[k++]=(uint8_t)*name++;a[k++]=0;
 for(unsigned i=0;i<P_COUNT;i++){int16_t v=param_desc_of(engine,i)->def;
  if(i==P_LEVEL)v=90;if(i==P_PAN)v=-20;if(i==P_ED_FLT)v=-40;
  if(i==P_LD_FLT)v=param_desc_of(engine,i)->min;if(i==P_E1)v=param_desc_of(engine,i)->max;
  if(want)want[i]=v;unsigned u=(unsigned)(v+8192);a[k++]=u&127;a[k++]=u>>7;
 }
 for(unsigned i=0;i<16;i++){a[k++]=i&1?0:48+i;a[k++]=0;}return k;
}
static int native_preferences(void) {
 int bad=0;uint8_t a[4]={0,7,0,0};reset();unsigned n=request(ED_INFO,a,0)-3;
 bad+=check("native INFO original tags report no full backup and retain discovery",host_wire[n-31]==CHAIN_ROWS&&host_wire[n-30]==0x55&&host_wire[n-29]==1&&host_wire[n-28]==ed_ui_caps()&&host_wire[n-23]==0x42&&host_wire[n-21]==0&&host_wire[n-4]==0x4c&&host_wire[n-2]==1);
 bad+=native_refuse(ED_UI_SET,a,2);a[1]=NPALETTES;bad+=native_refuse(ED_UI_SET,a,2);
 a[0]=1;a[1]=1;bad+=native_refuse(ED_UI_SET,a,2);a[0]=2;bad+=native_refuse(ED_UI_SET,a,2);
 a[0]=ENGI_DRUM;a[1]=0;a[2]=64;a[3]=1;bad+=native_refuse(ED_FAV_SET,a,4);
 favorite_set(ENGI_DRUM,0,1); /* Independent known favorite for the bounded read. */
 request(ED_FAV_GET,a,4);native_reads++;bad+=check("native FAV_GET valid independent favorite",host_wire[4]==ED_FAV_GET&&host_wire[10]==1);
 a[0]=NENGINES;a[1]=31;bad+=native_refuse(ED_FAV_SET,a,4);up_store(31,"Saved");bad+=native_refuse(ED_FAV_SET,a,4);
 a[3]=32;request(ED_FAV_GET,a,4);bad+=check("native favorite read cannot cross user slots",host_wire[4]==ED_FAV_GET&&host_wire[5]==1);
 bad+=native_refuse(ED_FAV_SET,a,3);bad+=check("native UI_STATE malformed read is silent",!request(ED_UI_STATE,a,1));
 a[0]=0;request(ED_SONG,a,1);bad+=check("native SONG bounded query retains original command",host_wire_n>6&&host_wire[4]==ED_SONG&&host_wire[5]==0);return bad;
}
static int native_motion_locks(void) {
 int bad=0;uint8_t a[8]={1,3,2,P_REV,(60+8192)&127,(60+8192)>>7};reset();motion_clear(&trk[1]);
 bad+=native_refuse(ED_MOTION,a,6);a[1]=5;a[2]=5;a[4]=(100+8192)&127;a[5]=(100+8192)>>7;bad+=native_refuse(ED_MOTION,a,6);
 motion_set_event(&trk[1],2,P_REV,60);motion_set_lock(&trk[1],5,P_REV,100); /* independent read fixture */
 unsigned n=request(ED_MOTION,a,1);bad+=check("native legacy MOTION read retains two independently seeded records",n==19&&host_wire[15]==P_REV);
 a[1]=7;n=request(ED_MOTION,a,2);bad+=check("native kind-aware MOTION read retains lock kind",n==21&&host_wire[19]==1);
 a[1]=5;a[2]=64;bad+=native_refuse(ED_MOTION,a,6);a[1]=6;a[2]=2;bad+=native_refuse(ED_MOTION,a,3);a[2]=127;bad+=native_refuse(ED_MOTION,a,3);
 uint8_t v[7]={1,1,5,63,P_REV,(100+8192)&127,(100+8192)>>7};n=request(ED_D8_MOTION,v,7);
 bad+=check("native schema1 MOTION really writes track/step lock",n&&host_wire[5]==1&&host_wire[7]==0&&motion_lock_get(&trk[1],63,P_REV,&(int16_t){0}));
 motion_store_t old=motion;v[0]=2;native_snapshot();request(ED_D8_MOTION,v,7);bad+=check("native unknown MOTION schema unchanged",native_error(ED_D8_MOTION,2)&&!memcmp(&old,&motion,sizeof old));
 v[0]=1;v[1]=NTRK;native_snapshot();request(ED_D8_MOTION,v,7);bad+=check("native out-of-range MOTION cannot alias",native_error(ED_D8_MOTION,3)&&!memcmp(&old,&motion,sizeof old));motion_clear(&trk[1]);return bad;
}
static int native_steps(void) {
 uint8_t a[80]={0,1,60,0,0,0,ST_NOTE,SF_ACCENT,100,0x12,2,1};int bad=0;reset();TSEL->step[0].hit=TSEL->step[0].acc=0x80;
 for(unsigned n=2;n<=sizeof a;n++)bad+=native_refuse(ED_STEP_SET,a,n);
 for(unsigned n=10;n<=15;n++){a[0]=1;a[1]=0;bad+=native_refuse(ED_TRACK_STEP,a,n);}
 for(unsigned r=0;r<8;r++){a[13]=(uint8_t)(r<4?0:5+r);bad+=native_refuse(ED_STEP_SET,a,14);}
 /* Exercise every admitted envelope length through the actual handler. */
 uint8_t v[80]={1,ED_TRACK_STEP,7,0,1,64,0,0,0,ST_NOTE,SF_ACCENT,99,0x12,2,1,80,3};
 step_t *st=&trk[7].step[0];st->hit=st->acc=0x80;
 unsigned n=request(ED_D8_TRACK,v,12);
 bad+=check("native note-only envelope preserves grid lane bytes",n==23&&st->note[0]==64&&st->hit==0x80&&st->acc==0x80&&song.sel==0);
 n=request(ED_D8_TRACK,v,15);
 bad+=check("native grid envelope preserves high lane bits and constrains accents",n==23&&st->hit==0x92&&st->acc==2);
 n=request(ED_D8_TRACK,v,17);
 bad+=check("native track8 chance/ratchet writes with explicit outer reply",n==23&&host_wire[4]==ED_D8_TRACK&&step_ratchet(st)==3&&step_chance(st)==80&&host_wire[n-2]==3&&host_wire[n-3]==80);
 v[10]=SF_SLIDE;n=request(ED_D8_TRACK,v,16);
 bad+=check("native chance-only envelope retains ratchet and slide",n==23&&step_ratchet(st)==3&&st->flags==(SF_SLIDE|(2u<<SF_RATCH_SH)));
 n=request(ED_D8_TRACK,v,12);
 bad+=check("native note-only envelope retains existing ratchet",n==23&&step_ratchet(st)==3);
 step_t before=*st;n=request(ED_D8_TRACK,v,4);
 bad+=check("native step read envelope preserves entire step",n==23&&!memcmp(&before,st,sizeof before));
 for(unsigned r=0;r<8;r++){
  v[16]=(uint8_t)(r<4?0:5+r);native_snapshot();request(ED_D8_TRACK,v,17);
  bad+=check("native invalid versioned ratchet rejects with exact bounds reason and state",native_error(ED_D8_TRACK,3)&&native_same());
 }
 v[16]=4;n=request(ED_D8_TRACK,v,17);bad+=check("native ratchet4 accepted",n==23&&step_ratchet(st)==4);
 v[16]=1;n=request(ED_D8_TRACK,v,17);bad+=check("native ratchet1 clears extra hits",n==23&&step_ratchet(st)==1&&!(st->flags&SF_RATCH));
 for(unsigned len=0;len<=sizeof v;len++)if(len!=4&&len!=12&&len!=15&&len!=16&&len!=17){
  native_snapshot();request(ED_D8_TRACK,v,len);
  bad+=check("native versioned partial/oversized step refuses exactly",native_error(ED_D8_TRACK,len?3:2)&&native_same());
 }

 return bad;
}
static int native_samples(void) {
 uint8_t a[640]={0},data[257],short_group[]={0,0,4,0,1};smp_user_hdr_t h;int bad=0;reset();memset(host_samples,0,sizeof host_samples);
 for(unsigned i=0;i<257;i++)data[i]=(uint8_t)(i*37);song.playing=1;host_progress=0;bad+=native_refuse(ED_SMP_BEGIN,a,1);host_progress=1;bad+=native_refuse(ED_SMP_BEGIN,a,1);
 a[2]=4;unsigned n=pack7(data,257,a+4)+4;bad+=native_refuse(ED_SMP_WRITE,a,n);bad+=native_refuse(ED_SMP_WRITE,short_group,sizeof short_group);
 n=pack7(data,256,a+4)+4;transport_req=1;bad+=native_refuse(ED_SMP_WRITE,a,n);
 memset(&h,0,sizeof h);h.magic=SMP_USER_MAGIC;h.version=1;h.nz=1;h.data_len=256;h.crc=st_crc32(data,256);h.zone[0].n=512;h.zone[0].le=511;h.zone[0].rate=65536;h.zone[0].hi=127;
 a[0]=0;n=pack7((uint8_t*)&h,sizeof h,a+1)+1;a[n]=a[n+1]=0;bad+=native_refuse(ED_SMP_END,a,n+2);bad+=native_refuse(ED_SMP_END,a,n);bad+=native_refuse(ED_SMP_END,a,n);
 h.zone[0].rate=131072;n=pack7((uint8_t*)&h,sizeof h,a+1)+1;bad+=native_refuse(ED_SMP_END,a,n);a[0]=a[1]=0;bad+=native_refuse(ED_UP_STORE,a,2);bad+=native_refuse(ED_UP_ERASE,a,1);return bad;
}
static int native_song_protocol(void) {
 uint8_t a[]={1,1,0,2};int bad=0;reset();bad+=native_refuse(ED_SONG,a,4);a[3]=0;bad+=native_refuse(ED_SONG,a,4);a[3]=2;chain.armed=1;bad+=native_refuse(ED_SONG,a,4);chain.armed=0;a[0]=2;bad+=native_refuse(ED_SONG,a,2);
 chain_config.count=1;chain_config.row[0].slot=0; /* independent query fixture, never failed-save data */
 a[0]=0;request(ED_SONG,a,1);bad+=check("native SONG read independently seeded chain",host_wire[4]==ED_SONG&&host_wire[5]==0);return bad;
}
static int native_malformed_saves(void) {
 int bad=0;reset();bad+=native_refuse(ED_PROJECT,(uint8_t[]){1,4},2);bad+=native_refuse(ED_UP_STORE,(uint8_t[]){0,'X',0,'Y'},4);bad+=native_refuse(ED_PROJECT,(uint8_t[]){1,0},2);
 TSEL->p[P_LEVEL]=21;song.playing=1;host_progress=0;bad+=native_refuse(ED_PROJECT,(uint8_t[]){1,0},2);return bad;
}
static void native_seed_fm6(unsigned track,const uint8_t*pk) {
 uint8_t patch[FP_SIZE+1];fm6_unpack(pk,patch);fm6_sanitize(patch);fm6_set_patch(track,patch);
 trk[track].engine=trk[track].eng_req=ENGI_FM6;trk[track].p[P_E7]=FM6_OWN;
 memcpy(fm6_own[track],pk,FM6_PACKED);fm6_own_ok|=(uint8_t)(1u<<track);fm6_slot[track]=FM6_OWN;
}
static int native_fm6_patches(void) {
 int bad=0;uint8_t a[2+FM6_PACKED],pk[FM6_PACKED];reset();upf_empty();a[0]=ED_FM6_FACTORY;a[1]=3;
 unsigned n=request(ED_FM6_GET,a,2);bad+=check("native factoryFM6 packed read",n==9+FM6_PACKED&&host_wire[7]==0&&!memcmp(host_wire+8,FM6_FACTORY[3],FM6_PACKED));
 memcpy(pk,FM6_FACTORY[3],FM6_PACKED);memcpy(pk+118,"MY PATCH  ",10);a[0]=ED_FM6_TRACK;a[1]=2;memcpy(a+2,pk,FM6_PACKED);
 bad+=native_refuse(ED_FM6_PUT,a,sizeof a);native_seed_fm6(2,pk);fm6_poll();bad+=check("independent nativeFM6 own fixture survives mainloop",!memcmp(fm6_patch[2]+FP_NAME,"MY PATCH  ",10));
 request(ED_FM6_GET,a,2);bad+=check("nativeFM6 track read independently seeded patch",host_wire[7]==0&&!memcmp(host_wire+8,pk,FM6_PACKED));
 memcpy(a+2,FM6_FACTORY[5],FM6_PACKED);bad+=native_refuse(ED_FM6_PUT,a,sizeof a);memcpy(a+2,pk,FM6_PACKED);bad+=native_refuse(ED_FM6_PUT,a,sizeof a);
 trk[2].p[P_E7]=1;fm6_poll();bad+=check("native device SLOT factory switch",!memcmp(fm6_patch[2]+FP_NAME,FM6_FACTORY[1]+118,10));
 trk[2].p[P_E7]=6;fm6_poll();trk[2].p[P_E7]=FM6_OWN;fm6_poll();bad+=check("native OWN fixture restored after factory switch",!memcmp(fm6_patch[2]+FP_NAME,"MY PATCH  ",10)&&fm6_slot[2]==FM6_OWN);
 trk[2].p[P_E7]=20;fm6_poll();bad+=check("native stale bank slot clamps toOWN",trk[2].p[P_E7]==FM6_OWN&&!memcmp(fm6_patch[2]+FP_NAME,"MY PATCH  ",10));
 a[0]=ED_FM6_BANK;a[1]=4;request(ED_FM6_GET,a,2);bad+=check("native retiredFM6bank read explicitlyabsent",host_wire[7]==3);bad+=native_refuse(ED_FM6_PUT,a,sizeof a);a[0]=4;bad+=native_refuse(ED_FM6_ERASE,a,1);
 n=request(ED_FM6_LIST,a,0);unsigned p=7,k,ok=host_wire[5]==FM6_NFACTORY&&host_wire[6]==0;
 for(k=0;k<FM6_NFACTORY&&p<n;k++){ok&=host_wire[p++]==1;while(p<n&&host_wire[p])p++;p++;}bad+=check("nativeFM6 factory list complete/no bank",ok&&k==FM6_NFACTORY&&p==n-1);
 song.sel=2;int stored=up_store(5,"MINE");bad+=check("independent validFM6 userfixture seeded",(stored==0||stored==3)&&up_used(5));
 a[0]=ED_FM6_USER;a[1]=5;request(ED_FM6_GET,a,2);bad+=check("nativeFM6 user read retained own patch",host_wire[7]==0&&!memcmp(host_wire+8+118,"MY PATCH  ",10));
 memcpy(a+2,pk,FM6_PACKED);memcpy(a+2+118,"EDITOR    ",10);a[16]=120;bad+=native_refuse(ED_FM6_PUT,a,sizeof a);
 uint8_t patch[FP_SIZE+1],normalized[FM6_PACKED];fm6_unpack(a+2,patch);fm6_sanitize(patch);fm6_pack(patch,normalized);upf_set(5,normalized); /* independent valid patch, not refused publication */
 request(ED_FM6_GET,a,2);bad+=check("nativeFM6 independent sanitized userpatch read",host_wire[7]==0&&host_wire[22]==99&&!memcmp(host_wire+126,"EDITOR    ",10));
 fm6_load_slot(2,0);up_load(5);bad+=check("native localUP_LOAD plays validated ownfixture",!memcmp(fm6_patch[2]+FP_NAME,"EDITOR    ",10)&&trk[2].p[P_E7]==FM6_OWN);
 up_rename(5,"RENAMED");request(ED_FM6_GET,a,2);bad+=check("nativeFM6 rename retains tag-bound patch",host_wire[7]==0&&!memcmp(host_wire+126,"EDITOR    ",10));
 trk[0].eng_req=ENGI_DRUM;song.sel=0;up_store(6,"KIT");a[1]=6;request(ED_FM6_GET,a,2);bad+=check("native nonFM6user read hasno patch",host_wire[7]==2);bad+=native_refuse(ED_FM6_PUT,a,sizeof a);
 a[1]=9;bad+=native_refuse(ED_FM6_PUT,a,sizeof a);a[1]=UP_SLOTS;request(ED_FM6_GET,a,2);bad+=check("nativeFM6 read bounds",host_wire[7]==1);
 up_put(5,0);a[1]=5;request(ED_FM6_GET,a,2);bad+=check("native erasedpreset read hasno patch",host_wire[7]==2);
 a[0]=ED_FM6_TRACK;a[1]=4;bad+=native_refuse(ED_FM6_PUT,a,sizeof a);bad+=native_refuse(ED_FM6_PUT,a,20);
 a[0]=4;a[1]=0;request(ED_FM6_GET,a,2);bad+=check("native unknownFM6target read bounded",host_wire[7]==1);return bad;
}
static int native_user_preset_roundtrip(void) {
 uint8_t a[16+2*P_COUNT+32];int16_t want[P_COUNT],got[P_COUNT];int bad=0;reset();unsigned k=native_preset_bytes(a,3,0,"ROUND",want);
 bad+=native_refuse(ED_UP_PUT,a,k);bad+=check("native refusedUP_PUT leaves slot unused",!up_used(3));
 bad+=native_seed_up(a,k);if(!up_used(3))return bad+1;up_values(up_rec(3),got);
 bad+=check("native validseed keeps negative PAN/filter andbounds",!memcmp(want,got,sizeof want));
 a[0]=3;unsigned n=request(ED_UP_GET,a,1);unsigned ok=n>14&&host_wire[4]==ED_UP_GET&&host_wire[5]==3&&host_wire[6]==1&&host_wire[7]==0&&!memcmp(host_wire+8,"ROUND",6),p=14;
 for(unsigned i=0;i<P_COUNT;i++,p+=2)ok&=ed_rv(host_wire+p)==want[i];for(unsigned i=0;i<16;i++,p+=2)ok&=host_wire[p]==(i&1?0:48+i);bad+=check("nativeUP_GET independentvalidrecord values/pattern/name",ok);
 k=native_preset_bytes(a,3,0,"OTHER",want);bad+=native_refuse(ED_UP_PUT,a,k); /* occupied slot also immutable */
 up_rec(3)->np=0;a[0]=3;request(ED_UP_GET,a,1);bad+=check("nativeUP_GET invalidnp0 is unused,no directinvalidreader",host_wire[4]==ED_UP_GET&&host_wire[6]==0);return bad;
}
static int native_live_sync(void) {
 int bad=0;uint8_t a[4]={1,0,0,0};const uint8_t*x=0;reset();usb.resets=0;request(ED_WATCH,a,1);bad+=check("nativeWATCH starts",host_wire[5]==1&&ed_w.on&&!sync_pass());
 TSEL->p[P_LEVEL]=77;request(ED_WATCH,a,1);bad+=check("native repeatedWATCH keeps pendingdevicechange",sync_pass()&&wire_frames(ED_CHANGED,&x)==1&&x[1]==P_LEVEL&&ed_rv(x+2)==77);
 TSEL->p[P_LEVEL]=66;a[0]=0;request(ED_WATCH,a,1);a[0]=1;request(ED_WATCH,a,1);bad+=check("nativeWATCH restart snapshotscurrentvalues",!sync_pass());
 TSEL->p[P_LEVEL]=55;trk[1].p[P_PAN]=3;a[0]=3;request(ED_WATCH,a,1);sync_pass();bad+=check("nativeWATCH3 preservespendingselectedchange",wire_frames(ED_CHANGED,0)==1&&!wire_frames(ED_TRACK_CHANGED,0)&&ed_w.v4);
 usb.resets++;TSEL->p[P_LEVEL]=44;request(ED_WATCH,a,1);bad+=check("nativeWATCH USBreset resnapshots",!sync_pass());
 TSEL->p[P_LEVEL]=33;a[0]=0;a[1]=2;bad+=native_refuse(ED_PRESET,a,2);sync_pass();bad+=check("native refusedPRESET preservespendingCHANGED,no reload",wire_frames(ED_CHANGED,&x)==1&&x[1]==P_LEVEL&&!wire_frames(ED_RELOAD,0)&&!sync_pass());
 for(unsigned i=0;i<3;i++){a[0]=i?12:4;a[1]=i;bad+=native_refuse(ED_PRESET,a,2);bad+=check("native refusedotherenginePRESET addsno syncpush",!sync_pass()&&ed_w.eng==ed_eng(TSEL));}
 a[0]=1;a[1]=G_ENGSEL;a[2]=(8192+5)&127;a[3]=(8192+5)>>7;bad+=native_refuse(ED_SET,a,4);bad+=check("native refusedengineSET addsno reload",!sync_pass());
 sync_reload=1;a[0]=0;a[1]=1;bad+=native_refuse(ED_PRESET,a,2);sync_pass();bad+=check("native preexistingdeviceRELOAD survivesrefusedPRESET",wire_frames(ED_RELOAD,0)==1);
 a[0]=0;request(ED_WATCH,a,1);sync_reload=0;a[0]=0;a[1]=3;bad+=native_refuse(ED_PRESET,a,2);a[0]=1;request(ED_WATCH,a,1);bad+=check("native notwatchingrefusedPRESET leaveswatchstartquiet",!sync_pass());return bad;
}
static int native_usb_reply(uint8_t*f,unsigned n,uint8_t*u) {
 native_snapshot();usb_feed(u,usb_frame(f,n,u));host_wire_n=0;ed_service();host_drain();return check("native fullUSB frame: exactBACKUP quarantine and unchangedmirror/flash",native_error(ED_BACKUP_PUT,1)&&native_same()&&!sx_ready);
}
static int native_usb_burst(void) {
 uint8_t f[700],u[1000],u2[2000];int bad=0;reset();usb.rx_pend=0;bad+=native_usb_reply(f,put_frame(f,0,0,0),u);unsigned max=0;
 for(unsigned off=0;off<sizeof(project_store_t);off+=256){unsigned c=sizeof(project_store_t)-off>256?256:sizeof(project_store_t)-off,n=put_frame(f,1,off,c);if(n>max)max=n;bad+=native_usb_reply(f,n,u);}
 bad+=check("native fullsizeUSB pieces all parsed,no backup session",!ed_bk_pos&&max==306&&max<=sizeof sx_frame);bad+=native_usb_reply(f,put_frame(f,3,0,0),u);
 bad+=native_usb_reply(f,put_frame(f,0,0,0),u);unsigned k=usb_frame(f,put_frame(f,1,0,256),u2);k+=usb_frame(f,put_frame(f,1,256,256),u2+k);
 native_snapshot();usb_feed(u2,k);host_wire_n=0;ed_service();ed_service();host_drain();const uint8_t*x=0;
 bad+=check("native pipelinedUSB firstframe refused,second droppedwhole",native_error(ED_BACKUP_PUT,1)&&native_same()&&!sx_ready);
 bad+=native_usb_reply(f,put_frame(f,1,512,256),u);bad+=native_refuse(ED_BACKUP_PUT,(uint8_t[]){3,2},2);return bad;
}
static int native_menu_writes(void) {
 int bad=0;reset();for(unsigned id=0;id<18;id++){uint8_t a[3]={id,1,64};bad+=native_refuse(ED_MENU_SET,a,3);}
 const int values[]={50,-5,99,8191,7,9,-1};const unsigned ids[]={0,0,4,5,2,14,12};for(unsigned i=0;i<7;i++){unsigned v=values[i]+8192;uint8_t a[3]={ids[i],v&127,v>>7};bad+=native_refuse(ED_MENU_SET,a,3);}
 for(unsigned id=18;id<=127;id+=id==18?108:1){uint8_t a[3]={id,1,64};bad+=native_refuse(ED_MENU_SET,a,3);}
 bad+=native_refuse(ED_MENU_SET,(uint8_t[]){0,0,0},0);bad+=native_refuse(ED_MENU_SET,(uint8_t[]){0,0,0},1);bad+=native_refuse(ED_MENU_SET,(uint8_t[]){0,0,0},2);
 ui.menu=1;ui.force=0;bad+=native_refuse(ED_MENU_SET,(uint8_t[]){7,0,64},3);bad+=check("native MENU refusal neverforces redraw",!ui.force&&ui.menu==1);ui.menu=0;
 usb_cdc_on=1;usb.config=1;ui_prefs&=~PREF_SERIAL_OFF;usb_serial_apply();bad+=native_refuse(ED_MENU_SET,(uint8_t[]){11,1,64},3);
 uint32_t t=fm1_ms;fm1_ms=t+201;usb_serial_apply();bad+=check("native refusedSERIALOFF neverreenumerates",usb_cdc_on&&usb.config);
 /* independent local-menu fixture retains delayed USB reenumeration coverage */
 ui_prefs|=PREF_SERIAL_OFF;menu_serial_at=fm1_ms|1u;usb_serial_apply();t=fm1_ms;fm1_ms=t+150;usb_serial_apply();bad+=check("native localSERIALOFF waits150ms",usb_cdc_on&&usb.config);
 fm1_ms=t+201;usb_serial_apply();bad+=check("native localSERIALOFF appliesafter200ms",!usb_cdc_on&&!usb.config);
 usb.config=1;bad+=native_refuse(ED_MENU_SET,(uint8_t[]){11,0,64},3);ui_prefs&=~PREF_SERIAL_OFF;t=fm1_ms;ui.menu=1;fm1_ms=t+500;bad+=check("native localSERIALON waitsforMENUclose",!usb_cdc_on&&usb.config);ui.menu=0;usb_serial_apply();
 bad+=check("native localSERIALON thenreenumerates",usb_cdc_on&&!usb.config);usb.config=1;fm1_ms+=300;usb_serial_apply();bad+=check("native unchangedserialmode keepsenumeration",usb_cdc_on&&usb.config);reset();return bad;
}
static int native_drum_kit_retired(void) {
 static const uint8_t map[9]={0,6,5,8,4,5,6,7,8};uint8_t a[16+2*P_COUNT+32];int16_t got[P_COUNT];int bad=0;reset();uint8_t set[4]={1,G_ENGSEL,(8192+ENGI_DRUM)&127,(8192+ENGI_DRUM)>>7};bad+=native_refuse(ED_SET,set,4);
 set_engine_of(TSEL,ENGI_DRUM);apply_preset_to(TSEL,0);TSEL->engine=TSEL->eng_req; /* independent validDRUM DESC fixture */
 a[0]=0;a[1]=P_E0;request(ED_DESC,a,2);const char*n=(char*)host_wire+14;static const char*const names[9]={"STD","66","10","77","80","10","66","55","77"};n+=strlen(n)+1;n+=strlen(n)+1;unsigned ok=1;
 for(unsigned i=0;i<9;i++,n+=strlen(n)+1)ok&=!strcmp(n,names[i]);bad+=check("nativeDRUM DESC preservedhistoricalalias labels",ok);
 for(unsigned r=1;r<4;r++){unsigned v=r+8192;uint8_t s[4]={0,P_E0,v&127,v>>7};bad+=native_refuse(ED_SET,s,4);}
 unsigned len=native_preset_bytes(a,5,ENGI_DRUM,"OLDKIT",0);unsigned offset=3+6+2*P_E0,v=8192+3;a[offset]=v&127;a[offset+1]=v>>7;
 bad+=native_refuse(ED_UP_PUT,a,len);bad+=native_seed_up(a,len);if(!up_used(5))return bad+1;up_values(up_rec(5),got);bad+=check("nativeindependentDRUMKIT3preset normalizes to77",got[P_E0]==8);
 static project_t p;for(unsigned r=0;r<NTRK;r++){set_engine_of(&trk[r],ENGI_DRUM);apply_preset_to(&trk[r],0);trk[r].engine=trk[r].eng_req;}project_capture(&p);
 for(unsigned r=0;r<NTRK;r++)p.t[r].p[P_E0]=r;p.sum=proj_sum(&p);ok=!project_restore_runtime(&p);for(unsigned r=0;r<NTRK;r++)ok&=trk[r].p[P_E0]==map[r];
 for(unsigned r=0;r<NTRK;r++)p.t[r].p[P_E0]=r;proj_bound(&p);for(unsigned r=0;r<NTRK;r++)ok&=p.t[r].p[P_E0]==map[r];bad+=check("nativeall8DRUM tracks preservealiasesandordinarykits without4entrytestarrayoverrun",ok);return bad;
}
