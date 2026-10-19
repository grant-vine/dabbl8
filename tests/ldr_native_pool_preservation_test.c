/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline preservation contract for Dabbl8's native pool and ancillary ranges.
 * Uses the actual Felucca loader and its existing package/virtual-NOR adapter.
 * A valid project and synthetic nonempty ancillary bytes contain record-shaped
 * patterns. Ancillary bytes test preservation, not application format/reopening.
 * No device, vendor firmware or private owner backup is used. */
#define main existing_loader_test_main
#include "ldr_test.c"
#undef main
#include "../firmware/src/d8p1.h"
#include "../firmware/src/d8pool_mapped.h"

static int pool_read(void *context, uint32_t off, void *out, uint32_t n)
{
    (void)context;
    uint8_t *p = out;
    if (off > D8POOL_BYTES || n > D8POOL_BYTES - off) return -1;
    while (n) {
        uint32_t within = off % D8POOL_BLOCK;
        uint32_t k = D8POOL_BLOCK - within;
        if (k > n) k = n;
        memcpy(p, nor + d8pool_mapped_address(off / D8POOL_BLOCK) + within, k);
        off += k; p += k; n -= k;
    }
    return 0;
}
static int pool_erase(void *context, uint32_t off)
{
    (void)context;
    if (off >= D8POOL_BYTES || off % D8POOL_SECTOR) return -1;
    memset(nor + d8pool_mapped_address(off / D8POOL_BLOCK) + off % D8POOL_BLOCK,
           0xff, D8POOL_SECTOR);
    return 0;
}
static int pool_program(void *context, uint32_t off, const void *data, uint32_t n)
{
    (void)context;
    if (off > D8POOL_BYTES || n > D8POOL_BYTES - off ||
        (off % 256u) + n > 256u) return -1;
    uint8_t *dest = nor + d8pool_mapped_address(off / D8POOL_BLOCK) + off % D8POOL_BLOCK;
    const uint8_t *src = data;
    for (uint32_t i = 0; i < n; i++) dest[i] &= src[i];
    return 0;
}
static int pool_stopped(void *context) { (void)context; return 1; }

/* Independent approved-map oracle: cover both copies of settings/presets/FM6
 * and every byte of all three user sample allocations, including unused tails.
 * Do not derive these windows from the loader's exclusion conditions. */
static const struct { const char *name; uint32_t base, bytes; } ancillary[] = {
    {"settings pair", 0xfc000, 8192},
    {"preset bank 0 pair", 0xdc000, 8192},
    {"preset bank 1 pair", 0xde000, 8192},
    {"FM6 copy A", 0x9f000, 4096},
    {"FM6 copy B", 0xfe000, 4096},
    {"sample allocation 0", 0xa0000, 81920},
    {"sample allocation 1", 0xb4000, 81920},
    {"sample allocation 2", 0xc8000, 81920},
};
static uint8_t complete_before[sizeof nor];

static void seed_ancillary(const uint8_t record[80])
{
    for (unsigned k = 0; k < sizeof ancillary / sizeof *ancillary; k++) {
        uint32_t base = ancillary[k].base, n = ancillary[k].bytes;
        for (uint32_t i = 0; i < n; i++)
            nor[base + i] = (uint8_t)((i * 37u) ^ (i >> 7) ^ (k * 19u));
        for (uint32_t i = 0; i < n; i += 4096)
            memcpy(nor + base + i + 0xf00, record, 80);
    }
}

static int outside_update_windows_equal(void)
{
    for (uint32_t i = 0; i < sizeof nor; i++) {
        if ((i >= 0x4000 && i < 0x93000) ||
            (i >= 0xe4000 && i < 0xe5000) ||
            (i >= 0xe8000 && i < 0xe9000)) continue;
        if (nor[i] != complete_before[i]) return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    size_t old_len;
    uint8_t *old;
    uint8_t protected_copy[0x1000], extra_copy[0x2000], record[80] = {0};
    uint8_t project[D8P1_LIMIT], prior_project[D8P1_LIMIT], after_project[D8P1_LIMIT];
    uint8_t native_before[D8POOL_BYTES], native_after[D8POOL_BYTES];
    uint32_t old_off;
    int rc, failures = 0, corrupt_after = -1;
    int control = argc == 4 && !strcmp(argv[3], "--control");
    if (argc == 5 && !strcmp(argv[3], "--corrupt-after") &&
        strlen(argv[4]) == 1 && argv[4][0] >= '0' && argv[4][0] <= '7')
        corrupt_after = argv[4][0] - '0';
    if (argc != 3 && !control && corrupt_after < 0) {
        fprintf(stderr, "usage: ldr_native_pool_preservation_test OLD.fwsc NEW.fwsc [--control | --corrupt-after 0..7]\n");
        return 2;
    }
    old = load_logical(argv[1], &old_len);
    logical = load_logical(argv[2], &logical_len);
    old_off = flash_off(old);
    if (!old_off || old_off + 0x93000u > old_len) return 2;
    memset(nor, 0xff, sizeof nor);
    memcpy(nor, old + old_off, 0x93000);
    record[2] = 0x0d; record[3] = 0x5a;
    record[4] = 0x01; record[5] = 0x5a;
    record[6] = 0x41; record[7] = 0x54;
    ota_wr16(record, ota_crc16(record + 2, 78, 0));
    seed_ancillary(record);
    memcpy(nor + 0xe4f00, record, sizeof record);
    memcpy(nor + 0xe8f00, record, sizeof record);
    FILE *fixture = fopen("tests/fixtures/d8p1/minimal.d8p", "rb");
    if (!fixture) return 2;
    size_t n = fread(project, 1, sizeof project, fixture);
    int fixture_ok = !ferror(fixture) && fgetc(fixture) == EOF;
    fclose(fixture);
    if (!fixture_ok || n != 6599) return 2;
    memcpy(prior_project, project, n);
    /* Wire offset 3584 maps to E5F00 in block4. It is track6's last two
     * parameters, engine/preset and the start of its notes. Choose legal
     * parameter bytes for the record CRC; every other field remains legal. */
    unsigned choice;
    for (choice = 0; choice < 128; choice++) {
        record[4] = (uint8_t)choice;
        ota_wr16(record, ota_crc16(record + 2, 78, 0));
        if (record[0] <= 191 && record[1] <= 191) break;
    }
    if (choice == 128) return 2;
    if (control) record[0] ^= 1;
    memcpy(project + 3584, record, sizeof record);
    ota_wr32(project + 98, d8p1_crc32(project + 102, 5416));
    memset(project + 12, 0, 4);
    ota_wr32(project + 12, d8p1_crc32(project, n));
    d8p1_view view;
    if (d8p1_read(&view, project, n) != 1) return 2;
    d8pool pool = {NULL, pool_read, pool_erase, pool_program, pool_stopped};
    for (unsigned object = 0; object < 4; object++)
        if (d8pool_save(&pool, object, prior_project, n, 7) != D8POOL_OK) return 2;
    if (d8pool_save(&pool, 3, project, n, 7) != D8POOL_OK) return 2;
    d8pool_index inventory;
    if (d8pool_inventory(&pool, &inventory) != D8POOL_OK ||
        inventory.object[3].block != 4) return 2;
    puts("known writable D8P1 accepted and saved by actual pool writer; autosave in block4");
    /* Also exercise the second sector's cleanup address, outside this short
     * project's payload. Pool inventory accepts padding on data records. */
    memcpy(nor + 0xe6f00, record, sizeof record);
    if (d8pool_inventory(&pool, &inventory) != D8POOL_OK ||
        pool_read(NULL, 0, native_before, sizeof native_before)) return 2;
    memcpy(protected_copy, nor + 0xa0000, sizeof protected_copy);
    memcpy(extra_copy, nor + 0xe5000, sizeof extra_copy);
    memcpy(complete_before, nor, sizeof nor);
    rc = ldr_session();
    if (corrupt_after >= 0)
        nor[ancillary[corrupt_after].base + ancillary[corrupt_after].bytes - 1] ^= 1;
    failures += check("actual loader completes virtual upgrade", rc == 0);
    failures += check("real update records cleared",
                      record_cleared && nor[0xe4f06] == 0xff && nor[0xe8f06] == 0xff);
    failures += check("original Felucca data control preserved",
                      !memcmp(protected_copy, nor + 0xa0000, sizeof protected_copy));
    failures += check("Dabbl8 extra native pool block preserved",
                      !memcmp(extra_copy, nor + 0xe5000, sizeof extra_copy));
    failures += check("native pool inventory still valid after upgrade",
                      d8pool_inventory(&pool, &inventory) == D8POOL_OK);
    failures += check("all five native pool blocks preserved byte-for-byte",
                      !pool_read(NULL, 0, native_after, sizeof native_after) &&
                      !memcmp(native_before, native_after, sizeof native_before));
    size_t after_length = 0;
    failures += check("latest autosave content survives upgrade exactly",
                      d8pool_load(&pool, 3, after_project, sizeof after_project,
                                  &after_length, 7) == D8POOL_OK &&
                      after_length == n && !memcmp(after_project, project, n));
    for (unsigned k = 0; k < sizeof ancillary / sizeof *ancillary; k++)
        failures += check(ancillary[k].name,
                          !memcmp(complete_before + ancillary[k].base,
                                  nor + ancillary[k].base, ancillary[k].bytes));
    failures += check("all bytes outside app/update-record sectors unchanged",
                      outside_update_windows_equal() && !bad_range);
    printf("native pool sectors after upgrade: E5000=%02X E6000=%02X; failures=%d\n",
           nor[0xe5000], nor[0xe6000], failures);
    free(old); free(logical);
    if (corrupt_after >= 0) {
        /* The selected ancillary comparison and the independent whole-flash
         * comparison must both reject the injected final-byte corruption. */
        printf("ancillary corruption control %d: %s\n", corrupt_after,
               failures == 2 ? "both preservation checks rejected" : "CONTROL FAILED");
        return failures != 2;
    }
    return failures != 0;
}
