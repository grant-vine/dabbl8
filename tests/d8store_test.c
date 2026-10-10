/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "d8store.h"
#include "d8p1.h"

static uint8_t nor[D8STORE_REGION+512], snapshot[sizeof nor], baseline[sizeof nor];
static uint8_t old_data[D8P1_LIMIT], new_data[D8P1_LIMIT], out[D8P1_LIMIT];
static size_t old_n,new_n;
static unsigned checks,failures,reads,mutations;
static int cut_op=-1,cut_byte=-1,read_error=-1,busy_after=-1,protected_part,busy;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d: %s\n",__LINE__,#x); } } while(0)
static void reset_faults(void)
{ cut_op=cut_byte=read_error=busy_after=-1; protected_part=busy=0; reads=mutations=0; }
static int read_cb(void *ctx,uint32_t off,void *dst,uint32_t n)
{
    (void)ctx;
    CHECK(off<=D8STORE_REGION && n<=D8STORE_REGION-off);
    ++reads;
    if (read_error==0) return -1;
    if (read_error>0) --read_error;
    memcpy(dst,nor+256+off,n); return 0;
}
static int mutate(void)
{
    if (cut_op==0) return -1;
    if (cut_op>0) --cut_op;
    ++mutations; return 0;
}
static int erase_cb(void *ctx,uint32_t off)
{
    (void)ctx;
    CHECK(off%D8STORE_SECTOR==0 && off<=D8STORE_REGION-D8STORE_SECTOR);
    if (mutate()) return -1;
    if (!protected_part) memset(nor+256+off,255,D8STORE_SECTOR);
    return 0;
}
static int program_cb(void *ctx,uint32_t off,const void *src,uint32_t n)
{
    const uint8_t *p=src;
    (void)ctx;
    CHECK(n && n<=256 && off<=D8STORE_REGION-n && (off&255)+n<=256);
    if (mutate()) return -1;
    for (uint32_t i=0;i<n;++i) {
        if (cut_byte==0) return -1;
        if (cut_byte>0) --cut_byte;
        if (!protected_part) nor[256+off+i]&=p[i];
    }
    return 0;
}
static int stopped_cb(void *ctx)
{ (void)ctx; return !busy && (busy_after<0 || mutations<(unsigned)busy_after); }
static d8store store={NULL,17,read_cb,erase_cb,program_cb,stopped_cb};
static void blank(void)
{ memset(nor,0xa5,sizeof nor); memset(nor+256,255,D8STORE_REGION); reset_faults(); }
static void guards(void)
{ for(unsigned i=0;i<256;++i) CHECK(nor[i]==0xa5 && nor[256+D8STORE_REGION+i]==0xa5); }
static int equals(const uint8_t *data,size_t n)
{ size_t got=123; return d8store_load(&store,out,sizeof out,&got,15)==0 && got==n && !memcmp(out,data,n); }
static size_t fixture(const char *path,uint8_t *dst)
{
    FILE *f=fopen(path,"rb"); size_t n;
    if(!f) { perror(path); exit(2); }
    n=fread(dst,1,D8P1_LIMIT,f); CHECK(!ferror(f) && fgetc(f)==EOF); fclose(f); return n;
}
static void put(uint8_t *p,uint32_t n)
{ p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8); p[2]=(uint8_t)(n>>16); p[3]=(uint8_t)(n>>24); }
static void sequence(unsigned copy,uint32_t n)
{ uint8_t *h=nor+256+copy*D8STORE_COPY; put(h+12,n); put(h+28,d8p1_crc32(h,28)); }
int main(int argc,char **argv)
{
    d8store_record r,unchanged={9,8,7,1}; size_t written;
    CHECK(argc==4); if(argc!=4) return 2;
    old_n=fixture(argv[1],old_data); new_n=fixture(argv[2],new_data);
    CHECK(new_n==D8P1_MAX_FILE && old_n<new_n);
    blank(); r=unchanged; CHECK(d8store_current(&store,&r)==D8STORE_EMPTY && !memcmp(&r,&unchanged,sizeof r));
    CHECK(d8store_save(&store,old_data,old_n,15)==0 && equals(old_data,old_n));
    CHECK(d8store_current(&store,&r)==0 && r.copy==0 && r.sequence==1);
    CHECK(d8store_save(&store,new_data,new_n,15)==0 && equals(new_data,new_n));
    CHECK(d8store_current(&store,&r)==0 && r.copy==1 && r.sequence==2);
    CHECK(d8store_save(&store,old_data,old_n,15)==0 && equals(old_data,old_n));
    memcpy(baseline,nor,sizeof nor); /* A current; B destination already nonempty. */
    const unsigned operations=2+(unsigned)((new_n+255)/256)+1;
    for(unsigned cut=0;cut<operations;++cut) {
        memcpy(nor,baseline,sizeof nor); reset_faults(); cut_op=(int)cut;
        CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_IO);
        reset_faults(); CHECK(equals(old_data,old_n)); guards();
    }
    /* Power loss after every payload/commit byte, including the second sector. */
    for(unsigned cut=0;cut<new_n+32;++cut) {
        memcpy(nor,baseline,sizeof nor); reset_faults(); cut_byte=(int)cut;
        CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_IO);
        reset_faults(); CHECK(equals(old_data,old_n));
        CHECK(!memcmp(nor+256,baseline+256,D8STORE_COPY));
    }
    /* A cut during either erase leaves the old copy valid, independent of how
     * much of the destination sector the NOR managed to erase. */
    for(unsigned sector=0;sector<2;++sector) for(unsigned byte=0;byte<=4096;byte+=17) {
        memcpy(nor,baseline,sizeof nor); reset_faults();
        memset(nor+256+D8STORE_COPY+sector*4096,255,byte);
        CHECK(equals(old_data,old_n));
    }
    for(unsigned op=0;op<operations;++op) {
        memcpy(nor,baseline,sizeof nor); reset_faults(); busy_after=(int)op;
        CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_BUSY);
        CHECK(mutations==op); reset_faults(); CHECK(equals(old_data,old_n));
    }
    memcpy(nor,baseline,sizeof nor); reset_faults();
    CHECK(d8store_current(&store,&r)==0); unsigned scan_reads=reads;
    for(unsigned at=0;at<scan_reads;++at) {
        memcpy(nor,baseline,sizeof nor); reset_faults(); read_error=(int)at;
        CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_IO && mutations==0);
        CHECK(!memcmp(nor,baseline,sizeof nor));
    }
    /* A completed commit survives an error during final verification. */
    memcpy(nor,baseline,sizeof nor); reset_faults(); read_error=(int)scan_reads;
    CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_IO && mutations==operations);
    reset_faults(); CHECK(equals(new_data,new_n));
    memcpy(nor,baseline,sizeof nor); reset_faults(); cut_op=(int)operations;
    CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_OK);
    reset_faults(); CHECK(equals(new_data,new_n));
    memcpy(nor,baseline,sizeof nor); reset_faults(); protected_part=1;
    CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_IO);
    reset_faults(); CHECK(equals(old_data,old_n));
    CHECK(d8store_save(&store,new_data,new_n,15)==0);
    memcpy(snapshot,nor,sizeof nor);
    /* Payload corruption in the newest multi-sector copy falls back to A. */
    nor[256+D8STORE_COPY+4096+500]^=1; CHECK(equals(old_data,old_n));
    nor[256+D8STORE_PAYLOAD+500]^=1; r=unchanged;
    CHECK(d8store_current(&store,&r)==D8STORE_EMPTY && !memcmp(&r,&unchanged,sizeof r));
    for(unsigned i=0;i<32;++i) {
        memcpy(nor,snapshot,sizeof nor); nor[256+D8STORE_COPY+i]^=1;
        CHECK(equals(old_data,old_n));
    }
    /* CRC-valid invalid length/version/object/slot/reserved fields still refuse. */
    const unsigned offsets[]={5,6,8,16,24};
    for(unsigned i=0;i<sizeof offsets/sizeof *offsets;++i) {
        memcpy(nor,snapshot,sizeof nor); uint8_t *future=nor+256+D8STORE_COPY;
    future[4]=2; put(future+28,d8p1_crc32(future,28));
    memcpy(baseline,nor,sizeof nor); reset_faults(); r=unchanged;
    CHECK(d8store_current(&store,&r)==D8STORE_UNSUPPORTED && !memcmp(&r,&unchanged,sizeof r));
    CHECK(d8store_save(&store,old_data,old_n,15)==D8STORE_UNSUPPORTED && mutations==0);
    CHECK(!memcmp(nor,baseline,sizeof nor));
    memcpy(nor,snapshot,sizeof nor); uint8_t *h=nor+256+D8STORE_COPY;
        h[offsets[i]]^=128; put(h+28,d8p1_crc32(h,28)); CHECK(equals(old_data,old_n));
    }
    memcpy(nor,snapshot,sizeof nor); uint8_t *future=nor+256+D8STORE_COPY;
    future[4]=2; put(future+28,d8p1_crc32(future,28));
    memcpy(baseline,nor,sizeof nor); reset_faults(); r=unchanged;
    CHECK(d8store_current(&store,&r)==D8STORE_UNSUPPORTED && !memcmp(&r,&unchanged,sizeof r));
    CHECK(d8store_save(&store,old_data,old_n,15)==D8STORE_UNSUPPORTED && mutations==0);
    CHECK(!memcmp(nor,baseline,sizeof nor));
    memcpy(nor,snapshot,sizeof nor); uint8_t *h=nor+256+D8STORE_COPY;
    put(h+16,D8P1_LIMIT+1); put(h+28,d8p1_crc32(h,28)); CHECK(equals(old_data,old_n));
    memcpy(nor,snapshot,sizeof nor); sequence(0,0xfffffffe); sequence(1,0xffffffff);
    CHECK(equals(new_data,new_n)); CHECK(d8store_save(&store,old_data,old_n,15)==0);
    CHECK(d8store_current(&store,&r)==0 && r.sequence==0 && r.copy==0 && equals(old_data,old_n));
    CHECK(d8store_save(&store,new_data,new_n,15)==0 && equals(new_data,new_n));
    sequence(0,10); sequence(1,10); CHECK(equals(old_data,old_n));
    sequence(1,10+0x80000000u); CHECK(equals(old_data,old_n));
    memcpy(snapshot,nor,sizeof nor); reset_faults();
    CHECK(d8store_save(&store,new_data,D8P1_LIMIT+1,15)==D8STORE_INVALID && reads==0 && mutations==0);
    CHECK(d8store_save(&store,NULL,3,15)==D8STORE_INVALID && reads==0);
    CHECK(d8store_save(&store,new_data,new_n,16)==D8STORE_INVALID && reads==0);
    CHECK(d8store_save(&store,new_data,new_n-1,15)==D8STORE_INVALID && reads==0);
    size_t unknown_n=fixture(argv[3],out);
    CHECK(d8store_save(&store,out,unknown_n,15)==D8STORE_INVALID && reads==0);
    CHECK(d8store_save(&store,new_data,new_n,0)==D8STORE_INVALID && reads==0);
    busy=1; CHECK(d8store_save(&store,new_data,new_n,15)==D8STORE_BUSY && reads==0);
    CHECK(!memcmp(nor,snapshot,sizeof nor)); reset_faults();
    written=991; memset(out,123,sizeof out);
    CHECK(d8store_load(&store,out,old_n-1,&written,15)==D8STORE_INVALID && written==991 && out[0]==123);
    read_error=0; CHECK(d8store_load(&store,out,sizeof out,&written,15)==D8STORE_IO && written==991);
    reset_faults(); CHECK(d8store_load(&store,out,sizeof out,&written,16)==D8STORE_INVALID && reads==0);
    d8store invalid=store; invalid.program=NULL;
    CHECK(d8store_save(&invalid,new_data,new_n,15)==D8STORE_INVALID && mutations==0);
    guards();
    printf("D8P1 multi-sector records: %u checks, %u failures; %u operation cuts, %zu program-byte cuts; relative simulator only\n",
           checks,failures,operations,new_n+32);
    return failures!=0;
}
