/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only fixture: manufacture synthetic own-format NOR; never hardware. */
#include "felucca_web.c"
#if NTRK != 8
#error This fixture checks the eight-track native boot composition.
#endif
static int checks;
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"failed: %s:%d %s\n",__FILE__,__LINE__,#x);return 1;} } while(0)
static int mapped_read(void *c,uint32_t off,void *dst,uint32_t n)
{ (void)c; while(n){uint32_t k=D8POOL_BLOCK-off%D8POOL_BLOCK;if(k>n)k=n;if(st_read(d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK,dst,k))return -1;off+=k;dst=(uint8_t *)dst+k;n-=k;}return 0; }
static int mapped_erase(void *c,uint32_t off)
{ (void)c;return st_erase(d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK); }
static int mapped_program(void *c,uint32_t off,const void *src,uint32_t n)
{ (void)c;return st_prog(d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK,src,n); }
static int fixture_stopped(void *c) { (void)c;return 1; }
int main(int argc,char **argv)
{
    CHECK(argc==2);web_nor_erase();
    if(!strcmp(argv[1],"own")) {
        static uint8_t wire[D8P1_LIMIT];size_t written=0;
        felucca_init();for(unsigned t=0;t<8;t++)trk[t].p[P_LEVEL]=(uint8_t)(31+t);
        CHECK(d8p1_capture_runtime(wire,sizeof wire,&written)==D8RT_OK);
        d8pool pool={NULL,mapped_read,mapped_erase,mapped_program,fixture_stopped};
        CHECK(d8pool_save(&pool,3,wire,written,0)==D8POOL_OK);
    } else { CHECK(!strcmp(argv[1],"unknown"));nor[0x97000]=0x42; }
    static uint8_t before[D8POOL_BYTES],after[D8POOL_BYTES];
    CHECK(mapped_read(NULL,0,before,sizeof before)==0);web_boot();
    CHECK(project_native_owns_storage());CHECK(project_native.readonly);CHECK(!native_as.ready);
    if(!strcmp(argv[1],"own")) {CHECK(project_native_status()==1);for(unsigned t=0;t<8;t++)CHECK(trk[t].p[P_LEVEL]==31+t);}
    else CHECK(project_native_status()!=1);
    uint32_t writes=web_flash_writes_count();
    for(unsigned i=0;i<300;i++)web_render(128);
    CHECK(web_flash_writes_count()==writes);CHECK(mapped_read(NULL,0,after,sizeof after)==0);
    CHECK(!memcmp(before,after,sizeof before));CHECK(!native_as.ready);
    printf("emulator storage %s: %d checks PASS\n",argv[1],checks);return 0;
}
