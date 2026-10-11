/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dabbl8 contributors */
/* Actual immutable curated-profile wrappers; no test mask grants. */
#include "engine_profile_adoption_setup.h"
int main(void)
{
    static d8p1_project_state native;
    static uint8_t raw[D8P1_LIMIT], saved[D8P1_LIMIT];
    static uint8_t arena_before[sizeof main_workspace], screen_before[sizeof host_screen];
    unsigned refused=0;
    for(unsigned e=0;e<NENGINES;e++) if(e!=ENGI_DIGITAL && !(D8ENGINE_EFFECTIVE_MASK&(1u<<e))) {
        for(unsigned part=0;part<8;part+=7) {
            fresh();input.t[part].engine=e;input.t[part].p[P_E0]=0;
            input.sum=proj_sum(&input);saved_input=input;
            cv_begin(240,124,T_BG);cv_rect(1,2,17,19,T_ACCENT);cv_blit(0,20);
            REQUIRE(dma.p!=0);snapshot();
            memcpy(arena_before,&main_workspace,sizeof arena_before);memcpy(screen_before,host_screen,sizeof screen_before);
            REQUIRE(project_restore_runtime(&input)==PROJ_RUNTIME_MISSING_ENGINE);
            unchanged();REQUIRE(!memcmp(&saved_input,&input,sizeof input));
            REQUIRE(!memcmp(arena_before,&main_workspace,sizeof arena_before)&&!memcmp(screen_before,host_screen,sizeof screen_before));
            native.project=input;size_t n=0;
            REQUIRE(d8p1_project_encode(raw,sizeof raw,&n,&native));memcpy(saved,raw,n);snapshot();
            REQUIRE(d8p1_load_runtime(raw,n,15)==D8RT_MISSING_ENGINE);
            unchanged();REQUIRE(!memcmp(raw,saved,n));
            REQUIRE(!memcmp(arena_before,&main_workspace,sizeof arena_before)&&!memcmp(screen_before,host_screen,sizeof screen_before));refused++;
        }
    }
    fresh();saved_input=input;REQUIRE(project_restore_runtime(&input)==0);
    REQUIRE(!memcmp(&saved_input,&input,sizeof input));
    fresh();input.t[0].engine=ENGI_DIGITAL;input.sum=proj_sum(&input);
    REQUIRE(project_restore_runtime(&input)==0);REQUIRE(trk[0].eng_req==ENGI_FM6);
    printf("curated imports profile%u: %u checks PASS, %u early structured/wire refused cases; actual wrappers and historical FM6 conversion\n",D8BUILD_PROFILE,proofs,refused);
    return 0;
}
