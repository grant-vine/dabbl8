/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Frozen unchanged uac_tap48 from firmware/src/usb.c at
 * e63164bc41c563fa02fe6d5bb4affecaca05adbd. Test-only rename is external.
 * Function SHA-256: 91ed5076e4e1e4d9f2dace26100a6cd27c64dda01638776c6070e36bfaca83a2
 */
static __attribute__((noinline)) void uac_tap48(const int32_t *out, uint32_t n)
{
    uint32_t w = ua_w, i, j, k = 0, h = rs.w, ph = rs.ph;
    if (UA_N - (w - ua_r) < n + n / 8u + 2u) {         /* nobody reads: drop the block (<= n * 160 / 147 + 1) */
        uac.overruns++;
        return;
    }
    for (i = 0; i < n; i++) {
        uint32_t s = h & (RS_H - 1u);
        int16_t l = (int16_t)uac_s16(out[2u * i]), r = (int16_t)uac_s16(out[2u * i + 1u]);
#if NTRK == 8
        uq.history += (uint32_t)(l!=0||r!=0)-(uint32_t)(rs.l[s]!=0||rs.r[s]!=0);
#endif
        rs.l[s] = rs.l[s + RS_H] = l;
        rs.r[s] = rs.r[s + RS_H] = r;
        h++;
        s = (h - UAC_FIR_N) & (RS_H - 1u);              /* the window: oldest .. this frame */
        do {
            const int16_t *x = &rs.l[s];                /* R: x[2 * RS_H] */
            int64_t al = 0, ar = 0;
            int32_t yl, yr;
            if (ph < UAC_FIR_L / 2u) {
                const int32_t *c = UAC_FIR[ph];
                for (j = UAC_FIR_N; j; j--, x++) {
                    int32_t t = *c++;
                    al += (int64_t)t * x[0];
                    ar += (int64_t)t * x[2u * RS_H];
                }
            } else {
                const int32_t *c = &UAC_FIR[UAC_FIR_L - 1u - ph][UAC_FIR_N - 1u];
                for (j = UAC_FIR_N; j; j--, x++) {
                    int32_t t = *c--;
                    al += (int64_t)t * x[0];
                    ar += (int64_t)t * x[2u * RS_H];
                }
            }
            yl = uac_s16((int32_t)((al + (1 << (UAC_FIR_Q - 1))) >> UAC_FIR_Q));
            yr = uac_s16((int32_t)((ar + (1 << (UAC_FIR_Q - 1))) >> UAC_FIR_Q));
#if FELUCCA_UAC_TONE
            {   /* bench: the triangles at 48 kHz (not resampled), one per output frame */
                uint32_t t = rs.tone, q = t < 50u ? t : 100u - t, q2 = t < 50u ? 50u - t : t - 50u;
                rs.tone = t == 99u ? 0u : t + 1u;
                yl = (int32_t)q * 1000 - 25000;
                yr = (int32_t)q2 * 1000 - 25000;
            }
#endif
            UA_STORE(&ua_ring[(w + k++) & (UA_N - 1u)],(uint32_t)(uint16_t)yl | (uint32_t)(uint16_t)yr << 16);
            ph += UAC_FIR_M;
        } while (ph < UAC_FIR_L);
        ph -= UAC_FIR_L;
    }
    rs.w = h;
    rs.ph = ph;
    RING_PUBLISH();
    ua_w = w + k;
}
