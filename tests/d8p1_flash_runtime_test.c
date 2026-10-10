/* SPDX-License-Identifier: GPL-3.0-only */
/* Real runtime/existing-driver adapter; physical hooks are simulated, no hardware. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
#include "../firmware/src/d8pool_mapped.h"
static uint8_t nor[0x100000],baseline[sizeof nor],wire[D8P1_LIMIT],out[D8P1_LIMIT];
static unsigned checks,failures,reads,writes,irq_depth;
static int cut=-1,late_irq=-1;
static uint8_t flash_ok;
#define CHECK(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static uint32_t irq_save(void){if(late_irq==0)transport_req=1;if(late_irq>0)late_irq--;uint32_t was=irq_depth;irq_depth++;return was;}
static void irq_restore(uint32_t was){CHECK(irq_depth>was);irq_depth=was;}
static int allowed(uint32_t a,uint32_t n){for(unsigned b=0;b<5;b++){uint32_t base=d8pool_mapped_address(b);if(a>=base&&a-base<=8192&&n<=8192-(a-base))return 1;}return 0;}
static int st_read(uint32_t a,void *p,uint32_t n){CHECK(allowed(a,n));reads++;memcpy(p,nor+a,n);return 0;}
static int mutate(void){CHECK(irq_depth&&!transport_req&&!transport_busy()&&!cv_cpu_active);if(cut==0)return -1;if(cut>0)cut--;writes++;return 0;}
static int st_erase(uint32_t a){CHECK(allowed(a,4096)&&a%4096==0);if(mutate())return -1;memset(nor+a,255,4096);return 0;}
static int st_prog(uint32_t a,const void *p,uint32_t n){const uint8_t *q=p;CHECK(allowed(a,n)&&n&&n<=256&&(a&255)+n<=256);if(mutate())return -1;for(unsigned i=0;i<n;i++)nor[a+i]&=q[i];return 0;}
#include "../firmware/src/d8p1_flash_runtime.c"
static void reset(void){reads=writes=irq_depth=0;cut=late_irq=-1;transport_req=0;}
static void blank(void){memset(nor,0xa5,sizeof nor);for(unsigned b=0;b<5;b++)memset(nor+d8pool_mapped_address(b),255,8192);reset();}
/* Test-only seed for an explicitly migrated image; bypass is not device code. */
static int rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;memcpy(p,nor+d8pool_mapped_address(a/8192)+a%8192,n);return 0;}
static int er(void *c,uint32_t a){(void)c;memset(nor+d8pool_mapped_address(a/8192)+a%8192,255,4096);return 0;}
static int pg(void *c,uint32_t a,const void *p,uint32_t n){(void)c;const uint8_t *q=p;uint32_t base=d8pool_mapped_address(a/8192)+a%8192;for(unsigned i=0;i<n;i++)nor[base+i]&=q[i];return 0;}
static int stop(void *c){(void)c;return 1;}
static d8pool seed={NULL,rd,er,pg,stop};
static void unchanged(d8pool_index *index){d8pool_index now;CHECK(!d8pool_inventory(&seed,&now)&&now.present==index->present);for(unsigned o=0;o<4;o++)CHECK(!memcmp(&now.object[o],&index->object[o],sizeof now.object[o]));CHECK(!memcmp(nor,baseline,0x97000)&&!memcmp(nor+0x9f000,baseline+0x9f000,0x46000)&&!memcmp(nor+0xe7000,baseline+0xe7000,sizeof nor-0xe7000));}
int main(void){
 ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);
 FILE *f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)return 2;size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(!d8p1_load_runtime(wire,n,0));mix_block(audio,CTL);blank();flash_ok=1;
 CHECK(d8p1_save_flash(0,0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 CHECK(d8p1_load_flash(0,0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 CHECK(d8p1_restore_flash_autosave(1,0)==D8POOL_EMPTY&&!reads&&!writes);
 flash_ok=0;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&!reads&&!writes);flash_ok=1;
 CHECK(d8p1_save_flash(0,1)==D8POOL_EMPTY&&!writes);
 memcpy(nor+0x97000,"FELU",4);CHECK(d8p1_save_flash(0,1)==D8POOL_UNSUPPORTED&&!writes);
 blank();for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();
 for(unsigned o=0;o<4;o++){trk[7].p[P_LEVEL]=(int16_t)(42+o);CHECK(!d8p1_save_flash(o,1));n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));size_t got=0;CHECK(!d8pool_load(&seed,o,out,sizeof out,&got,7)&&n==got&&!memcmp(wire,out,n));trk[7].p[P_LEVEL]=1;CHECK(!d8p1_load_flash(o,1)&&trk[7].p[P_LEVEL]==42+(int)o);mix_block(audio,CTL);}
 reset();d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));memcpy(baseline,nor,sizeof nor);
 trk[7].p[P_LEVEL]=90;n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));uint8_t current=proj_cur;
 unsigned ops=2+(unsigned)((n+255)/256)+1;
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();cut=(int)c;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&proj_cur==current&&trk[7].p[P_LEVEL]==90&&!irq_depth);reset();unchanged(&index);}
 /* Inject PLAY at each critical-section entry, after prior stopped checks. */
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();late_irq=(int)c;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&writes==c&&transport_req&&!irq_depth&&proj_cur==current);reset();unchanged(&index);}
 memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8p1_restore_flash_autosave(1,1)&&trk[7].p[P_LEVEL]==45&&proj_cur==PROJ_NO_SLOT);mix_block(audio,CTL);
 reset();transport_req=1;CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&!reads&&!writes);transport_req=0;
 cv_begin(8,8,T_BG);reset();CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&!reads&&!writes);cv_blit(0,0);
 CHECK(d8p1_save_flash(4,1)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(!irq_depth&&!dma_errors);
 printf("D8P1 flash runtime: %u checks, %u failures; %u driver cuts, %u atomic late-start cuts; actual runtime, simulated physical hooks only\n",checks,failures,ops,ops);return failures!=0;
}
