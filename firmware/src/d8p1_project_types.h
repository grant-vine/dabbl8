/* SPDX-License-Identifier: GPL-3.0-only
 * Packed musical conventions derive from Felucca:
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. */
/* Requires core.h project_t; shared by bounded main-loop staging and adapters. */
#ifndef DABBL8_D8P1_PROJECT_TYPES_H
#define DABBL8_D8P1_PROJECT_TYPES_H
#include "d8p1.h"
typedef struct {
    char name[12];
    uint8_t project[8], track[8];
} d8p1_bank_state;
typedef struct {
    char name[12];
    uint8_t bank, apply, mute, level[8];
    int8_t pan[8], transpose[8];
} d8p1_scene_state;
typedef struct {
    uint8_t banks, scenes, rows;
    d8p1_bank_state bank[4];
    d8p1_scene_state scene[16];
    struct { uint8_t scene, repeat; } row[16];
} d8p1_arrangement;
typedef struct {
    project_t project;             /* chain is empty; arrangement owns new rows */
    d8p1_arrangement arrangement;
} d8p1_project_state;

/* Input and decoded state must be disjoint during validation/publication.
 * Neither member is persistent runtime state or retained across drawing. */
typedef struct {
    d8p1_project_state state;
    uint8_t wire[D8P1_LIMIT];
} d8p1_stage_workspace;
#endif
