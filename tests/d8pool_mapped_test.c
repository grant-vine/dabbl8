/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual mapped session and shared writer over 1MiB simulated NOR only. */
#include "d8pool_mapped.h"
#include "d8p1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char nor[0x100000],baseline[sizeof nor],wire[D8P1_LIMIT],fresh[D8P1_LIMIT],out[D8P1_LIMIT],all[D8POOL_BYTES];
static unsigned checks,failures,reads,writes;static int busy,cut=-1,read_error=-1,busy_after=-1,busy_on_read=-1;
#define CHECK(x) do {checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static int allowed(uint32_t a,uint32_t n){for(unsigned b=0;b<5;b++){uint32_t base=d8pool_mapped_address(b);if(a>=base&&a-base<=8192&&n<=8192-(a-base))return 1;}return 0;}
static void reset(void){reads=writes=0;busy=0;cut=read_error=busy_after=busy_on_read=-1;}
static int rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;CHECK(allowed(a,n));reads++;if(busy_on_read==(int)reads)busy=1;if(read_error==0)return -1;if(read_error>0)read_error--;memcpy(p,nor+a,n);return 0;}
static int mutation(void){if(cut==0)return -1;if(cut>0)cut--;writes++;return 0;}
static int er(void *c,uint32_t a){(void)c;CHECK(allowed(a,4096)&&a%4096==0);if(mutation())return -1;memset(nor+a,255,4096);return 0;}
static int pg(void *c,uint32_t a,const void *p,uint32_t n){(void)c;const unsigned char *q=p;CHECK(allowed(a,n)&&n&&n<=256&&(a&255)+n<=256);if(mutation())return -1;for(unsigned i=0;i<n;i++)nor[a+i]&=q[i];return 0;}
static int stopped(void *c){(void)c;return !busy&&(busy_after<0||writes<(unsigned)busy_after);}
static d8pool_physical io={NULL,rd,er,pg,stopped};
/* Explicit test-only native seed: not an implementation of device migration. */
static int seedread(void *c,uint32_t a,void *p,uint32_t n){return rd(c,d8pool_mapped_address(a/8192)+a%8192,p,n);}
static int seederase(void *c,uint32_t a){return er(c,d8pool_mapped_address(a/8192)+a%8192);}
static int seedprogram(void *c,uint32_t a,const void *p,uint32_t n){return pg(c,d8pool_mapped_address(a/8192)+a%8192,p,n);}
static d8pool seed={NULL,seedread,seederase,seedprogram,stopped};
static void blank(void){memset(nor,0xa5,sizeof nor);for(unsigned b=0;b<5;b++)memset(nor+d8pool_mapped_address(b),255,8192);reset();}
static size_t fixture(const char *name,unsigned char *p){FILE *f=fopen(name,"rb");if(!f)exit(2);size_t n=fread(p,1,D8P1_LIMIT,f);CHECK(fgetc(f)==EOF&&!ferror(f));fclose(f);return n;}
static void protected(void){CHECK(!memcmp(nor,baseline,0x97000));CHECK(!memcmp(nor+0x9f000,baseline+0x9f000,0x46000));CHECK(!memcmp(nor+0xe7000,baseline+0xe7000,sizeof nor-0xe7000));}
static void currents(const d8pool_index *before){d8pool_index now;CHECK(!d8pool_inventory(&seed,&now)&&now.present==before->present);for(unsigned o=0;o<4;o++)CHECK(!memcmp(&now.object[o],&before->object[o],sizeof before->object[o]));}
int main(int argc,char **argv){
 CHECK(argc==3);if(argc!=3)return 2;size_t n=fixture(argv[1],wire),fresh_n=fixture(argv[2],fresh);
 d8p1_view v;CHECK(d8p1_read(&v,fresh,fresh_n)==1);
 /* This synthetic copy resolves unavailable fourth-slot bank references to 2. */
 for(unsigned i=0;i<v.count;i++)if((v.chunk[i].type&32767u)==7){uint8_t *p=(uint8_t *)v.chunk[i].data;for(unsigned j=0;j<p[0];j++)for(unsigned t=0;t<8;t++){uint8_t *r=p+1+j*28+12+t*2;if(*r==3)*r=2;}uint32_t c=d8p1_crc32(p,v.chunk[i].length);for(unsigned j=0;j<4;j++)p[-4+(int)j]=(uint8_t)(c>>(8*j));}
 memset(fresh+12,0,4);uint32_t crc=d8p1_crc32(fresh,fresh_n);for(unsigned j=0;j<4;j++)fresh[12+j]=(uint8_t)(crc>>(8*j));CHECK(d8p1_read(&v,fresh,fresh_n)==1&&d8p1_refs_available(&v,7));
 blank();d8pool_mapped m;
 CHECK(d8pool_mapped_open(&m,&io,0)==D8POOL_UNSUPPORTED&&!reads&&!writes&&!m.state);
 CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_EMPTY&&!writes&&!m.state);
 CHECK(m.pool.erase(&m,0)!=0&&m.pool.program(&m,0,wire,32)!=0&&!writes);
 memcpy(nor+0x97000,"FELU",4);CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_UNSUPPORTED&&!writes&&!m.state);
 blank();for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();
 CHECK(!d8pool_mapped_open(&m,&io,1)&&m.state==2&&!writes);
 unsigned opened_reads=reads;
 CHECK(!m.pool.read(&m,0,all,sizeof all));for(unsigned b=0;b<5;b++)CHECK(!memcmp(all+b*8192,nor+d8pool_mapped_address(b),8192));
 const uint32_t expected[5]={0x97000,0x99000,0x9b000,0x9d000,0xe5000};for(unsigned b=0;b<5;b++)CHECK(d8pool_mapped_address(b)==expected[b]);
 CHECK(d8pool_mapped_address(4)==0xe5000&&d8pool_mapped_address(5)==0);
 unsigned before_reads=reads,before_writes=writes;
 CHECK(m.pool.read(&m,D8POOL_BYTES-1,out,2)!=0&&m.pool.read(&m,0xffffffff,out,1)!=0&&m.pool.read(&m,0,NULL,1)!=0);
 CHECK(m.pool.erase(&m,1)!=0&&m.pool.erase(&m,D8POOL_BYTES)!=0);
 CHECK(m.pool.program(&m,255,wire,2)!=0&&m.pool.program(&m,0,wire,257)!=0&&m.pool.program(&m,D8POOL_BYTES,wire,1)!=0);
 CHECK(reads==before_reads&&writes==before_writes);
 busy=1;CHECK(m.pool.erase(&m,0)!=0&&m.pool.program(&m,0,wire,32)!=0&&writes==before_writes);busy=0;
 d8pool_mapped_close(&m);CHECK(!m.state&&m.pool.read(&m,0,out,1)!=0&&m.pool.erase(&m,0)!=0&&!m.pool.stopped(&m));
 CHECK(!d8pool_mapped_open(&m,&m.physical,1));CHECK(d8pool_mapped_open(&m,&io,0)==D8POOL_UNSUPPORTED&&!m.state);
 CHECK(!d8pool_mapped_open(&m,&io,1));d8pool_index index;CHECK(!d8pool_inventory(&m.pool,&index));memcpy(baseline,nor,sizeof nor);
 unsigned ops=2+(unsigned)((fresh_n+255)/256)+1;
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8pool_mapped_open(&m,&io,1));cut=(int)c;CHECK(d8pool_save(&m.pool,0,fresh,fresh_n,7)==D8POOL_IO);reset();currents(&index);protected();d8pool_mapped_close(&m);}
 for(unsigned c=0;c<ops;c++){memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8pool_mapped_open(&m,&io,1));busy_after=(int)c;CHECK(d8pool_save(&m.pool,0,fresh,fresh_n,7)==D8POOL_BUSY);reset();currents(&index);protected();d8pool_mapped_close(&m);}
 for(unsigned c=0;c<opened_reads;c++){memcpy(nor,baseline,sizeof nor);reset();read_error=(int)c;CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_IO&&!m.state&&!writes);reset();currents(&index);protected();}
 memcpy(nor,baseline,sizeof nor);reset();busy_on_read=(int)opened_reads;CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_BUSY&&!m.state&&!writes);reset();currents(&index);protected();
 /* A stopped-state loss after commit may leave a valid new record: rescan. */
 memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8pool_mapped_open(&m,&io,1));busy_after=(int)ops;
 CHECK(d8pool_save(&m.pool,0,fresh,fresh_n,7)==D8POOL_IO&&writes==ops);reset();size_t got=0;
 CHECK(!d8pool_load(&seed,0,out,sizeof out,&got,7)&&got==fresh_n&&!memcmp(out,fresh,got));protected();
 memcpy(nor,baseline,sizeof nor);reset();CHECK(!d8pool_mapped_open(&m,&io,1));
 for(unsigned i=0;i<80;i++){unsigned o=i%4;CHECK(!d8pool_save(&m.pool,o,fresh,fresh_n,7));size_t size=0;CHECK(!d8pool_load(&m.pool,o,out,sizeof out,&size,7)&&size==fresh_n&&!memcmp(out,fresh,size));protected();}
 /* Raw failure/foreign records revoke a previously usable session. */
 memcpy(nor+0x97000,"FELU",4);CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_UNSUPPORTED&&!m.state);before_writes=writes;CHECK(m.pool.erase(&m,0)!=0&&writes==before_writes);
 memcpy(nor,baseline,sizeof nor);reset();busy=1;CHECK(d8pool_mapped_open(&m,&io,1)==D8POOL_BUSY&&!reads&&!writes&&!m.state);
 d8pool_physical incomplete=io;incomplete.program=NULL;CHECK(d8pool_mapped_open(&m,&incomplete,1)==D8POOL_INVALID&&!m.state);
 printf("D8P1 mapped pool: %u checks, %u failures; %u operation cuts, %u late-stop cuts, %u read cuts, 80 rotating saves; 1MiB simulated NOR only\n",checks,failures,ops,ops,opened_reads);return failures!=0;
}
