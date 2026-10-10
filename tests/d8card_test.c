/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8card.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static void put32(unsigned char *p,unsigned n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 1;} } while(0)
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    /* Independent fixed IDs and component dependency masks. Never renumber. */
    const unsigned ids[]={0,2,3,4,5,6,7,8,9,10,11,12,13};
    const unsigned deps[]={0,0,0,1,0,0,4,5,4,4,0,6,5};
    CHECK(D8CARD_ENGINES==13 && D8CARD_ALL==0x3ffdu);
    for(unsigned subset=0;subset<8192;subset++) {
        unsigned mask=0,need=0;
        for(unsigned i=0;i<13;i++)if(subset&(1u<<i)){mask|=1u<<ids[i];need|=deps[i];}
        for(unsigned components=0;components<8;components++) {
            int want=!mask?D8CARD_INVALID_PROFILE:((need&~components)?D8CARD_MISSING_COMPONENT:D8CARD_OK);
            CHECK(d8card_validate(mask,components)==want);
        }
    }
    CHECK(d8card_validate(2,7)==D8CARD_INVALID_PROFILE);
    CHECK(d8card_validate(D8CARD_ALL|0x4000,7)==D8CARD_INVALID_PROFILE);
    CHECK(d8card_validate(D8CARD_ALL,8)==D8CARD_INVALID_PROFILE);
    for(unsigned i=0;i<13;i++) {const d8card_engine *e=d8card_engine_by_id(ids[i]);CHECK(e&&e->id==ids[i]&&e->components==deps[i]);}
    CHECK(!d8card_engine_by_id(1)&&!d8card_engine_by_id(14)&&!d8card_engine_by_id(0xffffffffu));
    unsigned char raw[D8P1_LIMIT],saved[D8P1_LIMIT];
    for(unsigned fixture=1;fixture<=3;fixture++) {
        FILE *f=fopen(argv[fixture],"rb");CHECK(f);size_t n=fread(raw,1,sizeof raw,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);memcpy(saved,raw,n);
        d8card_report report={0xa5a5a5a5,0xa5a5a5a5},before=report;
        int rc=d8card_preflight(&report,raw,n,D8CARD_ALL,7);
        if(fixture==3){CHECK(rc==D8CARD_UNSUPPORTED_PROJECT&&!memcmp(&report,&before,sizeof report));continue;}
        CHECK(rc==D8CARD_UNSUPPORTED_PROJECT&&!memcmp(&report,&before,sizeof report));
        d8p1_view view;CHECK(d8p1_read(&view,raw,n)==1);
        for(unsigned i=0;i<view.count;i++)if((view.chunk[i].type&32767u)==2u) {
            unsigned char *tracks=(unsigned char *)view.chunk[i].data;
            tracks[677u+99u]=12; /* explicit synthetic normalization, never an importer mutation */
            put32(tracks-4,d8p1_crc32(tracks,view.chunk[i].length));
        }
        put32(raw+12,0);put32(raw+12,d8p1_crc32(raw,n));memcpy(saved,raw,n);
        CHECK(d8card_preflight(&report,raw,n,D8CARD_ALL,7)==D8CARD_OK&&report.required==0x10fdu&&!report.missing);
        CHECK(d8card_preflight(&report,raw,n,D8CARD_ALL&~(1u<<12),7)==D8CARD_MISSING_ENGINE&&report.required==0x10fdu&&report.missing==(1u<<12));
        report=before;CHECK(d8card_preflight(&report,raw,n,D8CARD_ALL,0)==D8CARD_MISSING_COMPONENT&&!memcmp(&report,&before,sizeof report));
        CHECK(d8card_preflight(NULL,raw,n,D8CARD_ALL,7)==D8CARD_INVALID_PROJECT);
        CHECK(d8card_preflight((d8card_report *)raw,raw,n,D8CARD_ALL,7)==D8CARD_INVALID_PROJECT&&!memcmp(raw,saved,n));
        for(size_t cut=0;cut<n;cut++){report=before;CHECK(d8card_preflight(&report,raw,cut,D8CARD_ALL,7)==D8CARD_INVALID_PROJECT&&!memcmp(&report,&before,sizeof report));}
        CHECK(!memcmp(raw,saved,n));
    }
    printf("d8card: %u checks passed\n",checks);return 0;
}
