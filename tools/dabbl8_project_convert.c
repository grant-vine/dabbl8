/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline proposed-project conversion only. No device I/O or firmware package. */
#define NPART 8
#define main hostsim_main
#include "../tests/hostsim.c"
#undef main
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include "../firmware/src/d8p1_project.h"
static union { uint32_t align; uint8_t bytes[D8P1_LIMIT]; } input;
static uint8_t encoded[D8P1_LIMIT];
static d8p1_project_state state,workspace,verified;
static void answer(FILE *f,int accepted,const char *reason,unsigned mask,const d8p1_legacy_report *report) {
    fprintf(f,"{\"format\":\"dabbl8-project-conversion\",\"version\":1,\"accepted\":%s,\"device_writes\":false,\"firmware_package\":false,\"available_project_mask\":%u",accepted?"true":"false",mask);
    if(reason)fprintf(f,",\"reason\":\"%s\"",reason);
    if(accepted){
        fprintf(f,",\"track_count\":8,\"shared_voices\":8,\"banks\":%u,\"scenes\":%u,\"chain_rows\":%u,\"motion_records\":%u",state.arrangement.banks,state.arrangement.scenes,state.arrangement.rows,state.project.motion.count);
        if(report){
            fprintf(f,",\"source_format\":\"FUN%u\",\"removed_motion_records\":%u,\"referenced_project_mask\":%u,\"chain_converted\":%s,\"slot_to_scene\":[",report->format,report->removed_motion_records,report->referenced_projects,report->chain_converted?"true":"false");
            for(unsigned i=0;i<4;i++)fprintf(f,"%s%u",i?",":"",report->scene_for_project[i]);
            fputs("],\"engine_changes\":[",f);unsigned changes=0;
            for(unsigned i=0;i<(report->format==1?1u:4u);i++)if(report->engine_before[i]!=report->engine_after[i])
                fprintf(f,"%s{\"track\":%u,\"before\":%u,\"after\":%u}",changes++?",":"",i+1,report->engine_before[i],report->engine_after[i]);
            fputs("]",f);
        }
    }fputs("}\n",f);
}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    int legacy=!strcmp(argv[1],"legacy"),check_only=!strcmp(argv[1],"check"),canonical=!strcmp(argv[1],"canonical");
    if(!legacy&&!check_only&&!canonical)return 2;
    char *end=NULL;unsigned long mask=strtoul(argv[3],&end,10);FILE *report_out=check_only?stdout:stderr;
    if(!argv[3][0]||*end||mask>15){answer(report_out,0,"Invalid reference context",0,NULL);return 1;}
    FILE *f=fopen(argv[2],"rb");if(!f){answer(report_out,0,"Input could not be read",(unsigned)mask,NULL);return 1;}
    size_t n=fread(input.bytes,1,sizeof input.bytes,f);int extra=fgetc(f),error=ferror(f);fclose(f);
    if(error||extra!=EOF){answer(report_out,0,"Input exceeds supported bounds or read failed",(unsigned)mask,NULL);return 1;}
    d8p1_legacy_report report;int ok=legacy?d8p1_legacy_convert(&state,&workspace,&report,input.bytes,n,(unsigned)mask):d8p1_project_decode(&state,input.bytes,n,(unsigned)mask);
    if(!ok){answer(report_out,0,"Unsupported, damaged, non-standalone or unavailable-reference project",(unsigned)mask,NULL);return 1;}
    size_t written=0;
    if(!d8p1_project_encode(encoded,sizeof encoded,&written,&state)||!d8p1_project_decode(&verified,encoded,written,(unsigned)mask)){
        answer(report_out,0,"Converted project failed structural verification",(unsigned)mask,NULL);return 1;
    }
    if(!check_only&&(fwrite(encoded,1,written,stdout)!=written||fflush(stdout)||ferror(stdout)))return 2;
    answer(report_out,1,NULL,(unsigned)mask,legacy?&report:NULL);return ferror(report_out)?2:0;
}
