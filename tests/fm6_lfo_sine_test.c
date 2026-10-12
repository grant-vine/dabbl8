/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual FM6 LFO over every Q24 phase, including its negative
 * sine half-cycle, against a widened bounded arithmetic reference. */
#define main hostsim_main
#include "hostsim.c"
#undef main
int main(void)
{
    fm6_lfo_t l = {0}; l.wave = 4;
    uint64_t hash = UINT64_C(14695981039346656037);
    int32_t lowest = INT32_MAX, highest = INT32_MIN;
    for (uint32_t phase = 0; phase < (1u << 24); phase++) {
        l.phase = phase << 8;
        int32_t sine = fm6_sin((int32_t)phase);
        int64_t expected = (INT64_C(1) << 23) + (int64_t)sine * 256;
        int32_t actual = fm6_lfo_sample(&l);
        if (sine < INT16_MIN || sine > INT16_MAX || expected < 0 ||
            expected > (1 << 24) || actual != expected) {
            fprintf(stderr,"FAIL FM6 sine phase %u: %d vs %lld\n",phase,actual,(long long)expected);
            return 1;
        }
        if (actual < lowest) lowest = actual;
        if (actual > highest) highest = actual;
        hash ^= (uint32_t)actual; hash *= UINT64_C(1099511628211);
    }
    l.phase = UINT32_MAX - 127u; l.delta = 256u;
    int32_t wrapped = fm6_lfo_sample(&l);
    if (l.phase != 128u || wrapped != (1 << 23)) return 1;
    printf("PASS FM6 LFO 16777216 phases and unsigned accumulator wrap; range %d..%d; hash %016llx\n",
           lowest,highest,(unsigned long long)hash);
    return 0;
}
