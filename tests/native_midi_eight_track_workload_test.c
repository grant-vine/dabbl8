/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
#include <assert.h>
#include "native_midi_project_fixture.h"
static uint8_t wire[D8P1_LIMIT],before[D8P1_LIMIT],after[D8P1_LIMIT];
static const unsigned keys[8]={60,64,67,36,72,76,79,84};
static unsigned peak,pool_peak,blocks;
static void render(unsigned count)
{
    int32_t out[CTL*2];
    for(unsigned b=0;b<count;b++){
        mix_block(out,CTL);assert(voices_busy()<=8);unsigned owned=0;
        for(unsigned i=0;i<8;i++)if(heavy_owner[i]){
            unsigned k=heavy_owner[i]-1u;
            owned+=heavy_live(7u+k/(NPART*NVOICE),(k/NVOICE)%NPART,k%NVOICE);
        }
        assert(owned<=8);if(owned>pool_peak)pool_peak=owned;
        if(voices_busy()>peak)peak=voices_busy();blocks++;
    }
}
static void note(unsigned channel,unsigned key,unsigned velocity)
{
    unsigned status=(velocity?0x90u:0x80u)|channel;
    midi_in_event((status>>4)|(status<<8)|(key<<16)|(velocity<<24));
}
int main(int argc,char **argv)
{
    assert(argc==1||argc==3);ui_power_on();render(1);
    size_t size=native_midi_project_fixture(wire,sizeof wire,argc==3?argv[1]:NULL);
    assert(!d8p1_load_runtime(wire,size,7));render(1);
    assert(!song.playing&&!song.rec&&!song.g[G_ROUTE]);
    const unsigned engines[8]={0,12,3,10,0,0,0,0};
    for(unsigned p=0;p<8;p++)assert(trk[p].engine==engines[p]);
    size_t n_before=0,n_after=0;assert(!d8p1_capture_runtime(before,sizeof before,&n_before));
    peak=pool_peak=blocks=0;
    for(unsigned burst=0;burst<16;burst++){
        for(unsigned p=0;p<8;p++)note(p,keys[p],32);
        render((FS/8+CTL-1)/CTL);
        for(unsigned p=0;p<8;p++){
            assert(midi_sel_on[p][keys[p]]==p+1);note(p,keys[p],0);
        }
        render((FS*3/8+CTL-1)/CTL);
    }
    render((FS*10+CTL-1)/CTL);
    assert(peak==8&&pool_peak==2&&!voices_busy()&&!song.playing&&!song.rec&&mi_r==mi_w);
    for(unsigned channel=0;channel<16;channel++)for(unsigned key=0;key<128;key++)assert(!midi_sel_on[channel][key]);
    assert(!d8p1_capture_runtime(after,sizeof after,&n_after));
    assert(n_before==n_after&&!memcmp(before,after,n_before));
    if(argc==3){
    FILE *contract=fopen(argv[2],"w");assert(contract);
    assert(fputs("{\"track_dumps\":[",contract)>=0);
    for(unsigned p=0;p<8;p++){
        fprintf(contract,"%s[%u,%u,%u",p?",":"",p,trk[p].engine,trk[p].preset);
        for(unsigned i=0;i<P_COUNT;i++){
            unsigned v=(unsigned)(trk[p].p[i]+8192);fprintf(contract,",%u,%u",v&127u,(v>>7)&127u);
        }
        assert(fputs("]",contract)>=0);
    }
    assert(fputs("]}\n",contract)>=0);assert(!fclose(contract));
    }
    printf("PASS native USB channels1-8 notes=128 peak_voices=%u heavy_slots=%u blocks=%u no_holds=1 project_unchanged=1 recording=0\n",peak,pool_peak,blocks);
    return 0;
}
