/* SPDX-License-Identifier: GPL-3.0-only */
/* Schema-2 observation during actual simulated capture/arena ownership. */
#define D8_INSTRUMENT_CAPTURE_NO_MAIN 1
#include "instrument_capture_test.c"
int main(void) {
    fixture();begin_capture(1);CHECK(!status() && ed_capture.active);
    __typeof__(ed_capture) retained=ed_capture;
    uint32_t arena=d8p1_crc32(&main_workspace,sizeof main_workspace), epoch=d8_capture_change.epoch;
    for(unsigned version=0;version<128;version++) {
        uint8_t frame[]={0x7d,0x46,0x4c,75,(uint8_t)version};
        ed_handle(frame,sizeof frame);so_r=so_w;
        CHECK(!memcmp(&retained,&ed_capture,sizeof retained));
        CHECK(arena==d8p1_crc32(&main_workspace,sizeof main_workspace));
        CHECK(epoch==d8_capture_change.epoch && !writes && !memcmp(nor,baseline,sizeof nor));
        CHECK(version==2?(ed_n==14&&ed_out[4]==75):(ed_n==9&&ed_out[4]==76&&ed_out[7]==2));
    }
    uint8_t malformed[]={0x7d,0x46,0x4c,75,2,0};ed_handle(malformed,sizeof malformed);so_r=so_w;
    CHECK(ed_n==9&&ed_out[7]==3&&!memcmp(&retained,&ed_capture,sizeof retained));
    op(6);uint32_t owner=0;CHECK(main_migration_begin(&owner)==D8MP_OK);memset(&main_workspace,0xa5,sizeof main_workspace);
    arena=d8p1_crc32(&main_workspace,sizeof main_workspace);
    for(unsigned version=0;version<128;version++) {
        uint8_t frame[]={0x7d,0x46,0x4c,75,(uint8_t)version};ed_handle(frame,sizeof frame);so_r=so_w;
        CHECK(migration_owner==owner&&arena==d8p1_crc32(&main_workspace,sizeof main_workspace));
        CHECK(!writes&&!memcmp(nor,baseline,sizeof nor));
    }
    CHECK(main_migration_end(owner)==D8MP_OK);
    printf("engine availability during capture/owned arena: %u checks %u failures; virtual NOR only\n",checks,failures);
    return failures!=0;
}
