/* SPDX-License-Identifier: GPL-3.0-only */
/* Current own-format records reopen through actual application readers after
 * the actual loader's virtual upgrade. Synthetic public data only; no device.
 * This is not a power-cut, timing, physical boot or ancillary-write test. */
#include <stdint.h>
static uint8_t nor[0x100000];
#define SMP_USER_XIP(k) (nor + 0xa0000u + (k) * 0x14000u)
#define D8_BACKUP_TEST_NO_MAIN 1
#include "backup_test.c"
#include "../firmware/src/d8p1.h"
#include "../firmware/src/d8pool_mapped.h"

_Static_assert(NTRK == 8, "qualify current eight-track readers");
int ancillary_seed_app(uint8_t *, size_t, const char *);
int ancillary_upgrade(uint8_t *, size_t, const char *);

static persist_t expected_settings;
static up_bank_t expected_banks[2];
static uint8_t expected_patches[UP_SLOTS][FM6_PACKED];
static smp_user_hdr_t expected_samples[3];
static uint8_t before_upgrade[sizeof nor];
static uint8_t native_project[D8P1_LIMIT];
static size_t native_length;

/* Independent fixture publisher. The payloads are current actual C formats;
 * production readers must validate/select them. Ancillary writer durability is
 * deliberately not claimed by manufacturing CRC-valid fixture commit records. */
static void publish(unsigned object, unsigned copy, unsigned sequence,
                    const void *payload, uint32_t bytes)
{
    st_hdr_t h = {0};
    uint32_t base = st_sector(object, copy);
    h.magic = ST_MAGIC; h.type = (uint16_t)object; h.slot = (uint16_t)copy;
    h.seq = sequence; h.len = bytes; h.crc = st_crc32(payload, bytes);
    h.hcrc = st_crc32(&h, sizeof h - 4);
    memset(nor + base, 255, ST_SECTOR);
    memcpy(nor + base + ST_PAYLOAD_OFF, payload, bytes);
    memcpy(nor + base, &h, sizeof h);
}

static int native_read(void *context, uint32_t off, void *out, uint32_t n)
{
    (void)context;
    if (off > D8POOL_BYTES || n > D8POOL_BYTES - off) return -1;
    while (n) {
        unsigned k = D8POOL_BLOCK - off % D8POOL_BLOCK;
        if (k > n) k = n;
        memcpy(out, nor + d8pool_mapped_address(off / D8POOL_BLOCK) + off % D8POOL_BLOCK, k);
        off += k; out = (uint8_t *)out + k; n -= k;
    }
    return 0;
}
static int native_erase(void *context, uint32_t off)
{
    (void)context;
    if (off >= D8POOL_BYTES || off % D8POOL_SECTOR) return -1;
    memset(nor + d8pool_mapped_address(off / D8POOL_BLOCK) + off % D8POOL_BLOCK, 255, D8POOL_SECTOR);
    return 0;
}
static int native_program(void *context, uint32_t off, const void *data, uint32_t n)
{
    (void)context;
    if (off > D8POOL_BYTES || n > D8POOL_BYTES - off || off % 256u + n > 256u) return -1;
    if (!n) return 0;
    uint8_t *dst = nor + d8pool_mapped_address(off / D8POOL_BLOCK) + off % D8POOL_BLOCK;
    for (unsigned i = 0; i < n; i++) dst[i] &= ((const uint8_t *)data)[i];
    return 0;
}
static int native_stopped(void *context) { (void)context; return 1; }
static d8pool native_pool = {NULL, native_read, native_erase, native_program, native_stopped};

static int seed_current_records(void)
{
    int bad = 0;
    panel = PANEL_DEFAULT; settings_init();
    settings.palette = (UI_DEFAULT_INDEX + 1u) % NPALETTES;
    settings.lowcut = 2; settings_hold = 3; settings_leds = LEDS_DIM_LO;
    favorites.filter = 1; favorites.user = 0x80000001u;
    favorites.factory[0][0] = 1;
    settings_export(&expected_settings);
    persist_t older = expected_settings; older.lowcut = 0;
    publish(OBJ_SETTINGS, 0, 7, &older, sizeof older);
    publish(OBJ_SETTINGS, 1, 8, &expected_settings, sizeof expected_settings);

    memset(up_bank, 0, sizeof up_bank); upf_empty();
    for (unsigned b = 0; b < 2; b++) {
        up_bank[b].magic = UP_BANK_MAGIC;
        up_bank[b].rsize = sizeof(up_rec_t); up_bank[b].nslot = UP_PER_BANK;
    }
    for (unsigned k = 0; k < UP_SLOTS; k++) {
        up_rec_t *r = up_rec(k);
        r->used = UP_USED; r->ver = UP_VER; r->engine = ENGI_FM6; r->np = P_COUNT;
        char name[13]; snprintf(name, sizeof name, "OWN %02u", k + 1);
        up_set_name(r, k, name);
        for (unsigned p = 0; p < P_COUNT; p++)
            up_set_value(r, p, (int16_t)param_desc_of(ENGI_FM6, p)->def);
        r->note[0] = (uint8_t)(40 + k); r->flags[0] = 1;
        upf_set(k, FM6_FACTORY[k % FM6_NFACTORY]);
        bad += check("synthetic current preset accepted", up_valid(r));
        bad += check("synthetic current FM6 patch accepted", !upf_get(k, expected_patches[k]));
    }
    memcpy(expected_banks, up_bank, sizeof expected_banks);
    for (unsigned b = 0; b < 2; b++) {
        up_bank_t old = up_bank[b]; old.r[0].name[0] = 'X';
        publish(OBJ_UPRESET0 + b, 0, 7, &old, sizeof old);
        publish(OBJ_UPRESET0 + b, 1, 8, &up_bank[b], sizeof up_bank[b]);
    }
    upf_t old_fm = upf; old_fm.used = 1;
    publish(OBJ_UPFM6, 0, 7, &old_fm, sizeof old_fm);
    publish(OBJ_UPFM6, 1, 8, &upf, sizeof upf);
    for (unsigned k = 0; k < 3; k++) {
        uint8_t *slot = nor + SMP_USER_BASE + k * SMP_USER_SIZE;
        smp_user_hdr_t *h = &expected_samples[k];
        memset(slot, 255, SMP_USER_SIZE); memset(h, 0, sizeof *h);
        h->magic = SMP_USER_MAGIC; h->version = 1; h->nz = 1;
        snprintf(h->name, sizeof h->name, "OWN%u", k + 1);
        h->data_len = 1025u + k * 2u;
        for (unsigned i = 0; i < h->data_len; i++) slot[SMP_USER_DATA + i] = (uint8_t)(i * 13u + k);
        h->crc = st_crc32(slot + SMP_USER_DATA, h->data_len);
        h->zone[0].n = 2u * h->data_len; h->zone[0].le = h->zone[0].n - 1;
        h->zone[0].rate = 65536; h->zone[0].hi = 127;
        memcpy(slot, h, sizeof *h);
    }
    FILE *f = fopen("tests/fixtures/d8p1/minimal.d8p", "rb");
    if (!f) return bad + 1;
    native_length = fread(native_project, 1, sizeof native_project, f);
    int valid = !ferror(f) && fgetc(f) == EOF && native_length == 6599;
    fclose(f);
    bad += check("current native fixture is complete", valid);
    if (!valid) return bad;
    for (unsigned i = 0; i < 4; i++)
        bad += check("current own project committed", d8pool_save(&native_pool, i, native_project, native_length, 7) == D8POOL_OK);
    return bad;
}

static void reopen_current_records(int got[7])
{
    /* Poison all cached content before actual flash readers run. */
    memset(&settings, 0xa5, sizeof settings); memset(&panel, 0xa5, sizeof panel);
    memset(&favorites, 0xa5, sizeof favorites); memset(up_bank, 0xa5, sizeof up_bank);
    memset(&upf, 0xa5, sizeof upf); memset(usr_zone, 0xa5, sizeof usr_zone);
    memset(usr_nz, 0xa5, sizeof usr_nz); flash_ok = 1;
    persist_t p = {0}, exported = {0};
    int n = st_load(OBJ_SETTINGS, &p, sizeof p);
    got[0] = n == sizeof p && settings_import(&p, n) == 1;
    if (got[0]) {
        settings_init(); settings_export(&exported);
        got[0] = !memcmp(&exported, &expected_settings, sizeof exported);
    }
    up_boot();
    for (unsigned b = 0; b < 2; b++)
        got[1 + b] = !memcmp(&up_bank[b], &expected_banks[b], sizeof up_bank[b]);
    got[3] = upf_valid(&upf) && upf.used == UINT32_MAX;
    for (unsigned k = 0; k < UP_SLOTS; k++) {
        uint8_t patch[FM6_PACKED];
        if (upf_get(k, patch) || memcmp(patch, expected_patches[k], sizeof patch)) got[3] = 0;
    }
    for (unsigned k = 0; k < 3; k++) {
        smp_user_scan(k);
        const smp_user_hdr_t *h = &expected_samples[k];
        got[4 + k] = usr_nz[k] == 1 && !memcmp(&usr_zone[k][0], &h->zone[0], sizeof h->zone[0]) &&
            st_crc32(smp_user_xip(k) + SMP_USER_DATA, h->data_len) == h->crc;
    }
}

int main(int argc, char **argv)
{
    int control = -1, bad = 0, got[7] = {0};
    const char *names[7] = {"settings", "preset bank 0", "preset bank 1", "32 FM6 patches", "sample 0", "sample 1", "sample 2"};
    /* FM6 patches are bound to their preset records, so losing either bank
     * must also make the complete 32-patch reopening check fail. */
    const unsigned invalid_masks[7] = {1u, 2u | 8u, 4u | 8u, 8u, 16u, 32u, 64u};
    if (argc == 5 && !strcmp(argv[3], "--invalidate") && strlen(argv[4]) == 1 && argv[4][0] >= '0' && argv[4][0] <= '6')
        control = argv[4][0] - '0';
    if (argc != 3 && control < 0) {
        fprintf(stderr, "usage: native_ancillary_upgrade_test OLD.fwsc NEW.fwsc [--invalidate 0..6]\n"); return 2;
    }
    reset();
    bad += check("cached old application seeded", !ancillary_seed_app(nor, sizeof nor, argv[1]));
    bad += seed_current_records();
    if (bad) return 1;
    reopen_current_records(got);
    for (unsigned i = 0; i < 7; i++) bad += check(names[i], got[i]);
    bad += check("current readers do not erase or program", !erases && !programs);
    if (bad) return 1;
    memcpy(before_upgrade, nor, sizeof nor);
    bad += check("actual loader completes virtual upgrade", !ancillary_upgrade(nor, sizeof nor, argv[2]));
    int unchanged = 1;
    for (unsigned i = 0; i < sizeof nor; i++) {
        if ((i >= 0x4000 && i < 0x93000) || (i >= 0xe4000 && i < 0xe5000) || (i >= 0xe8000 && i < 0xe9000)) continue;
        if (nor[i] != before_upgrade[i]) unchanged = 0;
    }
    bad += check("non-update NOR remains byte-exact", unchanged);
    if (control >= 0 && control < 4) {
        unsigned obj = control == 0 ? OBJ_SETTINGS : control == 3 ? OBJ_UPFM6 : OBJ_UPRESET0 + control - 1;
        for (unsigned copy = 0; copy < 2; copy++) nor[st_sector(obj, copy)] ^= 1;
    } else if (control >= 4) nor[SMP_USER_BASE + (control - 4) * SMP_USER_SIZE] ^= 1;
    reopen_current_records(got);
    for (unsigned i = 0; i < 7; i++) {
        int expected = control < 0 || !(invalid_masks[control] & (1u << i));
        printf("reopen %s: %s\n", names[i], got[i] ? "accepted" : "refused");
        bad += check(expected ? names[i] : "deliberate invalidation detected", got[i] == expected);
    }
    uint8_t restored[D8P1_LIMIT]; size_t length = 0;
    for (unsigned i = 0; i < 4; i++)
        bad += check("native own project still opens exactly", d8pool_load(&native_pool, i, restored, sizeof restored, &length, 7) == D8POOL_OK &&
            length == native_length && !memcmp(restored, native_project, length));
    bad += check("post-upgrade readers do not erase or program", !erases && !programs);
    printf("Current ancillary reopen: failures=%d, control=%d, no device; ancillary writer and physical boot not qualified\n", bad, control);
    return bad != 0;
}
