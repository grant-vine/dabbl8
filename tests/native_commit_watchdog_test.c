/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual dispatcher/COMMIT plus private NOR effects. No physical device. */
#include <stdint.h>
#include <setjmp.h>
static int checked_erase(uint32_t,uint32_t *);
static int checked_program(uint32_t,const uint8_t *,uint32_t,uint32_t *);
#define D8_PROJECT_TEST_ERASE checked_erase
#define D8_PROJECT_TEST_PROGRAM checked_program
#define D8_PROJECT_TRANSFER_NO_MAIN 1
#define D8_PROJECT_DIAGNOSTIC_TEST 1
#define DABBL8_TIMING_DIAGNOSTIC 1
#include "native_project_transfer_test.c"
static unsigned feed_calls,feed_failure,interrupt_erase,erase_calls;
static jmp_buf reset_point;
static void checked_feed(void)
{
 CHECK(irq_disabled&&pt_lifetime()&&d8ls.writing&&!d8ls.busy);
 CHECK(d8pt_diag.stage==PTD_STORE&&feed_calls==physical_calls);
 feed_calls++;
 if(feed_failure==feed_calls)fm1_p33_timeouts++;
}
static int checked_erase(uint32_t a,uint32_t*t)
{
 CHECK(irq_disabled&&d8ls.busy&&feed_calls==physical_calls+1);
 CHECK(d8pt_diag.address==a&&d8pt_diag.operation==1&&d8pt_diag.feeds==feed_calls);
 if(interrupt_erase&&++erase_calls==interrupt_erase)longjmp(reset_point,1);
 return mc_ram_erase_spy(a,t);
}
static int checked_program(uint32_t a,const uint8_t*p,uint32_t n,uint32_t*t)
{
 CHECK(irq_disabled&&d8ls.busy&&feed_calls==physical_calls+1);
 CHECK(d8pt_diag.address==a&&d8pt_diag.operation==2&&d8pt_diag.feeds==feed_calls);
 return mc_ram_program_spy(a,p,n,t);
}
static void prepare_commit(void)
{
 target_fresh();CHECK(import_start(15)==PT_OK);import_receive();approve_restore();
 feed_calls=feed_failure=interrupt_erase=erase_calls=fm1_p33_timeouts=0;
 host_wdt_hook=checked_feed;
}
static void retained_startup(void)
{
 /* Target cstart calls the retention hook before clearing ordinary BSS and
  * pool state. The linker check separately proves .noinit is excluded. */
 d8pt_diag_boot(4,1);d8pt_observing=0;host_wdt_hook=NULL;
 /* Unlike a returned error, a reset interrupts an owned workspace. These
  * .pool/BSS owners are also cleared by the actual startup loops. */
 migration_owner=migration_generation=0;memset(&main_workspace,0,sizeof main_workspace);
 cv_cpu_active=cv_canvas_valid=0;irq_disabled=0;
 retained_reboot();
}
int main(void)
{
 setvbuf(stdout,NULL,_IONBF,0);
 volatile uint8_t*raw=(volatile uint8_t*)&d8pt_diag;
 for(unsigned i=0;i<sizeof d8pt_diag;i++)raw[i]=0xa5;
 d8pt_diag_boot(1,0);
 CHECK(d8pt_diag.magic==D8PT_DIAG_MAGIC&&d8pt_diag.boot==1&&!d8pt_diag.record_boot&&!d8pt_diag.stage);
 boot_seed();CHECK(autosave_boot(1)==1);int32_t audio[CTL*2];mix_block(audio,CTL);
 CHECK(export_start()==PT_OK);memcpy(source_pool,pt_workspace()->plan,sizeof source_pool);
 archived_live_len=d8ls.length;memcpy(source_live,pt_workspace()->stage.wire,archived_live_len);
 pt_token(8);CHECK(pt_request(7)==PT_OK);

 prepare_commit();pt_token(7);CHECK(pt_request(7)==PT_OK);
 CHECK(physical_calls>100&&feed_calls==physical_calls&&d8pt_diag.completed==physical_calls);
 CHECK(d8pt_diag.stage==PTD_DONE&&!d8pt_diag.raw_rc&&!d8pt_diag.p33_failures&&!irq_disabled);
 verify_restored(15);host_wdt_hook=NULL;
 printf("Completed COMMIT: %u primitives, %u checked feed boundaries\n",physical_calls,feed_calls);
 /* Export copy runs after initial admission, with IRQs enabled. A same-boot
  * revoke there must not append its bits to the preceding import incident. */
 __typeof__(d8pt_diag) same_boot=d8pt_diag;
 unsigned completed_calls=physical_calls;pt_read_event=7;
 CHECK(export_start()==PT_IO&&!pt_read_event);
 CHECK(!memcmp((const void*)&same_boot,(const void*)&d8pt_diag,sizeof same_boot)&&physical_calls==completed_calls);

 for(unsigned boundary=1;boundary<=3;boundary++){
  prepare_commit();feed_failure=boundary;memcpy(local_before,nor,sizeof nor);
  pt_token(7);CHECK(pt_request(7)==PT_IO);
  CHECK(feed_calls==boundary&&physical_calls==boundary-1&&d8pt_diag.feeds==boundary-1);
  CHECK(d8pt_diag.p33_failures==1&&d8pt_diag.stage==PTD_STORE&&d8pt_diag.raw_rc&&!irq_disabled);
  CHECK(!d8ls.writing&&!d8ls.busy&&d8ls.phase==LS_IDLE&&!migration_owner);
  if(boundary==1)CHECK(!memcmp(nor,local_before,sizeof nor));
  unsigned before=physical_calls,feeds=feed_calls;pt_token(7);CHECK(pt_request(7)!=PT_OK);
  CHECK(physical_calls==before&&feed_calls==feeds);host_wdt_hook=NULL;
 }
 /* A failed actual NOR primitive receives no subsequent feed/write, and
  * leaves the completed count behind the attempted operation. */
 prepare_commit();cut=2;partial_failure=17;pt_token(7);CHECK(pt_request(7)==PT_IO);
 CHECK(feed_calls==3&&physical_calls==3&&d8pt_diag.completed==2&&!irq_disabled);
 host_wdt_hook=NULL;cut=-1;partial_failure=0;

 prepare_commit();interrupt_erase=3;
 if(!setjmp(reset_point)){pt_token(7);(void)pt_request(7);CHECK(0);}
 CHECK(d8pt_diag.stage==PTD_STORE&&d8pt_diag.raw_rc==(uint32_t)-32768&&d8pt_diag.operation==1);
 uint32_t record=d8pt_diag.record_boot,address=d8pt_diag.address,completed=d8pt_diag.completed;
 retained_startup();
 CHECK(d8pt_diag.boot==record+1&&d8pt_diag.record_boot==record&&d8pt_diag.reset_reason==4&&d8pt_diag.reset_src==1);
 CHECK(d8pt_diag.stage==PTD_STORE&&d8pt_diag.raw_rc==(uint32_t)-32768&&d8pt_diag.address==address&&d8pt_diag.completed==completed);
 CHECK(!d8pt_observing&&!d8ls.intent&&!d8ls.signature&&!d8ls.writing);
 unsigned before=physical_calls;pt_token(7);CHECK(pt_request(7)!=PT_OK&&physical_calls==before);
 /* Read-only begin/cancel/revocation and refused imports preserve incident. */
 __typeof__(d8pt_diag) incident=d8pt_diag;
 CHECK(export_start()==PT_OK);CHECK(import_start(15)==PT_BUSY);
 d8pt_observe_revoke(0x80000000u);pt_token(8);CHECK(pt_request(7)==PT_OK);
 CHECK(!memcmp((const void*)&incident,(const void*)&d8pt_diag,sizeof incident)&&physical_calls==before);
 CHECK(import_start(15)==PT_OK);
 CHECK(d8pt_diag.record_boot==d8pt_diag.boot&&!d8pt_diag.stage&&!d8pt_diag.raw_rc&&!d8pt_diag.completed&&!d8pt_diag.feeds);
 pt_token(8);CHECK(pt_request(7)==PT_OK);
 d8pt_diag.boot=UINT32_MAX;d8pt_diag_boot(4,1);
 CHECK(d8pt_diag.boot==1&&!d8pt_diag.record_boot&&!d8pt_diag.stage);
 printf("COMMIT watchdog/retention: %u checks %u failures\n",checks,failures);
 return failures!=0;
}
