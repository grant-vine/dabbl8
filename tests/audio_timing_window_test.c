/* SPDX-License-Identifier: GPL-3.0-only */
/* Literal independent windows; no hardware clock or USB qualification. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../firmware/src/audio_timing.h"
static void reset(void) { if(audio_timing.enabled)assert(audio_timing_stop());assert(audio_timing_start()); }
int main(void)
{
    struct d8_timing_snapshot a={UINT32_MAX-9,7,UINT32_MAX,UINT32_MAX-3,UINT32_MAX-4,0};
    struct d8_timing_snapshot b={20,7,0,2,3,0};
    reset();audio_timing_record_window(20,UINT32_MAX,&a,&b,0x50);
    assert(audio_timing.elapsed_lo==30 && !audio_timing.elapsed_hi);
    assert(audio_timing.run_late==1 && audio_timing.run_under==6 && audio_timing.run_over==8);
    assert(audio_timing.run_aux[0]==1 && !audio_timing.run_aux[1] && audio_timing.run_aux[2]==1);
    audio_timing_aux(0x20);a=b;a.ticks=31;b.ticks=50;b.late=1;b.under=3;b.over=4;
    audio_timing_record_window(10,0,&a,&b,0x10);
    assert(audio_timing.elapsed_lo==60 && audio_timing.first_ticks==UINT32_MAX-9 && audio_timing.last_ticks==50);
    assert(audio_timing.run_late==2 && audio_timing.run_under==7 && audio_timing.run_over==9);
    assert(audio_timing.run_aux[0]==2 && audio_timing.run_aux[1]==1 && audio_timing.run_aux[2]==1);
    audio_timing_aux(0x10);assert(audio_timing.run_aux[0]==2 && audio_timing.live_aux[0]==3);
    assert(audio_timing_stop());unsigned char paused[sizeof audio_timing];memcpy(paused,(const void*)&audio_timing,sizeof paused);
    audio_timing_aux(0x70);audio_timing_record_window(0,1,&a,&b,0x70);assert(!memcmp(paused,(const void*)&audio_timing,sizeof paused));
    reset();assert(!audio_timing.elapsed_lo && !audio_timing.taint && !audio_timing.window_started && !audio_timing.run_late);
    a=(struct d8_timing_snapshot){1,7,0,0,0,1};b=a;b.ticks=2;
    audio_timing_record_window(1,1,&a,&b,0);assert(audio_timing.taint==D8_TIMING_SNAPSHOT);
    reset();a.unstable=0;b.unstable=0;b.boots=8;audio_timing_record_window(1,1,&a,&b,0);assert(audio_timing.taint==D8_TIMING_BOOT);
    reset();b.boots=7;b.ticks=0x80000002u;audio_timing_record_window(1,1,&a,&b,0);assert(audio_timing.taint&D8_TIMING_JUMP);
    reset();a=(struct d8_timing_snapshot){1,7,0,17,0,0};b=a;b.ticks=2;b.under=0;
    audio_timing_record_window(1,1,&a,&b,0);assert(audio_timing.taint&D8_TIMING_JUMP);
    reset();a=(struct d8_timing_snapshot){1,7,0,0,0,0};b=a;b.ticks=2;
    audio_timing_record_window(1,1,&a,&b,0);audio_timing.run_under=UINT32_MAX;a=b;b.ticks=3;b.under=1;
    audio_timing_record_window(1,2,&a,&b,0);assert(audio_timing.saturated && !audio_timing.enabled && audio_timing.run_under==UINT32_MAX && audio_timing.taint&D8_TIMING_OVERFLOW);
    reset();a=(struct d8_timing_snapshot){1,7,0,0,0,0};b=a;b.ticks=2;audio_timing_record_window(1,1,&a,&b,0);
    audio_timing.elapsed_lo=UINT32_MAX-5;audio_timing.elapsed_hi=3;a=b;b.ticks=12;
    audio_timing_record_window(1,2,&a,&b,0);assert(audio_timing.elapsed_lo==4 && audio_timing.elapsed_hi==4 && !audio_timing.saturated);
    audio_timing.elapsed_lo=UINT32_MAX-5;audio_timing.elapsed_hi=UINT32_MAX;a=b;b.ticks=22;
    audio_timing_record_window(1,3,&a,&b,0);assert(audio_timing.saturated && audio_timing.taint&D8_TIMING_OVERFLOW && audio_timing.elapsed_hi==UINT32_MAX);
    puts("audio timing window: literal elapsed/counter wrap, endpoint/aux-only boundaries, pause/reset, snapshot/boot/reset-sized jumps and saturation PASS; simulated only");
}
