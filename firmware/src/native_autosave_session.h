/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_AUTOSAVE_SESSION_H
#define DABBL8_NATIVE_AUTOSAVE_SESSION_H
/* Trusted synchronous main-loop APIs; interrupts enabled, no reentrant callbacks.
 * Begin requires existing native ownership AND explicit completed original-
 * preserving migration authorization. No production boot caller grants it.
 * clean_boot permits restore only; disabled/unclean boot baselines current RAM
 * without writing until a later musical change. RESTORE LAST controls writes.
 * End revokes this session only, never changes storage ownership. */
int d8p1_autosave_session_begin(int completed_migration_authorized,int clean_boot);
void d8p1_autosave_session_end(void);
void d8p1_autosave_session_hold(void);
int d8p1_autosave_session_poll(void);
#endif
