/* SPDX-License-Identifier: GPL-3.0-only */
/* Historical PHYS migration remains usable without compiling an engine or DSP. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "phys_legacy.h"
#include "phys_legacy.h"          /* retained service is safe to include twice */
_Static_assert(PM_MODAL == 0 && PM_STRING == 1 && PM_MEMB == 2 && PM_SYMP == 3 && PM_COUNT == 4,
               "historical PHYS identities never follow a selected engine profile");
int main(void)
{
    unsigned checks = 0;
    for (int model = INT16_MIN; model <= INT16_MAX; model++) {
        int16_t actual[8] = {(int16_t)model, -32768, 32767, -123, 456, -7, 123, 789};
        int16_t expected[8]; memcpy(expected, actual, sizeof expected);
        /* Independent historical contract: DUST model 2 becomes MODAL 0, bow 64. */
        if (model == 2) { expected[0] = 0; expected[6] = 64; }
        phys_legacy(actual);
        checks++;
        if (memcmp(actual, expected, sizeof actual)) {
            fprintf(stderr, "historical migration changed unrelated parameters for model %d\n", model);
            return 1;
        }
        phys_legacy(actual); checks++;
        if (memcmp(actual, expected, sizeof actual)) return 1;
    }
    printf("standalone historical PHYS service: %u checks PASS\n", checks);
    return 0;
}
