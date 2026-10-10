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
static unsigned checks,failures,reads,writes,irq_disabled;
static int cut=-1,late_irq=-1,post_commit_start,post_commit_io;
static uint8_t flash_ok;
static int inject_at=-1;
static void (*irq_inject)(void),(*after_commit)(void);
#define CHECK(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
/* Match actual fm1_flash.h: cli, return 0; csync/sti always enables. */
static uint32_t irq_save(void){if(!irq_disabled){if(late_irq==0)transport_req=1;if(late_irq>0)late_irq--;if(inject_at==0&&irq_inject){inject_at=-1;irq_inject();}if(inject_at>0)inject_at--;}irq_disabled=1;return 0;}
static void irq_restore(uint32_t was){(void)was;irq_disabled=0;}
static int allowed(uint32_t a,uint32_t n){
#ifdef D8FLASH_INSTRUMENT_TEST
return a<=sizeof nor&&n<=sizeof nor-a;
#endif
for(unsigned b=0;b<5;b++){uint32_t base=d8pool_mapped_address(b);if(a>=base&&a-base<=8192&&n<=8192-(a-base))return 1;}return 0;}
static int st_read(uint32_t a,void *p,uint32_t n){CHECK(allowed(a,n));reads++;
#ifdef D8FLASH_READ_HOOK
D8FLASH_READ_HOOK();
#endif
if(post_commit_io==2)return -1;memcpy(p,nor+a,n);return 0;}
static int mutate(void){CHECK(irq_disabled&&!transport_req&&!transport_busy()&&!cv_cpu_active);if(cut==0)return -1;if(cut>0)cut--;writes++;return 0;}
static int st_erase(uint32_t a){CHECK(allowed(a,4096)&&a%4096==0);uint32_t f=irq_save();int rc=mutate();if(!rc)memset(nor+a,255,4096);irq_restore(f);return rc;}
static int st_prog(uint32_t a,const void *p,uint32_t n){const uint8_t *q=p;CHECK(allowed(a,n)&&n&&n<=256&&(a&255)+n<=256);uint32_t f=irq_save();int rc=mutate();if(!rc)for(unsigned i=0;i<n;i++)nor[a+i]&=q[i];irq_restore(f);if(!rc&&post_commit_io==1&&n==32&&a%8192==0x1000)post_commit_io=2;if(!rc&&post_commit_start&&n==32&&a%8192==0x1000)transport_req=1;if(!rc&&after_commit&&n==32&&a%8192==0x1000)after_commit();return rc;}
#ifdef D8FLASH_QUEUE_TEST
#include "native_queue_audio_fixture.h"
#else
/* Storage-only fixtures have no audio DMA or USB peripheral. */
static int audio_output_quiet(void){return 1;}
#endif
#include "../firmware/src/d8p1_flash_runtime.c"
static void reset(void){reads=writes=irq_disabled=0;cut=late_irq=-1;post_commit_start=post_commit_io=0;transport_req=0;inject_at=-1;irq_inject=after_commit=NULL;}
static void blank(void){memset(nor,0xa5,sizeof nor);for(unsigned b=0;b<5;b++)memset(nor+d8pool_mapped_address(b),255,8192);reset();}
/* Test-only seed for an explicitly migrated image; bypass is not device code. */
static int rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;memcpy(p,nor+d8pool_mapped_address(a/8192)+a%8192,n);return 0;}
static int er(void *c,uint32_t a){(void)c;memset(nor+d8pool_mapped_address(a/8192)+a%8192,255,4096);return 0;}
static int pg(void *c,uint32_t a,const void *p,uint32_t n){(void)c;const uint8_t *q=p;uint32_t base=d8pool_mapped_address(a/8192)+a%8192;for(unsigned i=0;i<n;i++)nor[base+i]&=q[i];return 0;}
static int stop(void *c){(void)c;return 1;}
static d8pool seed={NULL,rd,er,pg,stop};
static void unchanged(d8pool_index *index){d8pool_index now;CHECK(!d8pool_inventory(&seed,&now)&&now.present==index->present);for(unsigned o=0;o<4;o++)CHECK(!memcmp(&now.object[o],&index->object[o],sizeof now.object[o]));CHECK(!memcmp(nor,baseline,0x97000)&&!memcmp(nor+0x9f000,baseline+0x9f000,0x46000)&&!memcmp(nor+0xe7000,baseline+0xe7000,sizeof nor-0xe7000));}

#ifndef D8FLASH_RUNTIME_NO_MAIN
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
 d8p1_project_catalog catalog,previous;memset(&catalog,0xa5,sizeof catalog);previous=catalog;reset();
 CHECK(d8p1_catalog_flash(&catalog,0)==D8POOL_UNSUPPORTED&&!reads&&!writes&&!memcmp(&catalog,&previous,sizeof catalog));
 CHECK(d8p1_rename_flash(0,"NAME",0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 CHECK(d8p1_rename_flash(3,"NAME",1)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(!d8p1_catalog_flash(&catalog,1)&&catalog.present==7&&!writes);
 trk[7].p[P_LEVEL]=91;proj_cur=2;reset();CHECK(!d8p1_rename_flash(2,"NATIVE NAME",1)&&trk[7].p[P_LEVEL]==91&&!strcmp(proj_name,"NATIVE NAME")&&!irq_disabled);
 CHECK(!d8p1_catalog_flash(&catalog,1)&&!strcmp(catalog.name[2],"NATIVE NAME"));
 CHECK(!d8p1_load_flash(2,1)&&trk[7].p[P_LEVEL]==44&&!strcmp(proj_name,"NATIVE NAME"));mix_block(audio,CTL);
 reset();CHECK(d8p1_save_as_flash(0,"DENIED",0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 CHECK(d8p1_save_as_flash(3,"AUTO",1)==D8POOL_INVALID&&!reads&&!writes);
 trk[7].p[P_LEVEL]=92;CHECK(!d8p1_save_as_flash(2,"NAMED SAVE",1)&&proj_cur==2&&!strcmp(proj_name,"NAMED SAVE")&&trk[7].p[P_LEVEL]==92);
 trk[7].p[P_LEVEL]=7;CHECK(!d8p1_load_flash(2,1)&&trk[7].p[P_LEVEL]==92&&!strcmp(proj_name,"NAMED SAVE"));mix_block(audio,CTL);
 reset();d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));memcpy(baseline,nor,sizeof nor);
 trk[7].p[P_LEVEL]=90;n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));uint8_t current=proj_cur;
 unsigned ops=2+(unsigned)((n+255)/256)+1;
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();cut=(int)c;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&proj_cur==current&&trk[7].p[P_LEVEL]==90&&!irq_disabled);reset();unchanged(&index);}
 /* Inject PLAY at each critical-section entry, after prior stopped checks. */
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();late_irq=(int)c;CHECK(d8p1_save_flash(0,1)==D8POOL_IO&&writes==c&&transport_req&&!irq_disabled&&proj_cur==current);reset();unchanged(&index);}
 /* The actual existing-driver rename adapter has the same atomic guards. */
 char oldname[sizeof proj_name];memcpy(oldname,proj_name,sizeof oldname);
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();cut=(int)c;CHECK(d8p1_rename_flash(2,"CUT",1)==D8POOL_IO&&proj_cur==current&&!memcmp(oldname,proj_name,sizeof oldname)&&trk[7].p[P_LEVEL]==90&&!irq_disabled);reset();unchanged(&index);}
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();late_irq=(int)c;CHECK(d8p1_rename_flash(2,"LATE",1)==D8POOL_IO&&writes==c&&transport_req&&!irq_disabled&&!memcmp(oldname,proj_name,sizeof oldname));reset();unchanged(&index);}
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();cut=(int)c;CHECK(d8p1_save_as_flash(0,"CUT SAVE",1)==D8POOL_IO&&proj_cur==current&&!memcmp(oldname,proj_name,sizeof oldname)&&trk[7].p[P_LEVEL]==90&&!irq_disabled);reset();unchanged(&index);}
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();late_irq=(int)c;CHECK(d8p1_save_as_flash(0,"LATE SAVE",1)==D8POOL_IO&&writes==c&&transport_req&&!irq_disabled&&proj_cur==current&&!memcmp(oldname,proj_name,sizeof oldname));reset();unchanged(&index);}
 memcpy(nor,baseline,sizeof nor);reset();post_commit_start=1;
 CHECK(d8p1_save_as_flash(0,"COMMITTED",1)==D8POOL_IO&&transport_req&&proj_cur==current&&!memcmp(oldname,proj_name,sizeof oldname)&&!irq_disabled);
 reset();CHECK(!d8p1_load_flash(0,1)&&proj_cur==0&&!strcmp(proj_name,"COMMITTED")&&trk[7].p[P_LEVEL]==90);mix_block(audio,CTL);
 /* The error meant final verification failed; the committed save exists.
  * Restore test baseline/live metadata before the remaining legacy cases. */
 proj_cur=current;memcpy(proj_name,oldname,sizeof oldname);
 memcpy(nor,baseline,sizeof nor);reset();post_commit_io=1;
 CHECK(d8p1_save_as_flash(0,"READBACK",1)==D8POOL_IO&&post_commit_io==2&&proj_cur==current&&!memcmp(oldname,proj_name,sizeof oldname)&&!irq_disabled);
 reset();CHECK(!d8p1_load_flash(0,1)&&proj_cur==0&&!strcmp(proj_name,"READBACK")&&trk[7].p[P_LEVEL]==90);mix_block(audio,CTL);
 proj_cur=current;memcpy(proj_name,oldname,sizeof oldname);
 memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8p1_restore_flash_autosave(1,1)&&trk[7].p[P_LEVEL]==45&&proj_cur==PROJ_NO_SLOT);mix_block(audio,CTL);
 reset();transport_req=1;CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&!reads&&!writes);transport_req=0;
 cv_begin(8,8,T_BG);reset();CHECK(d8p1_save_flash(0,1)==D8POOL_BUSY&&!reads&&!writes);cv_blit(0,0);
 CHECK(d8p1_save_flash(4,1)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(!irq_disabled&&!dma_errors);
 printf("D8P1 flash runtime: %u checks, %u failures; %u driver cuts, %u atomic late-start cuts; actual runtime, simulated physical hooks only\n",checks,failures,ops,ops);return failures!=0;
}

#endif /* D8FLASH_RUNTIME_NO_MAIN */
