/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Actual eight-track project and drawing paths against an asynchronous LCD.
 * Pixel sources stay live until sync, so premature arena writes are observable.
 * A child process exercises the production fail-before-write ownership trap.
 * This is a host lifetime test, not physical DMA or stack qualification. */
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#define UI_TEST_NO_MAIN 1
#define UI_ASYNC_LCD 1
#include "ui_test.c"
#include "../firmware/src/d8p1_project.h"
static d8p1_project_state staged_before;
static uint8_t staged_encoded[D8P1_LIMIT];
_Static_assert(NPART >= NVOICE, "test the expanded main workspace");
int main(int argc,char **argv) {
    int bad=0; char name[13]; setvbuf(stdout,0,_IONBF,0);
    if(argc!=2)return 2;
    ui_power_on();
    FILE *f=fopen(argv[1],"rb"); if(!f)return 2;
    if(fread(&proj_slot[0],1,sizeof proj_slot[0],f)!=sizeof proj_slot[0])return 2;
    fclose(f);
    bad+=check("typed arena retains full canvas extent",sizeof main_workspace==59520u && sizeof(project_t)==8764u);
    cv_begin(240,124,T_SURF);cv_rect(3,4,80,70,T_ACCENT);uint32_t hash=pixels_hash(cv_px,CV_MAX);
    cv_blit(0,20); uint32_t before=dma_consumed;
    bad+=check("LCD keeps canvas pointer pending after blit",dma.p==cv_px && !cv_cpu_active);
    bad+=check("project name waits for DMA before overwriting arena",project_name(0,name) && dma_consumed==before+1 && !dma.p && !dma_errors);
    bad+=check("consumed pixels match original canvas",pixels_hash(host_screen+20*240,CV_MAX)==hash);
    pid_t stale=fork();if(stale<0)return 2;
    if(!stale){cv_blit(0,0);_exit(99);}
    int stale_status=0;waitpid(stale,&stale_status,0);
    bad+=check("blitting a project-owned arena traps before LCD access",WIFSIGNALED(stale_status) && (WTERMSIG(stale_status)==SIGILL || WTERMSIG(stale_status)==SIGTRAP));
    cv_begin(240,124,T_BG);cv_rect(1,1,23,29,T_ACCENT);
    hash=pixels_hash(cv_px,CV_MAX);cv_blit(0,20);before=dma_consumed;
    d8p1_stage_workspace *staging=main_d8p1_workspace();
    bad+=check("D8P1 borrow fences pending LCD pixels",dma_consumed==before+1 && !dma.p && !dma_errors && pixels_hash(host_screen+20*240,CV_MAX)==hash);
    bad+=check("decoded state and bounded wire fit unchanged arena",sizeof *staging==17472u && sizeof main_workspace==59520u && sizeof staging->state==9536u);
    f=fopen("tests/fixtures/d8p1/maximum.d8p","rb");if(!f)return 2;
    size_t wire_n=fread(staging->wire,1,sizeof staging->wire,f);if(fgetc(f)!=EOF)return 2;fclose(f);
    bad+=check("actual maximum D8P1 decodes in display arena",wire_n==7705u && d8p1_project_decode(&staging->state,staging->wire,wire_n,15));
    size_t encoded_n=0;
    bad+=check("arena decode retains exact source bytes",d8p1_project_encode(staged_encoded,sizeof staged_encoded,&encoded_n,&staging->state) && encoded_n==wire_n && !memcmp(staged_encoded,staging->wire,wire_n));
    memcpy(&staged_before,&staging->state,sizeof staged_before);staging->wire[wire_n-1]^=1;
    bad+=check("corrupt staged wire preserves decoded state",!d8p1_project_decode(&staging->state,staging->wire,wire_n,15) && !memcmp(&staged_before,&staging->state,sizeof staged_before));
    cv_begin(8,8,T_BG);
    pid_t d8child=fork();if(d8child<0)return 2;
    if(!d8child){main_d8p1_workspace();_exit(99);}
    int d8status=0;waitpid(d8child,&d8status,0);
    bad+=check("D8P1 borrow during drawing traps before writes",WIFSIGNALED(d8status) && (WTERMSIG(d8status)==SIGILL || WTERMSIG(d8status)==SIGTRAP));
    cv_blit(0,0);(void)main_project_workspace();
    trk[7].p[P_LEVEL]=83;project_capture(&proj_scratch);
    bad+=check("expanded project captures track eight",proj_scratch.t[7].p[P_LEVEL]==83);
    project_store_t wire;
    bad+=check("historical export refuses eight tracks",!proj_pack(&wire,&proj_scratch));
    project_load(0);bad+=check("historical runtime load works after display ownership",proj_cur==0);
    cv_begin(240,124,T_BG);hash=pixels_hash(cv_px,CV_MAX);
    pid_t child=fork();if(child<0)return 2;
    if(!child){main_project_workspace();_exit(99);}
    int status=0;waitpid(child,&status,0);
    bad+=check("project borrow during CPU drawing traps",WIFSIGNALED(status) && (WTERMSIG(status)==SIGILL || WTERMSIG(status)==SIGTRAP));
    bad+=check("parent canvas remains valid after rejected borrow",cv_cpu_active && pixels_hash(cv_px,CV_MAX)==hash);
    cv_blit(0,0);before=dma_consumed;cv_blit_from(0,0,4);
    bad+=check("an unchanged canvas can still be blitted again",dma_consumed==before+1 && dma.p && !dma_errors);
    before=dma_consumed;(void)main_project_workspace();
    bad+=check("partial-row DMA also completes before project borrow",dma_consumed==before+1 && !dma.p && !dma_errors);
    cv_begin(8,8,T_BG);cv_blit_from(0,0,8);
    bad+=check("empty partial blit ends CPU canvas ownership",!cv_cpu_active && main_project_workspace()!=0);
    ui.force=1;ui.frame++;uint32_t mask=(uint32_t)graph_project_used(0);
    cv_begin(240,124,T_BG);hash=pixels_hash(cv_px,CV_MAX);before=sync_calls;
    fm1_ms+=1000;ui.frame++;ui.force=1;
    bad+=check("name-cache expiry cannot import inside a canvas",graph_project_used(0)==(int)mask && sync_calls==before && pixels_hash(cv_px,CV_MAX)==hash);
    cv_blit(0,0);(void)graph_project_used(0);
    bad+=check("next safe cache refresh consumes pending DMA",!dma.p && !dma_errors);
    uint32_t pages=0;
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].graph==GR_SLOTS || PAGES[p].graph==GR_SONG){
        ui.home=0;ui.page=(uint8_t)p;ui.layer=ui.menu=ui.confirm=ui.uboot=0;
        for(uint32_t layout=0;layout<2;layout++){
            ui_prefs=(uint8_t)((ui_prefs & ~PREF_LARGE) | (layout ? PREF_LARGE : 0));
            for(uint32_t k=0;k<600;k++){
                fm1_ms+=501;ui.force=1;ui_draw();
                if(cv_cpu_active)return 3;
                if(!project_name(0,name))bad++;
                project_capture(&proj_scratch);
            }
        }pages++;
    }
    bad+=check("2400 slot/song frames preserve asynchronous pixel sources",pages==2 && dma_consumed>2400 && !dma_errors && !cv_cpu_active);
    printf("main workspace: %d failures, %u consumed DMA sources, %u ownership errors\n",bad,dma_consumed,dma_errors);
    return !!bad;
}
