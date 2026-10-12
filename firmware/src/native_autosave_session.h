/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_AUTOSAVE_SESSION_H
#define DABBL8_NATIVE_AUTOSAVE_SESSION_H
/* Trusted synchronous main-loop APIs; interrupts enabled, no reentrant callbacks.
 * Begin requires existing native ownership AND separately checked migration
 * or visible local native editing intent. Read-only boot does not grant it.
 * clean_boot permits restore only; disabled/unclean boot baselines current RAM
 * without writing until a later musical change. RESTORE LAST controls writes.
 * The private local committed-SAVE path instead baselines the verified stored
 * AS signature, permitting unchanged current music to synchronize after idle.
 * It requires an existing verified AS record; it never writes during begin.
 * End revokes this session only, never changes storage ownership. */
int d8p1_autosave_session_begin(int completed_migration_authorized,int clean_boot);
void d8p1_autosave_session_end(void);
void d8p1_autosave_session_hold(void);
int d8p1_autosave_session_poll(void);
#endif
