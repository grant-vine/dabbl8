/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only exact-byte project-set inspection/rebuild. No mapped flash backend. */
#include "d8p1.h"
#include "d8pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t image[D8POOL_BYTES], wire[4][D8P1_LIMIT], output[D8P1_LIMIT];
static size_t lengths[4];
static int rd(void *c,uint32_t a,void *p,uint32_t n) {
    (void)c;
    if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;
    memcpy(p,image+a,n);
    return 0;
}
static int er(void *c,uint32_t a) {
    (void)c;
    if(a%4096||a>D8POOL_BYTES-4096)return -1;
    memset(image+a,255,4096);
    return 0;
}
static int pg(void *c,uint32_t a,const void *p,uint32_t n) {
    (void)c;
    if(!n||n>256||a>D8POOL_BYTES-n||(a&255)+n>256)return -1;
    const uint8_t*b=p;
    for(unsigned i=0;i<n;i++)image[a+i]&=b[i];
    return 0;
}
static int stopped(void *c) {
    (void)c;
    return 1;
}
static d8pool pool= {
    NULL,rd,er,pg,stopped
};
static int number(const char *p,unsigned maximum,unsigned *out) {
    char *end;
    unsigned long n=strtoul(p,&end,10);
    if(!*p||*end||n>maximum)return 0;
    *out=(unsigned)n;
    return 1;
}
static int read_file(const char *path,void *p,size_t cap,size_t *n) {
    FILE*f=fopen(path,"rb");
    if(!f)return 0;
    *n=fread(p,1,cap,f);
    int extra=fgetc(f);
    int ok=!ferror(f)&&extra==EOF;
    if(fclose(f))ok=0;
    return ok;
}
static int known(const void *p,size_t n,unsigned mask) {
    d8p1_view v;
    int rc=d8p1_read(&v,p,n);
    if(rc!=1) {
        fprintf(stderr,rc==2?"Unknown optional data is archival only\n":"Unsupported or damaged D8P1\n");
        return 0;
    }
    if(!d8p1_refs_available(&v,mask)) {
        fprintf(stderr,"Unavailable saved-project reference\n");
        return 0;
    }
    return 1;
}
static uint32_t u32(const uint8_t*p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int inspect(const char *path,d8pool_index *index) {
    size_t n;
    if(!read_file(path,image,sizeof image,&n)||n!=sizeof image) {
        fprintf(stderr,"Virtual pool must be exactly 40960 bytes\n");
        return 0;
    }
    int rc=d8pool_inventory(&pool,index);
    if(rc) {
        fprintf(stderr,"Pool inventory refused: %d\n",rc);
        return 0;
    }
    unsigned nonempty=0;
    for(unsigned b=0;b<D8POOL_BLOCKS;b++) {
        const uint8_t*h=image+b*D8POOL_BLOCK;
        unsigned residue=0;
        for(unsigned i=0;i<D8POOL_BLOCK;i++)if(h[i]!=255) {
            nonempty++;residue=1;
            break;
        }
        unsigned trusted=u32(h+28)==d8p1_crc32(h,28)&&u32(h)==D8POOL_MAGIC&&h[4]==1&&u32(h+8)<4;
        /* A damaged sole copy can otherwise look like an absent identity.
         * An unknown torn spare is safe for export only when no role is absent. */
        if(residue&&!trusted&&index->present!=15u) {
            fprintf(stderr,"Unrecognized nonempty residue cannot prove sparse absence\n");
            return 0;
        }
        if(trusted&&!(index->present&(1u<<u32(h+8)))) {
            fprintf(stderr,"Unrecoverable committed object cannot be declared absent\n");
            return 0;
        }
    }
    if(!index->present&&nonempty) {
        fprintf(stderr,"Nonempty pool is not proven empty\n");
        return 0;
    }
    for(unsigned o=0;o<4;o++)if(index->present&(1u<<o)) {
        size_t count=0;
        rc=d8pool_load(&pool,o,wire[o],sizeof wire[o],&count,index->present&7u);
        if(rc) {
            fprintf(stderr,"Stored object %u refused: %d\n",o,rc);
            return 0;
        }
        lengths[o]=count;
    }
    return 1;
}
int main(int argc,char **argv) {
    if(argc==4&&!strcmp(argv[1],"check")) {
        unsigned mask;
        size_t n;
        if(!number(argv[3],7,&mask)||!read_file(argv[2],output,sizeof output,&n)||!known(output,n,mask))return 1;
        printf("{\"format\":\"dabbl8-project-archive-check\",\"version\":1,\"bytes\":%zu}\n",n);
        return 0;
    }
    if((argc==3&&!strcmp(argv[1],"inventory"))||(argc==4&&!strcmp(argv[1],"read"))) {
        d8pool_index index;
        if(!inspect(argv[2],&index))return 1;
        if(argc==4) {
            unsigned o;
            if(!number(argv[3],3,&o)||!(index.present&(1u<<o)))return 1;
            return fwrite(wire[o],1,lengths[o],stdout)!=lengths[o]||fflush(stdout)||ferror(stdout);
        }
        printf("{\"format\":\"dabbl8-project-archive-pool\",\"version\":1,\"present\":%u,\"records\":[",index.present);
        for(unsigned o=0;o<4;o++) {
            if(o)putchar(',');
            if(index.present&(1u<<o))printf("{\"sequence\":%u,\"length\":%u,\"crc\":%u,\"block\":%u}",index.object[o].sequence,index.object[o].length,index.object[o].crc,index.object[o].block);
            else printf("null");
        }
        puts("]}");
        return 0;
    }
    if(argc==8&&!strcmp(argv[1],"build")) {
        unsigned mask;
        if(!number(argv[3],15,&mask))return 1;
        for(unsigned o=0;o<4;o++) {
            if(!strcmp(argv[4+o],"-")) {
                if(mask&(1u<<o))return 1;
                continue;
            }
            if(!(mask&(1u<<o))||!read_file(argv[4+o],wire[o],sizeof wire[o],&lengths[o])||!known(wire[o],lengths[o],mask&7u))return 1;
        }
        memset(image,255,sizeof image);
        for(unsigned o=0;o<4;o++)if(mask&(1u<<o))if(d8pool_save(&pool,o,wire[o],lengths[o],mask&7u))return 1;
        d8pool_index index;
        if(d8pool_inventory(&pool,&index)||index.present!=mask)return 1;
        for(unsigned o=0;o<4;o++)if(mask&(1u<<o)) {
            size_t n=0;
            if(d8pool_load(&pool,o,output,sizeof output,&n,mask&7u)||n!=lengths[o]||memcmp(output,wire[o],n))return 1;
        }
        FILE*f=fopen(argv[2],"wbx");
        if(!f)return 1;
        int error=fwrite(image,1,sizeof image,f)!=sizeof image;
        if(fclose(f))error=1;
        if(error) {
            remove(argv[2]);
            return 1;
        }
        printf("{\"format\":\"dabbl8-project-archive-pool\",\"version\":1,\"present\":%u,\"bytes\":%u}\n",mask,D8POOL_BYTES);
        return 0;
    }
    fprintf(stderr,"inventory POOL | read POOL OBJECT | check PROJECT MASK | build OUTPUT PRESENT PROJECT0 PROJECT1 PROJECT2 AUTOSAVE\n");
    return 2;
}
