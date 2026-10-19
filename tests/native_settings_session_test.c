/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#define D8_PRESET_SESSION 1
#define D8_SETTINGS_SESSION_TEST 1
#define D8_UP_ENCODER_VALUE(i) host_enc[i]
#define D8_NATIVE_LOCAL_SAVE_NO_MAIN 1
#include "native_local_save_test.c"
static int settings_test_erase(uint32_t a,uint32_t*t){(void)t;return mc_virtual_erase(a);}
static int settings_test_program(uint32_t a,const void*p,uint32_t n,uint32_t*t){(void)t;return mc_virtual_program(a,p,n);}
#define D8_UP_ERASE(a,t) settings_test_erase(a,t)
#define D8_UP_PROGRAM(a,p,n,t) settings_test_program(a,p,n,t)
#include "../firmware/src/native_preset_session.c"
#include "../firmware/src/native_settings_session.c"
static void settings_seed(void)
{
 boot_seed();CHECK(autosave_boot(1)==1&&project_native_status()==1);memset(nor+0xfc000,255,8192);
 memset(&persist_saved,0,sizeof persist_saved);persist_pending=0;memset(&d8up,0,sizeof d8up);
 memset(&favorites,0,sizeof favorites);settings.magic=SETTINGS_MAGIC;settings.palette=UI_GREY_INDEX;settings.lowcut=settings.zoom=0;panel=PANEL_DEFAULT;
 settings_leds=LEDS_DIM;settings_hold=HOLD_DEF;int32_t audio[CTL*2];mix_block(audio,CTL);reset();physical_calls=0;
}
static void settings_reboot(void)
{
 persist_t p;CHECK(st_load(OBJ_SETTINGS,&p,sizeof p)==sizeof p);settings.palette=0;panel.magic=0;
 CHECK(settings_import(&p,sizeof p)==1);persist_saved=p;settings_init();panel_init();
}
/* #197: real UI detents coalesce before the existing quiet-gated settings writer. */
static void deferred_favourites(void)
{
 settings_seed();settings_save();go_page(GR_BROWSE);
 unsigned calls=physical_calls;
 turn(EN_K4,1);CHECK(favorites.filter==1&&physical_calls==calls);
 fm1_ms+=10001;ui_input();CHECK(physical_calls>calls&&!persist_pending);
 calls=physical_calls;song.playing=1;turn(EN_K3,1);go_page(GR_FX);ui_input();
 CHECK(physical_calls==calls&&persist_pending);song.playing=0;settings_poll();
 CHECK(physical_calls>calls&&!persist_pending);
}
int main(void)
{
 deferred_favourites();
 /* Both appended brand IDs must retain their meaning in own-format settings.
  * Exercise the real serializer, importer and private virtual-NOR writer;
  * this does not claim full-device backup or physical recovery. */
 const unsigned brand_palettes[]={UI_DABBL_DARK_INDEX,UI_DABBL_LIGHT_INDEX};
 for(unsigned i=0;i<2;i++) {
  settings_seed();settings.palette=brand_palettes[i];settings.lowcut=2;
  favorites.user=4;settings_hold=2;settings_leds=LEDS_INV;
  persist_t exported=persist_saved;settings_export(&exported);
  CHECK(exported.palette==palette_to_stored(brand_palettes[i]));
  persist_t original=exported;settings.palette=UI_GREY_INDEX;
  CHECK(settings_import(&exported,sizeof exported)==1);
  CHECK(settings.palette==brand_palettes[i]&&!memcmp(&original,&exported,sizeof exported));
  settings_save();CHECK(!persist_pending&&physical_calls>0);
  settings_reboot();CHECK(settings.palette==brand_palettes[i]&&settings.lowcut==2&&favorites.user==4&&settings_hold==2&&settings_leds==LEDS_INV);
  unsigned before=physical_calls;settings_save();CHECK(!persist_pending&&physical_calls==before);
 }
 settings_seed();unsigned calls=physical_calls;settings_poll();CHECK(!physical_calls&&!persist_pending); /* normalization alone never saves */
 settings.palette=(UI_GREY_INDEX+1)%NPALETTES;settings.lowcut=2;favorites.filter=1;favorites.user=4;settings_hold=2;settings_leds=LEDS_INV;
 memcpy(baseline,nor,sizeof nor);settings_save();CHECK(!persist_pending&&physical_calls>calls&&!d8up.active);
 CHECK(!memcmp(nor,baseline,0xfc000)&&!memcmp(nor+0xfe000,baseline+0xfe000,sizeof nor-0xfe000));
 unsigned palette=settings.palette;settings_reboot();CHECK(settings.palette==palette&&settings.lowcut==2&&favorites.filter==1&&favorites.user==4&&settings_hold==2&&settings_leds==LEDS_INV);
 calls=physical_calls;settings_save();CHECK(!persist_pending&&physical_calls==calls); /* actual unchanged request */
 settings.palette=(palette+1)%NPALETTES;song.playing=1;settings_save();CHECK(persist_pending==1&&physical_calls==calls);
 song.playing=0;int32_t audio[CTL*2];mix_block(audio,CTL);settings_poll();CHECK(!persist_pending&&physical_calls>calls);
 /* Current supported settings plus future alternative must not be erased. */
 st_hdr_t h;int copy=st_current(OBJ_SETTINGS,&h);CHECK(copy>=0);uint32_t other=st_sector(OBJ_SETTINGS,(unsigned)(1-copy));
 persist_t p=persist_saved;p.magic++;memset(nor+other,255,4096);memcpy(nor+other+ST_PAYLOAD_OFF,&p,sizeof p);h.slot=(uint16_t)(1-copy);h.seq-=1;h.crc=st_crc32(&p,sizeof p);h.hcrc=st_crc32(&h,sizeof h-4);memcpy(nor+other,&h,sizeof h);
 calls=physical_calls;memcpy(baseline,nor,sizeof nor);settings.palette=(settings.palette+1)%NPALETTES;settings_save();CHECK(persist_pending==3&&physical_calls==calls&&!memcmp(nor,baseline,sizeof nor));
 fm1_ms+=2000;settings_poll();CHECK(persist_pending==3&&physical_calls==calls); /* permanent unsupported refusal, no endless retry */
 /* Equal differing generations and half-range ordering cannot choose A. */
 for(unsigned kind=0;kind<2;kind++) {
  settings_seed();settings_save();st_hdr_t first;int supported_copy=st_current(OBJ_SETTINGS,&first);CHECK(supported_copy>=0);
  persist_t alt=persist_saved;alt.lowcut=1;uint32_t address=st_sector(OBJ_SETTINGS,(unsigned)(1-supported_copy));
  memcpy(nor+address+ST_PAYLOAD_OFF,&alt,sizeof alt);first.slot=(uint16_t)(1-supported_copy);first.seq=kind?first.seq+0x80000000u:first.seq;
  first.crc=st_crc32(&alt,sizeof alt);first.hcrc=st_crc32(&first,sizeof first-4);memcpy(nor+address,&first,sizeof first);
  calls=physical_calls;memcpy(baseline,nor,sizeof nor);settings.palette=(UI_GREY_INDEX+1)%NPALETTES;settings_save();
  CHECK(persist_pending==3&&physical_calls==calls&&!memcmp(nor,baseline,sizeof nor));
 }
 /* A late USB/quiet change while proving ownership refuses before erase. */
 settings_seed();settings_save();calls=physical_calls;memcpy(baseline,nor,sizeof nor);
 settings.palette=(UI_GREY_INDEX+1)%NPALETTES;boot_inject=0;boot_event=2;settings_save();
 CHECK(persist_pending&&physical_calls==calls&&!memcmp(nor,baseline,sizeof nor));usb.ota_req=0;boot_inject=-1;
 /* Native pool corruption also refuses before touching settings. */
 settings_seed();settings_save();d8pool_index ix;CHECK(!d8pool_inventory(&seed,&ix));nor[d8pool_mapped_address(ix.object[0].block)+D8POOL_HEADER]^=1;
 calls=physical_calls;memcpy(baseline,nor,sizeof nor);settings.palette=(UI_GREY_INDEX+1)%NPALETTES;settings_save();CHECK(persist_pending>=2&&physical_calls==calls&&!memcmp(nor,baseline,sizeof nor));
 /* One private sector/page transaction: cuts retain previous supported prefs. */
 settings_seed();settings_save();palette=settings.palette;memcpy(baseline,nor,sizeof nor);
 unsigned ops=1+(sizeof(persist_t)+255)/256+1;
 for(unsigned c=0;c<ops;c++) {
  memcpy(nor,baseline,sizeof nor);reset();physical_calls=0;partial_failure=0;cut=(int)c;
  settings.palette=(palette+1)%NPALETTES;persist_pending=0;settings_save();CHECK(persist_pending==2&&!d8up.active);
  cut=-1;settings_reboot();CHECK(settings.palette==palette);
 }
 /* Torn payload/header writes retry from the retained supported previous
  * settings. A fully uncertain final header is reconciled rather than erased. */
 for(unsigned prefix=0;prefix<3;prefix++) {
  memcpy(nor,baseline,sizeof nor);reset();physical_calls=0;settings_reboot();partial_failure=prefix==0?1:prefix==1?31:32;cut=(int)(ops-1);
  settings.palette=(palette+1)%NPALETTES;persist_pending=0;settings_save();CHECK(persist_pending==2&&!d8up.active);
  unsigned uncertain_calls=physical_calls;cut=-1;partial_failure=0;fm1_ms+=1000;settings_poll();CHECK(!persist_pending);
  if(prefix==2)CHECK(physical_calls==uncertain_calls);
  settings_reboot();CHECK(settings.palette==(palette+1)%NPALETTES);
 }
 memcpy(nor,baseline,sizeof nor);reset();physical_calls=0;settings_reboot();partial_failure=1;cut=1;
 settings.palette=(palette+1)%NPALETTES;persist_pending=0;settings_save();CHECK(persist_pending==2);
 cut=-1;partial_failure=0;fm1_ms+=1000;settings_poll();CHECK(!persist_pending);settings_reboot();CHECK(settings.palette==(palette+1)%NPALETTES);
 CHECK(!d8_instrument_write_allowed());
 printf("Own settings: %u checks, %u failures; save/reboot/no-op/deferred/future/corrupt and %u operation cuts; virtual NOR only\n",checks,failures,ops);return failures!=0;
}
