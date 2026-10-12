/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_PROJECT_NATIVE_FRONTEND_H
#define DABBL8_PROJECT_NATIVE_FRONTEND_H
#include "d8p1_pool_runtime.h"
/* Trusted, synchronous main-loop callbacks; context must outlive the binding.
 * Writable bindings require separately checked migration or local native intent. Boot recognition
 * uses the separately named read-only binder; it grants no write authority.
 * Callbacks validate the whole current set and must never reenter the UI. */
typedef struct {
    void *context;
    int (*catalog)(void *,d8p1_project_catalog *);
    int (*save)(void *,unsigned,const char *);
    int (*load)(void *,unsigned);
    int (*rename)(void *,unsigned,const char *);
    int (*prepare_arrangement)(void *); /* optional read-only playback preparation */
} project_native_ops;
/* A failed binding that reaches preflight retains native ownership but goes
 * offline, never falls back to stale historical RAM slots. A busy attempt
 * retains the previous binding without access. False authorization
 * never grants ownership or performs I/O. Boot recognition may latch read-only ownership, never write authorization. */
static int project_native_bind(const project_native_ops *,int completed_migration_authorized);
static int project_native_bind_readonly(const project_native_ops *);
#if NTRK == 8 && (FELUCCA_FLASH || defined(D8_NATIVE_READONLY_BOOT_TEST))
/* 0 historical container/erased, 1 native restored, negative owned without restore.
 * Historical musical schema remains the existing fallback decoder's job. */
int project_native_boot_readonly(int allowed);
#endif
int project_native_bind_flash(int completed_migration_authorized);
#endif
