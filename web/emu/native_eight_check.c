/* SPDX-License-Identifier: GPL-3.0-only */
/* Native oracle for the explicitly manufactured eight-active WASM audio fixture. */
#include "felucca_web.c"
#if NTRK != 8
#error This oracle requires eight tracks.
#endif
static FILE *output;
static void render(unsigned ms, int capture)
{
    unsigned frames = (ms * 441u + 5u) / 10u;
    for (unsigned k = 0; k < frames; k += 128) {
        unsigned n = frames - k < 128 ? frames - k : 128;
        web_render(n);
        for (unsigned i = 0; capture && i < n; i++) {
            float sample[2] = {out_l[i], out_r[i]};
            if (fwrite(sample, sizeof sample, 1, output) != 1) exit(3);
        }
    }
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    FILE *seed = fopen(argv[1], "rb");
    if (!seed) return 2;
    web_nor_erase();
    size_t n = fread(nor, 1, sizeof nor, seed);
    int extra = fgetc(seed);
    fclose(seed);
    if (n != sizeof nor || extra != EOF) return 2;
    output = fopen(argv[2], "wb");
    if (!output) return 2;
    web_boot();render(1500, 0);web_test_eight_active();
    web_buttons(1u << B_PLAY);render(60, 0);web_buttons(0);render(60, 0);
    render(2000, 1);
    return fclose(output) != 0;
}
