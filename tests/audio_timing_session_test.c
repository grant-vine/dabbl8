/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/audio_timing.h"
static void integrity(void)
{
    uint64_t a=0,b=0;
    for (unsigned i=0;i<D8_TIMING_BINS;i++) {
        a+=audio_timing.render.bins[i];b+=audio_timing.inclusive.bins[i];
    }
    assert(a==audio_timing.render.count && b==audio_timing.inclusive.count);
    assert(a==b);
}
int main(void)
{
    assert(sizeof audio_timing==1064);
    assert(!audio_timing_stop());
    audio_timing_record(1,2,1);
    assert(!audio_timing.render.count);
    assert(audio_timing_start() && audio_timing.generation==1);
    assert(!audio_timing_start());
    for (uint32_t i=0;i<131072;i++) audio_timing_record(i,i,UINT32_MAX-100+i);
    assert(audio_timing.first_half==UINT32_MAX-100);
    assert(audio_timing.last_half==(uint32_t)(UINT32_MAX-100+131071));
    assert((uint32_t)(audio_timing.last_half-audio_timing.first_half+1)==131072);
    assert(audio_timing.render.max_ticks==131071);integrity();
    audio_timing_record(5,4,0);
    assert(audio_timing.invalid==1 && audio_timing.render.count==131072);
    assert(audio_timing_stop());
    unsigned char snapshot[sizeof audio_timing];
    memcpy(snapshot,(const void*)&audio_timing,sizeof snapshot);
    audio_timing_record(100,200,5);
    assert(!memcmp(snapshot,(const void*)&audio_timing,sizeof snapshot));
    assert(!audio_timing_stop());
    assert(audio_timing_start() && audio_timing.generation==2);
    assert(!audio_timing.invalid && !audio_timing.render.count && !audio_timing.render.max_ticks);
    integrity();
    audio_timing.render.count=audio_timing.inclusive.count=UINT32_MAX-1;
    audio_timing.render.bins[0]=audio_timing.inclusive.bins[0]=UINT32_MAX-1;
    audio_timing_record(0,0,7);integrity();
    assert(!audio_timing.saturated && audio_timing.enabled);
    audio_timing_record(5,5,8);integrity();
    assert(audio_timing.saturated && !audio_timing.enabled);
    assert(audio_timing_start());integrity();
    audio_timing.invalid=UINT32_MAX;
    audio_timing_record(2,1,9);
    assert(audio_timing.saturated && !audio_timing.enabled && !audio_timing.render.count);
    audio_timing.generation=UINT32_MAX-1;
    assert(audio_timing_start() && audio_timing.generation==UINT32_MAX);
    assert(audio_timing_stop());
    assert(!audio_timing_start());
    puts("audio timing session: exhaustive bins, paired counts, start/reset/refusal, immutable pause, interval and generation wrap, invalid/loss saturation PASS; no hardware proof");
}
