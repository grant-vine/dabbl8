/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual dispatch/arena/capture/source/executor/adoption; private NOR only. */
#include <stdint.h>
static int mc_virtual_erase(uint32_t);
static int mc_virtual_program(uint32_t,const void*,uint32_t);
#define D8_MIGRATION_SESSION 1
#ifndef D8_MC_PRODUCTION_TEST
#define D8_MC_VIRTUAL_NOR_FIXTURE 1
#endif
#define D8_MC_VIRTUAL_ERASE mc_virtual_erase
#define D8_MC_VIRTUAL_PROGRAM mc_virtual_program
#define D8_MC_ENCODER_VALUE(i) host_enc[i]
#define D8_MC_PENDING_PRESS() (host_pressed||host_notes)
#define D8_SOURCE_EQ_NO_MAIN 1
static void mc_preflight_read_hook(void);
#define D8FLASH_EXTRA_READ_HOOK mc_preflight_read_hook
#include "native_source_equivalence_test.c"
#include "../firmware/src/native_migration_capture_binding.c"
#include "../firmware/src/native_migration_execute.c"
#include "../firmware/src/native_migration_session.c"
static unsigned physical_calls,partial_failure,callback_action;
static const char*mc_selected_fixture="tests/fixtures/projects/fun1.bin";
static const char*mc_archival_fixture;
static unsigned mc_selected_supported;
static int mc_virtual_erase(uint32_t a){CHECK(irq_disabled);physical_calls++;int rc=mutate();if(!rc)memset(nor+a,255,4096);else if(partial_failure){unsigned k=partial_failure;if(k>4096)k=4096;memset(nor+a,255,k);}if(callback_action==1){callback_action=0;d8_capture_store_changed();}else if(callback_action==2){callback_action=0;transport_req=1;}return rc;}
static int mc_virtual_program(uint32_t a,const void*p,uint32_t n){CHECK(irq_disabled);physical_calls++;int rc=mutate();unsigned k=rc?partial_failure:n;if(k>n)k=n;for(unsigned i=0;i<k;i++)nor[a+i]&=((const uint8_t*)p)[i];if(callback_action==1){callback_action=0;d8_capture_store_changed();}else if(callback_action==2){callback_action=0;transport_req=1;}return rc;}
static void mc_put32(uint8_t*p,uint32_t n){for(unsigned i=0;i<5;i++)p[i]=(n>>(i*7))&127;}
static unsigned mc_request(const uint8_t*p,unsigned n){uint8_t f[600]={0x7d,0x46,0x4c,79};CHECK(n<=sizeof f-4);memcpy(f+4,p,n);ed_handle(f,n+4);so_r=so_w;CHECK(ed_n==29&&ed_out[4]==79&&ed_out[5]==1&&ed_out[28]==0xf7);return ed_out[6];}
static unsigned mc_op(unsigned op){uint8_t a[12]={1,(uint8_t)op};mc_put32(a+2,d8mc.generation);mc_put32(a+7,d8mc.request+1);return mc_request(a,sizeof a);}
static void mc_setup(unsigned choice)
{
 d8cb_cancel();d8mx_cancel();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);cold();blank();flash_ok=1;native_as.ready=0;project_native_reset();ui_prefs&=~PREF_RESTORE_OFF;fm1_ms=0;
 FILE*f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");CHECK(f!=NULL);size_t n=fread(wire,1,sizeof wire,f);fclose(f);CHECK(!d8p1_load_runtime(wire,n,0));int32_t a[CTL*2];mix_block(a,CTL);
 for(unsigned t=0;t<8;t++){trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=0;trk[t].nheld=0;sl[t].w=sl[t].loop=0;}
 usb.config=1;usb.ota_req=usb.uboot_req=0;memset(&ed_capture,0,sizeof ed_capture);memset(&d8_capture_change,0,sizeof d8_capture_change);
 host_pressed=host_notes=0;memset(host_enc,0,sizeof host_enc);memset(host_enc_late,0,sizeof host_enc_late);
 for(unsigned i=0;i<sizeof nor;i++)nor[i]=(uint8_t)(i^(i>>9)^0xa5);
 for(unsigned r=0;r<5;r++)memset(nor+edc_roles[r].a,255,8192);
 n=sv_load(mc_selected_fixture);for(unsigned r=0;r<4;r++)sv_record(r,0,1,sv_raw,n);sv_record(4,0,1,sv_raw,n);
 memset(sv_plan,255,sizeof sv_plan);d8pool pool={NULL,sv_pr,sv_pe,sv_pp,stop};d8p1_legacy_report report;
 mc_selected_supported=d8p1_legacy_stage(&sv_state,&report,sv_raw,n,7);
 if(!mc_selected_supported){n=sv_load("tests/fixtures/projects/fun1.bin");CHECK(d8p1_legacy_stage(&sv_state,&report,sv_raw,n,7));}
 size_t z=0;CHECK(d8p1_project_encode(out,sizeof out,&z,&sv_state));for(unsigned r=0;r<3;r++)CHECK(!d8pool_save(&pool,r,out,z,7));
 if(choice==D8SV_CURRENT){memset(&sv_state,0,sizeof sv_state);project_capture(&sv_state.project);CHECK(d8p1_project_encode(out,sizeof out,&z,&sv_state));}CHECK(!d8pool_save(&pool,3,out,z,7));
 d8pool_index index;CHECK(!d8pool_inventory(&pool,&index));unsigned used=0;for(unsigned r=0;r<4;r++)used|=1u<<index.object[r].block;for(unsigned b=0;b<5;b++)if(!(used&(1u<<b)))memset(sv_plan+b*8192,255,8192);
 if(mc_archival_fixture){n=sv_load(mc_archival_fixture);sv_record(3,0,1,sv_raw,n);sv_record(4,0,1,sv_raw,n);}
 reset();physical_calls=partial_failure=callback_action=0;memcpy(baseline,nor,sizeof nor);CHECK(mc_quiet());
 begin_capture(1);CHECK(!status());scan();uint8_t bytes[256];for(unsigned role=0;role<17;role++)for(unsigned off=0;off<edc_length(role);off+=256){unsigned k=edc_length(role)-off;if(k>256)k=256;CHECK(reply_bytes(bytes,role,off,k));}
 op(5);CHECK(!status());scan();CHECK(ed_capture.phase==4);
}
static void mc_begin(unsigned resume,unsigned choice){uint8_t a[10]={1,1};mc_put32(a+2,ed_capture.token);a[7]=resume;a[8]=choice;a[9]=1;CHECK(mc_request(a,sizeof a)==MC_OK&&d8mc_edit.locked);}
static void mc_transfer(void){for(unsigned off=0;off<D8POOL_BYTES;off+=256){uint8_t a[312]={1,2};mc_put32(a+2,d8mc.generation);mc_put32(a+7,d8mc.request+1);mc_put32(a+12,off);a[17]=0;a[18]=2;unsigned pos=19;for(unsigned j=0;j<256;j+=7){unsigned k=256-j;if(k>7)k=7;unsigned mask=0;for(unsigned i=0;i<k;i++)mask|=(sv_plan[off+j+i]>>7)<<i;a[pos++]=mask;for(unsigned i=0;i<k;i++)a[pos++]=sv_plan[off+j+i]&127;}CHECK(mc_request(a,pos)==MC_OK);}}
static void mc_consent(void){fm1_in.buttons=1u<<panel.btn[B_SAVE];host_pressed=1u<<panel.btn[B_SAVE];ui_input();CHECK(d8mc.phase==MC_RELEASE_BUTTON);fm1_in.buttons=0;ui_input();}
static void mc_finish(void){unsigned steps=0;while(d8mc.phase!=MC_READY&&d8mc.phase!=MC_FAILED&&steps++<5000)d8mc_poll();CHECK(d8mc.phase==MC_READY&&!migration_owner&&!d8mc_edit.locked&&project_native_status()==1&&native_as.ready);}
static unsigned mc_preflight_note,mc_readback_note;
static uint32_t mc_preflight_delay;
static void mc_preflight_read_hook(void)
{
 if(mc_preflight_delay&&d8mc_edit.writer&&!d8mc.write_active){uint32_t delay=mc_preflight_delay;mc_preflight_delay=0;fm1_ms+=delay;}
 if(mc_readback_note&&d8mc_edit.writer&&d8mc.write_phase==3){
  mc_readback_note=0;fm1_in.notes=host_notes=1;ui_input();fm1_in.notes=0;
 }
 if(mc_preflight_note&&d8mc_edit.writer&&!d8mc.write_active){
  mc_preflight_note=0;fm1_in.notes=host_notes=1;ui_input();
  fm1_in.notes=0; /* Physical release before the next preflight check. */
 }
}
static unsigned mc_prewrite_input;
static void mc_inject_prewrite(void)
{
 CHECK(d8mc_edit.locked&&d8mc_edit.writer&&!d8mc.write_attempted);
 if(mc_prewrite_input==0){fm1_in.notes=host_notes=1;ui_input();}
 else {transport_req=1;events_block(CTL);}
#ifdef D8_MC_PRIOR_REVOCATION_DIAGNOSTIC
 /* Private regression witness of the prior whole-session input policy only;
  * not the final shipping implementation or an old-source equivalence claim. */
 d8mc_edit.revoked=1;
#endif
}
static void mc_prewrite_cases(void)
{
 for(unsigned input=0;input<2;input++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
  unsigned steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();mc_consent();mc_finish();
  memcpy(baseline,nor,sizeof nor);unsigned calls=physical_calls;uint32_t own=d8mc.own,epoch=d8_capture_change.epoch;
  uint32_t generation=d8mc.generation,token=d8mc.token,music=st_crc32(trk,sizeof trk);
  mc_prewrite_input=input;inject_at=0;irq_inject=mc_inject_prewrite;
  int rc=d8p1_save_flash(0,1);irq_inject=NULL;inject_at=-1;
  CHECK(rc==D8POOL_BUSY&&!d8mc.write_active&&!d8mc_edit.locked&&!d8mc_edit.writer);
  CHECK(d8mc.phase==MC_READY&&d8mc_session_valid()&&project_native_status()==1&&native_as.ready);
  CHECK(d8mc.generation==generation&&d8mc.token==token&&d8mc.own==own&&d8_capture_change.epoch==epoch);
  CHECK(!memcmp(nor,baseline,sizeof nor)&&physical_calls==calls&&music==st_crc32(trk,sizeof trk));
  CHECK(input?transport_req==1:fm1_in.notes==1);
  CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&physical_calls==calls);
  /* Explicit external note release / withdrawal of the queued transport
   * request in this fixture, after proving it was retained. No observer or
   * completed-session identity is reset to grant the next operation. */
  fm1_in.notes=0;transport_req=0;
#ifndef D8_MC_PRIOR_REVOCATION_DIAGNOSTIC
  CHECK(!d8p1_save_flash(0,1)&&physical_calls>calls&&d8mc_session_valid());
  CHECK(d8mc.generation==generation&&d8mc.token==token&&d8_capture_change.epoch==epoch+d8mc.own-own);
#endif
 }
}
static void mc_preflight_case(void)
{
 mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
 unsigned steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();mc_consent();mc_finish();
 memcpy(baseline,nor,sizeof nor);unsigned calls=physical_calls;uint32_t epoch=d8_capture_change.epoch,own=d8mc.own;
 uint32_t music=st_crc32(trk,sizeof trk);mc_preflight_note=1;
 CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&!mc_preflight_note&&d8mc_edit.operation_revoked);
 CHECK(d8mc.phase==MC_READY&&d8mc_session_valid()&&!d8mc.write_active&&physical_calls==calls);
 CHECK(!memcmp(nor,baseline,sizeof nor)&&music==st_crc32(trk,sizeof trk)&&epoch==d8_capture_change.epoch&&own==d8mc.own);
 CHECK(!d8p1_save_flash(0,1)&&physical_calls>calls&&d8mc_session_valid());
}
static void mc_preflight_deadlines(void)
{
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned kind=0;kind<3;kind++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
  unsigned steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();mc_consent();mc_finish();
  memcpy(baseline,nor,sizeof nor);unsigned calls=physical_calls;uint32_t epoch=d8_capture_change.epoch,own=d8mc.own;
  uint32_t generation=d8mc.generation,token=d8mc.token;fm1_ms=wrap?UINT32_MAX-31u:5000u;
  mc_preflight_delay=kind==0?MC_IDLE_MS+1u:kind==1?MC_HARD_MS+1u:100u;
  int rc=d8p1_save_flash(0,1);CHECK(!mc_preflight_delay);
  if(kind<2){
   CHECK(rc==D8POOL_BUSY&&physical_calls==calls&&!d8mc.write_active&&!d8mc_edit.locked);
   CHECK(!memcmp(nor,baseline,sizeof nor)&&d8_capture_change.epoch==epoch&&d8mc.own==own);
   CHECK(d8mc.phase==MC_READY&&d8mc_session_valid()&&d8mc.generation==generation&&d8mc.token==token);
   CHECK(!d8p1_save_flash(0,1)&&physical_calls>calls&&d8mc_session_valid());
  }else CHECK(!rc&&physical_calls>calls&&d8mc_session_valid());
 }
}
static void mc_test_le32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void mc_header_refusals(void)
{
 mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
 unsigned steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();mc_consent();mc_finish();
 memcpy(baseline,nor,sizeof nor);unsigned calls=physical_calls;uint32_t epoch=d8_capture_change.epoch;
 for(unsigned kind=0;kind<3;kind++){
  CHECK(!d8mc_native_begin(0));size_t n=0;CHECK(!d8p1_capture_runtime(main_workspace.d8p1.wire,D8P1_LIMIT,&n));
  /* Private malformed queued-header state: no simulated erase/program is
   * performed to set up this validator fixture. Shipping has no such setter. */
  d8mc.write_phase=2;d8mc.write_offset=256+(uint32_t)n;uint8_t h[32]={0};
  mc_test_le32(h,D8POOL_MAGIC);h[4]=1;h[5]=d8mc.write_block;mc_test_le32(h+8,0);mc_test_le32(h+12,d8mc.write_sequence);
  mc_test_le32(h+16,(uint32_t)n);mc_test_le32(h+20,d8p1_crc32(main_workspace.d8p1.wire,n));mc_test_le32(h+24,UINT32_MAX);
  if(kind<2)h[6+kind]=1;else mc_test_le32(h+24,0);mc_test_le32(h+28,d8p1_crc32(h,28));
  CHECK(d8mc_driver_program(d8pool_mapped_address(d8mc.write_block),h,32)==-10&&!d8mc.write_attempted);
  CHECK(d8mc_native_finish(D8POOL_INVALID)==D8POOL_INVALID&&d8mc_session_valid());
  CHECK(physical_calls==calls&&d8_capture_change.epoch==epoch&&!memcmp(nor,baseline,sizeof nor));
 }
}
static void mc_writer_attempt_cases(void)
{
 for(unsigned action=1;action<=3;action++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
  unsigned steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();mc_consent();mc_finish();
  unsigned calls=physical_calls;if(action==3)mc_readback_note=1;else callback_action=action;
  CHECK(d8p1_save_flash(0,1)!=D8POOL_OK&&physical_calls>calls&&d8mc.write_attempted);
  if(action<3)CHECK(physical_calls==calls+1);else CHECK(!mc_readback_note);
  CHECK(d8mc.phase==MC_FAILED&&project_native_status()==2&&!native_as.ready&&!d8mc_edit.locked);
  unsigned stopped=physical_calls;CHECK(d8p1_save_flash(0,1)==D8POOL_UNSUPPORTED&&physical_calls==stopped);
 }
}
static int mc_bridge(void)
{
 char line[2048];mc_setup(D8SV_CURRENT);op(6);CHECK(!ed_capture.active);
 while(fgets(line,sizeof line,stdin)){
  if(!strcmp(line,"#reboot\n")){
   d8cb_cancel();d8mx_cancel();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
   cold();project_native_reset();native_as.ready=0;usb.config=1;usb.ota_req=usb.uboot_req=0;flash_ok=1;
   memset(&d8_capture_change,0,sizeof d8_capture_change);memset(&ed_capture,0,sizeof ed_capture);reset();
   host_pressed=host_notes=0;memset(host_enc,0,sizeof host_enc);fm1_in.notes=fm1_in.buttons=0;
   int32_t quiet[CTL*2];mix_block(quiet,CTL);puts("ok");fflush(stdout);continue;
  }
  if(!strcmp(line,"#tick\n")){d8mc_poll();puts("ok");fflush(stdout);continue;}
  if(!strncmp(line,"#cut ",5)){cut=(int)strtol(line+5,NULL,10);puts("ok");fflush(stdout);continue;}
  if(!strcmp(line,"#consent\n")){mc_consent();d8mc_poll();puts("ok");fflush(stdout);continue;}
  if(!strcmp(line,"#stats\n")){printf("{\"phase\":%u,\"own\":%u,\"physical_calls\":%u,\"writes\":%u,\"nor_crc\":%u,\"production_ownership\":false}\n",d8mc.phase,d8mc.own,physical_calls,writes,st_crc32(nor,sizeof nor));fflush(stdout);continue;}
  if(!strncmp(line,"#time ",6)){fm1_ms+=(uint32_t)strtoul(line+6,NULL,10);puts("ok");fflush(stdout);continue;}
  uint8_t frame[600];unsigned n=0;char*p=line;int malformed=0;
  while(*p&&*p!='\n'){char*end;unsigned long value=strtoul(p,&end,16);if(end==p||value>255||n>=sizeof frame){malformed=1;break;}frame[n++]=(uint8_t)value;p=end;while(*p==' ')p++;}
  if(malformed||n<6||frame[0]!=0xf0||frame[n-1]!=0xf7)return 2;
  ed_handle(frame+1,n-2);so_r=so_w;
  for(unsigned i=0;i<ed_n;i++)printf("%s%02x",i?" ":"",ed_out[i]);puts("");fflush(stdout);
 }
 return failures?1:0;
}
int main(int argc,char**argv){
 if(argc==2&&!strcmp(argv[1],"--bridge"))return mc_bridge();
 if(argc==2&&!strcmp(argv[1],"--prewrite-only")){mc_prewrite_cases();
#ifndef D8_MC_PRIOR_REVOCATION_DIAGNOSTIC
 mc_writer_attempt_cases();mc_preflight_case();mc_preflight_deadlines();mc_header_refusals();
#endif
printf("Prewrite session identity: %u checks, %u failures\n",checks,failures);return failures!=0;}
#ifdef D8_MC_PRODUCTION_TEST
 mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
 while(d8mc.phase==MC_PROOF)d8mc_poll();CHECK(d8mc.phase==MC_CONSENT);mc_consent();d8mc_poll();
 CHECK(d8mc.phase==MC_FAILED&&d8mc.status==MC_QUARANTINED&&!physical_calls&&!writes&&!migration_owner&&!memcmp(nor,baseline,sizeof nor));
 printf("Connected shipping quarantine: %u checks, %u failures; zero RAM mutation callbacks\n",checks,failures);return failures!=0;
#endif
 for(unsigned choice=0;choice<2;choice++){mc_setup(choice);mc_begin(0,choice);mc_transfer();CHECK(!mc_op(3));unsigned n=0;while(d8mc.phase==MC_PROOF){CHECK(!mc_op(3));CHECK(++n<1000);}CHECK(d8mc.phase==MC_CONSENT&&!physical_calls);mc_consent();mc_finish();CHECK(physical_calls==180&&d8mc.own==180);for(unsigned b=0;b<5;b++)CHECK(!memcmp(nor+d8pool_mapped_address(b),sv_plan+b*8192,8192));
  for(unsigned i=0;i<sizeof nor;i++){int changed=0;for(unsigned b=0;b<5;b++)if(i>=d8pool_mapped_address(b)&&i<d8pool_mapped_address(b)+8192)changed=1;if(!changed)CHECK(nor[i]==baseline[i]);}
 }
 /* A real native session writer must use the selected shared spare and
  * ordered erase/body/header ticket, then a reboot destroys all permission. */
 trk[7].p[P_LEVEL]=111;unsigned old=physical_calls;
 CHECK(!d8p1_save_flash(0,1)&&physical_calls>old&&d8mc.phase==MC_READY);
 CHECK(d8mc_driver_erase(d8pool_mapped_address(0))==-10&&physical_calls>old);
 uint32_t native_signature=0;CHECK(!d8p1_signature_runtime(&native_signature));
 d8pool_index rotating;CHECK(!d8pool_inventory(&seed,&rotating));
 uint32_t sequence=rotating.object[0].sequence;CHECK(sequence==2);
 d8cb_cancel();d8mx_cancel();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);
 cold();project_native_reset();native_as.ready=0;usb.config=1;usb.ota_req=usb.uboot_req=0;memset(&d8_capture_change,0,sizeof d8_capture_change);memset(&ed_capture,0,sizeof ed_capture);reset();
 CHECK(project_native_status()==0&&d8p1_save_flash(0,1)==D8POOL_UNSUPPORTED);
 int32_t quiet[CTL*2];mix_block(quiet,CTL);CHECK(mc_quiet());
 begin_capture(1);CHECK(!status());scan();uint8_t retained_chunk[256];
 for(unsigned role=0;role<17;role++)for(unsigned off=0;off<edc_length(role);off+=256){unsigned k=edc_length(role)-off;if(k>256)k=256;CHECK(reply_bytes(retained_chunk,role,off,k));}
 op(5);CHECK(!status());scan();old=physical_calls;memcpy(baseline,nor,sizeof nor);
 mc_begin(1,D8SV_PERSISTED);unsigned progress=0;while(d8mc.phase==MC_RESUME_READ){d8mc_poll();CHECK(++progress<=160);}CHECK(d8mc.phase==MC_CONSENT);
 mc_consent();mc_finish();CHECK(physical_calls==old&&!memcmp(nor,baseline,sizeof nor));
 CHECK(!d8pool_inventory(&seed,&rotating)&&rotating.object[0].sequence==sequence);
 mc_prewrite_cases();mc_writer_attempt_cases();mc_preflight_case();mc_preflight_deadlines();mc_header_refusals();
 /* Replayed/empty/noncanonical input never refreshes an idle lease. */
 for(unsigned kind=0;kind<5;kind++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);uint32_t last=d8mc.last;
  uint8_t a[20]={1,2};mc_put32(a+2,d8mc.generation);mc_put32(a+7,d8mc.request+1);a[17]=1;
  if(kind==0)a[7]=0;else if(kind==1)a[2]=0;else if(kind==2)a[17]=0;else if(kind==3)a[19]=128;else a[6]=16;
  fm1_ms=last+100;CHECK(mc_request(a,sizeof a)!=MC_OK&&d8mc.last==last&&!writes);
  fm1_ms=last+MC_IDLE_MS+1;d8mc_poll();CHECK(d8mc.phase==MC_FAILED&&!migration_owner&&!writes&&!memcmp(nor,baseline,sizeof nor));
 }
 /* Actual main-loop and IRQ input gates refuse before changing music. */
 for(unsigned kind=0;kind<4;kind++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);uint32_t before=st_crc32(trk,sizeof trk);
  if(kind==0){host_notes=fm1_in.notes=1;ui_input();}else if(kind==1){host_enc[0]=1;ui_input();}else if(kind==2){transport_req=1;events_block(CTL);}else {midi_in_q[mi_w%MQ]=0x643c9009u;mi_w++;events_block(CTL);}
  CHECK(d8mc_edit.revoked&&before==st_crc32(trk,sizeof trk));d8mc_poll();CHECK(d8mc.phase==MC_FAILED&&!writes&&!memcmp(nor,baseline,sizeof nor));
 }
 /* Every immutable FUN fixture participates in the connected path. Actual
  * fourth-slot chain references refuse as selected targets, never remap. */
 const char*names[]={"fun1.bin","fun1-digital.bin","fun2.bin","fun3.bin","fun4.bin","fun4-phys-drum.bin","fun5.bin","fun6.bin","fun7.bin","fun8.bin","fun8-perc.bin","fun9.bin","fun9-digital.bin"};
 for(unsigned f=0;f<13;f++){
  char path[128];snprintf(path,sizeof path,"tests/fixtures/projects/%s",names[f]);mc_selected_fixture=path;mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));unsigned steps=0;
  while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();
  if(mc_selected_supported){CHECK(d8mc.phase==MC_CONSENT);mc_consent();mc_finish();}
  else CHECK(f>=7&&d8mc.phase==MC_FAILED&&!physical_calls&&!writes&&!memcmp(nor,baseline,sizeof nor));
  mc_selected_fixture="tests/fixtures/projects/fun1.bin";mc_archival_fixture=path;mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));steps=0;while(d8mc.phase==MC_PROOF&&steps++<1000)d8mc_poll();CHECK(d8mc.phase==MC_CONSENT);mc_consent();mc_finish();mc_archival_fixture=NULL;
 }
 /* Hard deadline is never renewed by forward chunks, STATUS or consent. */
 mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);
 for(unsigned step=0;step<8;step++){fm1_ms+=MC_IDLE_MS-1;uint32_t last=d8mc.last;CHECK(!mc_op(5)&&d8mc.last==last);uint8_t a[21]={1,2};mc_put32(a+2,d8mc.generation);mc_put32(a+7,d8mc.request+1);mc_put32(a+12,d8mp.received);a[17]=1;a[19]=sv_plan[d8mp.received]>>7;a[20]=sv_plan[d8mp.received]&127;CHECK(!mc_request(a,sizeof a));}
 fm1_ms=d8mc.started+MC_HARD_MS+1;d8mc_poll();CHECK(d8mc.phase==MC_FAILED&&!writes&&!migration_owner&&!memcmp(nor,baseline,sizeof nor));
 /* Actual SETCONFIG ABA, reset, sticky observer and flash loss revoke. */
 for(unsigned action=0;action<4;action++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);
  if(action==0){configuration(0);configuration(1);}else if(action==1)usb.resets++;else if(action==2)d8_capture_store_changed();else flash_ok=0;
  d8mc_poll();CHECK(d8mc.phase==MC_FAILED&&!writes&&!migration_owner&&!memcmp(nor,baseline,sizeof nor));
 }
 /* Admission must already be quiet; never stop playback or discard a held
  * shared voice, effect tail, DAC half or uncertain USB ownership. */
 for(unsigned action=0;action<12;action++){
  mc_setup(D8SV_CURRENT);uint32_t before=st_crc32(trk,sizeof trk);
  if(action<8)trk[action].v[0].active=1;
  else if(action==8)audio_nonzero[0]=1;else if(action==9)uq.uncertain=1;else if(action==10)transport_req=1;else host_pressed=1;
  uint8_t a[10]={1,1};mc_put32(a+2,ed_capture.token);a[8]=D8SV_CURRENT;a[9]=1;
  CHECK(mc_request(a,sizeof a)==MC_CHANGED&&!migration_owner&&!writes&&!memcmp(nor,baseline,sizeof nor));
  if(action>=8)CHECK(before==st_crc32(trk,sizeof trk));else CHECK(trk[action].v[0].active);
 }
 /* Every mutation-call cut, including partial erase/program, is either an
  * exactly readback-reconciled complete image or a sticky native offline state.
  * No destructive failure may silently fall back to the historical frontend. */
 for(unsigned failure=0;failure<180;failure++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));
  unsigned steps=0;while(d8mc.phase==MC_PROOF){d8mc_poll();CHECK(++steps<1000);}
  CHECK(d8mc.phase==MC_CONSENT);cut=(int)failure;partial_failure=failure%3==0?1:failure%3==1?128:0;
  mc_consent();steps=0;while(d8mc.phase!=MC_READY&&d8mc.phase!=MC_FAILED&&steps++<5000)d8mc_poll();
  if(d8mc.phase==MC_READY){for(unsigned b=0;b<5;b++)CHECK(!memcmp(nor+d8pool_mapped_address(b),sv_plan+b*8192,8192));}
  else CHECK(d8mc.attempted&&project_native_status()==2&&!native_as.ready&&!migration_owner);
  for(unsigned role=5;role<12;role++){CHECK(!memcmp(nor+edc_roles[role].a,baseline+edc_roles[role].a,edc_roles[role].n));if(edc_roles[role].m)CHECK(!memcmp(nor+edc_roles[role].b,baseline+edc_roles[role].b,edc_roles[role].m));}
 }
 for(unsigned action=1;action<=2;action++){
  mc_setup(D8SV_CURRENT);mc_begin(0,D8SV_CURRENT);mc_transfer();CHECK(!mc_op(3));while(d8mc.phase==MC_PROOF)d8mc_poll();
  callback_action=action;mc_consent();unsigned steps=0;while(d8mc.phase!=MC_FAILED&&d8mc.phase!=MC_READY&&steps++<5000)d8mc_poll();
  CHECK(d8mc.phase==MC_FAILED&&project_native_status()==2&&!native_as.ready&&physical_calls==1);
 }
 printf("Connected native migration session CUTS: %u checks, %u failures; private simulation only\n",checks,failures);return failures!=0;
}
