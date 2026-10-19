/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop status only. No state mutation, ISR instrumentation or IO under
 * the existing IRQ save/restore guard. Inactive union fields are not read. */
#if DABBL8_TIMING_DIAGNOSTIC && NTRK == 8
static void con_arrangement_status(void)
{
    uint32_t word[19], crc = UINT32_MAX;
    uint32_t key = fm1__lock();
    word[0] = fm1_ticks();
    word[1] = felucca_dbg.boots;
    word[2] = usb.resets;
    word[3] = chain.native_mode == 1 ? chain.native.policy.generation : 0;
    word[4] = chain.native_mode;
    word[5] = song.playing;
    word[6] = chain.armed;
    word[7] = chain.running;
    word[8] = chain.native_mode == 1 ? chain.row : 255;
    word[9] = chain.native_mode == 1 ? chain.native.policy.scene : 255;
    word[10] = chain.native_mode == 1 ? chain.native.policy.rows : 0;
    word[11] = chain.native_mode == 1 ? chain.native.policy.valid : 0;
    word[12] = chain.native_mode == 1 ? chain.native.policy.suspended : 0;
    word[13] = chain.native_mode == 1 ? chain.native.policy.pending : 255;
    word[14] = chain.native_mode == 1 ? chain.native.policy.fill_pending : 255;
    word[15] = chain.native_mode == 1 ? chain.native.policy.fill_active : 0;
    word[16] = song.rec;
    word[17] = transport_req;
    word[18] = cin_left != 0;
    fm1__unlock(key);
    con_timing_crc_word(&crc, 0x44384131u); /* D8A1 */
    con_puts("d8a1");
    for (unsigned i = 0; i < 19; i++) con_timing_word(&crc, word[i]);
    if (con.stalled) return;
    con_putc(' '); con_hex(crc ^ UINT32_MAX, 8); con_puts("\r\n");
}
#endif
