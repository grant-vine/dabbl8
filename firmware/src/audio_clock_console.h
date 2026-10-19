/* SPDX-License-Identifier: GPL-3.0-only */
/* One main-loop snapshot for host-bracketed clock comparison. No interrupt
 * masking, counter reset, recorder mutation, SFR writes or persistent state. */
static void con_timing_clock(void)
{
    uint32_t boots = felucca_dbg.boots, resets = usb.resets;
    uint32_t config = fm1_time_config(), period = fm1_time_period();
    uint32_t ticks = fm1_ticks(), crc = UINT32_MAX;
    if (boots != felucca_dbg.boots || resets != usb.resets ||
        ((config ^ fm1_time_config()) & 255u) || period != fm1_time_period()) {
        con_puts("timing clock changed\r\n");
        return;
    }
    con_timing_crc_word(&crc, 0x44384331u); /* D8C1 */
    con_puts("d8c1");
    con_timing_word(&crc, ticks);
    con_timing_word(&crc, boots);
    con_timing_word(&crc, resets);
    con_timing_word(&crc, config);
    con_timing_word(&crc, period);
    con_timing_word(&crc, FELUCCA_UAC);
    if (con.stalled) return;
    con_putc(' '); con_hex(crc ^ UINT32_MAX, 8); con_puts("\r\n");
}
