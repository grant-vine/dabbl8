/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_SOURCE_EQUIVALENCE_H
#define DABBL8_NATIVE_SOURCE_EQUIVALENCE_H
/* Private stopped main-loop read-only component. Not a permission or retained
 * originals receipt. Each step performs at most one physical read <=256 B.
 * CURRENT is an explicit policy choice, never fallback from erased PERSISTED. */
typedef struct { void *context; int (*read)(void *,uint32_t,void *,uint32_t); int (*valid)(void *,uint32_t); } d8sv_io;
enum { D8SV_PERSISTED, D8SV_CURRENT };
enum { D8SV_MORE, D8SV_COMPLETE, D8SV_BAD, D8SV_STALE, D8SV_IO, D8SV_CHANGED, D8SV_UNSUPPORTED };
static int d8sv_begin(uint32_t generation,const d8sv_io *,const uint32_t source_crc[5],unsigned choice);
static int d8sv_step(void);
static int d8sv_completed(uint32_t generation);
static void d8sv_cancel(void);
#endif
