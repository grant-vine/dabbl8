/* SPDX-License-Identifier: GPL-3.0-only */
/* Invalid/unavailable requests remain separate from the valid WHEEL stress mix.
 * Actual UI refusals preserve sounding state, undo, presets and labels. */
#if NPART != 8
#error Build with -DNPART=8
#endif
#define D8_SOUND_UI_TEST 1
#define ENGINE_SOUND_TEST_NO_MAIN 1
#include "engine_sound_selection_test.c"
static heavy_state_t saved_heavy[NVOICE];
static uint16_t saved_owner[NVOICE];
static void refuse(unsigned request,unsigned mask,int expected)
{
    snapshot();
    memcpy(saved_heavy,heavy_pool,sizeof saved_heavy);
    memcpy(saved_owner,heavy_owner,sizeof saved_owner);
    REQUIRE(select_engine_mask(request,mask)==expected);
    unchanged();
    REQUIRE(!memcmp(saved_heavy,heavy_pool,sizeof saved_heavy));
    REQUIRE(!memcmp(saved_owner,heavy_owner,sizeof saved_owner));
}
int main(void)
{
    seed();
    const unsigned full=D8ENGINE_EFFECTIVE_MASK;
    refuse(7u,full&~(1u<<7),D8ENGINE_UNAVAILABLE);
#if FELUCCA_SLICE
    refuse(13u,full&~(1u<<13),D8ENGINE_UNAVAILABLE);
#else
    /* ID 13 is beyond the compatibility build's descriptors; never alias it. */
    refuse(13u,full,D8ENGINE_INVALID_REQUEST);
#endif
    refuse(NENGINES,full,D8ENGINE_INVALID_REQUEST);
    refuse(14u,full,D8ENGINE_INVALID_REQUEST);
    refuse(UINT32_MAX,full,D8ENGINE_INVALID_REQUEST);
    refuse(7u,0,D8ENGINE_INVALID_MASK);
    refuse(7u,full|0x4000u,D8ENGINE_INVALID_MASK);
    printf("WHEEL unavailable requests: %u proofs FM4=%u SLICE=%u; actual UI state/undo/presets/labels/heavy ownership unchanged\n",proofs,FELUCCA_FM4,FELUCCA_SLICE);
    return 0;
}
