/* SPDX-License-Identifier: GPL-3.0-only */
#define D8POOL_TEST_NO_MAIN 1
#include "d8pool_test.c"
static int uncertain;
static d8pool_index before;
static int upg(void*c,uint32_t a,const void*p,uint32_t n){int rc=pg(c,a,p,n);return !rc&&uncertain&&n==32?-1:rc;}
static d8pool remove_pool={NULL,rd,er,upg,stopped};
static void seed_all(void)
{
 blank();uncertain=0;for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&remove_pool,o,old,old_n,7));
 reset();CHECK(!d8pool_inventory(&remove_pool,&before)&&before.present==15&&before.anchors==15);
 memcpy(snapshot,nor,sizeof nor);
}
static void restore(void){memcpy(nor,snapshot,sizeof nor);reset();uncertain=0;}
int main(void)
{
 old_n=fixture("tests/fixtures/d8p1/minimal.d8p",old);
 blank();CHECK(!d8pool_remove(&remove_pool,1)&&!writes);
 seed_all();unsigned dest=0,used=0;for(unsigned o=0;o<4;o++)used|=1u<<before.object[o].block;while(used&(1u<<dest))dest++;
 for(unsigned c=0;c<3;c++){restore();cut=(int)c;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO);reset();CHECK(equal(1,old,old_n));unchanged(&before);guards();}
 for(unsigned c=0;c<32;c++){restore();byte=(int)c;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO);reset();CHECK(equal(1,old,old_n));}
 restore();CHECK(!d8pool_remove(&remove_pool,1)&&writes==3);
 d8pool_index ix;CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==13&&ix.anchors==15);
 CHECK(ix.object[1].block==dest&&!ix.object[1].length&&!ix.object[1].crc&&ix.object[1].sequence==2);
 d8pool_record r={9,8,7,6},sent=r;CHECK(d8pool_current(&remove_pool,1,&r)==D8POOL_EMPTY&&!memcmp(&r,&sent,sizeof r));
 size_t count=123;CHECK(d8pool_load(&remove_pool,1,out,sizeof out,&count,7)==D8POOL_EMPTY&&count==123);
 reset();CHECK(!d8pool_remove(&remove_pool,1)&&!writes);
 for(unsigned i=0;i<20;i++)CHECK(!d8pool_save(&remove_pool,i%2?0:2,old,old_n,7));
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==13&&ix.anchors==15&&ix.object[1].block==dest);
 CHECK(!d8pool_save(&remove_pool,1,old,old_n,7)&&equal(1,old,old_n));
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==15&&ix.anchors==15&&ix.object[1].sequence==3);
 /* Uncertain full header publishes deletion; repeated request reconciles. */
 restore();uncertain=1;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO);uncertain=0;reset();
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==13&&ix.anchors==15);
 CHECK(!d8pool_remove(&remove_pool,1)&&!writes);
 /* Readback failure after complete publication also preserves deletion. */
 restore();CHECK(!d8pool_remove(&remove_pool,1));unsigned precommit_reads=reads-33;
 restore();read_error=(int)precommit_reads;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO&&writes==3);
 reset();CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==13&&ix.anchors==15);
 CHECK(!d8pool_remove(&remove_pool,1)&&!writes);
 /* Stop/read/write-protection and future/malformed tombstones never grant erase. */
 for(unsigned c=0;c<3;c++){restore();busy_after=(int)c;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_BUSY&&writes==c);reset();CHECK(equal(1,old,old_n));}
 restore();read_error=0;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO&&!writes);
 restore();protect=1;CHECK(d8pool_remove(&remove_pool,1)==D8POOL_IO);reset();CHECK(equal(1,old,old_n));
 for(unsigned kind=0;kind<8;kind++){
  restore();CHECK(!d8pool_remove(&remove_pool,1));reset();uint8_t*h=nor+256+dest*8192;
  if(kind==0)h[4]=3;if(kind==1)put(h+16,1);if(kind==2)put(h+20,1);if(kind==3)h[5]=(uint8_t)((dest+1)%5);
  if(kind==4)h[6]=1;if(kind==5)put(h+24,0);if(kind==6)h[32]=0;if(kind==7)put(h+8,4);
  put(h+28,d8p1_crc32(h,28));d8pool_index sentinel;memset(&sentinel,0xa5,sizeof sentinel);ix=sentinel;
  CHECK(d8pool_inventory(&remove_pool,&ix)==D8POOL_UNSUPPORTED&&!memcmp(&ix,&sentinel,sizeof ix));
  CHECK(d8pool_remove(&remove_pool,1)==D8POOL_UNSUPPORTED&&!writes);
  CHECK(d8pool_save(&remove_pool,0,old,old_n,7)==D8POOL_UNSUPPORTED&&!writes);
 }
 /* Tombstones participate in duplicate, half-range and wrap ordering. */
 for(unsigned kind=0;kind<2;kind++){restore();CHECK(!d8pool_remove(&remove_pool,1));uint8_t*h=nor+256+dest*8192;
  put(h+12,before.object[1].sequence+(kind?0x80000000u:0));put(h+28,d8p1_crc32(h,28));reset();
  CHECK(d8pool_inventory(&remove_pool,&ix)==D8POOL_AMBIGUOUS);
  CHECK(d8pool_remove(&remove_pool,1)==D8POOL_AMBIGUOUS&&!writes);}
 blank();header(0,1,0xffffffffu,old,old_n);CHECK(!d8pool_remove(&remove_pool,1));
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&!ix.present&&ix.anchors==2&&!ix.object[1].sequence);
 CHECK(!d8pool_save(&remove_pool,1,old,old_n,7)&&!d8pool_current(&remove_pool,1,&r)&&r.sequence==1);
 /* Deleting every role retains four identity anchors and one rotating spare. */
 seed_all();for(unsigned o=0;o<4;o++)CHECK(!d8pool_remove(&remove_pool,o));
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&!ix.present&&ix.anchors==15);
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&remove_pool,o,old,old_n,7));
 CHECK(!d8pool_inventory(&remove_pool,&ix)&&ix.present==15&&ix.anchors==15);
 reset();CHECK(d8pool_remove(&remove_pool,4)==D8POOL_INVALID&&!reads&&!writes);guards();
 printf("Own pool remove: %u checks, %u failures; cuts3/header-prefix32/uncertain/tomb-anchor/restore/wrap/future; index%zu bytes\n",checks,failures,sizeof(d8pool_index));
 return failures!=0;
}
