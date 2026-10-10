/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8pool.h"
#include "d8p1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t nor[D8POOL_BYTES+512],snapshot[sizeof nor],old[D8P1_LIMIT],fresh[D8P1_LIMIT],out[D8P1_LIMIT];
static unsigned checks,failures,reads,writes;static int cut=-1,byte=-1,read_error=-1,busy_after=-1,busy,protect;
static size_t old_n,new_n;
#define CHECK(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static void reset(void){cut=byte=read_error=busy_after=-1;busy=protect=0;reads=writes=0;}
static void put(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int rd(void *ctx,uint32_t a,void *p,uint32_t n){(void)ctx;CHECK(a<=D8POOL_BYTES&&n<=D8POOL_BYTES-a);reads++;if(read_error==0)return -1;if(read_error>0)read_error--;memcpy(p,nor+256+a,n);return 0;}
static int mutate(void){if(cut==0)return -1;if(cut>0)cut--;writes++;return 0;}
static int er(void *ctx,uint32_t a){(void)ctx;CHECK(a%4096==0&&a<=D8POOL_BYTES-4096);if(mutate())return -1;if(!protect)memset(nor+256+a,255,4096);return 0;}
static int pg(void *ctx,uint32_t a,const void *p,uint32_t n){(void)ctx;const uint8_t *b=p;CHECK(n&&n<=256&&a<=D8POOL_BYTES-n&&(a&255)+n<=256);if(mutate())return -1;for(unsigned i=0;i<n;i++){if(byte==0)return -1;if(byte>0)byte--;if(!protect)nor[256+a+i]&=b[i];}return 0;}
static int stopped(void *ctx){(void)ctx;return !busy&&(busy_after<0||writes<(unsigned)busy_after);}
static d8pool pool={NULL,rd,er,pg,stopped};
static void blank(void){memset(nor,0xa5,sizeof nor);memset(nor+256,255,D8POOL_BYTES);reset();}
static size_t fixture(const char *name,uint8_t *p){FILE *f=fopen(name,"rb");if(!f)exit(2);size_t n=fread(p,1,D8P1_LIMIT,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);return n;}
static void guards(void){for(unsigned i=0;i<256;i++)CHECK(nor[i]==0xa5&&nor[256+D8POOL_BYTES+i]==0xa5);}
static int equal(unsigned object,const uint8_t *p,size_t n){size_t got=911;return !d8pool_load(&pool,object,out,sizeof out,&got,7)&&got==n&&!memcmp(p,out,n);}
static void unchanged(const d8pool_index *want){d8pool_index got;CHECK(!d8pool_inventory(&pool,&got));CHECK(got.present==want->present);for(unsigned o=0;o<4;o++)if(want->present&(1u<<o))CHECK(!memcmp(&got.object[o],&want->object[o],sizeof got.object[o]));}
static void header(unsigned b,unsigned o,uint32_t seq,const uint8_t *p,size_t n){uint8_t *h=nor+256+b*8192;memset(h,0,32);put(h,D8POOL_MAGIC);h[4]=1;h[5]=(uint8_t)b;put(h+8,o);put(h+12,seq);put(h+16,(uint32_t)n);put(h+20,d8p1_crc32(p,n));put(h+24,0xffffffffu);put(h+28,d8p1_crc32(h,28));memcpy(h+256,p,n);}
int main(int argc,char **argv){
 CHECK(argc==4);if(argc!=4)return 2;old_n=fixture(argv[1],old);new_n=fixture(argv[2],fresh);
 blank();d8pool_index index,before,sentinel;memset(&sentinel,0xa5,sizeof sentinel);index=sentinel;
 CHECK(!d8pool_inventory(&pool,&index)&&!index.present);
 d8pool_record r={9,8,7,6},saved=r;CHECK(d8pool_current(&pool,0,&r)==D8POOL_EMPTY&&!memcmp(&r,&saved,sizeof r));
 reset();
 /* Raw maximum fixture references legacy project 4: refuse before mutation. */
 CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_INVALID&&!reads&&!writes);
 d8p1_view v;CHECK(d8p1_read(&v,fresh,new_n)==1);
 for(unsigned i=0;i<v.count;i++)if((v.chunk[i].type&32767u)==7){uint8_t *b=(uint8_t *)v.chunk[i].data;for(unsigned j=0;j<b[0];j++)for(unsigned t=0;t<8;t++){uint8_t *ref=b+1+j*28+12+t*2;if(*ref==3)*ref=2;}put(b-4,d8p1_crc32(b,v.chunk[i].length));}
 put(fresh+12,0);put(fresh+12,d8p1_crc32(fresh,new_n));CHECK(d8p1_read(&v,fresh,new_n)==1&&d8p1_refs_available(&v,7));
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&pool,o,old,old_n,7)&&equal(o,old,old_n));
 CHECK(!d8pool_save(&pool,0,old,old_n,7));CHECK(!d8pool_inventory(&pool,&before)&&before.present==15);memcpy(snapshot,nor,sizeof nor);
 unsigned used=0;for(unsigned o=0;o<4;o++)used|=1u<<before.object[o].block;unsigned dest=0;while(used&(1u<<dest))dest++;
 unsigned ops=2+(unsigned)((new_n+255)/256)+1;
 for(unsigned c=0;c<ops;c++){memcpy(nor,snapshot,sizeof nor);reset();cut=(int)c;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_IO);reset();unchanged(&before);guards();}
 for(unsigned c=0;c<new_n+32;c++){memcpy(nor,snapshot,sizeof nor);reset();byte=(int)c;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_IO);reset();unchanged(&before);}
 for(unsigned sector=0;sector<2;sector++)for(unsigned n=0;n<=4096;n++){memcpy(nor,snapshot,sizeof nor);reset();if(sector)memset(nor+256+dest*8192,255,4096);memset(nor+256+dest*8192+sector*4096,255,n);unchanged(&before);}
 /* Every interrupted magic bit can still identify a torn new-format header. */
 for(unsigned m=0;m<32;m++){memcpy(nor,snapshot,sizeof nor);reset();uint8_t *h=nor+256+dest*8192;h[m/8]|=(uint8_t)(1u<<(m%8));unchanged(&before);}
 for(unsigned c=0;c<ops;c++){memcpy(nor,snapshot,sizeof nor);reset();busy_after=(int)c;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_BUSY&&writes==c);reset();unchanged(&before);}
 memcpy(nor,snapshot,sizeof nor);reset();CHECK(!d8pool_inventory(&pool,&index));unsigned scan_reads=reads;
 for(unsigned c=0;c<scan_reads;c++){memcpy(nor,snapshot,sizeof nor);reset();read_error=(int)c;index=sentinel;CHECK(d8pool_inventory(&pool,&index)==D8POOL_IO&&!memcmp(&index,&sentinel,sizeof index));reset();read_error=(int)c;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_IO&&!writes);}
 memcpy(nor,snapshot,sizeof nor);reset();CHECK(!d8pool_save(&pool,0,fresh,new_n,7));
 unsigned precommit_reads=reads-1-(unsigned)((new_n+255)/256);
 memcpy(nor,snapshot,sizeof nor);reset();read_error=(int)precommit_reads;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_IO&&writes==ops);reset();CHECK(equal(0,fresh,new_n));for(unsigned o=1;o<4;o++)CHECK(equal(o,old,old_n));
 memcpy(nor,snapshot,sizeof nor);reset();protect=1;CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_IO);reset();unchanged(&before);
 /* Unknown/foreign committed records and ambiguous sequences protect pool. */
 for(unsigned kind=0;kind<4;kind++){memcpy(nor,snapshot,sizeof nor);reset();uint8_t *h=nor+256+dest*8192;if(kind==0)h[4]=2;if(kind==1)put(h+8,4);if(kind==2)put(h,0x554c4546u);if(kind==3)h[6]=1;put(h+28,d8p1_crc32(h,28));index=sentinel;CHECK(d8pool_inventory(&pool,&index)==D8POOL_UNSUPPORTED&&!memcmp(&index,&sentinel,sizeof index));CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_UNSUPPORTED&&!writes);}
 memcpy(nor,snapshot,sizeof nor);reset();memcpy(nor+256+dest*8192,"FELU",4);CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_UNSUPPORTED&&!writes);
 for(unsigned kind=0;kind<2;kind++){memcpy(nor,snapshot,sizeof nor);reset();header(dest,0,before.object[0].sequence+(kind?0x80000000u:0),old,old_n);index=sentinel;CHECK(d8pool_inventory(&pool,&index)==D8POOL_AMBIGUOUS&&!memcmp(&index,&sentinel,sizeof index));CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_AMBIGUOUS&&!writes);}
 blank();header(0,0,0xfffffffeu,old,old_n);header(1,0,0xffffffffu,old,old_n);CHECK(!d8pool_save(&pool,0,fresh,new_n,7));CHECK(!d8pool_current(&pool,0,&r)&&r.sequence==0&&equal(0,fresh,new_n));
 blank();header(1,0,0,old,old_n);header(3,1,1,old,old_n);header(2,0,0x7fffffffu,old,old_n);CHECK(!d8pool_inventory(&pool,&index));reset();CHECK(d8pool_save(&pool,0,fresh,new_n,7)==D8POOL_AMBIGUOUS&&!writes);
 blank();header(0,0,0,old,old_n);header(1,0,0x60000000u,old,old_n);header(2,0,0xc0000000u,old,old_n);CHECK(d8pool_inventory(&pool,&index)==D8POOL_AMBIGUOUS);
 memcpy(nor,snapshot,sizeof nor);reset();CHECK(!d8pool_save(&pool,0,fresh,new_n,7));nor[256+dest*8192+256+500]^=1;CHECK(equal(0,old,old_n));
 /* Fill and rotate every object; there is no permanent paired location. */
 memcpy(nor,snapshot,sizeof nor);reset();for(unsigned i=0;i<1000;i++){unsigned o=i%4;const uint8_t *p=(i/4)%2?fresh:old;size_t n=(i/4)%2?new_n:old_n;CHECK(!d8pool_save(&pool,o,p,n,7)&&equal(o,p,n));CHECK(!d8pool_inventory(&pool,&index)&&index.present==15);unsigned occupied=0;for(unsigned j=0;j<4;j++){CHECK(!(occupied&(1u<<index.object[j].block)));occupied|=1u<<index.object[j].block;}}
 reset();CHECK(d8pool_save(&pool,4,old,old_n,7)==D8POOL_INVALID&&!reads&&!writes);CHECK(d8pool_save(&pool,0,old,old_n,8)==D8POOL_INVALID&&!reads);CHECK(d8pool_save(&pool,0,NULL,old_n,7)==D8POOL_INVALID);CHECK(d8pool_save(&pool,0,old,D8P1_LIMIT+1,7)==D8POOL_INVALID);busy=1;CHECK(d8pool_save(&pool,0,old,old_n,7)==D8POOL_BUSY&&!reads);reset();
 size_t unknown=fixture(argv[3],out);CHECK(d8pool_save(&pool,0,out,unknown,7)==D8POOL_INVALID&&!reads);
 size_t count=99;CHECK(d8pool_load(&pool,0,out,1,&count,7)==D8POOL_INVALID&&count==99);size_t *alias=(size_t *)out;CHECK(d8pool_load(&pool,0,out,sizeof out,alias,7)==D8POOL_INVALID);read_error=0;CHECK(d8pool_load(&pool,0,out,sizeof out,&count,7)==D8POOL_IO&&count==99);reset();guards();
 printf("D8P1 shared pool: %u checks, %u failures; %u operation cuts, %zu program-byte cuts, 8194 erase prefixes, 1000 rotating saves; virtual NOR only\n",checks,failures,ops,new_n+32);return failures!=0;
}
