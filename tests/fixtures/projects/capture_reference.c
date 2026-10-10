/* SPDX-License-Identifier: GPL-3.0-only
 * Reference fixture capture: compile against pristine Felucca v1.1.5 only. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include <stddef.h>
static project_t seed;
static const char *outdir;
static void put(const char *name, const void *p, size_t n) {
    char path[1024]; snprintf(path,sizeof path,"%s/%s.bin",outdir,name);
    FILE *f=fopen(path,"wb"); if(!f || fwrite(p,1,n,f)!=n || fclose(f)) exit(2);
}
static void emit(const char *name,const void *p,size_t n) {
    project_t q; project_store_t st; char expected[128];
    if(!proj_import(&q,p,n) || !proj_pack(&st,&q)) {fprintf(stderr,"cannot capture %s\n",name);exit(3);}
    put(name,p,n); snprintf(expected,sizeof expected,"%s.expected-fun9",name); put(expected,&st,sizeof st);
    printf("%s %zu\n",name,n);
}
static void init(void) {
    memset(&seed,0,sizeof seed); seed.magic=PROJ_MAGIC;seed.size=sizeof seed;seed.parts=4;seed.phys=PROJ_PHYS;seed.sel=3;
    for(unsigned i=0;i<G_COUNT;i++) seed.g[i]=GP[i].def;
    chain_defaults(&seed.chain);seed.chain.count=2;seed.chain.row[0].slot=0;seed.chain.row[0].repeat=2;seed.chain.row[1].slot=3;seed.chain.row[1].repeat=1;
    const unsigned eng[4]={0,ENGI_FM6,ENGI_PHYS,ENGI_DRUM};
    for(unsigned t=0;t<4;t++) {
        seed.t[t].engine=eng[t];seed.t[t].preset=t;
        for(unsigned i=0;i<P_COUNT;i++) seed.t[t].p[i]=param_desc_of(eng[t],i)->def;
        seed.t[t].p[P_PAN]=(int16_t)(-40+(int)t*25);seed.t[t].p[P_REV]=10+t*12;
        for(unsigned j=0;j<64;j++) {
            step_t *s=&seed.t[t].step[j];s->time=ST_REST;
            if(!(j%16)) {*s=(step_t){{60+t,64+t},2,ST_NOTE,SF_ACCENT,90+t,0,0,0};}
        }
        memcpy(seed.fm6[t],FM6_INIT,FM6_PACKED);
    }
    seed.motion.on=15;seed.motion.count=2;
    seed.motion.event[0]=(motion_event_t){0,P_REV,70};
    seed.motion.event[1]=(motion_event_t){255,P_REV|MOTION_LOCK,40};
    memcpy(seed.name,"LEGACY TEST",11);seed.sum=proj_sum(&seed);
}
static void params(int16_t *p,unsigned np,unsigned t) {
    for(unsigned i=0;i<np-8;i++) p[i]=seed.t[t].p[i];
    for(unsigned i=0;i<8;i++) p[np-8+i]=seed.t[t].p[P_E0+i];
}
#define TRACKS(v,np,nt) do {for(unsigned t=0;t<(nt);t++){params((v).t[t].p,np,t);(v).t[t].engine=seed.t[t].engine;(v).t[t].preset=seed.t[t].preset;for(unsigned j=0;j<64;j++)memcpy(&(v).t[t].step[j],&seed.t[t].step[j],sizeof (v).t[t].step[j]);}}while(0)
#define HEADER(v,magicval) do{memset(&(v),0,sizeof(v));(v).magic=magicval;(v).size=sizeof(v);memcpy((v).g,seed.g,sizeof(v).g);}while(0)
#define FINISH(v,name) do{(v).sum=proj_hash(&(v),sizeof(v)-4);emit(name,&(v),sizeof(v));}while(0)
static void compact(unsigned version,const char *name) {
    project_store_t st; if(!proj_pack(&st,&seed))exit(4);
    if(version==9) {emit(name,&st,sizeof st);return;}
    unsigned size=version==7?3388:3584,np=91,pos=68,src=68;
    union{uint32_t align;uint8_t b[3648];} old={0};
    memcpy(old.b,st.raw,68);uint32_t magic=0x46554e30u+version;memcpy(old.b,&magic,4);memcpy(old.b+4,&size,4);old.b[66]=np;
    for(unsigned t=0;t<4;t++) {
        memcpy(old.b+pos,st.raw+src,np-8);pos+=np-8;
        memcpy(old.b+pos,st.raw+src+P_E0,8);pos+=8;
        memcpy(old.b+pos,st.raw+src+P_COUNT,2+64*9);pos+=2+64*9;src+=P_COUNT+2+64*9;
    }
    memcpy(old.b+pos,st.raw+src,sizeof seed.chain+sizeof seed.motion);
    unsigned nameoff=size-16;
    if(version==8)memcpy(old.b+nameoff-512,seed.fm6,512);
    memcpy(old.b+nameoff,seed.name,12);uint32_t hash=proj_hash(old.b,size-4);memcpy(old.b+size-4,&hash,4);emit(name,old.b,size);
}
int main(int argc,char **argv) {
    if(argc!=2)return 1;outdir=argv[1];init();
    project_v1_t v1;HEADER(v1,PROJ_MAGIC_V1);params(v1.t.p,53,0);v1.t.engine=seed.t[0].engine;v1.t.preset=0;for(unsigned j=0;j<64;j++)memcpy(&v1.t.step[j],&seed.t[0].step[j],8);FINISH(v1,"fun1");
    v1.t.engine=1;FINISH(v1,"fun1-digital");
    project_v2_t v2;HEADER(v2,PROJ_MAGIC_V2);v2.sel=3;TRACKS(v2,53,4);v2.t[3].engine=0;FINISH(v2,"fun2");
    project_v3_t v3;HEADER(v3,PROJ_MAGIC_V3);v3.sel=3;v3.parts=4;TRACKS(v3,57,4);FINISH(v3,"fun3");
    project_v4_t v4;HEADER(v4,PROJ_MAGIC_V4);v4.sel=3;v4.parts=4;v4.phys=PROJ_PHYS;TRACKS(v4,69,4);FINISH(v4,"fun4");
    v4.phys=1;v4.t[2].p[61]=4;FINISH(v4,"fun4-phys-drum");
    project_v5_t v5;HEADER(v5,PROJ_MAGIC_V5);v5.sel=3;v5.parts=4;v5.phys=PROJ_PHYS;TRACKS(v5,69,4);FINISH(v5,"fun5");
    project_v6_t v6;HEADER(v6,PROJ_MAGIC_V6);v6.sel=3;v6.parts=4;v6.phys=PROJ_PHYS;TRACKS(v6,69,4);v6.chain=seed.chain;FINISH(v6,"fun6");
    compact(7,"fun7");compact(8,"fun8");compact(9,"fun9");
    seed.t[0].engine=ENGI_SAMPLE;seed.t[0].p[P_E0]=4;compact(8,"fun8-perc");
    init();seed.t[0].engine=1;compact(9,"fun9-digital");
    return 0;
}
