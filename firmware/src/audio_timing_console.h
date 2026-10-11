/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop-only paused snapshot. The CRC covers magic, ordered little-endian
 * words, and metric IDs. Report printing never runs in the audio interrupt. */
static void con_timing_crc_word(uint32_t *crc, uint32_t value)
{
    uint32_t b, bit;
    for (b = 0; b < 4u; b++) {
        *crc ^= (value >> (8u * b)) & 255u;
        for (bit = 0; bit < 8u; bit++)
            *crc = (*crc >> 1) ^ ((*crc & 1u) ? 0xEDB88320u : 0u);
    }
}
static void con_timing_word(uint32_t *crc, uint32_t value)
{
    con_putc(' ');
    con_hex(value, 8);
    con_timing_crc_word(crc, value);
}
static void con_timing_hist(uint32_t *crc, const char *name, uint32_t id,
                           const volatile struct d8_timing_histogram *h)
{
    uint32_t i;
    con_puts(name);
    con_timing_crc_word(crc, id);
    con_timing_word(crc, h->count);
    con_timing_word(crc, h->max_ticks);
    for (i = 0; i < D8_TIMING_BINS; i++)
        con_timing_word(crc, h->bins[i]);
    con_puts("\r\n");
}
static void con_timing_report(void)
{
    uint32_t crc = UINT32_MAX, generation, boots;
    if (audio_timing.enabled || !audio_timing.generation) {
        con_puts("timing report requires a stopped run\r\n");
        return;
    }
    generation = audio_timing.generation;
    boots = felucca_dbg.boots;
    con_timing_crc_word(&crc, 0x44385431u); /* D8T1 */
    con_puts("d8t1 begin");
    con_timing_word(&crc, generation);
    con_timing_word(&crc, boots);
    con_puts("\r\nd8t1 config");
    con_timing_word(&crc, NTRK);
    con_timing_word(&crc, HALF_FRAMES);
    con_timing_word(&crc, FS);
    con_timing_word(&crc, FM1_TICKS_PER_US); /* nominal, not measured calibration */
    con_timing_word(&crc, D8_TIMING_SHIFT);
    con_timing_word(&crc, D8_TIMING_BINS);
    con_puts("\r\nd8t1 state");
    con_timing_word(&crc, audio_timing.first_half);
    con_timing_word(&crc, audio_timing.last_half);
    con_timing_word(&crc, audio_timing.saturated);
    con_timing_word(&crc, audio_timing.invalid);
    con_puts("\r\n");
    con_timing_hist(&crc, "d8t1 render", 0x52u, &audio_timing.render);
    con_timing_hist(&crc, "d8t1 inclusive", 0x49u, &audio_timing.inclusive);
    if (con.stalled)
        return; /* do not close a reply after a recorded transport drop */
    con_puts("d8t1 end ");
    con_hex(generation, 8);
    con_putc(' ');
    con_hex(boots, 8);
    con_putc(' ');
    con_hex(crc ^ UINT32_MAX, 8);
    con_puts("\r\n");
}
static void con_timing(const char *p)
{
    uint32_t command = 0;
    if (con_word(&p, "start")) command = 1;
    else if (con_word(&p, "stop")) command = 2;
    else if (con_word(&p, "report")) command = 3;
    while (*p == ' ') p++;
    if (!command || *p) {
        con_puts("usage: timing start|stop|report\r\n");
        return;
    }
    if (command == 1)
        con_puts(audio_timing_start() ? "timing started\r\n" : "timing start refused\r\n");
    else if (command == 2)
        con_puts(audio_timing_stop() ? "timing stopped\r\n" : "timing not running\r\n");
    else
        con_timing_report();
}
