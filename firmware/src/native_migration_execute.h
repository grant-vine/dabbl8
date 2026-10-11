/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_MIGRATION_EXECUTE_H
#define DABBL8_NATIVE_MIGRATION_EXECUTE_H
#include <stdint.h>
/* Internal stopped main-loop coordinator component, not a protocol grant.
 * Physical callbacks must enforce private authority and stopped/session checks
 * inside each actual driver IRQ boundary. No callback may reenter the arena.
 * Original CRCs require independently retained external originals and source
 * conversion equivalence; CRCs themselves prove neither retention nor consent.
 * One step performs at most one <=256B read/program or one 4KiB erase.
 * Whole migration is not power atomic; uncertain writes are read back.
 * Completion requires later deliberate live adoption and native binding. */
typedef struct {
 void *context;
 int (*read)(void *,uint32_t,void *,uint32_t);
 int (*erase)(void *,uint32_t);
 int (*program)(void *,uint32_t,const void *,uint32_t);
 int (*permit)(void *,uint32_t); /* private lifetime/authority, not recognition */
} d8mx_io;
enum { D8MX_MORE, D8MX_COMPLETE, D8MX_BAD, D8MX_STALE, D8MX_IO, D8MX_CHANGED, D8MX_REVOKED };
static int d8mx_begin(uint32_t generation,const d8mx_io *,const uint32_t originals[12]);
static int d8mx_step(void);
static void d8mx_cancel(void);
#endif
