/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline virtual pool proposal using the real writer/reader; no flash backend. */
#include "d8pool.h"
#include "d8p1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char image[D8POOL_BYTES], input[4][D8P1_LIMIT], output[D8P1_LIMIT];
static size_t lengths[4];
static int rd(void *ctx,uint32_t a,void *p,uint32_t n) {
    (void)ctx;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;
    memcpy(p,image+a,n);return 0;
}
static int er(void *ctx,uint32_t a) {
    (void)ctx;if(a%4096||a>D8POOL_BYTES-4096)return -1;
    memset(image+a,255,4096);return 0;
}
static int pg(void *ctx,uint32_t a,const void *p,uint32_t n) {
    (void)ctx;if(!n||n>256||a>D8POOL_BYTES-n||(a&255)+n>256)return -1;
    const unsigned char *b=p;for(unsigned i=0;i<n;i++)image[a+i]&=b[i];return 0;
}
static int stopped(void *ctx){(void)ctx;return 1;}
int main(int argc,char **argv) {
    /* output mask project0 project1 project2 autosave; '-' means empty slot. */
    if(argc!=7)return 2;
    char *end=NULL;unsigned long mask=strtoul(argv[2],&end,10);
    if(!*argv[2]||*end||mask>7)return 2;
    for(unsigned o=0;o<4;o++) {
        if(!strcmp(argv[3+o],"-")){if(o==3||(mask&(1u<<o)))return 2;continue;}
        if(o<3&&!(mask&(1u<<o)))return 2;
        FILE *f=fopen(argv[3+o],"rb");if(!f)return 1;
        lengths[o]=fread(input[o],1,D8P1_LIMIT,f);int extra=fgetc(f),error=ferror(f);fclose(f);
        d8p1_view view;
        if(error||extra!=EOF||d8p1_read(&view,input[o],lengths[o])!=1||!d8p1_refs_available(&view,(unsigned)mask))return 1;
    }
    memset(image,255,sizeof image);d8pool pool={NULL,rd,er,pg,stopped};
    for(unsigned o=0;o<4;o++)if(lengths[o]&&d8pool_save(&pool,o,input[o],lengths[o],(unsigned)mask))return 1;
    d8pool_index index;if(d8pool_inventory(&pool,&index)||index.present!=((unsigned)mask|8u))return 1;
    for(unsigned o=0;o<4;o++)if(lengths[o]) {
        size_t n=0;if(d8pool_load(&pool,o,output,sizeof output,&n,(unsigned)mask)||n!=lengths[o]||memcmp(output,input[o],n))return 1;
    }
    /* Exclusive output only after complete readback, never a firmware package. */
    FILE *f=fopen(argv[1],"wbx");if(!f)return 1;
    int error=fwrite(image,1,sizeof image,f)!=sizeof image;
    if(fclose(f))error=1;if(error){remove(argv[1]);return 1;}
    printf("{\"format\":\"dabbl8-virtual-pool\",\"version\":1,\"device_writes\":false,\"present\":%u,\"bytes\":%u}\n",index.present,D8POOL_BYTES);
    return 0;
}
