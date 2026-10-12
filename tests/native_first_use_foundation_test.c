/* SPDX-License-Identifier: GPL-3.0-only */
#define D8_PRESET_SESSION 1
#define D8_UP_ENCODER_VALUE(i) host_enc[i]
#include <stdint.h>
static int foundation_result(int,uint32_t,uint32_t);
#define D8_FI_TEST_RESULT(rc,a,n) foundation_result(rc,a,n)
#define D8_FIRST_USE_SESSION_NO_MAIN 1
#include "native_first_use_session_test.c"
/* The included session fixture already supplies preset/settings coordinators. */
static unsigned after_cut,wire_disturb;
static int foundation_result(int rc,uint32_t a,uint32_t n)
{
 (void)a;(void)n;
 if(!rc&&wire_disturb&&d8fi.format.phase==3&&physical_calls==2){wire_disturb=0;main_workspace.migration.stage.wire[200]^=1;}
 if(!rc&&after_cut&&physical_calls==after_cut)return -1;
 return rc;
}
static void foundation_start(int foreign)
{
 after_cut=wire_disturb=0;cold();project_native_reset();native_as.ready=0;d8ls_reset();memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);memset(&d8up,0,sizeof d8up);
 memset(&d8_capture_change,0,sizeof d8_capture_change);usb.config=0;usb.ota_req=usb.uboot_req=0;flash_ok=1;
 if(foreign)memset(nor,0xa5,sizeof nor);reset();physical_calls=0;host_pressed=host_notes=0;fm1_in.buttons=fm1_in.notes=0;fm1_ms=0;
 /* Poison cached settings/banks: default export must not preserve them. */
 favorites.filter=1;favorites.user=7;settings.palette=(UI_GREY_INDEX+1)%NPALETTES;settings.lowcut=2;settings_hold=3;settings_leds=LEDS_INV;
 memset(up_bank,0xa5,sizeof up_bank);memset(&upf,0xa5,sizeof upf);memset(&persist_saved,0xa5,sizeof persist_saved);
 int32_t audio[CTL*2];mix_block(audio,CTL);CHECK(d8fi_boot_classify());d8fi_draw();CHECK(d8fi.shown);
 uint32_t save=1u<<panel.btn[B_SAVE];host_pressed=save;fm1_in.buttons=save;ui_input();CHECK(d8fi.phase==FI_RELEASE);
 host_pressed=0;fm1_in.buttons=0;ui_input();unsigned steps=0;
 while(d8fi.phase==FI_RELEASE||(d8fi.phase==FI_EXECUTE&&d8fi.format.phase!=3)){d8fi_poll();CHECK(++steps<2000);if(steps>=2000)break;}
 CHECK(d8fi.phase==FI_EXECUTE&&d8fi.format.phase==3&&!d8fi.foundation);
 reset();physical_calls=0;
}
static void foundation_finish(void)
{
 unsigned steps=0;while(d8fi.phase==FI_EXECUTE&&steps++<30)d8fi_poll();CHECK(steps<30);
}
static void foundation_records(void)
{
 st_hdr_t h;const unsigned objects[]={OBJ_UPRESET0,OBJ_UPRESET0+1,OBJ_UPFM6,OBJ_SETTINGS};
 for(unsigned i=0;i<4;i++)CHECK(st_current(objects[i],&h)==0&&h.seq==1&&h.slot==0);
 CHECK(st_load(OBJ_SETTINGS,&persist_saved,sizeof persist_saved)==sizeof persist_saved&&ds_payload(&persist_saved,sizeof persist_saved));
 CHECK(persist_saved.palette==palette_to_stored(UI_DEFAULT_INDEX)&&!persist_saved.lowcut&&!persist_saved.favorites.filter&&!persist_saved.favorites.user);
 CHECK(!memcmp(&persist_saved.panel,&PANEL_DEFAULT,sizeof PANEL_DEFAULT));
 CHECK(st_load(OBJ_UPRESET0,&up_bank[0],sizeof up_bank[0])==sizeof up_bank[0]);
 CHECK(st_load(OBJ_UPRESET0+1,&up_bank[1],sizeof up_bank[1])==sizeof up_bank[1]);
 CHECK(!up_used(0)&&!up_used(31));CHECK(st_load(OBJ_UPFM6,&upf,sizeof upf)==sizeof upf&&upf_valid(&upf));
 d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix)&&ix.present==1&&d8fi.phase==FI_IDLE);
}
int main(void)
{
 foundation_start(1);foundation_finish();CHECK(d8fi.phase==FI_IDLE);unsigned operations=physical_calls;foundation_records();
 /* Cut only changed foundation/native writes; unchanged78-sector cuts reuse
  * existing pure-helper evidence. Reboots run the real disclosed formatter. */
 for(unsigned c=0;c<operations;c++) {
  foundation_start(1);cut=(int)c;foundation_finish();CHECK(d8fi.phase==FI_FAILED&&!migration_owner);
  cut=-1;partial_failure=0;d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix)&&!ix.present);
  foundation_start(0);foundation_finish();CHECK(d8fi.phase==FI_IDLE);foundation_records();
 }
 for(unsigned c=1;c<=operations;c++) {
  foundation_start(1);after_cut=c;foundation_finish();CHECK(d8fi.phase==FI_FAILED&&!migration_owner);
  after_cut=0;d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix));
  if(c==operations){
   CHECK(ix.present==1);memcpy(baseline,nor,sizeof nor);cold();memset(&d8mc,0,sizeof d8mc);
   CHECK(!d8fi_boot_classify()&&!memcmp(nor,baseline,sizeof nor));
  }else{
   CHECK(!ix.present);foundation_start(0);foundation_finish();foundation_records();
  }
 }
 foundation_start(1);wire_disturb=1;foundation_finish();
 CHECK(d8fi.phase==FI_FAILED&&physical_calls==2&&!migration_owner);
 d8pool_index disturbed;CHECK(!d8pool_inventory(&seed,&disturbed)&&!disturbed.present);
 foundation_start(0);foundation_finish();foundation_records();
 foundation_start(1);foundation_finish();foundation_records();memcpy(baseline,nor,sizeof nor);
 FILE *snapshot=fopen("build/foundation/initialized.nor","wb");CHECK(snapshot!=NULL);if(snapshot){CHECK(fwrite(nor,1,sizeof nor,snapshot)==sizeof nor);fclose(snapshot);}
 unsigned setting_ops=1+(sizeof(persist_t)+255)/256+1;
 for(unsigned c=0;c<setting_ops;c++) {
  memcpy(nor,baseline,sizeof nor);reset();physical_calls=0;cut=-1;partial_failure=0;
  persist_t p;CHECK(st_load(OBJ_SETTINGS,&p,sizeof p)==sizeof p);CHECK(settings_import(&p,sizeof p)==1);persist_saved=p;persist_pending=0;
  settings.palette=(UI_GREY_INDEX+1)%NPALETTES;cut=(int)c;settings_save();CHECK(persist_pending==2&&!d8up.active);
  cut=-1;CHECK(st_load(OBJ_SETTINGS,&p,sizeof p)==sizeof p&&p.palette==palette_to_stored(UI_DEFAULT_INDEX));
 }
 CHECK(!d8_instrument_write_allowed());
 printf("First-use foundation: %u checks, %u failures; %u before + %u after foundation/native cuts+reboots, %u first settings cuts; host context %zu B; virtual NOR only\n",checks,failures,operations,operations,setting_ops,sizeof d8fi);return failures!=0;
}
