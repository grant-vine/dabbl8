/* SPDX-License-Identifier: GPL-3.0-only */
/* Native virtual-pool seed with actual codec adoption and reference preparation. */
#define NPART 8
#define UI_RENDER_NO_MAIN 1
#define UI_ASYNC_LCD 1
/* Include order supplies host/UI definitions before native runtime adapters. */
// clang-format off
#include "ui_render.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
#include "../firmware/src/d8arr_prepare.c"
// clang-format on
static uint8_t native_nor[D8POOL_BYTES], native_wire[D8P1_LIMIT];
static d8p1_project_state native_source;
static unsigned checks, failures, reads, writes, render_frames, fail_read;
static void proof(int ok, const char *why)
{
    checks++;
    if (!ok) {
        failures++;
        fprintf(stderr, "FAIL %s: %s\n", cur_name ? cur_name : "seed", why);
    }
}
static int nr(void *c, uint32_t a, void *p, uint32_t n)
{
    (void)c;
    reads++;
    if (fail_read || a > D8POOL_BYTES || n > D8POOL_BYTES - a)
        return -1;
    memcpy(p, native_nor + a, n);
    return 0;
}
static int ne(void *c, uint32_t a)
{
    (void)c;
    if (a % 4096 || a > D8POOL_BYTES - 4096)
        return -1;
    writes++;
    memset(native_nor + a, 255, 4096);
    return 0;
}
static int np(void *c, uint32_t a, const void *p, uint32_t n)
{
    (void)c;
    if (!n || n > 256 || a > D8POOL_BYTES - n || (a & 255) + n > 256)
        return -1;
    writes++;
    for (unsigned i = 0; i < n; i++)
        native_nor[a + i] &= ((const uint8_t *)p)[i];
    return 0;
}
static int nq(void *c)
{
    (void)c;
    return 1;
}
static d8pool native_pool = {NULL, nr, ne, np, nq};
static int nc(void *c, d8p1_project_catalog *out)
{
    (void)c;
    return d8p1_catalog_pool(&native_pool, out);
}
static int ns(void *c, unsigned o, const char *n)
{
    (void)c;
    return d8p1_save_as_pool(&native_pool, o, n);
}
static int nl(void *c, unsigned o)
{
    (void)c;
    return d8p1_load_pool(&native_pool, o);
}
static int nn(void *c, unsigned o, const char *n)
{
    (void)c;
    return d8p1_rename_pool(&native_pool, o, n);
}
static int na(void *c)
{
    (void)c;
    return d8arr_prepare_pool(&native_pool);
}
static project_native_ops native_ops = {NULL, nc, ns, nl, nn, na};
static void seed(void)
{
    ui_power_on();
    int32_t audio[CTL * 2];
    mix_block(audio, CTL);
    transport_req = 0;
    song.g[G_CLOCK] = 0;
    memset(native_nor, 255, sizeof native_nor);
    FILE *f = fopen("tests/fixtures/d8p1/minimal.d8p", "rb");
    if (!f)
        exit(2);
    size_t n = fread(native_wire, 1, sizeof native_wire, f);
    fclose(f);
    proof(d8p1_project_decode(&native_source, native_wire, n, 0), "decode original native fixture");
    memset(&native_source.arrangement, 0, sizeof native_source.arrangement);
    for (unsigned o = 0; o < 3; o += 2) {
        memcpy(native_source.project.name, o ? "UPPER BANK C" : "LOWER BANK A", 12);
        for (unsigned t = 0; t < 8; t++)
            native_source.project.t[t].p[P_LEVEL] = (int16_t)(90 + t + o);
        proof(d8p1_project_encode(native_wire, sizeof native_wire, &n, &native_source),
              "encode distinct native project");
        proof(!d8pool_save(&native_pool, o, native_wire, n, 5), "seed sparse actual native catalog");
    }
    d8p1_arrangement *a = &native_source.arrangement;
    a->banks = 2;
    a->scenes = 3;
    a->rows = 3;
    for (unsigned b = 0; b < 2; b++)
        for (unsigned t = 0; t < 8; t++) {
            a->bank[b].project[t] = (uint8_t)(b * 2);
            a->bank[b].track[t] = (uint8_t)(7 - t);
        }
    for (unsigned s = 0; s < 3; s++) {
        memcpy(a->scene[s].name, s == 0 ? "LONG SCENE A" : s == 1 ? "LONG SCENE B" : "REPLACE FILL", 12);
        a->scene[s].bank = (uint8_t)(s % 2);
        a->scene[s].apply = 7;
        a->scene[s].mute = (uint8_t)(1u << s);
        for (unsigned t = 0; t < 8; t++) {
            a->scene[s].level[t] = (uint8_t)(70 + s + t);
            a->scene[s].pan[t] = (int8_t)((int)t - 4);
            a->scene[s].transpose[t] = (int16_t)s;
        }
        a->row[s].scene = (uint8_t)s;
        a->row[s].repeat = (uint8_t)(s ? 1 : 2);
    }
    proof(d8p1_project_encode(native_wire, sizeof native_wire, &n, &native_source),
          "encode complete native arrangement");
    proof(!d8p1_load_runtime(native_wire, n, 5), "adopt actual decoded native arrangement");
    mix_block(audio, CTL);
    proof(!project_native_bind(&native_ops, 1), "explicit fixture-only catalog binding");
    proof(project_native.catalog.present == 5 && d8arr_ui_rows() == 3,
          "actual sparse catalog and arrangement available");
}
static void start(void)
{
    song.g[G_CLOCK] = 0;
    transport_req = 0;
    events_block(1);
    proof(!chain_prepare() && chain.armed, "real source-backed preparation");
    transport_req = 0;
    seq_start();
    proof(d8arr_running() && song.playing, "real transport starts");
}
