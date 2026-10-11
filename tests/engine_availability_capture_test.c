/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual native schema-2 capture/arena retention; virtual NOR only.
 * Four-track firmware has no native capture/arena protocol. Its separate
 * engine_availability_caps_test covers schema/refusal state preservation. */
#if defined(NPART) && NPART != 8
#error "native capture fixture requires NPART=8; four-track capture is not applicable"
#endif
#define D8_INSTRUMENT_CAPTURE_NO_MAIN 1
#include "instrument_capture_test.c"
_Static_assert(NTRK == 8, "actual native capture configuration");
static __typeof__(ed_capture) retained_capture;
static __typeof__(d8_capture_change) retained_change;
static uint8_t retained_arena[sizeof main_workspace];
static void query_matrix(void)
{
    memcpy(&retained_capture,&ed_capture,sizeof ed_capture);
    memcpy(&retained_change,&d8_capture_change,sizeof d8_capture_change);
    memcpy(retained_arena,&main_workspace,sizeof main_workspace);
    const uint32_t owner=migration_owner,generation=migration_generation;
    const unsigned before_reads=hw_read_calls,before_writes=writes;
    const unsigned expected_mask=0x1ffdu|(FELUCCA_FM4?2u:0u)|(FELUCCA_SLICE?0x2000u:0u);
    for(unsigned version=0;version<128;version++)for(unsigned len=1;len<=3;len++) {
        uint8_t frame[]={0x7d,0x46,0x4c,75,(uint8_t)version,0,0,0xa5};
        uint8_t input[sizeof frame];memcpy(input,frame,sizeof input);
        ed_handle(frame,4+len);so_r=so_w;
        CHECK(!memcmp(input,frame,sizeof input));
        CHECK(!memcmp(&retained_capture,&ed_capture,sizeof ed_capture));
        CHECK(!memcmp(&retained_change,&d8_capture_change,sizeof d8_capture_change));
        CHECK(!memcmp(retained_arena,&main_workspace,sizeof main_workspace));
        CHECK(migration_owner==owner&&migration_generation==generation);
        CHECK(hw_read_calls==before_reads&&writes==before_writes&&!memcmp(nor,baseline,sizeof nor));
        if(version==2&&len==1) {
            const uint8_t expected[]={0xf0,0x7d,0x46,0x4c,75,'D','8',2,1,NENGINES,
                expected_mask&127,(expected_mask>>7)&127,0,0xf7};
            CHECK(ed_n==sizeof expected&&!memcmp(ed_out,expected,sizeof expected));
        } else {
            const uint8_t expected[]={0xf0,0x7d,0x46,0x4c,76,1,75,version==2?3:2,0xf7};
            CHECK(ed_n==sizeof expected&&!memcmp(ed_out,expected,sizeof expected));
        }
    }
}
int main(void)
{
    fixture();begin_capture(1);CHECK(!status()&&ed_capture.active);
    query_matrix();
    op(2);CHECK(!status()&&ed_capture.active);query_matrix();
    op(6);CHECK(!ed_capture.active);
    uint32_t owner=0;CHECK(main_migration_begin(&owner)==D8MP_OK);
    memset(&main_workspace,0xa5,sizeof main_workspace);query_matrix();
    CHECK(migration_owner==owner&&main_migration_end(owner)==D8MP_OK);
    printf("engine availability during capture/owned arena: %u checks %u failures tracks=%u FM4=%u SLICE=%u; virtual NOR only; four-track capture not applicable\n",checks,failures,NTRK,FELUCCA_FM4,FELUCCA_SLICE);
    return failures!=0;
}
