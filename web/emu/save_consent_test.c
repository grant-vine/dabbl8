/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual emulator panel, LCD and virtual NOR; no hardware or seeded consent. */
static void consent_lcd_hook(void);
#define D8_EMU_LCD_SYNC_HOOK consent_lcd_hook
#include "felucca_web.c"
static unsigned checks, failures, lcd_event;
static uint8_t before_nor[sizeof nor];
#define CHECK(x) do { checks++; if (!(x)) { failures++; printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
static void consent_lcd_hook(void)
{
    if (d8ls.phase != LS_CONSENT || !lcd_event) return;
    unsigned event = lcd_event; lcd_event = 0;
    if (event == 1) d8_capture_store_changed();
    if (event == 2) fm1_lcd_timeouts++;
}
static void advance(unsigned ms) { while (ms--) web_ms(); }
static void tap(unsigned button)
{
    web_buttons(1u << button); advance(40);
    web_buttons(0); advance(40);
}
static void turn(unsigned encoder, int steps) { web_enc(encoder, steps); advance(40); }
static void consent_reboot(void)
{
    web_buttons(0); web_keys(0); web_power_on(); advance(1000);
    CHECK(d8fi.phase == FI_IDLE && project_native_status() == 1);
    CHECK(!d8ls.intent && d8ls.phase == LS_IDLE);
}
static void propose_save(unsigned slot)
{
    /* All navigation and NAME acceptance are actual panel events. */
    for (unsigned i = 0; i < 4 && (ui.home || cur_page()->graph != GR_SLOTS); i++) tap(B_SAVE);
    CHECK(!ui.home && cur_page()->graph == GR_SLOTS);
    turn(EN_K1, (int)slot + 1 - song.g[G_SLOT]);
    turn(EN_K4, 1); tap(B_OCTUP);
    if (ui.confirm == CF_OVR_PROJ) tap(B_OCTUP);
    CHECK(name_on() && nm.kind == NK_PROJ_SAVE && nm.slot == slot);
    unsigned invalidated = lcd_event != 0;
    tap(B_OCTUP);
    CHECK(!name_on() && d8ls.object == slot);
    CHECK(d8ls.phase == (invalidated ? LS_IDLE : LS_CONSENT));
}
static void snapshot(void) { memcpy(before_nor, nor, sizeof nor); }
static void unchanged(unsigned writes)
{
    CHECK(web_flash_writes == writes);
    CHECK(!memcmp(nor, before_nor, sizeof nor));
}
static unsigned visible_pixels(unsigned y, unsigned height)
{
    uint16_t bg = (uint16_t)((T_BG >> 8) | (T_BG << 8));
    unsigned n = 0;
    for (unsigned j = y; j < y + height; j++)
        for (unsigned x = 12; x < 228; x++) n += web_fb[j * 240 + x] != bg;
    return n;
}
static void capture(const char *directory, const char *theme)
{
    if (!directory) return;
    char path[1024];
    int n = snprintf(path, sizeof path, "%s/%s.rgb565", directory, theme);
    CHECK(n > 0 && (unsigned)n < sizeof path);
    if (n <= 0 || (unsigned)n >= sizeof path) return;
    FILE *f = fopen(path, "wb"); CHECK(f != NULL);
    if (!f) return;
    CHECK(fwrite(web_fb, sizeof web_fb, 1, f) == 1); CHECK(!fclose(f));
}
static void disclosed_pixels(const char *directory)
{
    /* Palette assignment controls only theme, after actual UI proposal. */
    static const unsigned palettes[] = {UI_DABBL_DARK_INDEX, UI_DABBL_LIGHT_INDEX};
    static const char *names[] = {"dark", "light"};
    for (unsigned i = 0; i < 2; i++) {
        settings.palette = palettes[i]; palette_set(settings.palette);
        ui.force = 1; ui_draw();
        CHECK(d8ls.shown == d8ls.generation && d8ls.shown != 0);
        unsigned name = visible_pixels(76, AF_M.h), body = visible_pixels(122, AF_S.h);
        printf("PIXELS %s name=%u body=%u bg=%04x ink=%04x text=%04x project=%s\n",
               names[i], name, body, T_BG, T_INK, T_TEXT, d8ls.name);
        CHECK(name > 30); CHECK(body > 30);
        capture(directory, names[i]);
    }
}
static void cancelled_proposal(unsigned event)
{
    printf("CANCEL event=%u\n", event);
    unsigned writes = web_flash_writes; snapshot();
    if (event >= 3) lcd_event = event - 2;
    propose_save(1);
    if (event == 0) tap(B_HOME);
    if (event == 1) { web_keys(1); advance(40); web_keys(0); advance(40); }
    if (event == 2) turn(EN_K1, 1);
    if (event >= 3) advance(40);
    CHECK(d8ls.phase == LS_IDLE && !d8ls.intent);
    unchanged(writes);
}
int main(int argc, char **argv)
{
    web_nor_erase(); web_boot(); advance(1000);
    CHECK(d8fi.phase == FI_REQUIRED && d8fi.shown && !web_flash_writes);
    web_buttons(1u << B_SAVE); advance(40);
    CHECK(d8fi.phase == FI_RELEASE && !web_flash_writes);
    web_buttons(0);
    for (unsigned i = 0; i < 30000 && d8fi.phase != FI_IDLE && d8fi.phase != FI_FAILED; i++) web_ms();
    CHECK(d8fi.phase == FI_IDLE && web_flash_writes);
    consent_reboot();
    if (argc > 2) {
        unsigned event = (unsigned)atoi(argv[2]); CHECK(event < 5);
        if (event < 5) cancelled_proposal(event);
        printf("Emulator actual save cancellation: %u checks %u failures\n", checks, failures);
        return failures != 0;
    }
    unsigned writes = web_flash_writes; snapshot(); propose_save(1);
    unchanged(writes);
    disclosed_pixels(argc > 1 ? argv[1] : NULL);
    unchanged(writes);
    /* SAVE down alone cannot authorize any primitive. */
    web_buttons(1u << B_SAVE); advance(40);
    CHECK(d8ls.phase == LS_RELEASE && !d8ls.intent);
    unchanged(writes);
    web_buttons(0); advance(80);
    CHECK(d8ls.phase == LS_IDLE && d8ls.intent && native_as.ready);
    CHECK(web_flash_writes > writes && project_used(1) == 1 && proj_cur == 1);
    advance(AS_IDLE_MS + AS_POLL_MS + 100);
    char saved_name[13]; memcpy(saved_name, proj_name, sizeof saved_name);
    /* Same wrapper power-on, retained NOR, without host reseeding/restoration. */
    consent_reboot(); CHECK(project_used(1) == 1); CHECK(!memcmp(saved_name, proj_name, sizeof saved_name));
    printf("Emulator actual save disclosure: %u checks %u failures\n", checks, failures);
    return failures != 0;
}
