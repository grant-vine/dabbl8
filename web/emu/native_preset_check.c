/* SPDX-License-Identifier: GPL-3.0-only */
/* Synthetic emulator NOR only: native ownership precedes actual NAME gestures. */
#include <stdint.h>
static unsigned lcd_fault;
#define D8_EMU_LCD_SYNC_HOOK() do{if(lcd_fault)fm1_lcd_timeouts++;}while(0)
static int preset_program(uint32_t,const void *,uint32_t,uint32_t *);
static int preset_erase(uint32_t,uint32_t*);
#define D8_UP_ERASE(a,t) preset_erase(a,t)
#define D8_UP_PROGRAM(a,p,n,t) preset_program(a,p,n,t)
#include "felucca_web.c"
#if NTRK != 8
#error Native preset fixture requires eight tracks.
#endif
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void elapsed(unsigned ms){unsigned samples=ms*FS/1000;while(samples){unsigned n=samples>128?128:samples;web_render(n);samples-=n;}}
static void tap(unsigned label){web_buttons(1u<<label);elapsed(60);web_buttons(0);elapsed(60);}
static int pr(void*c,uint32_t a,void*p,uint32_t n){(void)c;while(n){unsigned k=D8POOL_BLOCK-a%D8POOL_BLOCK;if(k>n)k=n;if(st_read(d8pool_mapped_address(a/D8POOL_BLOCK)+a%D8POOL_BLOCK,p,k))return -1;a+=k;p=(uint8_t*)p+k;n-=k;}return 0;}
static int pe(void*c,uint32_t a){(void)c;return st_erase(d8pool_mapped_address(a/D8POOL_BLOCK)+a%D8POOL_BLOCK);}
static int pp(void*c,uint32_t a,const void*p,uint32_t n){(void)c;return st_prog(d8pool_mapped_address(a/D8POOL_BLOCK)+a%D8POOL_BLOCK,p,n);}
static int ps(void*c){(void)c;return 1;}
static unsigned programs,reentries,fail_at,late_change,primitive_count,cut_at,cut_after;
static int preset_erase(uint32_t a,uint32_t*t){(void)t;primitive_count++;if(cut_at==primitive_count&&!cut_after)return -1;int rc=st_erase(a);return cut_at==primitive_count?-1:rc;}
static int preset_program(uint32_t a,const void*p,uint32_t n,uint32_t*t)
{
 (void)t;programs++;primitive_count++;uint32_t writes=web_flash_writes_count();
 if(d8up.busy){if(d8up_ui_begin(2,0,"ANALOG 01")||d8up_save(OBJ_UPRESET0,&up_bank[0],sizeof up_bank[0],-1)!=-10)return -9;reentries++;}
 if(web_flash_writes_count()!=writes)return -9;
 if(late_change){d8_capture_store_changed();return -1;}
 if(fail_at&&programs==fail_at)return -1;
 if(cut_at==primitive_count&&!cut_after)return -1;
 int rc=st_prog(a,p,n);return cut_at==primitive_count?-1:rc;
}
#ifndef PRESET_MATRIX
int main(int argc,char **argv)
{
 CHECK(argc>=2);const char *mode=argv[1];web_nor_erase();felucca_init();static uint8_t wire[D8P1_LIMIT];size_t n=0;
 if(strcmp(mode,"no-own")){CHECK(d8p1_capture_runtime(wire,sizeof wire,&n)==D8RT_OK);d8pool pool={NULL,pr,pe,pp,ps};CHECK(d8pool_save(&pool,0,wire,n,1)==D8POOL_OK);}
 if(argc==3){FILE*f=fopen(argv[2],"wb");CHECK(f);CHECK(fwrite(nor,1,sizeof nor,f)==sizeof nor);CHECK(fclose(f)==0);}
 if(!strcmp(mode,"bad-bank"))nor[0xDC100]=0x42;
 if(!strcmp(mode,"future-bank")){st_hdr_t h={0};h.magic=ST_MAGIC;h.type=OBJ_UPRESET0;h.slot=0;h.len=sizeof(up_bank_t)+1;h.rsv[0]=h.rsv[1]=~0u;h.hcrc=st_crc32(&h,sizeof h-4);memcpy(nor+0xDC000,&h,sizeof h);}
 web_boot();elapsed(500);CHECK(!d8_instrument_write_allowed());
 if(!strcmp(mode,"no-own")||!strcmp(mode,"bad-bank")||!strcmp(mode,"future-bank")){
  static uint8_t before[sizeof nor];memcpy(before,nor,sizeof nor);unsigned writes=web_flash_writes_count();tap(B_SAVE);tap(B_OCTUP);tap(B_OCTUP);CHECK(!up_used(0));CHECK(web_flash_writes_count()==writes);CHECK(!memcmp(before,nor,sizeof nor));CHECK(!d8up.active);printf("own preset %s: %u checks PASS\n",mode,checks);return 0;
 }
 CHECK(project_native_status()==1&&project_native.readonly);
 if(!strcmp(mode,"fm6")){host_preset(&trk[7],ENGI_FM6,0);song.sel=7;}
 if(!strcmp(mode,"undisplayed")){unsigned writes=web_flash_writes_count();name_open(NK_USER_SAVE,0);web_buttons(1u<<B_OCTUP);ui_input();web_buttons(0);ui_input();CHECK(!up_used(0));CHECK(web_flash_writes_count()==writes);CHECK(!d8up.active);printf("own preset undisplayed: %u checks PASS\n",checks);return 0;}
 unsigned writes=web_flash_writes_count();tap(B_SAVE);tap(B_OCTUP);CHECK(name_on());
 if(!strcmp(mode,"lcd-fault")){lcd_fault=1;ui.force=1;elapsed(60);}
 // Direct NAME helper calls cannot fabricate the actual release event.
 CHECK(!d8up_ui_begin(2,0,nm.s));CHECK(web_flash_writes_count()==writes);
 tap(B_OCTUP);
 if(!strcmp(mode,"lcd-fault")){CHECK(!up_used(0));CHECK(web_flash_writes_count()==writes);CHECK(!d8up.active);printf("own preset LCD fault: %u checks PASS\n",checks);return 0;}
 CHECK(up_used(0));CHECK(web_flash_writes_count()>writes);CHECK(!d8up.active);CHECK(!native_as.ready);CHECK(reentries==programs);
 up_rec_t saved=*up_rec(0);uint8_t patch[FM6_PACKED];if(!strcmp(mode,"fm6"))CHECK(upf_get(0,patch)==0);
 memset(up_bank,0,sizeof up_bank);memset(&upf,0,sizeof upf);up_boot();CHECK(up_used(0));CHECK(!memcmp(&saved,up_rec(0),sizeof saved));if(!strcmp(mode,"fm6")){uint8_t loaded[FM6_PACKED];CHECK(upf_get(0,loaded)==0);CHECK(!memcmp(patch,loaded,sizeof patch));}
 if(!strcmp(mode,"rename")||!strcmp(mode,"erase")||!strcmp(mode,"future-copy")||!strcmp(mode,"reserved-copy")||!strcmp(mode,"tie-copy")||!strcmp(mode,"half-copy")){
  if(!strcmp(mode,"rename")){name_open(NK_USER_RENAME,0);memcpy(nm.s,"RENAMED",8);nm.len=nm.cur=7;tap(B_OCTUP);CHECK(!memcmp(up_rec(0)->name,"RENAMED",8));up_boot();CHECK(!d8up_recover_banks());CHECK(!memcmp(up_rec(0)->name,"RENAMED",8));printf("own preset rename: %u checks PASS\n",checks);return 0;}
  if(!strcmp(mode,"erase")){confirm_open(CF_ERASE_USER,0);tap(B_OCTUP);CHECK(!up_used(0));up_boot();CHECK(!d8up_recover_banks());CHECK(!up_used(0));printf("own preset erase: %u checks PASS\n",checks);return 0;}
  st_hdr_t h;CHECK(!st_head(OBJ_UPRESET0,0,&h));up_bank_t future=up_bank[0];h.slot=1;h.seq--;
  if(!strcmp(mode,"future-copy"))future.r[0].ver=UP_VER_GRID+1;
  if(!strcmp(mode,"reserved-copy"))h.rsv[0]=0;
  if(!strcmp(mode,"tie-copy"))h.seq++;
  if(!strcmp(mode,"half-copy"))h.seq+=1+0x80000000u;
  h.crc=st_crc32(&future,sizeof future);h.hcrc=st_crc32(&h,sizeof h-4);memcpy(nor+st_sector(OBJ_UPRESET0,1),&h,sizeof h);memcpy(nor+st_sector(OBJ_UPRESET0,1)+ST_PAYLOAD_OFF,&future,sizeof future);
  static uint8_t before[sizeof nor];memcpy(before,nor,sizeof nor);up_boot();CHECK(d8up_recover_banks()==-1);CHECK(!up_used(0));unsigned writes=web_flash_writes_count();name_open(NK_USER_SAVE,0);tap(B_OCTUP);CHECK(!up_used(0));CHECK(web_flash_writes_count()==writes);CHECK(!memcmp(before,nor,sizeof nor));printf("own preset %s: %u checks PASS\n",mode,checks);return 0;
 }
 writes=web_flash_writes_count();CHECK(st_save(OBJ_SETTINGS,wire,16)==D8_INSTRUMENT_QUARANTINED);CHECK(st_save(OBJ_UPRESET0,&up_bank[0],sizeof up_bank[0])==D8_INSTRUMENT_QUARANTINED);CHECK(up_put(0,NULL)==2);CHECK(web_flash_writes_count()==writes);CHECK(up_used(0));
 if(!strcmp(mode,"fault")||!strcmp(mode,"late")){
  trk[song.sel].p[P_LEVEL]=11;programs=0;if(!strcmp(mode,"fault"))fail_at=5;else late_change=1;
  name_open(NK_USER_SAVE,0);tap(B_OCTUP);CHECK(!d8up.active);CHECK(!memcmp(&saved,up_rec(0),sizeof saved));memset(up_bank,0,sizeof up_bank);up_boot();CHECK(!d8up_recover_banks());CHECK(!memcmp(&saved,up_rec(0),sizeof saved));
 }
 printf("own preset %s: %u checks PASS; metadata %zu bytes\n",mode,checks,sizeof d8up);return 0;
}

#endif /* standalone fixture */
