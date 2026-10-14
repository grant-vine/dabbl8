/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static uint8_t nor[0x100000];
#define SMP_USER_XIP(k) (nor+0xa0000u+(k)*0x14000u)
#define D8_SAMPLE_SESSION 1
#define D8_NATIVE_LOCAL_SAVE_NO_MAIN 1
#include "native_local_save_test.c"
#define D8_SM_ERASE(a,t) ((void)(t),mc_virtual_erase(a))
#define D8_SM_PROGRAM(a,p,n,t) ((void)(t),mc_virtual_program(a,p,n))
#include "../firmware/src/native_sample_session.c"
static uint8_t sm_data[4097],sm_msg[600],max_data[SMP_USER_SIZE-SMP_USER_DATA];
static unsigned pack7(const uint8_t*p,unsigned n,uint8_t*out){unsigned z=0;while(n){unsigned k=n>7?7:n,m=0;for(unsigned i=0;i<k;i++)m|=(p[i]>>7)<<i;out[z++]=(uint8_t)m;for(unsigned i=0;i<k;i++)out[z++]=p[i]&127;p+=k;n-=k;}return z;}
static unsigned sm_request(unsigned op,unsigned count){uint8_t f[600]={0x7d,0x46,0x4c,80};memcpy(f+4,sm_msg,count);ed_handle(f,count+4);so_r=so_w;CHECK(ed_n==20&&ed_out[4]==80&&ed_out[5]==1&&ed_out[6]==op);return ed_out[7];}
static void sm_token(unsigned op){memset(sm_msg,0,sizeof sm_msg);sm_msg[0]=1;sm_msg[1]=op;mc_put32(sm_msg+2,d8ls.generation);}
#include "native_consent_pixels.h"
int main(void){
 boot_seed();CHECK(project_native_status()==0||project_native_status()==1);CHECK(autosave_boot(1)==1);int32_t audio[CTL*2];mix_block(audio,CTL);
 for(unsigned i=0;i<sizeof sm_data;i++)sm_data[i]=(uint8_t)(i*37u);
 smp_user_hdr_t h={0};h.magic=SMP_USER_MAGIC;h.version=1;h.nz=1;h.data_len=sizeof sm_data;h.crc=st_crc32(sm_data,sizeof sm_data);h.zone[0].n=2*sizeof sm_data;h.zone[0].le=h.zone[0].n-1;h.zone[0].rate=65536;h.zone[0].hi=127;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;unsigned n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;
 CHECK(sm_request(1,n)==SM_OK&&d8ls.phase==LS_CONSENT&&!physical_calls);unsigned sample_generation=smp_user_gen;consent_pixels("sample",70,AF_M.h,108,AF_S.h);CHECK(!physical_calls);d8ls_draw();CHECK(d8ls.shown==d8ls.generation);mc_ship_tap(B_SAVE);CHECK(d8ls.phase==LS_RELEASE);d8ls_poll();CHECK(d8ls.phase==LS_EXECUTE&&!usr_nz[0]&&!slc_usr[0].len&&smp_user_gen==sample_generation+1);
 for(unsigned off=0;off<sizeof sm_data;off+=256){unsigned count=sizeof sm_data-off;if(count>256)count=256;sm_token(3);mc_put32(sm_msg+7,off+512);n=pack7(sm_data+off,count,sm_msg+12)+12;CHECK(sm_request(3,n)==SM_OK);unsigned before=physical_calls;CHECK(sm_request(3,n)==SM_OK&&physical_calls==before);if(!off){uint8_t bad[256];memcpy(bad,sm_data,256);bad[0]^=1;n=pack7(bad,256,sm_msg+12)+12;CHECK(sm_request(3,n)==SM_BAD&&physical_calls==before);}}
 sm_token(4);CHECK(sm_request(4,7)==SM_OK&&d8ls.phase==LS_IDLE&&usr_nz[0]==1&&!migration_owner);CHECK(!memcmp(smp_user_xip(0)+512,sm_data,sizeof sm_data));CHECK(slc_usr[0].len==h.zone[0].n&&smp_user_gen==sample_generation+2);CHECK(!d8_instrument_write_allowed());
 sm_token(5);CHECK(sm_request(5,7)==SM_OK&&d8ls.write_phase==SM_DONE&&smp_user_gen==sample_generation+2);
 uint8_t legacy[]={0x7d,0x46,0x4c,ED_SMP_BEGIN,0};unsigned legacy_calls=physical_calls;ed_handle(legacy,sizeof legacy);so_r=so_w;CHECK(ed_n==9&&ed_out[4]==ED_D8_ERROR&&physical_calls==legacy_calls);
 /* Every destructive primitive cut, including sector boundary and final magic.
  * Old selected sample may be lost; other slots/pool must stay byte-exact. */
 unsigned operations=physical_calls;
 for(unsigned point=0;point<operations;point++){
  boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);partial_failure=0;
  sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;
  CHECK(sm_request(1,n)==SM_OK);memcpy(local_before,nor,sizeof nor);
  d8ls_draw();mc_ship_tap(B_SAVE);cut=(int)point;partial_failure=17;d8ls_poll();
  for(unsigned off=0;off<sizeof sm_data&&d8ls.phase==LS_EXECUTE;off+=256){
   unsigned count=sizeof sm_data-off;if(count>256)count=256;sm_token(3);mc_put32(sm_msg+7,off+512);
   n=pack7(sm_data+off,count,sm_msg+12)+12;(void)sm_request(3,n);
  }
  if(d8ls.phase==LS_EXECUTE){sm_token(4);(void)sm_request(4,7);}
  CHECK(d8ls.phase==LS_IDLE&&!migration_owner);CHECK(!memcmp(nor,local_before,SMP_USER_BASE));
  CHECK(!memcmp(nor+SMP_USER_BASE+SMP_USER_SIZE,local_before+SMP_USER_BASE+SMP_USER_SIZE,sizeof nor-SMP_USER_BASE-SMP_USER_SIZE));
  smp_user_scan(0);if(point+1==operations)CHECK(usr_nz[0]==1);CHECK(!usr_nz[0]||(!memcmp(smp_user_xip(0),&h,sizeof h)&&!memcmp(smp_user_xip(0)+512,sm_data,sizeof sm_data)));
 }
 /* A partially programmed magic remains unpublished even after all data/header. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);partial_failure=0;cut=-1;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;
 CHECK(sm_request(1,n)==SM_OK);d8ls_draw();mc_ship_tap(B_SAVE);d8ls_poll();
 for(unsigned off=0;off<sizeof sm_data;off+=256){unsigned count=sizeof sm_data-off;if(count>256)count=256;sm_token(3);mc_put32(sm_msg+7,off+512);n=pack7(sm_data+off,count,sm_msg+12)+12;CHECK(sm_request(3,n)==SM_OK);}
 cut=2;partial_failure=2;sm_token(4);CHECK(sm_request(4,7)==SM_IO&&!usr_nz[0]&&!migration_owner);
 /* Malformed preparation refuses before mutation; visible LCD fault cancels. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);partial_failure=0;cut=-1;
 unsigned before=physical_calls;h.zone[0].off=h.data_len+1;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;
 CHECK(sm_request(1,n)==SM_BAD&&physical_calls==before);h.zone[0].off=0;
 n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_OK);
 local_lcd_event=4;d8ls_draw();CHECK(!d8ls.shown);mc_ship_tap(B_SAVE);CHECK(d8ls.phase!=LS_RELEASE&&physical_calls==before);
 d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&!migration_owner);
 /* Other editor commands revoke but cannot overwrite the prepared header. */
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;
 CHECK(sm_request(1,n)==SM_OK);uint32_t header_crc=d8p1_crc32(ed_smp_buf,sizeof h);
 uint8_t other[]={0x7d,0x46,0x4c,ED_SMP_INFO};ed_handle(other,sizeof other);so_r=so_w;
 CHECK(d8ls.revoked&&header_crc==d8p1_crc32(ed_smp_buf,sizeof h));d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&physical_calls==before);
 /* Slot2 maximum ends exactly at the third slot boundary; oversize refuses. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);partial_failure=0;cut=-1;
 before=physical_calls;h.data_len=sizeof max_data+1;sm_msg[0]=sm_msg[1]=1;sm_msg[2]=2;
 n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_BAD&&physical_calls==before);
 for(unsigned i=0;i<sizeof max_data;i++)max_data[i]=(uint8_t)(i*13u);
 h.data_len=sizeof max_data;h.crc=st_crc32(max_data,sizeof max_data);h.zone[0].n=2*sizeof max_data;h.zone[0].le=h.zone[0].n-1;
 n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_OK);memcpy(local_before,nor,sizeof nor);d8ls_draw();mc_ship_tap(B_SAVE);d8ls_poll();
 for(unsigned off=0;off<sizeof max_data;off+=256){sm_token(3);mc_put32(sm_msg+7,off+512);n=pack7(max_data+off,256,sm_msg+12)+12;CHECK(sm_request(3,n)==SM_OK);if(!((off+512)&4095u)){before=physical_calls;CHECK(sm_request(3,n)==SM_OK&&physical_calls==before);}}
 sm_token(4);CHECK(sm_request(4,7)==SM_OK&&usr_nz[2]==1);CHECK(!memcmp(nor+SMP_USER_BASE+2*SMP_USER_SIZE+512,max_data,sizeof max_data));
 CHECK(!memcmp(nor,local_before,SMP_USER_BASE+2*SMP_USER_SIZE));CHECK(!memcmp(nor+SMP_USER_BASE+3*SMP_USER_SIZE,local_before+SMP_USER_BASE+3*SMP_USER_SIZE,sizeof nor-SMP_USER_BASE-3*SMP_USER_SIZE));
 /* Lost quiet/arena revokes immediately, not after the long upload timeout. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);h.data_len=sizeof sm_data;h.crc=st_crc32(sm_data,sizeof sm_data);h.zone[0].n=2*sizeof sm_data;h.zone[0].le=h.zone[0].n-1;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_OK);d8ls_draw();mc_ship_tap(B_SAVE);d8ls_poll();before=physical_calls;transport_req=1;d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&!migration_owner&&physical_calls==before);transport_req=0;
 /* Actual wire shape/token and out-of-order guards are non-destructive. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);cut=-1;partial_failure=0;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_OK);
 before=physical_calls;sm_token(2);sm_msg[6]=16;CHECK(sm_request(2,7)==SM_STALE&&physical_calls==before&&d8ls.phase==LS_CONSENT);
 sm_token(2);sm_msg[2]^=1;CHECK(sm_request(2,7)==SM_STALE&&physical_calls==before);
 d8ls_draw();mc_ship_tap(B_SAVE);d8ls_poll();before=physical_calls;sm_token(3);mc_put32(sm_msg+7,768);n=pack7(sm_data,256,sm_msg+12)+12;CHECK(sm_request(3,n)==SM_BAD&&physical_calls==before&&d8ls.offset==512);
 /* Lose the lease to another owner: never release that other owner's arena. */
 uint32_t old=d8ls.signature,other_owner=0;CHECK(!main_migration_end(old));CHECK(!main_migration_begin(&other_owner));d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&migration_owner==other_owner&&physical_calls==before);CHECK(!main_migration_end(other_owner));
 /* Display-bound original slot identity cannot be replaced behind consent. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_OK);d8ls_draw();nor[SMP_USER_BASE+1000]^=1;before=physical_calls;mc_ship_tap(B_SAVE);d8ls_poll();CHECK(d8ls.phase==LS_IDLE&&physical_calls==before&&!migration_owner);
 /* Valid future own header defeats a cached READY state before any erase. */
 boot_seed();CHECK(autosave_boot(1)==1);mix_block(audio,CTL);uint8_t*future=nor+d8pool_mapped_address(0);future[4]=99;uint32_t future_crc=d8p1_crc32(future,28);for(unsigned i=0;i<4;i++)future[28+i]=(uint8_t)(future_crc>>(8*i));before=physical_calls;
 sm_msg[0]=sm_msg[1]=1;sm_msg[2]=0;n=pack7((const uint8_t*)&h,sizeof h,sm_msg+3)+3;CHECK(sm_request(1,n)==SM_BUSY&&physical_calls==before&&!migration_owner);
 printf("Native own sample upload: %u checks %u failures, %u primitive cuts; actual4097B/boundary/finalpartial, slot2max, virtualNOR only\n",checks,failures,operations);return failures!=0;
}
