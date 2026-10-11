/* SPDX-License-Identifier: GPL-3.0-only */
/* Exact frozen parent producer plus independent full-buffer occupancy oracles.
 * No device, physical FIFO, IRQ-off deadline or whole-stack qualification. */
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include "native_output_queue_test.c"
#define uac_tap48 uac_tap48_reference
#include "fixtures/uac/e631-uac-tap48.inc"
#undef uac_tap48

struct history_state {
    __typeof__(rs) resampler;
    __typeof__(uac) service;
    uint32_t ring[UA_N], write_index, read_index, occupied, history;
    uint8_t packet, uncertain;
};
static struct history_state initial, expected, actual;
static uint32_t history_seed = 0x12345678u;
static uint32_t history_random(void)
{
    history_seed ^= history_seed << 13;
    history_seed ^= history_seed >> 17;
    history_seed ^= history_seed << 5;
    return history_seed;
}
static void history_take(struct history_state *s)
{
    memset(s, 0, sizeof *s);
    memcpy(&s->resampler, &rs, sizeof rs);
    memcpy(&s->service, &uac, sizeof uac);
    memcpy(s->ring, ua_ring, sizeof ua_ring);
    s->write_index = ua_w; s->read_index = ua_r;
    s->occupied = uq.ring; s->history = uq.history;
    s->packet = uq.packet; s->uncertain = uq.uncertain;
}
static void history_restore(const struct history_state *s)
{
    memcpy(&rs, &s->resampler, sizeof rs);
    memcpy(&uac, &s->service, sizeof uac);
    memcpy(ua_ring, s->ring, sizeof ua_ring);
    ua_w = s->write_index; ua_r = s->read_index;
    uq.ring = s->occupied; uq.history = s->history;
    uq.packet = s->packet; uq.uncertain = s->uncertain;
}
static void history_case(unsigned mode, unsigned phase, unsigned length)
{
    int32_t input[128];
    memset(&rs, 0, sizeof rs); memset(&uq, 0, sizeof uq);
    memset(&uac, 0, sizeof uac);
    ua_w = UINT32_MAX - 100u; ua_r = ua_w - 8u;
    rs.ph = phase; rs.w = history_random();
    for (unsigned i = 0; i < RS_H; i++) {
        rs.l[i] = rs.l[i + RS_H] = mode ? 0 : (int16_t)history_random();
        rs.r[i] = rs.r[i + RS_H] = mode ? 0 : (int16_t)history_random();
    }
    if (mode == 2) {
        unsigned last = (rs.w - 1u) & (RS_H - 1u);
        rs.l[last] = rs.l[last + RS_H] = 1000;
    }
    for (unsigned i = 0; i < UA_N; i++) {
        /* Retained consumed slots are included, regardless of read index. */
        ua_ring[i] = i % 3u ? history_random() : 0;
        uq.ring += ua_ring[i] != 0;
    }
    for (unsigned i = 0; i < RS_H; i++)
        uq.history += rs.l[i] != 0 || rs.r[i] != 0;
    for (unsigned i = 0; i < 128; i++)
        input[i] = mode ? 0 : (int32_t)history_random();
    /* Producer must preserve consumer-owned state even when it is nonquiet. */
    uq.packet = phase & 1u; uq.uncertain = (phase >> 1) & 1u;
    uac.last = history_random(); uac.overruns = 7;
    history_take(&initial);
    uac_tap48_reference(input, length); oracle(); history_take(&expected);
    history_restore(&initial);
    uac_tap48(input, length); oracle(); history_take(&actual);
    CHECK(!memcmp(&actual, &expected, sizeof actual));
    CHECK(uq.packet == initial.packet && uq.uncertain == initial.uncertain);
#if FELUCCA_UAC_TONE
    CHECK(!uac_output_quiet());
#endif
}
int main(void)
{
    cold();
    unsigned cases = 0;
    for (unsigned mode = 0; mode < 3; mode++)
        for (unsigned phase = 0; phase < 160; phase++)
            for (unsigned length = 0; length <= 64; length++) {
                history_case(mode, phase, length); cases++;
            }
    /* Overrun is an unchanged early return, including history and packet state. */
    history_restore(&initial); ua_r = ua_w - UA_N;
    history_take(&initial); int32_t input[128] = {0};
    uac_tap48_reference(input, 64); history_take(&expected);
    history_restore(&initial); uac_tap48(input, 64); history_take(&actual);
    CHECK(!memcmp(&actual, &expected, sizeof actual));
    printf("Native SRC history%s: %u checks, %u failures; %u exact frozen-parent cases (160 phases x lengths 0..64 x 3 histories), independent full-buffer oracles; no physical timing claim\n",
           FELUCCA_UAC_TONE ? " TONE" : "", checks, failures, cases);
    return failures != 0;
}
