/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_D8P1_POOL_RUNTIME_H
#define DABBL8_D8P1_POOL_RUNTIME_H
#include "d8pool.h"
/* Eight-track main-loop APIs. Backend must already own explicitly migrated
 * native storage; this controller never initializes a blank or legacy pool.
 * Serialize editor/backup operations and arena ownership. No callbacks may
 * reenter drawing/runtime code. Status codes are D8POOL_*.
 * Validate all current objects before capture/adoption. A postcommit I/O
 * failure can leave a new save; rescan before retry. No automatic scheduler,
 * physical backend, project-menu binding or legacy migration is provided. */
int d8p1_save_pool(const d8pool *,unsigned object);
int d8p1_load_pool(const d8pool *,unsigned object);
/* Caller supplies the boot/recovery policy and RESTORE LAST decision. */
int d8p1_restore_pool_autosave(const d8pool *,int allowed);
#endif
