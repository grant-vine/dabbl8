/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual pre-migration application paths; simulated NOR, no grant/test bypass. */
#define NPART 8
#define FELUCCA_OTA 1
#define FELUCCA_VERSION "quarantine-test"
static unsigned char nor[0x100000],before[sizeof nor];
#define SMP_USER_XIP(k) (nor+0xa0000u+(k)*0x14000u)
#define main hostsim_main
#include "hostsim.c"
#undef main
static int32_t fm1_enc_take(uint32_t e){(void)e;return 0;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
static void fm1_wdt_feed(void){}
static void lcd_sync(void){}
static void lcd_power(uint32_t s){(void)s;}
static void lcd_wake_now(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){(void)x;(void)y;(void)w;(void)h;(void)p;}
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
static void panel_setup(void){}
static uint32_t reads,erases,programs,checks,failures;
static uint32_t irq_save(void){return 0;}
static void irq_restore(uint32_t f){(void)f;}
static int16_t abuf[512];
static unsigned audio_nonzero[2];
static int fl_read_ram(uint32_t a,void*p,uint32_t n){reads++;if(a>sizeof nor||n>sizeof nor-a)return -1;memcpy(p,nor+a,n);return 0;}
static int fl_erase4k_ram(uint32_t a,uint32_t*t){erases++;(void)t;memset(nor+a,255,4096);return 0;}
static int fl_write(uint32_t a,const void*p,uint32_t n){programs++;memcpy(nor+a,p,n);return 0;}
static int fl_erase4k(uint32_t a,uint32_t*t){return fl_erase4k_ram(a,t);}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;}
static uint32_t fl_jedec_ram(void){return 0x856014;}
static void fl_plain_window_init(void){}
#define FL_FAR(fn) (fn)
#define FL_STORE_OK(a,n) ((a)>=0x97000u&&(a)<=0xe0000u&&(n)<=0xe0000u-(a) || (a)>=0xfc000u&&(a)<=0xff000u&&(n)<=0xff000u-(a) || (a)>=0xe5000u&&(a)<=0xe7000u&&(n)<=0xe7000u-(a))
#include "../firmware/src/storage_hw.c"
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
static int audio_output_quiet(void){return 1;}
#include "../firmware/src/d8p1_flash_runtime.c"
#include "../firmware/src/project_native_flash.c"
#include "../firmware/src/native_autosave_session.c"
#include "../firmware/src/editor.c"
#define CHECK(x) do{checks++;if(!(x)){failures++;fprintf(stderr,"line%d: %s\n",__LINE__,#x);}}while(0)
/* Fixture injection writes bytes directly, never grants production permission. */
static void seed(unsigned o,const void*p,unsigned n){st_hdr_t h={0};h.magic=ST_MAGIC;h.type=o;h.slot=0;h.seq=1;h.len=n;h.crc=st_crc32(p,n);h.rsv[0]=h.rsv[1]=0xffffffff;h.hcrc=st_crc32(&h,28);unsigned a=st_sector(o,0);memcpy(nor+a,&h,32);memcpy(nor+a+256,p,n);}
static unsigned pack(const uint8_t*p,unsigned n,uint8_t*out){unsigned pos=0;while(n){unsigned k=n>7?7:n,m=0;for(unsigned j=0;j<k;j++)m|=(p[j]>>7)<<j;out[pos++]=m;for(unsigned j=0;j<k;j++)out[pos++]=p[j]&127;p+=k;n-=k;}return pos;}
/* Independent committed fixture builder. These callbacks are never wired to
 * application writers and cannot authorize them. */
static int fixture_read(void*c,uint32_t o,void*p,uint32_t n){(void)c;memcpy(p,nor+d8pool_mapped_address(o/D8POOL_BLOCK)+o%D8POOL_BLOCK,n);return 0;}
static int fixture_erase(void*c,uint32_t o){(void)c;memset(nor+d8pool_mapped_address(o/D8POOL_BLOCK)+o%D8POOL_BLOCK,255,D8POOL_SECTOR);return 0;}
static int fixture_program(void*c,uint32_t o,const void*p,uint32_t n){(void)c;memcpy(nor+d8pool_mapped_address(o/D8POOL_BLOCK)+o%D8POOL_BLOCK,p,n);return 0;}
static int fixture_stopped(void*c){(void)c;return 1;}
static void native_fixture(void){
 uint8_t wire[D8P1_LIMIT];FILE*f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");CHECK(f!=NULL);if(!f)exit(2);
 size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 for(unsigned b=0;b<D8POOL_BLOCKS;b++)memset(nor+d8pool_mapped_address(b),255,D8POOL_BLOCK);
 d8pool pool={NULL,fixture_read,fixture_erase,fixture_program,fixture_stopped};
 for(unsigned o=0;o<4;o++)CHECK(d8pool_save(&pool,o,wire,n,7)==D8POOL_OK);
 CHECK(d8p1_load_runtime(wire,n,0)==D8RT_OK);
}
int main(void){
 memset(nor,255,sizeof nor);flash_ok=1;panel=PANEL_DEFAULT;settings_init();host_tracks_init();
 up_bank_t bank={0};bank.magic=UP_BANK_MAGIC;bank.rsize=sizeof(up_rec_t);bank.nslot=UP_PER_BANK;
 up_rec_t*r=&bank.r[0];r->used=UP_USED;r->ver=UP_VER;r->engine=ENGI_FM6;r->np=P_COUNT;
 for(unsigned i=0;i<P_COUNT;i++)up_set_value(r,i,param_desc_of(ENGI_FM6,i)->def);
 up_set_value(r,P_E7,FM6_NFACTORY+2);memcpy(r->name,"ORIGINAL",8);
 fm6_bank_t old={0};old.magic=FM6_BANK_MAGIC;old.ver=1;old.nslot=FM6_BANK_N;old.used=1u<<2;memcpy(old.v[2],FM6_FACTORY[5],FM6_PACKED);
 seed(OBJ_UPRESET0,&bank,sizeof bank);seed(OBJ_FM6BANK,&old,sizeof old);memcpy(before,nor,sizeof nor);
 persist_boot();uint8_t pk[FM6_PACKED];CHECK(flash_ok&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 CHECK(!upf_get(0,pk)&&!memcmp(pk,old.v[2],sizeof pk));CHECK(st_current(OBJ_UPFM6,(st_hdr_t*)st_buf)<0);
 upf_boot();CHECK(!erases&&!programs&&!memcmp(nor,before,sizeof nor));CHECK(upf_save()==2&&!erases&&!programs);
 reads=0;uint8_t byte=1;for(unsigned o=0;o<OBJ_COUNT;o++)for(int to=-1;to<=1;to++)CHECK(st_save_to(o,&byte,1,to)!=0);
 CHECK(!reads&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 for(unsigned a=0x97000;a<0xe0000;a+=4096){CHECK(st_erase(a)==D8_INSTRUMENT_QUARANTINED);CHECK(st_prog(a,&byte,1)==D8_INSTRUMENT_QUARANTINED);}
 for(unsigned a=0xfc000;a<0xff000;a+=4096)CHECK(st_erase(a)==D8_INSTRUMENT_QUARANTINED);
 CHECK(st_erase(0xe5000)==D8_INSTRUMENT_QUARANTINED&&st_prog(0xe6000,&byte,1)==D8_INSTRUMENT_QUARANTINED);
 CHECK(st_erase(0)==-8&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 persist_t prior_settings=persist_saved;settings.lowcut=2;settings_save();CHECK(persist_pending==2&&!erases&&!programs&&!memcmp(&prior_settings,&persist_saved,sizeof prior_settings));
 up_rec_t prior=*up_rec(0);CHECK(up_put(0,NULL)==2&&!memcmp(&prior,up_rec(0),sizeof prior));
 unsigned nz=usr_nz[0];usr_nz[0]=1;usr_zone[0][0].n=99;CHECK(ed_smp_erase(0,1)==D8_INSTRUMENT_QUARANTINED&&usr_nz[0]==1&&usr_zone[0][0].n==99);usr_nz[0]=nz;
 smp_user_hdr_t h={0};h.magic=SMP_USER_MAGIC;h.version=1;h.nz=1;h.data_len=0;uint8_t packed[600];unsigned n=pack((const uint8_t*)&h,sizeof h,packed);
 CHECK(ed_smp_end(0,packed,n)==4&&!erases&&!programs);
 /* A real no-write idempotent END still acknowledges existing material. */
 memcpy(nor+SMP_USER_BASE,&h,sizeof h);usr_nz[0]=1;memcpy(before,nor,sizeof nor);
 CHECK(ed_smp_end(0,packed,n)==0&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 for(unsigned k=0;k<SMP_USER_SLOTS;k++){usr_nz[k]=1;((smp_user_hdr_t*)(nor+SMP_USER_BASE+k*SMP_USER_SIZE))->data_len=0;CHECK(slc_store_write(k)==2);}
 memcpy(before,nor,sizeof nor);CHECK(!erases&&!programs);
 slc_man_save=7;slc_store_poll();CHECK(!strcmp(ui.msg,"SAVE ERROR")&&!slc_man_save&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 /* Binding is not migration completion; even a direct native save caller
  * cannot bypass the actual guarded application hardware callbacks. */
 CHECK(!d8_instrument_write_allowed());CHECK(!erases&&!programs);
 CHECK(project_native_bind_flash(1)!=D8POOL_OK);CHECK(!d8_instrument_write_allowed());
 native_fixture();memcpy(before,nor,sizeof nor);transport_req=0;
 CHECK(project_native_bind_flash(0)!=D8POOL_OK);CHECK(!project_native_bind_flash(1));
 CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 flash_ok=0;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&!erases&&!programs);flash_ok=1;
 usb.config=1;uint8_t begin[]={1,1,1};ed_n=0;ed_capture_handle(begin,sizeof begin);
 CHECK(ed_capture.active&&edc_valid());CHECK(st_prog(0x97000,&byte,1)==D8_INSTRUMENT_QUARANTINED);
 CHECK(ed_capture.active&&edc_valid()&&!d8_capture_change.changed&&!erases&&!programs);
 CHECK(edc_read(8,0,256)==EDC_OK&&!memcmp(ed_smp_buf,nor+0x9f000,256));
 CHECK(edc_ram(16,0,256,0)==EDC_OK&&!memcmp(ed_smp_buf,&upf,256));
 CHECK(!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 uint8_t out[256];CHECK(!st_read(0x9f000,out,sizeof out)&&!memcmp(out,nor+0x9f000,sizeof out));
 CHECK(!erases&&!programs&&!memcmp(nor,before,sizeof nor));
 printf("Instrument write quarantine: %u checks, %u failures; actual boot/object/sample/slice guards, no grant or device writes\n",checks,failures);return failures!=0;
}
