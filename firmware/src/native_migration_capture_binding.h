/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_MIGRATION_CAPTURE_BINDING_H
#define DABBL8_NATIVE_MIGRATION_CAPTURE_BINDING_H
/* Private main-loop binding to the actual full capture, not retention, consent,
 * write permission or a public protocol receipt. Metadata guards do not borrow
 * the stage and cannot detect unobserved direct RAM edits after sealing. */
enum { D8CB_OK, D8CB_BAD, D8CB_BUSY, D8CB_CHANGED, D8CB_STALE };
static int d8cb_begin(uint32_t *generation);
static int d8cb_receive(uint32_t generation,uint32_t offset,const void *,uint32_t);
static int d8cb_source_begin(uint32_t generation,const d8sv_io *,unsigned choice);
static int d8cb_source_step(void);
static int d8cb_seal(uint32_t generation);
static int d8cb_metadata_valid(uint32_t generation);
static void d8cb_cancel(void);
#endif
