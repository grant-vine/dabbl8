/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_MIGRATION_PREFLIGHT_H
#define DABBL8_NATIVE_MIGRATION_PREFLIGHT_H
#include "d8pool.h"
/* Requires core project_t; as other native staging adapters. */
#include "d8p1_project_types.h"
/* Read-only staging, never a trusted completion receipt or write permission. */
typedef struct {
    d8p1_stage_workspace stage;
    uint8_t plan[D8POOL_BYTES];
} d8mp_workspace;
typedef struct {
    uint32_t generation, crc;
    d8pool_index index;
} d8mp_result;
enum { D8MP_OK, D8MP_BUSY, D8MP_BAD, D8MP_STALE, D8MP_INVALID };
static int d8mp_begin(uint32_t *generation);
static int d8mp_receive(uint32_t generation,uint32_t offset,const void *bytes,uint32_t length);
static int d8mp_validate(uint32_t generation,d8mp_result *result);
static int d8mp_end(uint32_t generation);
#endif
