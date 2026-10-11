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
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL];
static uint8_t fm1_led[FM1_NCOL], fm1_led_dim[FM1_NCOL], fm1_led_breath[FM1_NCOL], fm1_led_mid[FM1_NCOL];
static uint32_t host_dim_lo;                      /* hal/fm1_input.h: the glow MENU > LEDS asked for (1: DIM LO) */
static void fm1_led_dim_level(uint32_t lo) { host_dim_lo = lo; }
static int host_anim;                             /* hal/fm1_input.h: the power-on sweep running (it has the LEDs) */
static int fm1_led_anim_on(void) { return host_anim; }
#define FM1_TICKS_PER_US 1u
static uint32_t host_ticks, host_pressed, host_notes;
static int32_t host_enc[7];
static uint32_t fm1_ticks(void) { return host_ticks; }
static uint32_t fm1_input_edges(int x) { uint32_t p = host_pressed; (void)x; host_pressed = 0; return p; }
static uint32_t fm1_input_note_edges(void) { uint32_t n = host_notes; host_notes = 0; return n; }

#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N];static uint32_t scope_w;static struct {uint32_t stage;} felucca_dbg;
static int32_t fm1_enc_take(uint32_t e){(void)e;return 0;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
static void fm1_wdt_feed(void){}
static void caller_lcd_hook(void);
static void lcd_sync(void){caller_lcd_hook();}
static void lcd_power(uint32_t s){(void)s;}
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){(void)x;(void)y;(void)w;(void)h;(void)c;caller_lcd_hook();}
static void lcd_wake_now(void){}
static unsigned blits;
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t*p){blits++;(void)x;(void)y;(void)w;(void)h;(void)p;}
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
#include "../firmware/src/icons.c"
#include "../firmware/src/ui_graph.c"
#include "../firmware/src/ui_draw.c"
#include "../firmware/src/ui_menu.c"
#include "../firmware/src/ui_input.c"
#include "../firmware/src/ui_layer.c"

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
static uint32_t ota_now_ms(void){return fm1_ms;}
static void ota_idle(void){so_r=so_w;fm1_ms++;}
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

#include "../firmware/src/native_migration_preflight.c"
static unsigned inject, late_acquired;static uint32_t held;
static uint8_t arena_before[sizeof main_workspace];
static project_t input,before_live,after_live;
static __typeof__(d8p1_runtime_cache) old_cache;
static __typeof__(project_native) old_binding;
static void caller_lcd_hook(void){
 if(!inject)return;inject=0;
 uint32_t g=0;CHECK(d8mp_begin(&g)==D8MP_OK);held=g;late_acquired++;
 memset(main_migration_workspace(g),0xa5,sizeof(d8mp_workspace));
 memcpy(arena_before,&main_workspace,sizeof main_workspace);
}
static void hold(void){CHECK(d8mp_begin(&held)==D8MP_OK);memset(main_migration_workspace(held),0xa5,sizeof(d8mp_workspace));memcpy(arena_before,&main_workspace,sizeof main_workspace);}
static void release(void){CHECK(held&&d8mp_end(held)==D8MP_OK);held=0;}
static void preserve(void){CHECK(!memcmp(arena_before,&main_workspace,sizeof main_workspace));project_capture(&after_live);CHECK(!memcmp(&before_live,&after_live,sizeof before_live));CHECK(!erases&&!programs);CHECK(!memcmp(&old_cache,&d8p1_runtime_cache,sizeof old_cache)&&!memcmp(&old_binding,&project_native,sizeof old_binding));}
static void command(unsigned cmd,unsigned op){uint8_t f[6]={0x7d,0x46,0x4c,(uint8_t)cmd,(uint8_t)op,0};ed_n=0;ed_handle(f,cmd==ED_BACKUP_LIST?4:6);CHECK(!ed_n);}
int main(void){
 memset(nor,255,sizeof nor);flash_ok=1;panel=PANEL_DEFAULT;settings_init();host_tracks_init();native_fixture();project_native_reset();
 FILE*f=fopen("tests/fixtures/projects/fun9.bin","rb");CHECK(f!=NULL);if(!f)return 2;CHECK(fread(&proj_slot[0],1,sizeof proj_slot[0],f)==sizeof proj_slot[0]);fclose(f);
 for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].graph==GR_SLOTS){ui.page=p;break;}ui.home=0;
 chain_config.count=1;chain_config.row[0].slot=0;chain_config.row[0].repeat=1;
 project_capture(&input);project_capture(&before_live);old_cache=d8p1_runtime_cache;old_binding=project_native;reads=erases=programs=0;
 ui.force=1;graph_project_used(0);char names_before[sizeof graph_pname];memcpy(names_before,graph_pname,sizeof names_before);uint32_t names_sig=graph_pname_sig;
 char name[13],saved_name[13];memset(name,0x5a,sizeof name);memcpy(saved_name,name,sizeof name);
 usb.config=1;uint8_t capture_begin[]={0x7d,0x46,0x4c,ED_D8_CAPTURE,1,1,0};ed_handle(capture_begin,sizeof capture_begin);CHECK(ed_capture.active&&ed_capture.phase==1);__typeof__(ed_capture) old_capture=ed_capture;
 __typeof__(chain) old_chain=chain;
 hold();CHECK(chain_prepare()==2&&!memcmp(&old_chain,&chain,sizeof chain));CHECK(project_name(0,name)==-D8POOL_BUSY&&!memcmp(name,saved_name,sizeof name));CHECK(project_used(0)==-D8POOL_BUSY);CHECK(project_load(0)==D8POOL_BUSY);CHECK(project_restore_runtime(&input)==2);
 unsigned wiregen=proj_wire_gen;__typeof__(as) old_as=as;__typeof__(native_as) old_native_as=native_as;__typeof__(nm) old_nm=nm;uint8_t raw[sizeof proj_wire_u];memcpy(raw,&proj_wire_u,sizeof raw);
 unsigned oldreads=reads;persist_boot();CHECK(reads==oldreads);CHECK(autosave_boot(1)==D8POOL_BUSY);autosave_poll();CHECK(!memcmp(&old_as,&as,sizeof as)&&!memcmp(&old_native_as,&native_as,sizeof native_as));CHECK(proj_wire_gen==wiregen&&!memcmp(raw,&proj_wire_u,sizeof raw));
 name_open(NK_PROJ_RENAME,0);name_rename();CHECK(!memcmp(&old_nm,&nm,sizeof nm));fm1_ms+=1000;ui.frame++;graph_project_used(0);CHECK(!memcmp(names_before,graph_pname,sizeof names_before)&&graph_pname_sig==names_sig);
 command(ED_PROJECT,0);command(ED_PROJECT,1);command(ED_PROJECT,2);command(ED_BACKUP_LIST,0);CHECK(!memcmp(&old_capture,&ed_capture,sizeof old_capture));CHECK(ed_bk_capture()==3&&ed_bk_commit()==3);uint32_t length=123;CHECK(!ed_bk_object(2,&length)&&length==123);
 uint32_t w=cv_w,h=cv_h;uint16_t bg=cv_bg;CHECK(!cv_begin_try(10,10,T_BG)&&cv_w==w&&cv_h==h&&cv_bg==bg);draw_text_line(0,0,50,&AF_S,"HELD",T_TEXT,T_BG,0);CHECK(cv_draw_deferred);
 ui.force=1;draw_head();draw_foot();draw_graph();draw_menu();nm_draw_field();nm_draw_panel();draw_layer();draw_confirm();ui_draw();CHECK(ui.force);preserve();release();
 /* Nested acquisition at the real LCD completion boundary, not primitive traps. */
 for(unsigned op=0;op<16;op++){
  cv_draw_deferred=0;inject=1;late_acquired=0;memcpy(name,saved_name,sizeof name);wiregen=proj_wire_gen;old_as=as;old_native_as=native_as;old_nm=nm;
  if(op==0)CHECK(project_name(0,name)==-D8POOL_BUSY&&!memcmp(name,saved_name,sizeof name));
  if(op==1)CHECK(project_used(0)==-D8POOL_BUSY);
  if(op==2)CHECK(project_load(0)==D8POOL_BUSY);
  if(op==3)CHECK(project_restore_runtime(&input)==2);
  if(op==4)CHECK(!cv_begin_try(10,10,T_BG));
  if(op==5)draw_text_line(0,0,50,&AF_S,"LATE",T_TEXT,T_BG,0);
  if(op==6)command(ED_PROJECT,2);
  if(op==7)command(ED_BACKUP_LIST,0);
  if(op==8)CHECK(autosave_boot(1)==D8POOL_BUSY);
  if(op==9)persist_boot();
  if(op==10){ui.force=1;ui_draw();CHECK(ui.force);}
  if(op==11)autosave_poll();
  if(op==12){fm1_ms+=1000;ui.frame++;ui.force=1;graph_project_used(0);CHECK(!memcmp(names_before,graph_pname,sizeof names_before)&&graph_pname_sig==names_sig);}
  if(op==13)name_open(NK_PROJ_RENAME,0);
  if(op==14){ui.force=1;draw_head();CHECK(ui.force);}
  if(op==15)CHECK(chain_prepare()==2&&!memcmp(&old_chain,&chain,sizeof chain));

  CHECK(late_acquired==1&&held&&migration_owner==held);CHECK(proj_wire_gen==wiregen&&!memcmp(&old_as,&as,sizeof as)&&!memcmp(&old_native_as,&native_as,sizeof native_as)&&!memcmp(&old_nm,&nm,sizeof nm));preserve();release();
 }
 /* CPU-owned drawing also refuses operational users, preserving real pixels. */
 CHECK(cv_begin_try(10,10,T_BG));memcpy(arena_before,&main_workspace,sizeof main_workspace);memcpy(name,saved_name,sizeof name);
 CHECK(project_name(0,name)==-D8POOL_BUSY&&!memcmp(name,saved_name,sizeof name)&&project_used(0)==-D8POOL_BUSY);
 CHECK(project_load(0)==D8POOL_BUSY&&project_restore_runtime(&input)==2&&chain_prepare()==2);
 command(ED_PROJECT,2);command(ED_BACKUP_LIST,0);CHECK(!memcmp(&old_capture,&ed_capture,sizeof old_capture));
 CHECK(!cv_begin_try(11,11,T_SURF));draw_text_line(0,0,50,&AF_S,"CPU",T_TEXT,T_BG,0);CHECK(cv_cpu_active&&cv_w==10&&cv_h==10);preserve();cv_blit(0,0);
 /* Queries/drawing recover after release; no permanent stale busy state. */
 CHECK(project_used(0)>0&&project_name(0,name)>0&&name[0]);CHECK(!chain_prepare());chain=old_chain;cv_draw_deferred=0;CHECK(cv_begin_try(10,10,T_BG));cv_blit(0,0);CHECK(!cv_cpu_active&&!migration_owner);CHECK(!erases&&!programs);
 /* Refused one-shot survives release before next clean-signature UI frame. */
 ui.force=1;ui_draw();CHECK(!ui.force);hold();unsigned prior_blits=blits;draw_text_line(0,0,50,&AF_S,"RETRY",T_TEXT,T_BG,0);CHECK(cv_draw_deferred&&!ui.force&&blits==prior_blits);release();ui_draw();CHECK(blits>prior_blits&&!cv_draw_deferred&&!ui.force);
 printf("Competing workspace callers: %u checks, %u failures; actual historical/project/editor/boot/drawing, early and LCD-late refusal; no authority/device qualification\n",checks,failures);return failures?1:0;
}
