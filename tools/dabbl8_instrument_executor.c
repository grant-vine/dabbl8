/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only original-preserving simulator. Never linked into firmware. */
#include "d8pool.h"
#include "d8pool_mapped.h"
#include "d8p1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define NOR_BYTES 1048576u
static unsigned char nor[NOR_BYTES], before[NOR_BYTES], raw[12][81920], plan[D8POOL_BYTES];
static unsigned char wire[D8P1_LIMIT];
static const char *names[12]={"project-pair-0","project-pair-1","project-pair-2","project-pair-3","persisted-autosave-pair","settings-pair","preset-pair-0","preset-pair-1","fm6-pair","sample-allocation-0","sample-allocation-1","sample-allocation-2"};
static const uint32_t addresses[12]={0x97000,0x99000,0x9b000,0x9d000,0xe5000,0xfc000,0xdc000,0xde000,0x9f000,0xa0000,0xb4000,0xc8000};
static const uint32_t lengths[12]={8192,8192,8192,8192,8192,8192,8192,8192,8192,81920,81920,81920};
static unsigned reads,erases,programs,mutations,cut,partial,read_failure;
static uint32_t get32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned get16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static int load(const char *path,unsigned char *data,size_t n){FILE*f=fopen(path,"rb");if(!f)return -1;size_t got=fread(data,1,n,f);int extra=fgetc(f),err=ferror(f);fclose(f);return got!=n||extra!=EOF||err?-1:0;}
static uint32_t role_address(unsigned role,uint32_t off){return role==8&&off>=4096?0xfe000+off-4096:addresses[role]+off;}
static int fixed_range(uint32_t a,uint32_t n,unsigned count){for(unsigned r=0;r<count;r++){for(uint32_t off=0;off<lengths[r];off+=4096){uint32_t base=role_address(r,off);if(a>=base&&a-base<4096&&n<=4096-(a-base))return 1;}}return 0;}
static unsigned scope;
static int rd(void*ctx,uint32_t a,void*p,uint32_t n){(void)ctx;if(++reads==read_failure)return -1;if(!p||!n||n>256||a>NOR_BYTES-n)return -1;memcpy(p,nor+a,n);return 0;}
static int er(void*ctx,uint32_t a){(void)ctx;if(a%4096||!fixed_range(a,4096,scope))return -1;erases++;unsigned k=4096;if(++mutations==cut){k=partial<k?partial:k;memset(nor+a,255,k);return -1;}memset(nor+a,255,k);return 0;}
static int pg(void*ctx,uint32_t a,const void*p,uint32_t n){(void)ctx;if(!p||!n||n>256||(a&255)+n>256||!fixed_range(a,n,scope))return -1;programs++;const unsigned char*b=p;unsigned k=n;if(++mutations==cut)k=partial<k?partial:k;for(unsigned i=0;i<k;i++)nor[a+i]&=b[i];return mutations==cut?-1:0;}
static int stop(void*ctx){(void)ctx;return 1;}
static int prd(void*ctx,uint32_t a,void*p,uint32_t n){(void)ctx;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;memcpy(p,plan+a,n);return 0;}
static int reject_erase(void*c,uint32_t a){(void)c;(void)a;return -1;}
static int reject_prog(void*c,uint32_t a,const void*p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static int valid_plan(void){d8pool p={NULL,prd,reject_erase,reject_prog,stop};d8pool_index idx;if(d8pool_inventory(&p,&idx)||!(idx.present&8))return -1;unsigned mask=idx.present&7;for(unsigned o=0;o<4;o++)if(idx.present&(1u<<o)){size_t n=0;if(d8pool_load(&p,o,wire,sizeof wire,&n,mask))return -1;}/* Require canonical erased spare rather than ignored unknown residue. */
unsigned used=0;for(unsigned o=0;o<4;o++)if(idx.present&(1u<<o))used|=1u<<idx.object[o].block;
for(unsigned b=0;b<5;b++)if(!(used&(1u<<b)))for(unsigned i=0;i<8192;i++)if(plan[b*8192+i]!=255)return -1;return 0;}
static int erased(const unsigned char*p,unsigned n){for(unsigned i=0;i<n;i++)if(p[i]!=255)return 0;return 1;}
/* Frozen FUN1..FUN9 framing/checksum only; actual musical conversion and
 * source-to-plan equivalence remain separately reviewed host decisions. */
static int legacy_frame(const unsigned char*p,unsigned n){static const unsigned sizes[9]={688,2552,2584,2680,3352,3388,3388,3584,3648};if(n<12)return 0;uint32_t tag=get32(p);if(tag<0x46554e31||tag>0x46554e39)return 0;unsigned f=tag-0x46554e31;if(n!=sizes[f]||get32(p+4)!=n)return 0;uint32_t h=2166136261u;for(unsigned i=0;i<n-4;i++)h=(h^p[i])*16777619u;return get32(p+n-4)==h;}
static int legacy_source(void){for(unsigned r=0;r<5;r++){unsigned valid=0;uint32_t seq[2]={0};for(unsigned s=0;s<2;s++){const unsigned char*h=raw[r]+s*4096;if(erased(h,4096))continue;unsigned kind=r==4?9:r+1;uint32_t n=get32(h+12);if(get32(h)!=0x554c4546||get16(h+4)!=kind||get16(h+6)!=s||n>3840||get32(h+28)!=d8p1_crc32(h,28)||get32(h+16)!=d8p1_crc32(h+256,n)||!legacy_frame(h+256,n))return -1;valid|=1u<<s;seq[s]=get32(h+8);}if(valid==3&&(seq[0]==seq[1]||seq[0]-seq[1]==0x80000000u))return -1;}return 0;}
static int unchanged_outside(void){for(uint32_t a=0;a<NOR_BYTES;a++){int allowed=(a>=0x97000&&a<(scope==5?0x9f000:0xe0000))||(a>=0xe5000&&a<0xe7000)||(scope==12&&a>=0xfc000&&a<0xff000);if(!allowed&&nor[a]!=before[a])return 0;}return 1;}
static int originals_match(void){for(unsigned r=0;r<12;r++)for(uint32_t off=0;off<lengths[r];off+=256){unsigned char b[256];if(rd(NULL,role_address(r,off),b,256)||memcmp(b,raw[r]+off,256))return 0;}return 1;}
static int write_sector(uint32_t a,const unsigned char*src,unsigned header){if(er(NULL,a))return -1;/* Exact padding and payload first, commit bytes last. */
for(unsigned off=header;off<4096;){unsigned n=256-(off&255);if(n>4096-off)n=4096-off;if(pg(NULL,a+off,src+off,n))return -1;off+=n;}
/* A 512-byte sample header is committed by writing its second page before magic. */
if(header==512&&pg(NULL,a+256,src+256,256))return -1;if(pg(NULL,a,src,header==512?256:header))return -1;
for(unsigned off=0;off<4096;off+=256){unsigned char b[256];if(rd(NULL,a+off,b,256)||memcmp(b,src+off,256))return -1;}return 0;}
static int physical_read(void*c,uint32_t a,void*p,uint32_t n){unsigned char*b=p;while(n){unsigned k=n>256?256:n;if(rd(c,a,b,k))return -1;a+=k;b+=k;n-=k;}return 0;}
static int verified_result(int apply){scope=apply?5:12;if(!unchanged_outside())return 0;for(unsigned r=0;r<scope;r++){const unsigned char*src=apply?plan+r*8192:raw[r];for(unsigned off=0;off<lengths[r];off+=256){unsigned char b[256];if(rd(NULL,role_address(r,off),b,256)||memcmp(b,src+off,256))return 0;}}
if(apply){d8pool_mapped m={0};d8pool_physical io={NULL,physical_read,er,pg,stop};/* Simulator permission only, never a production migration receipt. */if(d8pool_mapped_open(&m,&io,1))return 0;d8pool_index idx;if(d8pool_inventory(&m.pool,&idx)){d8pool_mapped_close(&m);return 0;}unsigned mask=idx.present&7;for(unsigned o=0;o<4;o++)if(idx.present&(1u<<o)){size_t n=0;if(d8pool_load(&m.pool,o,wire,sizeof wire,&n,mask)||n!=idx.object[o].length||memcmp(wire,plan+idx.object[o].block*8192+256,n)){d8pool_mapped_close(&m);return 0;}}d8pool_mapped_close(&m);}return 1;}
static int execute(int apply){scope=apply?5:12;/* Session remains quarantined before, during and after every attempt. */
for(unsigned r=0;r<scope;r++){const unsigned char*src=apply?plan+r*8192:raw[r];unsigned len=apply?8192:lengths[r];if(r>=9){/* Sample body/tails before the header-bearing first sector. */for(unsigned off=4096;off<len;off+=4096)if(write_sector(role_address(r,off),src+off,32))return -1;if(write_sector(role_address(r,0),src,512))return -1;}else if(apply){if(write_sector(role_address(r,4096),src+4096,32)||write_sector(role_address(r,0),src,32))return -1;}else{for(unsigned off=0;off<len;off+=4096)if(write_sector(role_address(r,off),src+off,32))return -1;}}
if(!unchanged_outside())return -1;for(unsigned r=0;r<scope;r++){const unsigned char*src=apply?plan+r*8192:raw[r];for(unsigned off=0;off<lengths[r];off+=256){unsigned char b[256];if(rd(NULL,role_address(r,off),b,256)||memcmp(b,src+off,256))return -1;}}return 0;}
static int save(const char*path){FILE*f=fopen(path,"wbx");if(!f)return -1;int bad=fwrite(nor,1,sizeof nor,f)!=sizeof nor;if(fclose(f))bad=1;return bad?-1:0;}
static int number(const char*s,unsigned*out){char*end;unsigned long v=strtoul(s,&end,10);if(!*s||*end||v>0xfffffffful)return -1;*out=(unsigned)v;return 0;}
int main(int argc,char**argv){int apply=argc>=3&&!strcmp(argv[2],"apply"),restore=argc>=3&&!strcmp(argv[2],"restore");int base=apply?7:6;if(argc<base||argc>base+2||strcmp(argv[1],"--simulate")||(!apply&&!restore))return 2;if(argc>base&&number(argv[base],&cut))return 2;if(argc>base+1&&number(argv[base+1],&partial))return 2;const char*current=argv[apply?5:4],*output=argv[apply?6:5];
if(load(current,nor,sizeof nor))return 2;memcpy(before,nor,sizeof nor);char path[4096];for(unsigned r=0;r<12;r++){int n=snprintf(path,sizeof path,"%s/original-%s.bin",argv[3],names[r]);if(n<0||(size_t)n>=sizeof path||load(path,raw[r],lengths[r]))return 2;}
const char*status="complete";int ok=1;if(apply&&(load(argv[4],plan,sizeof plan)||valid_plan())){status="invalid-plan";ok=0;}else if(apply&&!originals_match()){status="source-changed";ok=0;}else if(apply&&legacy_source()){status="unsupported-legacy-source";ok=0;}else if(execute(apply)){/* Callback failure can occur after commitment. Reconcile actual bytes before any retry. */if(!verified_result(apply)){status="interrupted-or-readback-failed";ok=0;}}else if(!verified_result(apply)){status="interrupted-or-readback-failed";ok=0;}if(!unchanged_outside()&&scope){status="scope-failure";ok=0;}if(save(output))return 2;
printf("{\"format\":\"dabbl8-instrument-simulation\",\"version\":1,\"operation\":\"%s\",\"status\":\"%s\",\"completed\":%s,\"quarantined\":true,\"production_ownership\":false,\"hardware_qualified\":false,\"device_writes\":false,\"post_boot_originals\":true,\"source_plan_equivalence\":false,\"plan_review_required\":true,\"reads\":%u,\"erases\":%u,\"programs\":%u,\"mutation_calls\":%u,\"cut_operation\":%u,\"partial_bytes\":%u}\n",apply?"apply":"restore",status,ok?"true":"false",reads,erases,programs,mutations,cut,partial);return ok?0:1;}
