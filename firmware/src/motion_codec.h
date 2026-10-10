/* SPDX-License-Identifier: GPL-3.0-only */
/* Included after motion_param. Disk bytes are explicit little-endian, never C structs. */
#define MOTION_V1_HEADER 8u
#define MOTION_V1_MAX (MOTION_V1_HEADER + MOTION_MAX * 5u)
static int motion_legacy_decode(motion_store_t *out, const uint8_t *b)
{
    motion_store_t m; memset(&m, 0, sizeof m);
    if (b[0] > MOTION_MAX || (b[1] & ~15u)) return 0;
    m.count = b[0]; m.on = b[1]; m.rsv[0] = b[2]; m.rsv[1] = b[3];
    for (uint32_t i = 0; i < MOTION_MAX; i++) {
        const uint8_t *e = b + 4u + i * 4u;
        int32_t v = e[2] | (uint32_t)e[3] << 8;
        if (v >= 32768) v -= 65536;
        if (i < m.count && (v < -64 || v > 127)) return 0;
        m.event[i].place = e[0]; m.event[i].param = e[1]; m.event[i].value = (int16_t)v;
    }
    *out = m; return 1;
}
static int motion_legacy_encode(uint8_t *b, const motion_store_t *m)
{
    if (m->count > MOTION_MAX || (m->on & ~15u)) return 0;
    for (uint32_t i = 0; i < MOTION_MAX; i++)
        if (m->event[i].place > 255u || (i < m->count && (m->event[i].value < -64 || m->event[i].value > 127))) return 0;
    b[0] = m->count; b[1] = m->on; b[2] = m->rsv[0]; b[3] = m->rsv[1];
    for (uint32_t i = 0; i < MOTION_MAX; i++) {
        const motion_event_t *e = &m->event[i]; uint8_t *p = b + 4u + i * 4u;
        p[0] = (uint8_t)e->place; p[1] = e->param;
        p[2] = (uint8_t)e->value; p[3] = (uint8_t)((uint16_t)e->value >> 8);
    }
    return 1;
}
/* D8M1: magic, count, enabled mask, reserved zero x2, then count records:
 * explicit track, step, parameter (bit7 lock), signed LE16 value. Not FUN9. */
static int motion_v1_valid(const motion_store_t *m)
{
    if (m->count > MOTION_MAX || m->rsv[0] || m->rsv[1]) return 0;
    for (uint32_t i = 0; i < m->count; i++) {
        const motion_event_t *e = &m->event[i];
        if (e->place >= 512u || !motion_param(MOTION_ID(e)) || e->value < -64 || e->value > 127) return 0;
        for (uint32_t j = 0; j < i; j++)
            if (m->event[j].place == e->place && MOTION_ID(&m->event[j]) == MOTION_ID(e)) return 0;
    }
    return 1;
}
static uint32_t motion_v1_encode(uint8_t *out, uint32_t cap, const motion_store_t *m)
{
    uint32_t n = MOTION_V1_HEADER + m->count * 5u;
    if (!motion_v1_valid(m) || cap < n) return 0;
    memcpy(out, "D8M1", 4); out[4] = m->count; out[5] = m->on; out[6] = out[7] = 0;
    for (uint32_t i = 0; i < m->count; i++) {
        const motion_event_t *e = &m->event[i]; uint8_t *p = out + 8u + i * 5u;
        p[0] = (uint8_t)(e->place >> 6); p[1] = e->place & 63u; p[2] = e->param;
        p[3] = (uint8_t)e->value; p[4] = (uint8_t)((uint16_t)e->value >> 8);
    }
    return n;
}
static int motion_v1_decode(motion_store_t *out, const uint8_t *b, uint32_t n)
{
    motion_store_t m; memset(&m, 0, sizeof m);
    if (n < 8u || memcmp(b, "D8M1", 4) || b[4] > MOTION_MAX || b[6] || b[7] || n != 8u + b[4] * 5u) return 0;
    m.count = b[4]; m.on = b[5];
    for (uint32_t i = 0; i < m.count; i++) {
        const uint8_t *p = b + 8u + i * 5u; int32_t v = p[3] | (uint32_t)p[4] << 8;
        if (p[0] >= 8u || p[1] >= 64u) return 0;
        if (v >= 32768) v -= 65536;
        m.event[i].place = (uint16_t)(p[0] << 6 | p[1]); m.event[i].param = p[2]; m.event[i].value = (int16_t)v;
    }
    if (!motion_v1_valid(&m)) return 0;
    *out = m; return 1;
}
