/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dabbl8 contributors */
#define T_CDC 0
#define T_UAC 0
#define T_LAYOUT 0
#define T_ON 0
#define main upstream_usb_layout_main
#include "usb_desc_test.c"
#undef main
static int str_is(uint32_t index, const char *s) {
    const uint8_t *p; uint16_t n;
    if (!get_desc(0x0300u | index, &p, &n) || n != 2u + 2u * strlen(s) || p[0] != n || p[1] != 3) return 0;
    for (uint32_t i=0; i<strlen(s); i++) if (p[2+i*2] != (uint8_t)s[i] || p[3+i*2]) return 0;
    return 1;
}
int main(void) {
#if defined(FELUCCA_LOADER)
    check("unchanged loader product UTF-16 length/content", str_is(2, "Felucca Update"));
#elif DABBL8_IDENTITY
    check("Dabbl8 product UTF-16 length/content", str_is(2, "Dabbl8"));
    check("Dabbl manufacturer UTF-16 length/content", str_is(1, "Dabbl"));
    check("Dabbl8 display identity", !strcmp(APP_NAME,"Dabbl8") && !strcmp(APP_INFO_PREFIX,"DABBL8 "));
#else
    check("upstream product UTF-16 length/content", str_is(2, "Felucca"));
    check("upstream display identity", !strcmp(APP_NAME,"Felucca") && !strcmp(APP_INFO_PREFIX,"FELUCCA "));
#endif
#if !DABBL8_IDENTITY || defined(FELUCCA_LOADER)
    check("upstream manufacturer bytes retained", sizeof STR1 == 42 && STR1[0] == 42 && STR1[4] == 0xfc);
#endif
    const uint8_t *p; uint16_t n;
    check("unchanged test VID", get_desc(0x0100,&p,&n) && le16(p+8)==0x1209);
    check("no assigned USB ID claim", le16(p+10)==
#ifdef FELUCCA_LOADER
    2
#else
    1
#endif
    );
    return fails ? 1 : 0;
}
