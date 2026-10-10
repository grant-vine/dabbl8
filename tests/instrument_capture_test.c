/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual editor/USB/native capture against virtual NOR, never a device test. */
#define FELUCCA_OTA 1
#define D8_AUTOSAVE_SESSION_NO_MAIN 1
#define D8_INSTRUMENT_CAPTURE_TEST 1
#define D8FLASH_INSTRUMENT_TEST 1
#include "native_autosave_session_test.c"
static uint32_t ota_now_ms(void){return fm1_ms;}
static void ota_idle(void){so_r=so_w;fm1_ms++;}
static void audio_silence(void) {}
static void fl_inval(uint32_t a,uint32_t n){(void)a;(void)n;}
static int fl_erase4k(uint32_t a,uint32_t *t){if(a>sizeof nor-4096)return -1;memset(nor+a,255,4096);writes++;*t=0;return 0;}
static int fl_write(uint32_t a,const void *p,uint32_t n){if(a>sizeof nor||n>sizeof nor-a||cut==0)return -1;memcpy(nor+a,p,n);writes++;return 0;}
#include "../firmware/src/storage.c"
#include "../firmware/src/editor.c"
/* Compile actual hardware storage observation/read-window code under test-only
 * symbol aliases. Physical RAM primitives below remain simulated NOR. */
static unsigned hw_read_calls;
static int fl_read_ram(uint32_t a,void *p,uint32_t n){CHECK(irq_disabled&&n<=256);hw_read_calls++;if(a>sizeof nor||n>sizeof nor-a)return -1;memcpy(p,nor+a,n);return 0;}
#define FL_FAR(fn) fn
#define fl_erase4k_ram fl_erase4k
#define FL_STORE_OK(a,n) ((a)>=0x97000u&&(a)<=0xe0000u&&(n)<=0xe0000u-(a) || (a)>=0xfc000u&&(a)<=0xff000u&&(n)<=0xff000u-(a) || (a)>=0xe5000u&&(a)<=0xe7000u&&(n)<=0xe7000u-(a))
#define flash_ok capture_hw_flash_ok
#define st_read capture_hw_read
#define st_prog capture_hw_prog
#define st_erase capture_hw_erase
#define audio_silence capture_hw_silence
#include "../firmware/src/storage_hw.c"
#undef audio_silence
#undef st_erase
#undef st_prog
#undef st_read
#undef flash_ok
#undef FL_STORE_OK
#undef fl_erase4k_ram
#undef FL_FAR
_Static_assert(sizeof(persist_t)==572,"version1 PER4 role");
_Static_assert(sizeof(up_bank[0])==3080,"version1 preset role");
_Static_assert(sizeof(upf)==3728,"version1 FM6 role");
static void fixture(void){idle_fixture();usb.config=1;usb.ota_req=usb.uboot_req=0;memset(&ed_capture,0,sizeof ed_capture);memset(&d8_capture_change,0,sizeof d8_capture_change);for(unsigned a=0;a<sizeof nor;a++)nor[a]=(uint8_t)(a^(a>>9)^0xa5);reset();memcpy(baseline,nor,sizeof nor);}
static void request(const uint8_t *a,unsigned n){uint8_t f[64]={0x7d,0x46,0x4c,78};memcpy(f+4,a,n);ed_handle(f,n+4);so_r=so_w;CHECK(ed_n>=15&&ed_out[0]==0xf0&&ed_out[ed_n-1]==0xf7);}
static unsigned status(void){return ed_out[7];}
static void op(unsigned o){uint8_t a[7]={1,(uint8_t)o};for(unsigned i=0;i<5;i++)a[i+2]=(ed_capture.token>>(i*7))&127;request(a,7);}
static void begin_capture(unsigned full){uint8_t a[3]={1,1,(uint8_t)full};request(a,3);}
static void scan(void){unsigned n=0;while(ed_capture.active&&(ed_capture.phase==1||ed_capture.phase==3)&&n++<1300){op(2);CHECK(!status());}CHECK(n==1248&&ed_capture.active);}
static void get(unsigned role,unsigned off,unsigned count){uint8_t a[15]={1,4};for(unsigned i=0;i<5;i++){a[i+2]=(ed_capture.token>>(i*7))&127;a[i+8]=(off>>(i*7))&127;}a[7]=role;a[13]=count&127;a[14]=count>>7;request(a,15);}
static void configuration(unsigned c){memset(ep0buf,0,8);ep0buf[1]=9;ep0buf[2]=c;hs_regs[0][S_CSR0]=1;ep0_service();}
static void upgrade(unsigned c){uint8_t f[6]={0xf0,0x22,0x24,0x35,(uint8_t)c,0xf7};for(unsigned i=0;i<6;i++)sysex_byte(f[i]);}
static void printhex(const uint8_t *p,unsigned n){for(unsigned i=0;i<n;i++)printf("%s%02x",i?" ":"",p[i]);putchar('\n');fflush(stdout);}
static int bridge(void){char line[2048];fixture();while(fgets(line,sizeof line,stdin)){if(line[0]=='#'){
 if(!strncmp(line,"#fixture",8)){fixture();puts("ok");}
 else if(!strncmp(line,"#stats",6))printf("{\"writes\":%u,\"reads\":%u,\"unchanged\":%s}\n",writes,reads,(writes||memcmp(nor,baseline,sizeof nor))?"false":"true");
 else if(!strncmp(line,"#reference ",11)){unsigned role=(unsigned)strtoul(line+11,NULL,10);if(role==12){size_t n=0;if(d8p1_capture_runtime(wire,sizeof wire,&n))return 2;printhex(wire,(unsigned)n);}else if(role>=13&&role<17)printhex(edc_current(role),edc_length(role));else return 2;}
 else if(!strncmp(line,"#change ",8)){unsigned role=(unsigned)strtoul(line+8,NULL,10);if(role<12){nor[edc_roles[role].a]^=1;d8_capture_store_changed();}else if(role==12)strcpy(proj_name,"CHANGED");else if(role==13)settings.lowcut^=1;else if(role<16)((uint8_t*)&up_bank[role-14])[100]^=1;else if(role==16)((uint8_t*)&upf)[100]^=1;puts("ok");}
 else if(!strncmp(line,"#usb-cycle",10)){configuration(0);configuration(1);puts("ok");}
 else if(!strncmp(line,"#reset",6)){usb.resets++;puts("ok");}
 else if(!strncmp(line,"#timeout",8)){fm1_ms+=15001;puts("ok");}
 else if(!strncmp(line,"#ota",4)){upgrade(0x7f);puts("ok");}
 else return 2;
 fflush(stdout);continue;
 }uint8_t frame[1024];unsigned n=0;char *p=line,*end;while(*p){unsigned long v=strtoul(p,&end,16);if(end==p)break;if(v>255||n==sizeof frame)return 2;frame[n++]=(uint8_t)v;p=end;}if(n<6||frame[0]!=0xf0||frame[n-1]!=0xf7)return 2;ed_n=0;ed_handle(frame+1,n-2);so_r=so_w;printhex(ed_out,ed_n);}return failures?1:0;}
int main(int argc,char **argv){if(argc==2&&!strcmp(argv[1],"--bridge"))return bridge();
 fixture();memcpy(baseline,nor,sizeof nor);begin_capture(1);CHECK(!status());scan();CHECK(ed_capture.phase==2);
 unsigned oldzoom=settings.zoom;const uint8_t *current=edc_current(13);CHECK(settings.zoom==oldzoom&&current==(const uint8_t*)&ed_bk_settings);CHECK(ed_bk_settings.magic==PERSIST_MAGIC);
 for(unsigned r=0;r<17;r++){unsigned length=edc_length(r);for(unsigned off=0;off<length;off+=256){unsigned n=length-off;if(n>256)n=256;get(r,off,n);CHECK(!status()&&ed_n<=316);}}
 op(5);CHECK(!status());scan();CHECK(ed_capture.phase==4&&!writes&&!memcmp(nor,baseline,sizeof nor));op(6);CHECK(!ed_capture.active);
 fixture();begin_capture(0);scan();get(3,8191,2);CHECK(status()==EDC_BAD&&!ed_capture.active);
 fixture();begin_capture(1);scan();strcpy(proj_name,"DIFFERENT");get(12,0,32);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(1);scan();settings.lowcut^=1;get(13,0,32);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(1);scan();((uint8_t*)&up_bank[1])[100]^=1;get(15,0,32);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);scan();d8_capture_store_changed();d8_capture_change.epoch=ed_capture.epoch;get(0,0,32);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);fm1_ms+=15001;op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);usb.resets++;op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);uint32_t resets=usb.resets;configuration(0);configuration(1);CHECK(usb.config==1&&usb.resets==resets&&d8_capture_change.changed);op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);upgrade(0x7f);CHECK(usb.ota_req&&d8_capture_change.changed&&!writes);op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);upgrade(0x7d);CHECK(usb.uboot_req&&d8_capture_change.changed&&!writes);op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(1);scan();song.sel=1;get(12,0,32);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(1);scan();((uint8_t*)&upf)[100]^=1;op(5);CHECK(!status());unsigned count=0;while(ed_capture.active&&count++<1300)op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();fm1_ms=UINT32_MAX-100u;begin_capture(0);fm1_ms+=200;op(2);CHECK(!status()&&ed_capture.active);op(6);
 fixture();begin_capture(0);uint8_t other[4]={0x7d,0x46,0x4c,25};ed_handle(other,4);so_r=so_w;CHECK(!ed_capture.active);
 fixture();begin_capture(0);uint8_t value=0;cut=0;CHECK(capture_hw_prog(0x97000,&value,1)==-1&&d8_capture_change.changed&&!writes);cut=-1;op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);CHECK(!capture_hw_erase(0x97000)&&d8_capture_change.changed&&writes==1);op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();begin_capture(0);CHECK(capture_hw_prog(0,&value,1)==-8&&!d8_capture_change.changed&&!writes);op(2);CHECK(!status());op(6);
 fixture();hw_read_calls=0;CHECK(!capture_hw_read(0x97000,out,1024)&&hw_read_calls==4&&!irq_disabled&&!memcmp(out,nor+0x97000,1024));
 fixture();begin_capture(0);CHECK(!ed_smp_erase(0,0)&&d8_capture_change.changed&&writes==1);op(2);CHECK(status()==EDC_CHANGED&&!ed_capture.active);
 fixture();memcpy(out,&upf,sizeof upf);CHECK(upf_store(0,7)==3&&!writes&&memcmp(out,&upf,sizeof upf));begin_capture(1);CHECK(!status());scan();get(16,0,256);CHECK(!status()&&!writes);op(6);
 fixture();settings.lowcut=2;settings_leds=1;settings_hold=1;favorites.factory[15][27]=7;panel.dir[0]=-1;uint8_t settings_before[sizeof settings],panel_before[sizeof panel],favorites_before[sizeof favorites];memcpy(settings_before,&settings,sizeof settings);memcpy(panel_before,&panel,sizeof panel);memcpy(favorites_before,&favorites,sizeof favorites);begin_capture(1);CHECK(!status()&&!memcmp(settings_before,&settings,sizeof settings)&&!memcmp(panel_before,&panel,sizeof panel)&&!memcmp(favorites_before,&favorites,sizeof favorites));CHECK(ed_bk_settings.lowcut==2&&ed_bk_settings.favorites.factory[15][27]==7);op(6);
 fixture();song.sel=8;begin_capture(1);CHECK(status()==EDC_BAD&&!ed_capture.active);begin_capture(0);CHECK(!status()&&ed_capture.active);op(6);
 fixture();begin_capture(0);uint32_t token=ed_capture.token;begin_capture(0);CHECK(status()==EDC_BUSY&&ed_capture.active&&ed_capture.token==token);op(6);
 fixture();begin_capture(0);uint8_t stale[7]={1,2};request(stale,7);CHECK(status()==EDC_STALE&&!ed_capture.active);
 fixture();begin_capture(0);scan();get(0,UINT32_MAX,1);CHECK(status()==EDC_BAD&&!ed_capture.active);
 fixture();begin_capture(0);uint8_t bad[3]={1,1,128};request(bad,3);CHECK(status()==EDC_BAD&&!ed_capture.active);
 fixture();cv_begin(8,8,T_BG);begin_capture(1);CHECK(status()==EDC_BUSY&&!ed_capture.active);cv_blit(0,0);
 printf("Instrument capture: %u checks, %u failures; actual editor/native capture, virtual NOR, no physical qualification\n",checks,failures);return failures!=0;
}
