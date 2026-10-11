/* SPDX-License-Identifier: GPL-3.0-only */
#define AUDIO_TEST_NO_MAIN 1
#include AUDIO_SOURCE
#include <assert.h>
static uint32_t victim_rng_state=0xDABB1801u;
static uint32_t random_word(void) { victim_rng_state^=victim_rng_state<<13;victim_rng_state^=victim_rng_state>>17;victim_rng_state^=victim_rng_state<<5;return victim_rng_state; }
static uint64_t trace=UINT64_C(14695981039346656037);
static void hash_word(uint32_t v) { for(unsigned k=0;k<4;k++){trace^=(v>>(8*k))&255u;trace*=UINT64_C(1099511628211);} }
int main(void) {
    fresh();
    for(unsigned c=0;c<65536;c++) {
        memset(trk,0,sizeof trk);
        for(unsigned p=0;p<NPART;p++) { trk[p].engine=(random_word()&1)?0:ENGI_DRUM;trk[p].p[P_VOICE]=random_word()%4; }
        unsigned n=random_word()%9;
        for(unsigned i=0;i<n;) {
            unsigned slot=random_word()%(NPART*NVOICE);voice_t *v=&trk[slot/NVOICE].v[slot%NVOICE];if(v->active)continue;
            v->active=1;v->gate=(uint8_t)(random_word()&1);v->stage=v->gate?2:(random_word()&1)?3:4;
            v->note=(uint8_t)(random_word()%128);v->env=(int32_t)(random_word()%32768);v->age=random_word();i++;
        }
        shed_over=(uint8_t)random_word();shed_count=0;voice_kills=0;
        shed_voice();hash_word(shed_count);hash_word(voice_kills);
        for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++) {
            voice_t *v=&trk[p].v[i];hash_word(v->active);hash_word(v->gate);hash_word(v->stage);hash_word((uint32_t)v->env);hash_word((uint32_t)v->env_out);hash_word(v->note);hash_word(v->age);
        }
    }
    printf("victim parity: cases=65536 tracks=%u seed=DABB1801 trace=%016llx\n",NPART,(unsigned long long)trace);
    return 0;
}
