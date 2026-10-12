/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual shipping boot entry/frontend/runtime against private NOR, no device. */
#include <stdint.h>
static uint8_t flash_ok; /* actual legacy boot branch needs the later NOR fixture HAL declaration */
#define D8_NATIVE_READONLY_BOOT_TEST 1
#define D8_MC_PRODUCTION_TEST 1
#define D8_MIGRATION_SESSION_NO_MAIN 1
static void boot_read_hook(void);
static void boot_lcd_hook(void);
#define UI_LCD_SYNC_HOOK boot_lcd_hook
#define D8_BOOT_READ_HOOK boot_read_hook
#include "native_migration_session_test.c"
static int boot_inject=-1,boot_event,boot_lcd_late;
static void boot_lcd_hook(void)
{
 if(boot_lcd_late&&!--boot_lcd_late){if(boot_event==1)mi_w=mi_r+1;else if(boot_event==2)usb.ota_req=1;else transport_req=1;}
}
static void boot_read_hook(void)
{
 if(boot_inject==0){boot_inject=-1;if(boot_event==1)mi_w=mi_r+1;else if(boot_event==2)usb.ota_req=1;else transport_req=1;}
 else if(boot_inject>0)boot_inject--;
}
static project_t expected_boot;
static __typeof__(d8p1_runtime_cache) expected_cache;
static d8p1_project_state boot_state;
static void boot_seed(void)
{
 cold();blank();flash_ok=1;usb.config=0;usb.ota_req=usb.uboot_req=0;
 memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 project_native_reset();native_as.ready=0;
 FILE*f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");CHECK(f!=NULL);size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(!d8p1_load_runtime(wire,n,0));int32_t audio[CTL*2];mix_block(audio,CTL);
 for(unsigned t=0;t<8;t++){trk[t].p[P_LEVEL]=(int16_t)(20+t);trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=0;trk[t].nheld=0;sl[t].w=sl[t].loop=0;}
 strcpy(proj_name,"BOOT EIGHT");proj_cur=PROJ_NO_SLOT;project_capture(&expected_boot);n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));expected_cache=d8p1_runtime_cache;
 cold();project_native_reset();native_as.ready=0;memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 memset(&d8_capture_change,0,sizeof d8_capture_change);flash_ok=1;usb.config=0;usb.ota_req=usb.uboot_req=0;
 ui_prefs&=~PREF_RESTORE_OFF;mix_block(audio,CTL);reset();physical_calls=0;memcpy(baseline,nor,sizeof nor);
}
static unsigned boot_raw_mutations;
static int boot_raw_erase(void *c,uint32_t a)
{(void)c;boot_raw_mutations++;nor[a]=0;return 0;}
static int boot_raw_program(void *c,uint32_t a,const void *p,uint32_t n)
{(void)c;(void)p;(void)n;boot_raw_mutations++;nor[a]=0;return 0;}
static void boot_preserved(void)
{
 project_t before,after;project_capture(&before);int rc=autosave_boot(1);project_capture(&after);
 CHECK(rc==0&&project_native_owns_storage()&&!memcmp(&before,&after,sizeof before));
 CHECK(!writes&&!physical_calls&&!memcmp(nor,baseline,sizeof nor)&&!native_as.ready&&!mc_write_enabled());
}
int main(void)
{
 boot_seed();d8pool_mapped readonly;
 d8pool_physical raw={NULL,df_read,boot_raw_erase,boot_raw_program,pnr_stopped};
 CHECK(!d8pool_mapped_open_readonly(&readonly,&raw)&&readonly.state==1);
 uint8_t byte=0;CHECK(readonly.pool.erase(readonly.pool.context,0)!=0&&readonly.pool.program(readonly.pool.context,0,&byte,1)!=0);
 CHECK(!boot_raw_mutations&&!memcmp(nor,baseline,sizeof nor));d8pool_mapped_close(&readonly);
 memset(&d8p1_runtime_cache,0xa5,sizeof d8p1_runtime_cache);d8p1_runtime_cache.valid=1;chain.native_mode=2;chain.native.policy.valid=1;
 int rc=autosave_boot(1);project_t got;project_capture(&got);
 CHECK(rc==1&&project_native_status()==1&&project_native_owns_storage());
 CHECK(!memcmp(&got,&expected_boot,sizeof got)&&trk[7].p[P_LEVEL]==27&&!strcmp(proj_name,"BOOT EIGHT"));
 CHECK(!memcmp(&d8p1_runtime_cache,&expected_cache,sizeof expected_cache)&&!chain.native.policy.valid);
 CHECK(!native_as.ready&&!mc_write_enabled()&&!d8mc_session_valid());
 CHECK(!writes&&!physical_calls&&!memcmp(nor,baseline,sizeof nor));
 char name[13];CHECK(project_name(2,name)&&!strcmp(name,"BOOT EIGHT"));
 unsigned before_readonly=reads;
 CHECK(project_save_as(0,"DENIED")==1&&msg_is("READ ONLY")&&reads==before_readonly&&!writes&&!physical_calls);
 CHECK(project_rename(0,"DENIED")==1&&msg_is("READ ONLY")&&reads==before_readonly&&!writes&&!physical_calls&&!strcmp(proj_name,"BOOT EIGHT"));
 /* A boot preference or failed prior boot keeps defaults, but recognizes catalog. */
 boot_seed();project_t before,after;project_capture(&before);memset(&d8p1_runtime_cache,0x5a,sizeof d8p1_runtime_cache);d8p1_runtime_cache.valid=1;__typeof__(d8p1_runtime_cache) cache_before=d8p1_runtime_cache;ui_prefs|=PREF_RESTORE_OFF;
 CHECK(!autosave_boot(1)&&project_native_status()==1);project_capture(&after);CHECK(!memcmp(&before,&after,sizeof before)&&!memcmp(&cache_before,&d8p1_runtime_cache,sizeof cache_before));
 boot_seed();project_capture(&before);CHECK(!autosave_boot(0)&&project_native_status()==1);project_capture(&after);CHECK(!memcmp(&before,&after,sizeof before));
 /* Ownership is latched even when startup activity prevents reads/adoption. */
 for(unsigned event=1;event<=3;event++) {
   boot_seed();boot_event=(int)event;boot_inject=1;boot_preserved();
   CHECK(project_native_status()==2);CHECK(event==1?mi_w!=mi_r:event==2?usb.ota_req==1:transport_req==1);mi_w=mi_r;usb.ota_req=0;transport_req=0;
 }
 for(unsigned event=1;event<=3;event++) {
   boot_seed();boot_event=(int)event;boot_lcd_late=3;boot_preserved();
   CHECK(boot_lcd_late==0);CHECK(event==1?mi_w!=mi_r:event==2?usb.ota_req==1:transport_req==1);mi_w=mi_r;usb.ota_req=0;transport_req=0;
 }
 boot_seed();cv_cpu_active=1;boot_preserved();CHECK(project_native_status()==2);cv_cpu_active=0;
 boot_seed();trk_note_on(&trk[0],60,100);CHECK(trk[0].v[0].active);boot_preserved();CHECK(trk[0].v[0].active);
 /* Partial native and unknown foreign bytes must not take legacy fallback. */
 boot_seed();blank();nor[0x98000]=0x42;memcpy(baseline,nor,sizeof nor);boot_preserved();CHECK(project_native_status()==2);
 boot_seed();d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix));nor[d8pool_mapped_address(ix.object[3].block)+D8POOL_HEADER]^=1;memcpy(baseline,nor,sizeof nor);boot_preserved();CHECK(project_native_status()!=0);
 /* Structurally valid stored references still require the actual live set. */
 boot_seed();size_t n=0;CHECK(!d8pool_load(&seed,3,out,sizeof out,&n,7));
 CHECK(d8p1_project_decode(&boot_state,out,n,7));boot_state.arrangement.banks=1;
 strcpy(boot_state.arrangement.bank[0].name,"REF");
 for(unsigned t=0;t<8;t++){boot_state.arrangement.bank[0].project[t]=1;boot_state.arrangement.bank[0].track[t]=(uint8_t)t;}
 CHECK(d8p1_project_encode(wire,sizeof wire,&n,&boot_state));blank();
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));
 CHECK(!d8pool_inventory(&seed,&ix));memset(nor+d8pool_mapped_address(ix.object[1].block),255,D8POOL_BLOCK);
 reset();memcpy(baseline,nor,sizeof nor);boot_preserved();CHECK(project_native_status()==2);
#if !FELUCCA_SLICE
 /* Canonical SLICE remains valid stored data when that renderer is absent. */
 boot_seed();n=0;CHECK(!d8pool_load(&seed,3,out,sizeof out,&n,7));CHECK(d8p1_project_decode(&boot_state,out,n,7));
 boot_state.project.t[7].engine=13;CHECK(d8p1_project_encode(wire,sizeof wire,&n,&boot_state));blank();
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));
 reset();memcpy(baseline,nor,sizeof nor);boot_preserved();
#endif
 /* A real frozen FUN6 storage record is classified historical, not native. */
 boot_seed();blank();FILE *legacy=fopen("tests/fixtures/projects/fun6.bin","rb");CHECK(legacy!=NULL);
 n=fread(out,1,sizeof out,legacy);CHECK(!ferror(legacy)&&fgetc(legacy)==EOF&&n<=ST_PAYLOAD_MAX);fclose(legacy);
 st_hdr_t h={0};h.magic=ST_MAGIC;h.type=OBJ_AUTOSAVE;h.slot=0;h.seq=1;h.len=(uint32_t)n;h.crc=st_crc32(out,(uint32_t)n);h.hcrc=st_crc32(&h,28);
 memcpy(nor+0xe5000,&h,sizeof h);memcpy(nor+0xe5000+ST_PAYLOAD_OFF,out,n);reset();memcpy(baseline,nor,sizeof nor);
 CHECK(!project_native_boot_readonly(1)&&!project_native_owns_storage()&&!writes&&!memcmp(nor,baseline,sizeof nor));
 CHECK(autosave_boot(1)==1&&!project_native_owns_storage()&&!writes&&!memcmp(nor,baseline,sizeof nor));
 /* Storage-valid but musically malformed bytes reach the actual fallback
  * decoder and are refused without publication. */
 nor[0xe5000+ST_PAYLOAD_OFF]=0;h.crc=st_crc32(nor+0xe5000+ST_PAYLOAD_OFF,h.len);h.hcrc=st_crc32(&h,28);memcpy(nor+0xe5000,&h,sizeof h);
 reset();memcpy(baseline,nor,sizeof nor);project_capture(&before);CHECK(!autosave_boot(1)&&!project_native_owns_storage());project_capture(&after);CHECK(!memcmp(&before,&after,sizeof before)&&!writes&&!memcmp(nor,baseline,sizeof nor));
 /* Even with a valid old first sector, foreign staged bytes in the second
  * sector must suppress that route. */
 memcpy(nor+0xe5000+ST_PAYLOAD_OFF,out,n);h.crc=st_crc32(out,(uint32_t)n);h.hcrc=st_crc32(&h,28);memcpy(nor+0xe5000,&h,sizeof h);
 nor[0xe6000+300]=0x42;memcpy(baseline,nor,sizeof nor);boot_preserved();CHECK(project_native_status()==2);
 /* No inferred native grant from a blank pool. */
 boot_seed();blank();memcpy(baseline,nor,sizeof nor);CHECK(!autosave_boot(1)&&!project_native_owns_storage()&&!writes);
 printf("Native readonly boot: %u checks, %u failures; actual autosave_boot entry, zero native authority\n",checks,failures);return failures!=0;
}
