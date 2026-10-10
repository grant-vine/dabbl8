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
 * project-menu binding or legacy migration is provided. */
/* Caller-owned menu cache: refresh while stopped, render without flash/arena
 * access. Refusal leaves the output unchanged; successful refresh clears
 * absent entries. No retained project-sized cache is added. */
typedef struct { char name[3][13]; uint8_t present; } d8p1_project_catalog;
int d8p1_catalog_pool(const d8pool *,d8p1_project_catalog *);
/* name points to a NUL-terminated printable ASCII name, at most 12 bytes.
 * Rename the stored snapshot, never capture unsaved live edits. Successful
 * rename updates live name only if this is the current saved identity.
 * As with save, a postcommit failure requires rescan before retry. */
int d8p1_rename_pool(const d8pool *,unsigned object,const char *name);
/* Manual slots 0..2 only. NULL name keeps the current name; empty clears it.
 * Capture live music with the proposed name in staging. Publish current slot
 * and live name only after success; failures preserve both. */
int d8p1_save_as_pool(const d8pool *,unsigned object,const char *name);
int d8p1_save_pool(const d8pool *,unsigned object);
int d8p1_load_pool(const d8pool *,unsigned object);
/* Caller supplies the boot/recovery policy and RESTORE LAST decision. */
int d8p1_restore_pool_autosave(const d8pool *,int allowed);
/* Existing-driver adapter, only with FELUCCA_FLASH in an eight-track build.
 * Explicit approved migration ownership required; no boot/menu call grants it. */
int d8p1_catalog_flash(d8p1_project_catalog *,int native_authorized);
int d8p1_rename_flash(unsigned object,const char *name,int native_authorized);
int d8p1_save_as_flash(unsigned object,const char *name,int native_authorized);
int d8p1_save_flash(unsigned object,int native_authorized);
/* Fixed shared autosave identity, explicit completed migration ownership.
 * Rechecks stopped transport, inactive canvas, released panel and silent
 * voices, pending MIDI and known performance/slicer/held-arp state before each
 * physical mutation, including under IRQ-off. Resident DSP tail state is
 * checked without scanning audio buffers. Logical DAC halves/USB ring, FIR,
 * pending packet and underrun frame are also checked. Peripheral FIFOs/codec,
 * host capture and physical timing remain caller gates, not an inaudibility
 * proof. Stalled streams or uncertain SIE state may defer indefinitely.
 * This is not a scheduler; caller must apply RESTORE LAST/idle/wear policy. */
int d8p1_autosave_flash(int native_authorized);
int d8p1_load_flash(unsigned object,int native_authorized);
int d8p1_restore_flash_autosave(int native_authorized,int allowed);
#endif
