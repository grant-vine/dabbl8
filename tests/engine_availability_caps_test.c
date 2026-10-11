/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual handler/USB parser; read-only host observation, no device qualification. */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
static unsigned proofs;
#define REQUIRE(x) do { proofs++; if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
#define STATE(X) __typeof__(X) X
static struct { STATE(trk); STATE(song); STATE(motion); STATE(chain); STATE(chain_config);
    STATE(undo); STATE(ui); STATE(ed_w); STATE(fm6_patch); STATE(fm6_slot); STATE(up_bank); STATE(upf); STATE(favorites);
    uint8_t panic,sync,depth; } before;
#define SNAP(X) memcpy(&before.X,&(X),sizeof(X))
#define SAME(X) REQUIRE(!memcmp(&before.X,&(X),sizeof(X)))
int main(void) {
    reset(); int32_t audio[2u*CTL];mix_block(audio,CTL);
    for(unsigned k=0;k<NTRK;k++){trk[k].p[P_VOICE]=V_MONO;trk_note_on(&trk[k],60+k,100);}
    REQUIRE(voices_busy()==NTRK);
    SNAP(trk);SNAP(song);SNAP(motion);SNAP(chain);SNAP(chain_config);SNAP(undo);SNAP(ui);SNAP(ed_w);
    SNAP(fm6_patch);SNAP(fm6_slot);SNAP(up_bank);SNAP(upf);SNAP(favorites);
    before.panic=panic_req;before.sync=sync_reload;before.depth=undo_depth;
    uint8_t a[3]={2,0,0};
    for(unsigned schema=0;schema<128;schema++) {
        a[0]=(uint8_t)schema;unsigned n=request(ED_D8_CAPS,a,1);
        if(schema==2) {
            const unsigned expected=(0x1ffdu|(FELUCCA_FM4?2u:0u)|(FELUCCA_SLICE?0x2000u:0u));
            REQUIRE(n==14 && host_wire[4]==75 && host_wire[5]==68 && host_wire[6]==56 && host_wire[7]==2 &&
                host_wire[8]==1 && host_wire[9]==NENGINES && (host_wire[10]|(host_wire[11]<<7))==expected && host_wire[12]==0);
        } else REQUIRE(n==9 && host_wire[4]==76 && host_wire[5]==1 && host_wire[6]==75 && host_wire[7]==2);
        SAME(trk);SAME(song);SAME(motion);SAME(chain);SAME(chain_config);SAME(undo);SAME(ui);SAME(ed_w);
        SAME(fm6_patch);SAME(fm6_slot);SAME(up_bank);SAME(upf);SAME(favorites);
        REQUIRE(panic_req==before.panic && sync_reload==before.sync && undo_depth==before.depth && !host_writes && !host_erases);
    }
    a[0]=2;
    for(unsigned len=2;len<=3;len++) {
        unsigned n=request(ED_D8_CAPS,a,len);REQUIRE(n==9 && host_wire[4]==76 && host_wire[7]==3);
        SAME(trk);SAME(song);SAME(motion);SAME(chain);SAME(chain_config);SAME(undo);SAME(ui);SAME(ed_w);
        REQUIRE(!host_writes && !host_erases);
    }
    const uint8_t original[]={68,56,1,NTRK,64,8,64,99,27,NENGINES,1,1,1,9,1|(NTRK<=4?2:4)};
    unsigned n=request(ED_D8_CAPS,a,0);REQUIRE(n==21 && !memcmp(host_wire+5,original,sizeof original));
    REQUIRE(!host_writes && !host_erases);
    printf("engine availability: tracks=%u FM4=%u SLICE=%u %u assertions PASS; host only\n",NTRK,FELUCCA_FM4,FELUCCA_SLICE,proofs);
    return 0;
}
