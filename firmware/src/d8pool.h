/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_D8POOL_H
#define DABBL8_D8POOL_H
#include <stdint.h>
#include <stddef.h>
#define D8POOL_SECTOR 4096u
#define D8POOL_BLOCK 8192u
#define D8POOL_BLOCKS 5u
#define D8POOL_OBJECTS 4u /* projects 0..2, autosave 3 */
#define D8POOL_BYTES (D8POOL_BLOCK*D8POOL_BLOCKS)
#define D8POOL_HEADER 256u
#define D8POOL_MAGIC 0x50533844u /* D8SP */
enum { D8POOL_OK, D8POOL_INVALID, D8POOL_IO, D8POOL_EMPTY,
       D8POOL_BUSY, D8POOL_UNSUPPORTED, D8POOL_AMBIGUOUS };
/* Virtual offsets only. Backend owns an explicitly migrated new-format pool.
 * Physical block 4 is noncontiguous: do not bind a contiguous 40KiB NOR range.
 * Synchronous stopped main-loop use, immutable input, no callback reentry or
 * concurrent writers. No physical driver or automatic legacy erase provided. */
typedef struct {
    void *context;
    int (*read)(void *,uint32_t,void *,uint32_t);
    int (*erase)(void *,uint32_t);
    int (*program)(void *,uint32_t,const void *,uint32_t);
    int (*stopped)(void *);
} d8pool;
typedef struct { uint32_t sequence,length,crc; unsigned block; } d8pool_record;
typedef struct { unsigned present; d8pool_record object[D8POOL_OBJECTS]; } d8pool_index;
/* Complete streaming scan. Refusals preserve output. Foreign/future headers
 * and ambiguous generations prohibit use. No scan error authorizes erase. */
int d8pool_inventory(const d8pool *,d8pool_index *);
int d8pool_current(const d8pool *,unsigned object,d8pool_record *);
/* Caller stages wire separately before runtime adoption; errors can change
 * destination bytes but preserve written count. References need caller context. */
int d8pool_load(const d8pool *,unsigned object,void *,size_t,size_t *,unsigned available_projects);
/* Commit last into a noncurrent block; never erases a current object. Previous
 * remains until replacement commits; no permanent A/B guarantee after reuse.
 * A postcommit I/O/BUSY result may leave a valid new record: rescan on retry. */
int d8pool_save(const d8pool *,unsigned object,const void *,size_t,unsigned available_projects);
#endif
