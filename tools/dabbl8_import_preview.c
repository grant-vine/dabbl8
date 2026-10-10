/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline semantic preview using the pinned firmware importer. No device I/O. */
#define main hostsim_main
#include "../tests/hostsim.c"
#undef main
#undef NTRK
#define NTRK 8
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include <stddef.h>

static void bytes(const uint8_t *p, unsigned n) {
    putchar('['); for (unsigned i=0;i<n;i++) printf("%s%u",i?",":"",p[i]); putchar(']');
}
static void numbers(const int16_t *p, unsigned n) {
    putchar('['); for (unsigned i=0;i<n;i++) printf("%s%d",i?",":"",p[i]); putchar(']');
}
/* Read only accepted historical layouts to report engine migrations that
 * proj_import_any already applied (old GM drums and PHYS DRUM included). */
static uint8_t source_engine(const uint8_t *b, unsigned version, unsigned t) {
    size_t base=66, stride=0, off=0;
    switch(version) {
    case 1: base=offsetof(project_v1_t,t); stride=sizeof(proj_trk_v2_t); off=offsetof(proj_trk_v2_t,engine); break;
    case 2: stride=sizeof(proj_trk_v2_t); off=offsetof(proj_trk_v2_t,engine); break;
    case 3: stride=sizeof(proj_trk_v3_t); off=offsetof(proj_trk_v3_t,engine); break;
    case 4: stride=sizeof(proj_trk_v4_t); off=offsetof(proj_trk_v4_t,engine); break;
    case 5: case 6: stride=sizeof(proj_trk_v5_t); off=offsetof(proj_trk_v5_t,engine); break;
    default: base=68; stride=b[66]+2+NSTEP*9; off=b[66]; break;
    }
    return b[base+t*stride+off];
}
int main(int argc, char **argv) {
    if (argc!=2) { fputs("usage: dabbl8_import_preview INPUT\n",stderr); return 2; }
    union { uint32_t align; uint8_t raw[PROJ_STORE_SIZE]; } input;
    FILE *f=fopen(argv[1],"rb"); if (!f) { perror(argv[1]); return 2; }
    size_t n=fread(input.raw,1,sizeof input.raw,f);
    int extra=fgetc(f), err=ferror(f); fclose(f);
    uint32_t magic=0,size=0;
    if (n>=8) { memcpy(&magic,input.raw,4); memcpy(&size,input.raw+4,4); }
    /* Standalone disk records only; do not silently discard retained-slot tails. */
    project_t q,before;
    int ok=!err && extra==EOF && n>=8 && size==n && magic>=PROJ_MAGIC_V1 && magic<=PROJ_MAGIC &&
        proj_import_any(&before,input.raw,(int)n) && proj_import(&q,input.raw,(int)n);
    printf("{\"format\":\"dabbl8-import-preview\",\"version\":1,\"accepted\":%s",ok?"true":"false");
    if (!ok) { puts(",\"reason\":\"Unsupported, damaged, oversized or non-standalone FUN1..FUN9 project\"}"); return 1; }
    /* New companion tracks have explicit defaults, not historical zero padding.
     * This affects the preview only; it does not modify firmware imports. */
    for (unsigned t=4;t<NTRK;t++) {
        memset(&q.t[t],0,sizeof q.t[t]);
        q.t[t].engine=(uint8_t)trk_def_engine(t); q.t[t].preset=PROJ_DEF_SOUND;
        for (unsigned p=0;p<P_COUNT;p++) q.t[t].p[p]=param_desc_of(q.t[t].engine,p)->def;
        for (unsigned i=0;i<NSTEP;i++) q.t[t].step[i].time=ST_REST;
        memcpy(q.fm6[t],FM6_INIT,FM6_PACKED);
    }
    printf(",\"source_format\":\"FUN%u\",\"track_count\":8,\"step_count\":%u,\"parameter_count\":%u,\"shared_voices\":8,\"changes\":[",magic-PROJ_MAGIC_V1+1,NSTEP,P_COUNT);
    unsigned changes=0;
    unsigned version=magic-PROJ_MAGIC_V1+1;
    for (unsigned t=0;t<(version==1?1u:4u);t++) if (source_engine(input.raw,version,t)!=q.t[t].engine) {
        printf("%s{\"track\":%u,\"engine_before\":%u,\"engine_after\":%u}",changes++?",":"",t+1,source_engine(input.raw,version,t),q.t[t].engine);
    }
    printf("],\"removed_motion_records\":%u,\"globals\":",before.motion.count-q.motion.count); numbers(q.g,G_COUNT);
    printf(",\"selected_track\":%u,\"parts\":%u,\"phys\":%u,\"name_bytes\":",q.sel+1,q.parts,q.phys); bytes((const uint8_t*)q.name,PROJ_NAME_LEN);
    printf(",\"tracks\":[");
    for (unsigned t=0;t<NTRK;t++) {
        const proj_trk_t *tr=&q.t[t];
        printf("%s{\"track\":%u,\"engine\":%u,\"preset\":%u,\"params\":",t?",":"",t+1,tr->engine,tr->preset); numbers(tr->p,P_COUNT);
        printf(",\"fm6\":"); bytes(q.fm6[t],FM6_PACKED);
        printf(",\"steps\":[");
        for (unsigned i=0;i<NSTEP;i++) {
            const step_t *s=&tr->step[i];
            printf("%s{\"note\":",i?",":""); bytes(s->note,4);
            printf(",\"n\":%u,\"time\":%u,\"flags\":%u,\"vel\":%u,\"hit\":%u,\"acc\":%u,\"probability\":%u}",s->n,s->time,s->flags,s->vel,s->hit,s->acc,s->probability);
        }
        printf("]}");
    }
    printf("],\"chain\":{\"count\":%u,\"reserved\":",q.chain.count); bytes(q.chain.rsv,3); printf(",\"rows\":[");
    for(unsigned i=0;i<CHAIN_ROWS;i++) printf("%s[%u,%u]",i?",":"",q.chain.row[i].slot,q.chain.row[i].repeat);
    printf("]},\"motion\":{\"count\":%u,\"on\":%u,\"reserved\":",q.motion.count,q.motion.on); bytes(q.motion.rsv,2); printf(",\"events\":[");
    for(unsigned i=0;i<q.motion.count;i++) {
        const motion_event_t *e=&q.motion.event[i];
        printf("%s{\"place\":%u,\"param\":%u,\"value\":%d}",i?",":"",e->place,e->param,e->value);
    }
    puts("]}}"); return ferror(stdout)?2:0;
}
