/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "fm1_stack_watermark.h"

static int failures;
static void check(const char *scenario, int passed)
{
    printf("stack watermark: %s %s\n", scenario, passed ? "PASS" : "FAIL");
    failures += !passed;
}

#if D8_STACK_WATERMARK_DIAGNOSTIC
static void paint(uint32_t *words, size_t count)
{
    for (size_t i = 0; i < count; i++) words[i] = FM1_STACK_WATERMARK_PATTERN;
}
static fm1_stack_window_t window(uint32_t *words, size_t count)
{
    fm1_stack_window_t result = {(uintptr_t)words, (uintptr_t)(words + count)};
    return result;
}
static int observed(fm1_stack_watermark_t result, uint32_t capacity, uint32_t depth)
{
    return result.status == FM1_STACK_WATERMARK_OBSERVED &&
           result.capacity_bytes == capacity && result.observed_bytes == depth &&
           result.untouched_bytes == capacity - depth;
}
static int invalid(fm1_stack_watermarks_t result)
{
    return result.enabled == 1 && result.user.status == FM1_STACK_WATERMARK_INVALID &&
           result.supervisor.status == FM1_STACK_WATERMARK_INVALID &&
           !result.user.capacity_bytes && !result.supervisor.capacity_bytes &&
           !result.user.observed_bytes && !result.supervisor.observed_bytes;
}
static void pair_scenarios(void)
{
    /* Given two disjoint downward stacks surrounded by sentinel guard words. */
    uint32_t user[18], supervisor[10], before_user[18], before_supervisor[10];
    memset(user, 0x6b, sizeof user); memset(supervisor, 0x6b, sizeof supervisor);
    paint(user + 1, 16); paint(supervisor + 1, 8);
    fm1_stack_window_t u = window(user + 1, 16), s = window(supervisor + 1, 8);
    /* When reading stacks with no surviving non-pattern writes. */
    fm1_stack_watermarks_t result = fm1_stack_watermark_read_windows(u, s);
    /* Then both windows report independently, including their boundaries. */
    check("unwritten user and supervisor windows", result.enabled &&
          observed(result.user, 64, 0) && observed(result.supervisor, 32, 0) &&
          result.user.low == u.low && result.user.top == u.top &&
          result.supervisor.low == s.low && result.supervisor.top == s.top);

    /* Given different observed depths and unchanged guards. */
    user[13] = 1; supervisor[5] = 2;
    memcpy(before_user, user, sizeof user); memcpy(before_supervisor, supervisor, sizeof supervisor);
    /* When reading the two windows. */
    result = fm1_stack_watermark_read_windows(u, s);
    /* Then low-to-high depth is independent and scanning writes nothing. */
    check("two independent depths with untouched guard bands", observed(result.user, 64, 16) &&
          observed(result.supervisor, 32, 16) &&
          !memcmp(before_user, user, sizeof user) && !memcmp(before_supervisor, supervisor, sizeof supervisor));

    /* Given the lowest word overwritten in each window. */
    user[1] = 1; supervisor[1] = 2;
    /* When scanning the complete depth. */
    result = fm1_stack_watermark_read_windows(u, s);
    /* Then a saturated observed watermark equals each capacity. */
    check("lowest overwritten words report full depth", observed(result.user, 64, 64) &&
          observed(result.supervisor, 32, 32));

    /* Given a deeper write equal to the pattern and a later visible write. */
    paint(user + 1, 16); user[5] = FM1_STACK_WATERMARK_PATTERN; user[9] = 7;
    /* When scanning surviving contents. */
    result = fm1_stack_watermark_read_windows(u, s);
    /* Then the known collision is deliberately not claimed as observed use. */
    check("pattern collisions can understate written depth", observed(result.user, 64, 32));
}

static void protected_scenarios(void)
{
    /* Given two readable pages separated and surrounded by inaccessible pages. */
    long page_query = sysconf(_SC_PAGESIZE);
    if (page_query <= 0 || (unsigned long)page_query > FM1_STACK_WATERMARK_MAX_BYTES) exit(2);
    size_t page = (size_t)page_query;
    unsigned char *mapping = mmap(NULL, page * 5, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED) exit(2);
    uint32_t *user = (uint32_t *)(mapping + page), *supervisor = (uint32_t *)(mapping + page * 3);
    if (mprotect(user, page, PROT_READ | PROT_WRITE) ||
        mprotect(supervisor, page, PROT_READ | PROT_WRITE)) exit(2);
    paint(user, page / 4); paint(supervisor, page / 4);
    if (mprotect(user, page, PROT_READ) || mprotect(supervisor, page, PROT_READ)) exit(2);
    fm1_stack_window_t u = window(user, page / 4), s = window(supervisor, page / 4);
    /* When scanning all words on pages which cannot be written. */
    fm1_stack_watermarks_t result = fm1_stack_watermark_read_windows(u, s);
    /* Then success proves the scan did not write or read a guard page. */
    check("read-only pages and inaccessible bounds", observed(result.user, (uint32_t)page, 0) &&
          observed(result.supervisor, (uint32_t)page, 0));

    /* Given no accessible stack memory; invalid pairs must fail before any read. */
    if (mprotect(user, page, PROT_NONE) || mprotect(supervisor, page, PROT_NONE)) exit(2);
    fm1_stack_window_t bad[] = {{0, 4}, {u.low, u.low}, {u.top, u.low},
        {u.low + 1, u.top}, {u.low, u.top - 1},
        {u.low, u.low + FM1_STACK_WATERMARK_MAX_BYTES + 4u},
        {u.low, u.low + 4}, {s.low, s.top}};
    const char *names[] = {"null bounds reject without reads", "zero-length bounds reject without reads",
        "reversed bounds reject without reads", "misaligned low rejects without reads",
        "misaligned top rejects without reads", "oversize bounds reject without reads",
        "overlapping windows reject without reads", "equal windows reject without reads"};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        /* When validating the pair (including an otherwise valid user window). */
        result = i < 6 ? fm1_stack_watermark_read_windows(u, bad[i]) :
                        fm1_stack_watermark_read_windows(i == 6 ? u : s, bad[i]);
        /* Then both results are invalid and preserve the supplied boundaries. */
        check(names[i], invalid(result) && result.supervisor.low == bad[i].low &&
              result.supervisor.top == bad[i].top);
    }
    if (munmap(mapping, page * 5)) exit(2);
}
#endif

int main(void)
{
#if D8_STACK_WATERMARK_DIAGNOSTIC
    pair_scenarios(); protected_scenarios();
#else
    /* Given disabled diagnostics and deliberately unreadable bounds. */
    fm1_stack_window_t user = {1, 5}, supervisor = {9, 13};
    /* When requesting a readout without startup coloring. */
    fm1_stack_watermarks_t result = fm1_stack_watermark_read_windows(user, supervisor);
    /* Then disabled results are unavailable and no memory is read. */
    check("disabled scan unavailable without reads", !result.enabled &&
          result.user.status == FM1_STACK_WATERMARK_UNAVAILABLE &&
          result.supervisor.status == FM1_STACK_WATERMARK_UNAVAILABLE &&
          !result.user.observed_bytes && !result.supervisor.observed_bytes);
    /* Given no linker definitions in this host executable, when reading the
     * application wrapper, then disabled code still links and is unavailable. */
    result = fm1_stack_watermark_read();
    check("disabled application wrapper needs no linker symbols", !result.enabled &&
          result.user.status == FM1_STACK_WATERMARK_UNAVAILABLE &&
          result.supervisor.status == FM1_STACK_WATERMARK_UNAVAILABLE);
#endif
    printf("stack watermark result: %d failures\n", failures);
    return failures != 0;
}
