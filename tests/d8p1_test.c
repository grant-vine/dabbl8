/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../firmware/src/d8p1.h"
static unsigned checks,failures;
static void check(int yes,const char *what){checks++;if(!yes){failures++;fprintf(stderr,"FAIL: %s\n",what);}}
static uint8_t input[D8P1_LIMIT],baseline[D8P1_LIMIT],output[D8P1_LIMIT],before[D8P1_LIMIT];
static unsigned le16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static void put16(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static size_t load(const char *path){FILE *f=fopen(path,"rb");if(!f){perror(path);exit(2);}size_t n=fread(input,1,sizeof input,f);if(fgetc(f)!=EOF)exit(2);fclose(f);return n;}
static void fix_file(size_t n){memset(input+12,0,4);put32(input+12,d8p1_crc32(input,n));}
static void fix_chunk(size_t n,unsigned id){size_t pos=32;for(unsigned i=0;i<input[20];i++){unsigned length=le16(input+pos+2);if((le16(input+pos)&32767)==id){put32(input+pos+4,d8p1_crc32(input+pos+8,length));break;}pos+=8+length;}fix_file(n);}
static size_t offset(unsigned id){size_t pos=32;for(unsigned i=0;i<input[20];i++){if((le16(input+pos)&32767)==id)return pos;pos+=8+le16(input+pos+2);}exit(2);}
static void refused(size_t n,const char *label){d8p1_view view,old;memset(&view,0xA5,sizeof view);old=view;check(!d8p1_read(&view,input,n)&&!memcmp(&view,&old,sizeof view),label);}
static void reset(size_t n){memcpy(input,baseline,n);}
static void writer_refused(d8p1_view *view,const char *label){size_t written=12345;memset(output,0xC7,sizeof output);memcpy(before,output,sizeof output);check(!d8p1_write(output,sizeof output,&written,view)&&written==12345&&!memcmp(output,before,sizeof output),label);}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 check(d8p1_crc32("123456789",9)==0xCBF43926u&&d8p1_crc32(NULL,0)==0,"independent standard CRC vectors");
 d8p1_view view;size_t n=load(argv[1]),written=0;
 check(n==6599&&d8p1_read(&view,input,n)==1&&!view.readonly&&view.count==6,"minimal independent reference decodes");
 check(d8p1_write(output,sizeof output,&written,&view)&&written==n&&!memcmp(output,input,n),"minimal reference reencodes byte exact");
 check(d8p1_refs_available(&view,0),"absent optional banks/scenes default empty with no external references");
 n=load(argv[2]);memcpy(baseline,input,n);
 check(n==7705&&d8p1_read(&view,input,n)==1&&view.count==8,"maximum independent reference reaches modeled 7705 bytes");
 check(d8p1_write(output,sizeof output,&written,&view)&&written==n&&!memcmp(output,input,n),"all eight maximum chunks reencode byte exact");
 check(d8p1_refs_available(&view,15)&&!d8p1_refs_available(&view,3)&&!d8p1_refs_available(&view,7)&&!d8p1_refs_available(&view,16),"project references never alias missing slots or invalid capabilities");
 d8p1_view reverse=view;for(unsigned i=0;i<8;i++)reverse.chunk[i]=view.chunk[7-i];
 check(d8p1_write(output,sizeof output,&written,&reverse)&&!memcmp(output,input,n),"writer canonicalizes base-ID ordering");
 for(size_t i=0;i<n;i++)refused(i,"every truncation preserves output view");
 refused(SIZE_MAX,"untrusted huge length refuses before reading");refused(D8P1_LIMIT+1u,"independent parser length bound");
 for(unsigned i=0;i<32;i++){reset(n);input[i]^=1;if(i<12||i>=16)fix_file(n);refused(n,"all header fields reserved/version/count/crc guarded");}
 for(size_t i=32;i<n;i++){reset(n);input[i]^=1;refused(n,"single payload/header bit corruption CRC refused");}
 reset(n);input[20]=9;fix_file(n);refused(n,"chunk count over eight");
 reset(n);put16(input+34,65535);fix_file(n);refused(n,"chunk length beyond remaining bytes");
 reset(n);put16(input+offset(2),0x8001);fix_file(n);refused(n,"duplicate base chunk ID");
 reset(n);put16(input+offset(1),0x8009);fix_file(n);refused(n,"unknown required chunk");
 reset(n);put16(input+offset(1),9);fix_file(n);refused(n,"unknown optional cannot replace mandatory globals");
 for(unsigned id=1;id<=8;id++){
  reset(n);size_t pos=offset(id);put16(input+pos+2,le16(input+pos+2)-1);fix_file(n);refused(n,"every known chunk exact length validated");
 }
 struct mutation{unsigned id,position,value;} bad[]={
  {2,0,192},{2,99,14},{2,101,128},{2,105,5},{2,105,24},{2,105,128},{2,108,1},{2,109,102},
  {3,0,128},{4,0,'X'},{4,4,65},{4,6,1},{4,8,8},{4,9,64},{4,10,29},{4,11,128},{4,12,0},
  {5,0,17},{5,1,16},{5,2,0},{5,2,17},
  {6,0,31},{6,11,'X'},{6,12,8},{6,13,3},{6,14,1},{6,15,1},
  {7,0,5},{7,13,4},{7,14,8},{8,0,17},{8,13,4},{8,14,8},{8,16,128},{8,17,128},{8,18,49}
 };
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){
  reset(n);size_t pos=offset(bad[i].id)+8;input[pos+bad[i].position]=(uint8_t)bad[i].value;
  // Accent mismatch requires a missing hit rather than replacing an accent bit.
  if(bad[i].id==2&&bad[i].position==108)input[pos+107]=0;
  fix_chunk(n,bad[i].id);refused(n,"rechecksummed invalid musical/count/name/reference field");
 }
 reset(n);size_t mo=offset(4)+8;memcpy(input+mo+13,input+mo+8,5);input[mo+15]^=128;fix_chunk(n,4);refused(n,"duplicate motion address/id including lock kinds");
 for(int value=-65;value<=128;value++){if(value!=-65&&value!=-64&&value!=127&&value!=128)continue;
  reset(n);size_t motion=offset(4)+8;put16(input+motion+11,(unsigned)(uint16_t)value);fix_chunk(n,4);
  if(value== -64||value==127)check(d8p1_read(&view,input,n)==1,"signed motion boundary accepted without narrowing");else refused(n,"signed motion boundary overflow refused");
 }
 reset(n);size_t bank=offset(7),scene=offset(8);
 memmove(input+scene-28,input+scene,n-scene);input[bank+8]=3;put16(input+bank+2,85);put32(input+8,(uint32_t)(n-28));fix_chunk(n-28,7);refused(n-28,"scene references must exist within a correctly sized three-bank chunk");
 reset(n);scene=offset(8);input[scene+8]=15;put16(input+scene+2,586);put32(input+8,(uint32_t)(n-39));fix_chunk(n-39,8);refused(n-39,"chain references must exist within a correctly sized fifteen-scene chunk");
 reset(n);check(d8p1_read(&view,input,n)==1,"restore maximum reference before writer faults");
 view.chunk[0].length--;writer_refused(&view,"writer refuses invalid chunk before changing output");view.chunk[0].length++;
 writer_refused(&(d8p1_view){0},"writer refuses missing mandatory chunks");
 memset(output,0xC7,sizeof output);memcpy(before,output,sizeof output);written=12345;
 check(!d8p1_write(output,n-1,&written,&view)&&written==12345&&!memcmp(output,before,sizeof output),"short capacity never partially writes");
 memcpy(before,input,n);written=12345;
 check(!d8p1_write(input,n,&written,&view)&&written==12345&&!memcmp(input,before,n),"overlapping output preserves original file");
 check(!d8p1_write(output,sizeof output,(size_t *)(input+8),&view)&&!memcmp(input,before,n),"written-count alias cannot mutate original header");
 check(!d8p1_read((d8p1_view *)input,input,n)&&!memcmp(input,before,n),"decoded view cannot overwrite original bytes");
 n=load(argv[3]);memcpy(baseline,input,n);
 check(d8p1_read(&view,input,n)==2&&view.readonly&&view.original==input&&view.length==n,"unknown optional bounded inspection retains complete original");
 writer_refused(&view,"unknown optional import is readonly, never silently dropped");
 check(!d8p1_refs_available(&view,15),"unknown optional cannot grant playback/reference adoption");
 view.readonly=0;writer_refused(&view,"clearing readonly flag cannot bypass unknown-chunk refusal");
 view.count=6;writer_refused(&view,"dropping unknown view entry cannot bypass original read-only import");
 check(!d8p1_refs_available(&view,15),"dropping unknown entry cannot grant original import reference eligibility");
 reset(n);put16(input+offset(42),0x8000|42);fix_file(n);refused(n,"same unknown extension becomes refused if required");
 // Deterministic malformed files with repaired outer CRC exercise the chunk parser.
 uint32_t random=7;for(unsigned iteration=0;iteration<2000;iteration++){
  reset(n);random=random*1664525u+1013904223u;size_t at=32+random%(n-32);input[at]^=(uint8_t)(1u<<(iteration%8));fix_file(n);
  d8p1_view old;memset(&view,0xD3,sizeof view);old=view;int result=d8p1_read(&view,input,n);
  check(result?view.original==input&&view.length==n:!memcmp(&view,&old,sizeof view),"fuzz success is bounded or refusal preserves destination");
 }
 printf("D8P1: %u checks, %u failures; bounded buffers only, no flash/runtime adoption\n",checks,failures);return failures?1:0;
}
