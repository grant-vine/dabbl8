/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dabbl8 contributors */
#ifndef D8_IDENTITY_H
#define D8_IDENTITY_H
#ifndef DABBL8_IDENTITY
#define DABBL8_IDENTITY 0
#endif
#if DABBL8_IDENTITY
#define APP_NAME "Dabbl8"
#define APP_TITLE "DABBL8"
#define APP_INFO_PREFIX "DABBL8 "
#define APP_CONSOLE_PREFIX "dabbl8 "
#else
#define APP_NAME "Felucca"
#define APP_TITLE "FELUCCA"
#define APP_INFO_PREFIX "FELUCCA "
#define APP_CONSOLE_PREFIX "felucca "
#endif
#endif
