/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* The editor's full backup (editor_backup.c: LIST / GET / PUT) against simulated NOR flash:
 * CRC before any write, stale runtime copies, USB resets and timeouts, malformed objects, older
 * project formats, settings values and user preset banks kept byte for byte. */
static unsigned char host_samples[3][0x14000];
#define SMP_USER_XIP(k) host_samples[k]
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static void panel_setup(void) {}

static uint8_t nor[0x100000], flash_ok = 1;
static int erase_error;
static uint32_t erases, programs;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { if (erase_error) return -8; memset(nor + off, 0xFF, 4096); erases++; return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { programs++; memcpy(nor + off, src, n); return 0; }
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"
#include "legacy_wire_fixture.h"

enum { ED_BACKUP_LIST = 65, ED_BACKUP_GET, ED_BACKUP_PUT };
static uint8_t rep[4096];
static uint32_t rep_n;
static void ed_b(uint32_t v) { if (rep_n < sizeof rep) rep[rep_n++] = (uint8_t)(v & 127u); }
static uint32_t ed_unpack7(const uint8_t *a, uint32_t na, uint8_t *out, uint32_t max)
{
    uint32_t n = 0;
    while (na && n < max) {
        uint32_t m = *a++, j;
        na--;
        uint32_t k = na > 7u ? 7u : na;
        if (!k || n + k > max || (m >> k))
            return 0;
        for (j = 0; j < k; j++, na--)
            out[n++] = (uint8_t)(*a++ | ((m >> j) & 1u) << 7);
    }
    return na ? 0u : n;
}
static uint8_t ed_smp_buf[512] __attribute__((aligned(4)));
static void fm1_wdt_feed(void) {}
static int ed_flash_stop(void) { return transport_busy(); }
#include "../firmware/src/editor_backup.c"

static int check(const char *what, int ok)
{
    printf("backup: %-72s %s\n", what, ok ? "ok" : "FAIL");
    return !ok;
}

static void reset(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(up_bank, 0, sizeof up_bank);
    memset(&persist_saved, 0, sizeof persist_saved);
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    panel = PANEL_DEFAULT;
    settings_init();
    host_tracks_init();
    fm1_ms = 0;
    transport_req = 0;
    usb.resets = 0;
    erase_error = 0;
    erases = programs = 0;
    ed_bk_valid = ed_bk_put = 0;
}

static uint32_t call(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    rep_n = 0;
    ed_backup_handle(cmd, a, n);
    return rep_n;
}
static void put32(uint8_t *a, uint32_t v) { for (uint32_t i = 0; i < 5u; i++) a[i] = (uint8_t)((v >> (7u * i)) & 127u); }

/* PUT begin: rc */
static uint32_t put_begin(uint32_t id, uint32_t len, uint32_t crc)
{
    uint8_t a[12] = {0, (uint8_t)id};
    put32(a + 2, len);
    put32(a + 7, crc);
    call(ED_BACKUP_PUT, a, sizeof a);
    return rep[2];
}
/* PUT one chunk of data at off: rc */
static uint32_t put_chunk(uint32_t id, uint32_t off, const uint8_t *p, uint32_t n)
{
    uint8_t a[16 + 300];
    uint32_t k = 7;
    a[0] = 1;
    a[1] = (uint8_t)id;
    put32(a + 2, off);
    while (n) {
        uint32_t g = n > 7u ? 7u : n, m = 0, i;
        for (i = 0; i < g; i++) m |= (uint32_t)(p[i] >> 7) << i;
        a[k++] = (uint8_t)m;
        for (i = 0; i < g; i++) a[k++] = p[i] & 127u;
        p += g;
        n -= g;
    }
    call(ED_BACKUP_PUT, a, k);
    return rep[2];
}
static uint32_t put_end(uint32_t id, uint32_t op)   /* op 2 commit, 3 abort */
{
    uint8_t a[2] = {(uint8_t)op, (uint8_t)id};
    call(ED_BACKUP_PUT, a, 2);
    return rep[2];
}
/* a whole object: begin, 256-byte chunks, commit; the first failing rc or the commit's */
static uint32_t put_all(uint32_t id, const void *v, uint32_t len, uint32_t crc)
{
    const uint8_t *p = v;
    uint32_t off, rc = put_begin(id, len, crc);
    for (off = 0; !rc && off < len; off += 256u)
        rc = put_chunk(id, off, p + off, len - off > 256u ? 256u : len - off);
    return rc ? rc : put_end(id, 2);
}
/* LIST: rc; the length and CRC of object id */
static uint32_t list(uint32_t id, uint32_t *len, uint32_t *crc)
{
    uint32_t i;
    call(ED_BACKUP_LIST, 0, 0);
    if (rep[1])
        return rep[1];
    for (i = 0; i < rep[2]; i++) {
        const uint8_t *o = rep + 3 + i * 11u;
        if (o[0] == id) {
            *len = ed_bk_r32(o + 1);
            *crc = ed_bk_r32(o + 6);
        }
    }
    return 0;
}
static uint32_t get(uint32_t id, uint32_t off, uint32_t count)
{
    uint8_t a[8] = {(uint8_t)id};
    put32(a + 1, off);
    a[6] = (uint8_t)(count & 127u);
    a[7] = (uint8_t)(count >> 7);
    call(ED_BACKUP_GET, a, sizeof a);
    return rep[1];
}

static int legacy_backup_four(void)
{
    int bad = 0;
    uint32_t len = 0, crc = 0, before;
    static project_store_t st;
    static project_v6_t v6;
    persist_t ps;

    reset();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 96, 0, 0};
    bad += check("LIST captures the runtime: 13 objects (id 8 empty, id 9 the FM6 patches), runtime 3648 B (FUN9)",
                 list(0, &len, &crc) == 0 && rep[2] == 13u && len == sizeof(project_store_t) && len == 3648u &&
                 crc == st_crc32(ED_BK_RAW, len));
    bad += check("an empty project slot lists as length 0", list(2, &len, &crc) == 0 && len == 0);
    bad += check("GET of the runtime copy", get(0, 0, 64) == 0);
    project_save(0);
    bad += check("a project save after LIST reused the staging RAM: GET of the runtime is stale (5)",
                 get(0, 0, 64) == 5u);
    bad += check("a new LIST makes it readable again", list(0, &len, &crc) == 0 && get(0, 0, 64) == 0);
    usb.resets++;
    bad += check("a USB reset after LIST: GET is stale (5)", get(2, 0, 16) == 5u);

    /* settings */
    reset();
    memset(&ps, 0, sizeof ps);
    settings_export(&ps);
    ps.lowcut = 2;                                       /* SPEAKER BASS+ */
    ps.palette = palette_to_stored(6);
    bad += check("settings with SPEAKER BASS+ restore", put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 &&
                 settings.lowcut == 2u && fx_lowcut == 2u && settings.palette == 6u);
    ps.palette = 13;                                     /* an older backup: its PAPER (old id 13) */
    bad += check("settings of an older backup restore with the palette migrated",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && settings.palette == 6u);
    ps.palette = palette_to_stored(6);
    before = erases;
    ps.lowcut = 3;
    bad += check("settings with an unknown SPEAKER value are refused, nothing written",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 2u && erases == before && settings.lowcut == 2u);
    ps.lowcut = 0;
    ps.bold = hold_to_stored(0, 3);                      /* HOLD 0.6 s */
    bad += check("settings with HOLD 0.6 s restore",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && HOLD_MS[settings_hold] == 600u);
    ps.bold = 1;                                         /* an older backup (its font weight): HOLD 0.4 s */
    bad += check("settings of an older backup restore with HOLD 0.4 s",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && HOLD_MS[settings_hold] == 400u);
    before = erases;
    ps.bold = HOLD_TAG + 7u;
    bad += check("settings with an unknown HOLD value are refused, nothing written",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 2u && erases == before);
    ps.bold = 0;
    ps.zoom = LEDS_TAG | LEDS_INV;                       /* LEDS INV */
    bad += check("settings with LEDS INV restore",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && settings_leds == LEDS_INV);
    ps.zoom = 1;                                         /* an older backup (its large readout): LEDS DIM */
    bad += check("settings of an older backup restore with LEDS DIM",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && settings_leds == LEDS_DIM);
    before = erases;
    ps.zoom = LEDS_TAG | LEDS_DIM_LO;                    /* LEDS DIM LO, OFF (appended) */
    bad += check("settings with LEDS DIM LO restore",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && settings_leds == LEDS_DIM_LO);
    ps.zoom = LEDS_TAG | LEDS_OFF;
    bad += check("settings with LEDS OFF restore",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 0 && settings_leds == LEDS_OFF);
    before = erases;
    ps.zoom = LEDS_TAG + 4u;
    bad += check("settings with an unknown LEDS value are refused, nothing written",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps)) == 2u && erases == before);
    ps.zoom = 0;
    bad += check("a wrong CRC is refused before any write",
                 put_all(1, &ps, sizeof ps, st_crc32(&ps, sizeof ps) ^ 1u) == 2u && erases == before);

    /* malformed requests */
    bad += check("an unknown object id is refused", put_begin(8, sizeof ps, 0) == 1u);
    bad += check("a project of the wrong length is refused", put_begin(2, sizeof(project_store_t) - 4u, 0) == 1u);
    bad += check("settings of the wrong length are refused", put_begin(1, sizeof ps + 4u, 0) == 1u);
    bad += check("a chunk without a begin is refused (5)", (ed_bk_put = 0, put_chunk(1, 0, (const uint8_t *)&ps, 16)) == 5u);
    put_begin(1, sizeof ps, st_crc32(&ps, sizeof ps));
    bad += check("a chunk at the wrong offset is refused", put_chunk(1, 16, (const uint8_t *)&ps, 16) == 1u);
    bad += check("a chunk for another object is refused (5)", put_chunk(2, 0, (const uint8_t *)&ps, 16) == 5u);
    {
        uint32_t off = 0, step = sizeof ps - 8u;
        put_begin(1, sizeof ps, st_crc32(&ps, sizeof ps));
        for (; off + 256u <= step; off += 256u)
            put_chunk(1, off, (const uint8_t *)&ps + off, 256);
        if (off < step) put_chunk(1, off, (const uint8_t *)&ps + off, step - off);
        bad += check("a chunk past the length is refused", put_chunk(1, step, (const uint8_t *)&ps, 16) == 1u);
    }
    bad += check("commit before all bytes arrived is refused", put_end(1, 2) == 2u && erases == before);
    put_begin(1, sizeof ps, st_crc32(&ps, sizeof ps));
    put_chunk(1, 0, (const uint8_t *)&ps, 64);
    usb.resets++;
    bad += check("a USB reset during PUT ends it (5)", put_chunk(1, 64, (const uint8_t *)&ps + 64, 64) == 5u);
    put_begin(1, sizeof ps, st_crc32(&ps, sizeof ps));
    fm1_ms += 16000u;
    bad += check("16 s without a chunk ends the PUT (5)", put_chunk(1, 0, (const uint8_t *)&ps, 64) == 5u);
    put_begin(2, sizeof st, 0);
    put_chunk(2, 0, (const uint8_t *)&ps, 64);
    project_save(1);
    bad += check("a project save during PUT (the staging RAM) ends it (5)",
                 put_chunk(2, 64, (const uint8_t *)&ps, 64) == 5u);
    transport_req = 1;
    bad += check("PUT while starting PLAY is refused (3)", put_begin(1, sizeof ps, 0) == 3u);
    transport_req = 0;

    /* projects */
    reset();
    trk[1].step[3] = (step_t){{64}, 1, ST_NOTE, 0, 96, 0, 0};
    project_capture(&proj_scratch);
    proj_scratch.parts=PROJ_LEGACY_NTRK;
    fixture_pack(&st, &proj_scratch);
    bad += check("a FUN9 project restores into slot 3 (flash and RAM)",
                 put_all(4, &st, sizeof st, st_crc32(&st, sizeof st)) == 0 && project_used(2) &&
                 !memcmp(&proj_slot[2], &st, sizeof st) && st_load(OBJ_PROJECT0 + 2, &proj_wire, sizeof proj_wire) == (int)sizeof st);
    {   /* FUN8 of 1.0.x (3584 bytes, 91 parameters, the patches at 3056): restores as FUN9, E0 at P_E0, lanes 100 % */
        static uint8_t v8[3584];
        uint32_t pos = 68u, pos9 = 68u, k, j, sum;
        trk[2].p[P_E0] = 2;
        project_capture(&proj_scratch);
        proj_scratch.parts=PROJ_LEGACY_NTRK;
    fixture_pack(&st, &proj_scratch);
        memcpy(v8, st.raw, 68);
        v8[66] = 91;
        for (k = 0; k < PROJ_LEGACY_NTRK; k++) {
            for (j = 0; j < 91u; j++) v8[pos++] = st.raw[pos9 + (j < 83u ? j : j + 8u)];
            pos9 += P_COUNT;
            memcpy(v8 + pos, st.raw + pos9, 2u + NSTEP * 9u);
            pos += 2u + NSTEP * 9u; pos9 += 2u + NSTEP * 9u;
        }
        memcpy(v8 + pos, st.raw + pos9, sizeof(chain_config_t) + sizeof(motion_legacy_store_t));
        memcpy(v8 + 3056u, st.raw + PROJ_FM6_OFF, PROJ_LEGACY_NTRK * FM6_PACKED);
        ((uint32_t *)v8)[0] = 0x46554E38u;
        ((uint32_t *)v8)[1] = 3584u;
        sum = proj_hash(v8, 3580u);
        memcpy(v8 + 3580u, &sum, 4);
        bad += check("a FUN8 project (1.0.x) restores as FUN9 into slot 2, E0 at P_E0, lane levels 100 %",
                     put_all(3, v8, sizeof v8, st_crc32(v8, sizeof v8)) == 0 && ((uint32_t *)proj_slot[1].raw)[0] == PROJ_MAGIC &&
                     proj_import(&proj_scratch, &proj_slot[1], sizeof st) && proj_scratch.t[2].p[P_E0] == 2 &&
                     proj_scratch.t[2].p[P_LN3] == 127 && proj_scratch.t[1].step[3].note[0] == 64u);
    }
    memset(&v6, 0, sizeof v6);                           /* FUN6: 69 parameters, steps out of range */
    v6.magic = PROJ_MAGIC_V6;
    v6.size = sizeof v6;
    for (uint32_t i = 0; i < G_COUNT; i++)
        v6.g[i] = GP[i].def;
    for (uint32_t i = 0; i < PROJ_LEGACY_NTRK; i++)
        v6.t[i].engine = trk[i].engine;
    v6.t[0].step[0] = (step10_t){{255, 72}, 9, 7, 0, 96, 0, 0};
    v6.t[0].p[61] = 3;                                   /* old E0 */
    chain_defaults(&v6.chain);
    v6.sum = proj_hash(&v6, sizeof v6 - 4u);
    bad += check("a FUN6 project restores as FUN9, bounded, its E0 at P_E0",
                 put_all(5, &v6, sizeof v6, st_crc32(&v6, sizeof v6)) == 0 && ((uint32_t *)proj_slot[3].raw)[0] == PROJ_MAGIC &&
                 proj_import(&proj_scratch, &proj_slot[3], sizeof st) && proj_scratch.t[0].step[0].n == 4u &&
                 proj_scratch.t[0].step[0].note[0] == 127u && proj_scratch.t[0].p[P_E0] ==
                 clamp(3, param_desc_of(trk[0].engine, P_E0)->min, param_desc_of(trk[0].engine, P_E0)->max));
    {   /* 1.0.3: the FM6 patch bank (id 8) is retired; an older archive's bank moves into its user presets (ids 6, 7
         * restored first), the user presets' patches are id 9 */
        static up_bank_t ub;
        static fm6_bank_t bk;
        static upf_t got;
        uint8_t pk[FM6_PACKED];
        static const int16_t SL[5] = {FM6_NFACTORY + 2, 1, FM6_NFACTORY + 4, FM6_NFACTORY + 2, FM6_NFACTORY + 2};
        memset(&ub, 0, sizeof ub);
        ub.magic = UP_BANK_MAGIC; ub.rsize = sizeof(up_rec_t); ub.nslot = UP_PER_BANK;
        for (uint32_t k = 0; k < 5u; k++) {             /* slots 1..5: B3, F2, B5 (empty), B3 on DRUM, B3 */
            up_rec_t *r = &ub.r[k];
            uint32_t e = k == 3u ? ENGI_DRUM : ENGI_FM6;
            r->used = UP_USED; r->ver = UP_VER; r->engine = (uint8_t)e; r->np = P_COUNT;
            r->name[0] = (char)('A' + k);
            for (uint32_t i = 0; i < P_COUNT; i++) up_set_value(r, i, param_desc_of(e, i)->def);
            up_set_value(r, P_E7, SL[k]);                 /* (the raw stored value: B slots as 1.0.2 wrote them) */
        }
        ub.r[4].name[1] = 'X';
        up_set_value(&ub.r[4], P_E0, 3);                 /* (another sound than slot 1's) */
        memset(&bk, 0, sizeof bk);
        bk.magic = FM6_BANK_MAGIC; bk.ver = 1; bk.nslot = FM6_BANK_N; bk.used = 1u << 2;
        memcpy(bk.v[2], FM6_FACTORY[5], FM6_PACKED);
        memcpy(bk.v[2] + 118, "BANK B3   ", 10);
        upf_empty();
        bad += check("the user presets of an old archive (id 6) restore", put_all(6, &ub, sizeof ub, st_crc32(&ub, sizeof ub)) == 0);
        bad += check("an old archive's FM6 bank (id 8) is taken (0)", put_all(8, &bk, sizeof bk, st_crc32(&bk, sizeof bk)) == 0);
        bad += check("  .. its B3 patch is now the B3 presets' own (slots 1 and 5)",
                     !upf_get(0, pk) && !memcmp(pk + 118, "BANK B3   ", 10) && !upf_get(4, pk) && !memcmp(pk + 118, "BANK B3", 7));
        bad += check("  .. an F slot, an empty B slot and a DRUM sound get none (factory / init / no patch, as before)",
                     upf_get(1, pk) && upf_get(2, pk) && upf_get(3, pk));
        bad += check("  .. written to flash (the user presets' FM6 patches)",
                     st_load(OBJ_UPFM6, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &upf, sizeof got));
        up_load(0);
        bad += check("  .. and the B3 preset loads it, SLOT OWN", TSEL->eng_req == ENGI_FM6 &&
                     !memcmp(fm6_patch[song.sel] + FP_NAME, "BANK B3", 7) && TSEL->p[P_E7] == FM6_OWN);
        up_load(1);
        bad += check("  .. the F2 preset loads the factory patch, SLOT F2", TSEL->p[P_E7] == 1 &&
                     !memcmp(fm6_patch[song.sel] + FP_NAME, FM6_FACTORY[1] + 118, 10));
        bad += check("a restore of id 8 twice moves nothing more (idempotent)",
                     put_all(8, &bk, sizeof bk, st_crc32(&bk, sizeof bk)) == 0 && !memcmp(&got, &upf, sizeof got));
        bad += check("an empty id 8 (a 1.0.3 archive) is taken and ignored", put_all(8, 0, 0, st_crc32(0, 0)) == 0 &&
                     !memcmp(&got, &upf, sizeof got));
        bk.v[3][0] = 200;
        bad += check("an FM6 bank with a byte above 127 is refused (2)", put_all(8, &bk, sizeof bk, st_crc32(&bk, sizeof bk)) == 2u);
        bad += check("id 8 lists empty, id 9 with the patches' length", list(8, &len, &crc) == 0 && len == 0 &&
                     list(9, &len, &crc) == 0 && len == sizeof(upf_t) && crc == st_crc32(&upf, sizeof upf));
        memcpy(&got, &upf, sizeof got);
        upf_empty();
        bad += check("id 9 restores the user presets' FM6 patches (flash and RAM)",
                     put_all(9, &got, sizeof got, st_crc32(&got, sizeof got)) == 0 && !memcmp(&got, &upf, sizeof got) &&
                     !upf_get(0, pk) && !memcmp(pk + 118, "BANK B3", 7));
        got.magic ^= 1;
        bad += check("an id 9 of another layout is refused (2), the patches kept",
                     put_all(9, &got, sizeof got, st_crc32(&got, sizeof got)) == 2u && !upf_get(0, pk));
        bad += check("an id 9 of the wrong size is refused at begin (1)", put_begin(9, sizeof got - 4u, 0) == 1u);
        bad += check("an id 10 is refused (1)", put_begin(10, 0, 0) == 1u);
    }
    memcpy(&st, &proj_slot[2], sizeof st);
    erase_error = 1;
    proj_slot[2].raw[100] ^= 1;                          /* (marks the RAM copy, to see it is kept) */
    memcpy(&v6, &proj_slot[2], sizeof v6);
    bad += check("a flash error on commit (4) keeps the slot's RAM copy",
                 put_all(4, &st, sizeof st, st_crc32(&st, sizeof st)) == 4u && !memcmp(&proj_slot[2], &v6, sizeof v6));
    erase_error = 0;
    bad += check("an empty project object clears the slot", put_all(4, 0, 0, st_crc32(0, 0)) == 0 && !project_used(2));
    st.raw[200] ^= 1;                                    /* hash no longer matches */
    bad += check("a project whose own hash fails is refused", put_all(4, &st, sizeof st, st_crc32(&st, sizeof st)) == 2u);

    /* the runtime */
    reset();
    trk[2].step[5] = (step_t){{67}, 1, ST_NOTE, 0, 96, 0, 0};
    trk[2].p[P_LEVEL] = 77;
    project_capture(&proj_scratch);
    proj_scratch.parts=PROJ_LEGACY_NTRK;
    fixture_pack(&st, &proj_scratch);
    host_tracks_init();
    bad += check("the runtime object restores the tracks",
                 put_all(0, &st, sizeof st, st_crc32(&st, sizeof st)) == 0 && trk[2].step[5].note[0] == 67u &&
                 trk[2].p[P_LEVEL] == 77);

    /* user preset banks: unknown record versions are kept as bytes */
    reset();
    {
        static up_bank_t b;
        memset(&b, 0, sizeof b);
        b.magic = UP_BANK_MAGIC;
        b.rsize = sizeof(up_rec_t);
        b.nslot = UP_PER_BANK;
        b.r[4].used = UP_USED;
        b.r[4].ver = 99;                                 /* a future record */
        memcpy(b.r[4].name, "Future", 7);
        b.r[4].p[0] = 55;
        bad += check("a bank with a future record version restores byte for byte",
                     put_all(6, &b, sizeof b, st_crc32(&b, sizeof b)) == 0 &&
                     st_load(OBJ_UPRESET0, &proj_wire, sizeof proj_wire) == (int)sizeof b &&
                     !memcmp(&proj_wire, &b, sizeof b));
        b.nslot = 3;
        bad += check("a bank with the wrong slot count is refused", put_all(7, &b, sizeof b, st_crc32(&b, sizeof b)) == 2u);
    }
    printf("backup test %s\n", bad ? "FAILED" : "passed");
    return bad != 0;
}

#if NPART == 8
/* This fixture calls the internal transfer handler below ed_dispatch. Begin,
 * chunks and abort intentionally operate on staging/session state. Production
 * storage quarantine still refuses commits; public all-state refusal belongs
 * to the actual editor/robust dispatcher fixtures, not this helper boundary. */
static uint8_t native_nor_before[sizeof nor];
static struct {
    __typeof__(trk) tracks;__typeof__(song) song_state;__typeof__(fm6_patch) patches;
    __typeof__(motion) motion_state;__typeof__(chain_config) chain_state;
    __typeof__(proj_slot) slots;__typeof__(settings) settings_state;
} native_music_before;
static int native_put_check(const char *label,unsigned id,const void *p,unsigned n,unsigned crc,unsigned expected) {
    memcpy(native_nor_before,nor,sizeof nor);unsigned old_erases=erases, old_programs=programs;
    memcpy(native_music_before.tracks,trk,sizeof trk);native_music_before.song_state=song;
    memcpy(native_music_before.patches,fm6_patch,sizeof fm6_patch);native_music_before.motion_state=motion;
    native_music_before.chain_state=chain_config;memcpy(native_music_before.slots,proj_slot,sizeof proj_slot);
    native_music_before.settings_state=settings;
    int ok=put_all(id,p,n,crc)==expected && erases==old_erases && programs==old_programs && !memcmp(nor,native_nor_before,sizeof nor) &&
        !memcmp(native_music_before.tracks,trk,sizeof trk) && !memcmp(&native_music_before.song_state,&song,sizeof song) &&
        !memcmp(native_music_before.patches,fm6_patch,sizeof fm6_patch) && !memcmp(&native_music_before.motion_state,&motion,sizeof motion) &&
        !memcmp(&native_music_before.chain_state,&chain_config,sizeof chain_config) && !memcmp(native_music_before.slots,proj_slot,sizeof proj_slot) &&
        !memcmp(&native_music_before.settings_state,&settings,sizeof settings);
    return check(label,ok);
}
static int native_backup_eight(void) {
    int bad=0;unsigned len=0,crc=0;static project_store_t st;static project_v6_t v6;
    persist_t ps;reset();
    memset(&st,0xA5,sizeof st);project_store_t sentinel=st;
    project_capture(&proj_scratch);
    bad+=check("native packing refuses without altering historical output",!proj_pack(&st,&proj_scratch)&&!memcmp(&st,&sentinel,sizeof st));
    bad+=check("internal LIST cannot capture native runtime as FUN9 (2, zero objects)",list(0,&len,&crc)==2&&rep[2]==0&&!ed_bk_valid);
    bad+=check("internal GET after failed capture is stale (5)",get(0,0,64)==5);
    project_save(0);
    bad+=check("refused native save cannot make old runtime GET readable",get(0,0,64)==5);
    bad+=check("a repeated internal LIST still refuses historical capture",list(0,&len,&crc)==2&&!ed_bk_valid);
    usb.resets++;
    bad+=check("USB reset cannot authorize old GET",get(2,0,16)==5);
    reset();memset(&ps,0,sizeof ps);settings_export(&ps);
    /* Every historical valid settings encoding still reaches the lower
     * storage quarantine; invalid encodings fail validation before that gate. */
    __typeof__(settings) saved_settings=settings;
    ps.lowcut=2;ps.palette=palette_to_stored(6);
    bad+=native_put_check("SPEAKER BASS+ / palette validated, storage refused (4)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),4);
    ps.palette=13;
    bad+=native_put_check("legacy PAPER palette validated, storage refused (4)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),4);
    ps.palette=palette_to_stored(6);ps.lowcut=3;
    bad+=native_put_check("unknown SPEAKER refused before storage (2)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),2);
    ps.lowcut=0;
    ps.bold=hold_to_stored(0,3);
    bad+=native_put_check("HOLD 0.6 seconds validated, storage refused (4)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),4);
    ps.bold=1;
    bad+=native_put_check("legacy HOLD validated, storage refused (4)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),4);
    ps.bold=HOLD_TAG+7;
    bad+=native_put_check("unknown HOLD refused before storage (2)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),2);
    ps.bold=0;
    static const unsigned leds[]={LEDS_TAG|LEDS_INV,1,LEDS_TAG|LEDS_DIM_LO,LEDS_TAG|LEDS_OFF};
    for(unsigned i=0;i<4;i++){ps.zoom=leds[i];bad+=native_put_check("current/legacy LEDS encoding validated, storage refused (4)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),4);}
    ps.zoom=LEDS_TAG+4;
    bad+=native_put_check("unknown LEDS refused before storage (2)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps),2);
    ps.zoom=0;
    bad+=native_put_check("incorrect transfer CRC refused before storage (2)",1,&ps,sizeof ps,st_crc32(&ps,sizeof ps)^1,2);
    bad+=check("all refused settings commits preserve live settings",!memcmp(&settings,&saved_settings,sizeof settings));
    bad+=check("unknown object layout refused at begin",put_begin(8,sizeof ps,0)==1);
    bad+=check("wrong project length refused at begin",put_begin(2,sizeof st-4,0)==1);
    bad+=check("wrong settings length refused at begin",put_begin(1,sizeof ps+4,0)==1);
    ed_bk_put=0;
    bad+=check("chunk without begin remains stale (5)",put_chunk(1,0,(uint8_t*)&ps,16)==5);
    bad+=check("valid settings begin is internal staging only",put_begin(1,sizeof ps,st_crc32(&ps,sizeof ps))==0&&ed_bk_put);
    bad+=check("wrong offset refused",put_chunk(1,16,(uint8_t*)&ps,16)==1);
    bad+=check("another object's chunk remains stale (5)",put_chunk(2,0,(uint8_t*)&ps,16)==5);
    put_begin(1,sizeof ps,st_crc32(&ps,sizeof ps));unsigned step=sizeof ps-8,off=0;
    for(;off+256<=step;off+=256)put_chunk(1,off,(uint8_t*)&ps+off,256);
    if(off<step)put_chunk(1,off,(uint8_t*)&ps+off,step-off);
    bad+=check("chunk beyond declared length refused",put_chunk(1,step,(uint8_t*)&ps,16)==1);
    bad+=check("incomplete commit refused before any erase",put_end(1,2)==2&&erases==0);
    put_begin(1,sizeof ps,st_crc32(&ps,sizeof ps));put_chunk(1,0,(uint8_t*)&ps,64);usb.resets++;
    bad+=check("USB reset ends internal transfer (5)",put_chunk(1,64,(uint8_t*)&ps+64,64)==5);
    put_begin(1,sizeof ps,st_crc32(&ps,sizeof ps));fm1_ms+=16000;
    bad+=check("sixteen-second timeout ends internal transfer (5)",put_chunk(1,0,(uint8_t*)&ps,64)==5);
    put_begin(2,sizeof st,0);put_chunk(2,0,(uint8_t*)&ps,64);unsigned gen=proj_wire_gen;
    project_save(1);
    bad+=check("refused native save does not reuse transfer scratch or bump generation",proj_wire_gen==gen&&put_chunk(2,64,(uint8_t*)&ps,64)==0);
    put_end(2,3);
    bad+=check("internal abort ends staging without writes",!ed_bk_put&&erases==0);
    transport_req=1;bad+=check("starting PLAY refuses begin (3)",put_begin(1,sizeof ps,0)==3);transport_req=0;
    /* Original valid historical project input is constructed independently;
     * accepting its parser does not authorize repacking or flash publication. */
    reset();project_capture(&proj_scratch);proj_scratch.parts=4;
    bad+=check("independent valid FUN9 input constructed",fixture_fun9(&st,&proj_scratch));
    project_t imported;
    bad+=check("valid FUN9 parser accepts before isolated corruption",proj_import(&imported,&st,sizeof st));
    bad+=native_put_check("valid historical FUN9 commit refuses native repacking (2)",4,&st,sizeof st,st_crc32(&st,sizeof st),2);
    { uint8_t v8[3584]={0};unsigned pos=68,pos9=68;
      memcpy(v8,st.raw,68);v8[66]=91;
      for(unsigned t=0;t<4;t++) {
        for(unsigned j=0;j<91;j++)v8[pos++]=st.raw[pos9+(j<83?j:j+8)];pos9+=99;
        memcpy(v8+pos,st.raw+pos9,2+64*9);pos+=2+64*9;pos9+=2+64*9;
      }
      memcpy(v8+pos,st.raw+pos9,36+260);memcpy(v8+3056,st.raw+3120,4*128);
      fixture_le32(v8,0x46554e38);fixture_le32(v8+4,3584);fixture_le32(v8+3580,fixture_hash(v8,3580));
      bad+=check("valid FUN8 parser accepts historical lane/default mapping",proj_import(&imported,v8,sizeof v8)&&imported.t[2].p[P_LN3]==127);
      bad+=native_put_check("valid historical FUN8 commit refuses native repacking (2)",3,v8,sizeof v8,st_crc32(v8,sizeof v8),2);
    }
    st.raw[200]^=1;
    bad+=native_put_check("isolated bad FUN9 checksum refuses publication (2)",4,&st,sizeof st,st_crc32(&st,sizeof st),2);
    bad+=native_put_check("empty historical project cannot clear native slot (4)",4,0,0,st_crc32(0,0),4);
    memset(&v6,0,sizeof v6);v6.magic=PROJ_MAGIC_V6;v6.size=sizeof v6;
    for(unsigned i=0;i<G_COUNT;i++)v6.g[i]=GP[i].def;
    for(unsigned i=0;i<4;i++)v6.t[i].engine=trk[i].engine;
    v6.t[0].step[0]=(step10_t){{255,72},9,7,0,96,0,0};v6.t[0].p[61]=3;
    chain_defaults(&v6.chain);v6.sum=proj_hash(&v6,sizeof v6-4);
    bad+=check("legacy FUN6 parser still bounds original malformed step",proj_import(&imported,&v6,sizeof v6)&&(proj_bound(&imported),imported.t[0].step[0].n==4)&&imported.t[0].step[0].note[0]==127);
    bad+=native_put_check("bounded legacy FUN6 commit refuses native repacking (2)",5,&v6,sizeof v6,st_crc32(&v6,sizeof v6),2);
    erase_error=1;
    st.raw[200]^=1;
    bad+=native_put_check("project commit refuses packing before unreachable flash fault",4,&st,sizeof st,st_crc32(&st,sizeof st),2);
    erase_error=0;
    bad+=native_put_check("runtime restore refuses native repacking without adopting tracks",0,&st,sizeof st,st_crc32(&st,sizeof st),2);
    { static up_bank_t bank;static fm6_bank_t old_bank;static upf_t patches;
      reset();memset(&bank,0,sizeof bank);bank.magic=UP_BANK_MAGIC;bank.rsize=sizeof(up_rec_t);bank.nslot=UP_PER_BANK;
      bank.r[4].used=UP_USED;bank.r[4].ver=99;memcpy(bank.r[4].name,"Future",7);bank.r[4].p[0]=55;
      up_bank_t bank_before=up_bank[0];
      bad+=native_put_check("future preset record validated but storage quarantine refuses bank",6,&bank,sizeof bank,st_crc32(&bank,sizeof bank),4);
      bad+=check("refused bank leaves live user records unchanged",!memcmp(&up_bank[0],&bank_before,sizeof bank_before));
      bank.nslot=3;
      bad+=native_put_check("incorrect preset slot count refused before storage (2)",7,&bank,sizeof bank,st_crc32(&bank,sizeof bank),2);
      bank.nslot=UP_PER_BANK;
      /* Seed a pre-existing historical user bank to test the internal migration
       * path. These bytes are not described as a successful native restore. */
      static const int16_t slots[5]={FM6_NFACTORY+2,1,FM6_NFACTORY+4,FM6_NFACTORY+2,FM6_NFACTORY+2};
      for(unsigned k=0;k<5;k++) {
        memset(&bank.r[k],0,sizeof bank.r[k]);bank.r[k].used=UP_USED;bank.r[k].ver=UP_VER;bank.r[k].np=P_COUNT;bank.r[k].engine=k==3?ENGI_DRUM:ENGI_FM6;
        for(unsigned i=0;i<P_COUNT;i++)up_set_value(&bank.r[k],i,param_desc_of(bank.r[k].engine,i)->def);
        up_set_value(&bank.r[k],P_E7,slots[k]);bank.r[k].name[0]='A'+k;
      }
      bank.r[4].name[1]='X';up_set_value(&bank.r[4],P_E0,3);up_bank[0]=bank;
      memset(&old_bank,0,sizeof old_bank);old_bank.magic=FM6_BANK_MAGIC;old_bank.ver=1;old_bank.nslot=FM6_BANK_N;old_bank.used=1u<<2;
      memcpy(old_bank.v[2],FM6_FACTORY[5],128);memcpy(old_bank.v[2]+118,"BANK B3   ",10);upf_empty();
      bad+=native_put_check("internal old FM6 bank migration is RAM-only on failed storage (4)",8,&old_bank,sizeof old_bank,st_crc32(&old_bank,sizeof old_bank),4);
      uint8_t pk[128];
      bad+=check("RAM migration binds old B3 patch to seeded preset despite write refusal",!upf_get(0,pk)&&!memcmp(pk+118,"BANK B3",7));
      bad+=check("shared old B3 patch becomes each seeded preset's own",!upf_get(4,pk)&&!memcmp(pk+118,"BANK B3",7));
      bad+=check("factory F2, empty B5 and DRUM do not acquire owned patches",upf_get(1,pk)&&upf_get(2,pk)&&upf_get(3,pk));
      up_load(0);
      bad+=check("explicit seeded B3 user load uses owned RAM patch",TSEL->eng_req==ENGI_FM6&&!memcmp(fm6_patch[song.sel]+FP_NAME,"BANK B3",7)&&TSEL->p[P_E7]==FM6_OWN);
      up_load(1);
      bad+=check("explicit seeded F2 user load retains factory patch",TSEL->p[P_E7]==1&&!memcmp(fm6_patch[song.sel]+FP_NAME,FM6_FACTORY[1]+118,10));
      patches=upf;
      bad+=native_put_check("repeated internal old FM6 migration cannot write (4)",8,&old_bank,sizeof old_bank,st_crc32(&old_bank,sizeof old_bank),4);
      bad+=check("repeated RAM migration is idempotent",!memcmp(&patches,&upf,sizeof patches));
      bad+=native_put_check("empty retired FM6 bank is taken without writing (0)",8,0,0,st_crc32(0,0),0);
      old_bank.v[3][0]=200;
      bad+=native_put_check("bad FM6 patch byte refused before migration (2)",8,&old_bank,sizeof old_bank,st_crc32(&old_bank,sizeof old_bank),2);
      upf_empty();
      bad+=native_put_check("internal owned FM6 patches copy to RAM but storage refuses (4)",9,&patches,sizeof patches,st_crc32(&patches,sizeof patches),4);
      bad+=check("failed owned-patch write preserves documented restored RAM copy",!memcmp(&patches,&upf,sizeof patches)&&!upf_get(0,pk));
      patches.magic^=1;
      bad+=native_put_check("invalid owned-patch layout refused before RAM copy (2)",9,&patches,sizeof patches,st_crc32(&patches,sizeof patches),2);
      bad+=check("invalid patch copy keeps existing B3 RAM patch",!upf_get(0,pk)&&!memcmp(pk+118,"BANK B3",7));
      bad+=check("wrong owned-patch size refused at begin",put_begin(9,sizeof patches-4,0)==1);
      bad+=check("unknown object ten refused at begin",put_begin(10,0,0)==1);
      bad+=check("LIST cannot advertise old id8/id9 after native capture failure",list(8,&len,&crc)==2&&rep[2]==0);
    }
    printf("backup internal native test %s\n",bad?"FAILED":"passed");return bad!=0;
}
#endif
int main(void) {
#if NPART == 8
    return native_backup_eight();
#else
    return legacy_backup_four();
#endif
}
