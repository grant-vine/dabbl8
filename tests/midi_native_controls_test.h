/* SPDX-License-Identifier: GPL-3.0-only */
/* Added-track control integration, using the actual USB endpoint and TRS parser.
 * Basic channel 1..8 routing remains in eight_track_runtime_test.c. */
static void native_port(uint32_t src, uint32_t status, uint32_t d1, uint32_t d2)
{
    if (src == 2u) {
        um_byte((uint8_t)status); um_byte((uint8_t)d1);
        if ((status & 0xE0u) != 0xC0u) um_byte((uint8_t)d2);
    } else {
        uint8_t packet[4]={(uint8_t)(status>>4),(uint8_t)status,(uint8_t)d1,(uint8_t)d2};
        (void)ep1_take(packet,sizeof packet);
    }
    events_block(CTL);
}
static int native_controls_test(void)
{
    int bad=0;
    for(uint32_t src=1;src<=2;src++) for(uint32_t ch=4;ch<8;ch++) {
        midi_test_reset(); song.g[G_TUNE]=src==1 ? 17 : -23;
        track_t *t=&trk[ch]; int16_t saved=t->p[P_TRANS], saved_tune=song.g[G_TUNE];
        native_port(src,0x90|ch,60,100);
        native_port(src,0xB0|ch,101,0); native_port(src,0xB0|ch,100,0);
        native_port(src,0xB0|ch,6,127); native_port(src,0xB0|ch,38,127);
        native_port(src,0xE0|ch,127,127);
        bad+=check("native added-track RPN0 caps 24 semitones/99 cents, maximum bend +6397",midi_ch[ch].semis==24 && midi_ch[ch].cents==99 && midi_bend_target[ch]==6397);
        native_port(src,0xE0|ch,0,0);
        bad+=check("native added-track minimum bend -6397 preserves saved TRANS/TUNE and played note",midi_bend_target[ch]==-6397 && t->p[P_TRANS]==saved && song.g[G_TUNE]==saved_tune && gate_note(t,60));
        native_port(src,0xB0|ch,6,12); native_port(src,0xB0|ch,38,50); native_port(src,0xE0|ch,127,127);
        for(uint32_t rpn=1;rpn<=2;rpn++) {
            native_port(src,0xB0|ch,100,rpn); native_port(src,0xB0|ch,6,127); native_port(src,0xB0|ch,38,127);
            bad+=check("native unsupported tuning RPN preserves sensitivity, saved TRANS/TUNE and bend",midi_ch[ch].semis==12 && midi_ch[ch].cents==50 && midi_bend_target[ch]==3200 && t->p[P_TRANS]==saved && song.g[G_TUNE]==saved_tune);
        }
        native_port(src,0xB0|ch,99,0); native_port(src,0xB0|ch,6,127);
        bad+=check("native added-track NRPN cancels bend-sensitivity Data Entry",midi_ch[ch].semis==12 && midi_ch[ch].cents==50 && midi_ch[ch].rpn_msb==127 && midi_ch[ch].rpn_lsb==127);
        native_port(src,0xB0|ch,7,0); native_port(src,0xB0|ch,10,0);
        bad+=check("native added-track mapped LEVEL/PAN minimum bounds",t->p[P_LEVEL]==0 && t->p[P_PAN]==-64);
        native_port(src,0xB0|ch,7,127); native_port(src,0xB0|ch,10,127);
        native_port(src,0xB0|ch,1,127); native_port(src,0xB0|ch,11,0); native_port(src,0xD0|ch,127,0);
        bad+=check("native added-track mapped maximums and wheel/expression/aftertouch",t->p[P_LEVEL]==127 && t->p[P_PAN]==63 && t->mw==127 && t->ex_off==127 && t->at==127);
        native_port(src,0xB0|ch,121,0);
        bad+=check("native added-track Reset Controllers centres bend, retains sensitivity and held note",!midi_bend_target[ch] && midi_ch[ch].semis==12 && midi_ch[ch].cents==50 && t->p[P_TRANS]==saved && song.g[G_TUNE]==saved_tune && gate_note(t,60));
    }
    /* Two channel owners of track8, with bit7 retained after selection changes:
     * controller targets must follow the old owner, not just today's TSEL. */
    for(uint32_t port=1;port<=2;port++) {
        uint32_t other=3-port; midi_test_reset(); song.g[G_ROUTE]=1; song.sel=7; events_block(CTL);
        native_port(port,0x97,60,100); native_port(other,0x9F,60,100);
        bad+=check("native USB/TRS shared track8 owners retain target bit7 on both channels",midi_owners[7]==2 && midi_ch[7].targets==0x80 && midi_ch[15].targets==0x80 && gate_note(&trk[7],60));
        native_port(other,0xB7,64,127); native_port(port,0x87,60,0); song.sel=0; events_block(CTL);
        native_port(other,0xEF,127,127);
        bad+=check("native changed selection bend still reaches old track8 owner",midi_bend_target[7]==512 && midi_notes[7][60]==(8u|MIDI_PEDAL_NOTE) && midi_owners[7]==2);
        native_port(port,0xBF,123,0);
        bad+=check("native All Notes Off leaves other track8 pedal owner and bit7 intact",midi_owners[7]==1 && midi_ch[7].targets==0x80 && !midi_ch[15].targets && gate_note(&trk[7],60));
        song.g[G_ROUTE]=0; events_block(CTL);
        bad+=check("native channel-mode switch retains valid channel8 pedal owner",midi_owners[7]==1 && midi_notes[7][60]==(8u|MIDI_PEDAL_NOTE) && midi_ch[7].targets==0x80);
        native_port(other,0xB7,121,0);
        bad+=check("native channel8 Reset Controllers clears pedal, owner and target bit7",!midi_owners[7] && !midi_ch[7].targets && !midi_ch[7].pedal && !gate_note(&trk[7],60));
        song.g[G_ROUTE]=1; song.sel=7; events_block(CTL);
        native_port(port,0x97,62,100); native_port(other,0xB7,64,127); native_port(port,0x87,62,0);
        song.sel=0; events_block(CTL); native_port(other,0xB7,120,0);
        bad+=check("native track8 All Sound Off follows bit7 and kills pedal owner across selection",!midi_owners[7] && !midi_notes[7][62] && !midi_ch[7].targets && !gate_note(&trk[7],62));
    }
    return bad;
}
