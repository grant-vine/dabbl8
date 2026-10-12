/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_MIGRATION_SESSION_H
#define DABBL8_NATIVE_MIGRATION_SESSION_H
/* Shipping builds connect the workflow but retain constant physical quarantine.
 * Independent historical component fixtures do not implicitly gain authority. */
#ifndef D8_MIGRATION_SESSION
#define D8_MIGRATION_SESSION 0
#endif
#if NTRK == 8 && D8_MIGRATION_SESSION
#include "native_local_session.h"
#include "native_first_use_session.h"
static struct { uint8_t locked,revoked,busy,writer,operation_revoked; } d8mc_edit;
_Static_assert(LS_IDLE==0,"idle native local phase is zero");
/* Pure current-field predicate, no cached permission or atomic snapshot. */
static int d8mc_blocked(void) { return (d8mc_edit.locked|d8ls.phase|d8ls.busy)!=0
#if D8_FIRST_USE_SESSION
 ||d8fi_blocked()
#endif
 ; }
static void d8mc_revoke(void) { d8ls_revoke();
#if D8_FIRST_USE_SESSION
 d8fi_revoke();
#endif
 if(d8mc_edit.locked){if(d8mc_edit.writer)d8mc_edit.operation_revoked=1;else d8mc_edit.revoked=1;} }
static void d8mc_input(uint32_t pressed,uint32_t notes);
static void d8mc_handle(const uint8_t *,uint32_t);
static void d8mc_poll(void);
static int d8mc_session_valid(void);
static int d8mc_native_begin(unsigned object);
static int d8mc_native_finish(int result);
static int d8mc_driver_erase(uint32_t);
static int d8mc_driver_program(uint32_t,const void *,uint32_t);
#endif
#endif
