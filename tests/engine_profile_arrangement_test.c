/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dabbl8 contributors */
#define D8_ENGINE_PROFILE_HOST_SETUP 1
#define D8ARR_ENGINE_COMPAT_NO_MAIN 1
#include "arrangement_engine_compatibility_test.c"
int main(void)
{
    for(unsigned e=0;e<NENGINES;e++) if(!(D8ENGINE_EFFECTIVE_MASK&(1u<<e))) {
        pattern_source(e);snapshot();uint32_t crc=d8p1_crc32(nor,sizeof nor);
        proof(!d8arr_prepare_pool(&compatibility_pool)&&live_same()&&!writes,"actual profile permits unavailable source instruments in pattern-only references");
        for(unsigned o=0;o<3;o++)for(unsigned t=0;t<8;t++)proof(chain.native.policy.source_engine[o][t]==e,"actual profile preserves exact unavailable source-engine provenance");
        proof(crc==d8p1_crc32(nor,sizeof nor),"referenced source bytes unchanged");
        pattern_only(e); /* Existing actual apply/unlock behavior with deliberately narrow analog mask. */
        pattern_source(e);trk[7].eng_req=(uint8_t)e; /* Explicitly inject unavailable destination into existing live state. */
        snapshot();memcpy(arena,&main_workspace,sizeof arena);memcpy(screen,host_screen,sizeof screen);
        __typeof__(dma) old_dma=dma;unsigned fences=sync_calls,wiregen=proj_wire_gen;uint32_t req=transport_req;
        proof(d8arr_prepare_pool(&compatibility_pool)==D8POOL_UNSUPPORTED,"actual compiled profile refuses missing eighth destination before workspace");
        proof(unchanged()&&!memcmp(arena,&main_workspace,sizeof arena)&&!memcmp(screen,host_screen,sizeof screen)&&!memcmp(&old_dma,&dma,sizeof dma)&&sync_calls==fences&&proj_wire_gen==wiregen&&transport_req==req&&!reads&&!writes,"early missing destination preserves complete live/cache/arena/DMA state");
        pattern_source(e);chain.native_mode=1;chain.native.policy.valid=1;chain.native.policy.suspended=1;snapshot();late_engine=e;
        proof(d8arr_prepare_pool(&compatibility_pool)==D8POOL_UNSUPPORTED,"actual profile rechecks late injected unavailable destination");
        trk[7].eng_req=before.tracks[7].eng_req; /* Restore only caller-injected field, not any preparation mutation. */
        proof(live_same()&&!chain.armed&&!chain.native.policy.valid&&!chain.native.policy.preparing&&!writes&&!transport_req,"late refusal tombstones preparation without editing music");
        proof(!d8arr_continue()&&!chain.running,"refused cache cannot continue");
    }
    printf("curated arrangement profile%u: %u checks, %u failures; actual profile wrapper, pattern provenance and destination early/late refusal\n",D8BUILD_PROFILE,checks,failures);
    return failures?1:0;
}
