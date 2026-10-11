/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual host executor and mapped reader, synthetic NOR only. */
#define main executor_cli_main
#include "../tools/dabbl8_instrument_executor.c"
#undef main
static unsigned checks,failures;
#define CHECK(x) do{checks++;if(!(x)){failures++;fprintf(stderr,"line %d: %s\n",__LINE__,#x);}}while(0)
static unsigned char original[NOR_BYTES],project[3840];
static size_t project_n,minimal_n;
static void put32(unsigned char*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static void counters(void){reads=erases=programs=mutations=cut=partial=read_failure=0;scope=0;}
static int vp_read(void*c,uint32_t a,void*p,uint32_t n){return prd(c,a,p,n);}
static int vp_erase(void*c,uint32_t a){(void)c;if(a%4096||a>D8POOL_BYTES-4096)return -1;memset(plan+a,255,4096);return 0;}
static int vp_program(void*c,uint32_t a,const void*p,uint32_t n){(void)c;if(!n||n>256||a>D8POOL_BYTES-n||(a&255)+n>256)return -1;const unsigned char*b=p;for(unsigned i=0;i<n;i++)plan[a+i]&=b[i];return 0;}
static void init(void){for(unsigned a=0;a<NOR_BYTES;a++)original[a]=(unsigned char)(a^(a>>9)^0xa5);for(unsigned r=0;r<12;r++)for(unsigned off=0;off<lengths[r];off++)raw[r][off]=original[role_address(r,off)];for(unsigned r=0;r<5;r++){memset(raw[r],255,8192);unsigned char*h=raw[r];memset(h,0,32);put32(h,0x554c4546);h[4]=(unsigned char)(r==4?9:r+1);put32(h+8,1);put32(h+12,(uint32_t)project_n);put32(h+16,d8p1_crc32(project,project_n));put32(h+28,d8p1_crc32(h,28));memcpy(h+256,project,project_n);for(unsigned off=0;off<8192;off++)original[role_address(r,off)]=raw[r][off];}memcpy(before,original,sizeof before);memcpy(nor,original,sizeof nor);counters();}
static void restore_original(void){memcpy(nor,original,sizeof nor);memcpy(before,original,sizeof before);counters();}
static int exact_raw(void){for(unsigned r=0;r<12;r++)for(unsigned off=0;off<lengths[r];off++)if(nor[role_address(r,off)]!=raw[r][off])return 0;return 1;}
static int header_valid(unsigned r){const unsigned char*h=nor+addresses[r];return get32(h)==0x554c4546&&get32(h+28)==d8p1_crc32(h,28)&&get32(h+16)==d8p1_crc32(h+256,get32(h+12));}
int main(void){FILE*f=fopen("tests/fixtures/projects/fun9.bin","rb");if(!f)return 2;project_n=fread(project,1,sizeof project,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)return 2;minimal_n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
memset(plan,255,sizeof plan);d8pool p={NULL,vp_read,vp_erase,vp_program,stop};for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&p,o,wire,minimal_n,7));CHECK(!valid_plan());init();CHECK(!legacy_source()&&originals_match());CHECK(!execute(1)&&verified_result(1));unsigned apply_ops=mutations;CHECK(apply_ops==180);CHECK(unchanged_outside());
/* Every bounded erase/page/header call can fail before or during mutation. A
 * valid newly committed block always has its complete source payload. */
for(unsigned c=1;c<=apply_ops;c++)for(unsigned prefix=0;prefix<2;prefix++){restore_original();cut=c;partial=prefix?17:0;int rc=execute(1);CHECK(rc==-1);scope=5;CHECK(unchanged_outside());for(unsigned b=0;b<5;b++){unsigned char*h=nor+addresses[b];if(get32(h)==D8POOL_MAGIC&&get32(h+28)==d8p1_crc32(h,28)){unsigned n=get32(h+16);CHECK(n<=D8P1_LIMIT&&get32(h+20)==d8p1_crc32(h+256,n));}}/* Always recover exact originals using actual restore path before retry. */cut=partial=0;CHECK(!execute(0)&&exact_raw()&&verified_result(0));CHECK(!execute(1)&&verified_result(1));}
/* Failed readback after commitment remains uncertain until the actual
 * stored bytes are reread; no retry erases a potentially complete result. */
restore_original();CHECK(!execute(1));unsigned complete_reads=reads;CHECK(verified_result(1));unsigned verified_reads=reads-complete_reads;for(unsigned f=1;f<=verified_reads;f+=31){read_failure=reads+f;CHECK(!verified_result(1));read_failure=0;CHECK(verified_result(1));}nor[addresses[0]+256]^=1;CHECK(!verified_result(1));
/* Full-length callback failure at the last native operation is reconciled;
 * callers must inspect actual readback rather than assume definitely failed. */
restore_original();cut=apply_ops;partial=4096;CHECK(execute(1)==-1);CHECK(verified_result(1));
/* Restore opaque patterned/torn sectors byte exactly and hold RAM adoption.
 * Cut every restore mutation; retries start from retained originals, never
 * infer that a partial header grants ownership. */
restore_original();for(unsigned r=0;r<12;r++)for(unsigned off=0;off<lengths[r];off++)nor[role_address(r,off)]^=0x55;memcpy(before,nor,sizeof before);CHECK(!execute(0)&&exact_raw()&&verified_result(0));unsigned restore_ops=mutations;CHECK(restore_ops==1401);
for(unsigned c=1;c<=restore_ops;c++){restore_original();for(unsigned r=0;r<12;r++)for(unsigned off=0;off<lengths[r];off++)nor[role_address(r,off)]^=0x55;memcpy(before,nor,sizeof before);cut=c;partial=(c&1)?0:17;CHECK(execute(0)==-1);scope=12;CHECK(unchanged_outside());/* Reboot/restart simulation keeps quarantine; exact external archive retries. */cut=partial=0;CHECK(!execute(0)&&exact_raw()&&verified_result(0));}
restore_original();raw[0][0]^=1;CHECK(legacy_source()==-1);raw[0][0]^=1;raw[4][4]=7;put32(raw[4]+28,d8p1_crc32(raw[4],28));CHECK(legacy_source()==-1);raw[4][4]=9;put32(raw[4]+28,d8p1_crc32(raw[4],28));CHECK(!legacy_source());
/* Equal/half-range headers, corrupt sole copies, wrong type and unknown
 * residue refuse structurally; unknown FUN semantics remain external review. */
memcpy(raw[0]+4096,raw[0],4096);raw[0][4096+6]=1;put32(raw[0]+4096+28,d8p1_crc32(raw[0]+4096,28));CHECK(legacy_source()==-1);put32(raw[0]+4096+8,0x80000001);put32(raw[0]+4096+28,d8p1_crc32(raw[0]+4096,28));CHECK(legacy_source()==-1);memset(raw[0]+4096,255,4096);raw[0][4096+99]=1;CHECK(legacy_source()==-1);init();raw[0][256]^=1;put32(raw[0]+16,d8p1_crc32(raw[0]+256,project_n));put32(raw[0]+28,d8p1_crc32(raw[0],28));CHECK(legacy_source()==-1);init();raw[0][256+24]^=1;put32(raw[0]+16,d8p1_crc32(raw[0]+256,project_n));put32(raw[0]+28,d8p1_crc32(raw[0],28));CHECK(legacy_source()==-1);init();for(unsigned i=1;i<=9;i++){char name[100];snprintf(name,sizeof name,"tests/fixtures/projects/fun%u.bin",i);FILE*g=fopen(name,"rb");CHECK(g!=NULL);if(!g)return 2;unsigned char b[3840];size_t n=fread(b,1,sizeof b,g);CHECK(!ferror(g)&&fgetc(g)==EOF);fclose(g);CHECK(legacy_frame(b,(unsigned)n));CHECK(!legacy_frame(b,(unsigned)n-1));}plan[4*8192+32]=1;CHECK(valid_plan()==-1);plan[4*8192+32]=255;CHECK(!valid_plan());plan[0]^=1;CHECK(valid_plan()==-1);plan[0]^=1;
/* Backend refuses arbitrary boundaries and oversized/page-crossing writes. */scope=5;CHECK(er(NULL,0x96000)==-1);CHECK(er(NULL,0x9f000)==-1);CHECK(pg(NULL,0x970ff,wire,2)==-1);CHECK(pg(NULL,0x97000,wire,257)==-1);CHECK(pg(NULL,0x97000,wire,0)==-1);CHECK(rd(NULL,NOR_BYTES-1,wire,2)==-1);scope=12;CHECK(er(NULL,0xe0000)==-1);CHECK(er(NULL,0xfb000)==-1);
/* A restored legacy header never becomes valid before its payload and all
 * exact original slack bytes; sample first512 bytes are restored after tails. */restore_original();memset(nor+addresses[0],0,4096);memcpy(before,nor,sizeof before);scope=12;cut=18;partial=0;CHECK(write_sector(addresses[0],raw[0],32)==-1&&!header_valid(0));cut=partial=0;CHECK(!write_sector(addresses[0],raw[0],32)&&header_valid(0));CHECK(!memcmp(nor+addresses[0],raw[0],4096));
printf("Instrument migration executor: %u checks, %u failures; %u APPLY calls x2 cuts, %u RESTORE calls; simulator only, quarantine retained, no production ownership\n",checks,failures,apply_ops,restore_ops);return failures!=0;}
