/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_D8POOL_MAPPED_H
#define DABBL8_D8POOL_MAPPED_H
#include "d8pool.h"
/* Physical I/O through the existing storage driver. Main-loop serialized use;
 * callbacks must not reenter editor/runtime/backup or change this session. */
typedef struct {
    void *context;
    int (*read)(void *,uint32_t,void *,uint32_t);
    int (*erase)(void *,uint32_t);
    int (*program)(void *,uint32_t,const void *,uint32_t);
    int (*stopped)(void *);
} d8pool_physical;
typedef struct {
    d8pool pool;
    d8pool_physical physical;
    unsigned state; /* 0 revoked, 1 read-only preflight, 2 authorized session */
} d8pool_mapped;
/* Explicit migration authorization is required, never inferred from magic.
 * Every open revokes an old session, including failure. Complete inventory
 * must be native and nonempty; this does not initialize blank or legacy data.
 * Use pool only through d8pool/runtime APIs after successful open, then close.
 * Current-object semantic validation remains the runtime controller's job. */
int d8pool_mapped_open(d8pool_mapped *,const d8pool_physical *,int native_authorized);
void d8pool_mapped_close(d8pool_mapped *);
/* Existing project pairs 0..3 and historical autosave pair, not new boundaries.
 * Returns zero for an invalid index. Physical block 4 is noncontiguous. */
uint32_t d8pool_mapped_address(unsigned block);
#endif
