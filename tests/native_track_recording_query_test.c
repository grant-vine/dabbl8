/* SPDX-License-Identifier: GPL-3.0-only */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
#include <assert.h>
int main(void)
{
    assert(NPART==8);reset();song.sel=6;
    for(unsigned mask=0;mask<256;mask++){
        song.rec=(uint8_t)mask;
        unsigned writes=host_writes,erases=host_erases;
        assert(request(ED_TRACK,NULL,0)==56);
        assert(host_wire[0]==0xf0&&host_wire[4]==27&&host_wire[55]==0xf7);
        assert(host_wire[5]==6&&host_wire[6]==8);
        for(unsigned track=0;track<8;track++)assert(host_wire[7+track*6+5]==((mask>>track)&1u));
        assert(song.sel==6&&song.rec==mask&&host_writes==writes&&host_erases==erases);
    }
    puts("PASS actual read-only TRACK query: all256 recording masks, exact50-byte payload, no selection/data writes");
    return 0;
}
