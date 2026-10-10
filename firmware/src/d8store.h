/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_D8STORE_H
#define DABBL8_D8STORE_H
#include <stddef.h>
#include <stdint.h>
#define D8STORE_SECTOR 4096u
#define D8STORE_COPY 8192u
#define D8STORE_REGION 16384u
#define D8STORE_PAYLOAD 256u
enum { D8STORE_OK, D8STORE_INVALID, D8STORE_IO, D8STORE_EMPTY, D8STORE_BUSY, D8STORE_UNSUPPORTED };
/* Relative offsets ONLY. Backend must own exactly one approved 16KiB region.
 * Call synchronously in a stopped main loop; input must remain immutable.
 * No callback may reenter these APIs. No device address binding is supplied. */
typedef struct {
    void *context;
    uint32_t object;
    int (*read)(void *, uint32_t, void *, uint32_t);
    int (*erase)(void *, uint32_t);
    int (*program)(void *, uint32_t, const void *, uint32_t);
    int (*stopped)(void *);
} d8store;
typedef struct { uint32_t sequence, length, crc; unsigned copy; } d8store_record;
/* Refusals preserve *record. Any read error aborts; it never authorizes erase. */
int d8store_current(const d8store *, d8store_record *record);
/* Refusals preserve *written; destination bytes can change on read/format failure.
 * Stage into caller-owned wire, then use the separate runtime publication API. */
int d8store_load(const d8store *, void *out, size_t capacity, size_t *written,
                 unsigned available_projects);
/* Known writable D8P1 only; context verified by caller. Always overwrites the
 * other copy, erases BOTH sectors, programs bounded pages, commits header last,
 * and verifies the result. BUSY/I/O after commit can leave a valid new copy. */
int d8store_save(const d8store *, const void *data, size_t length,
                 unsigned available_projects);
#endif
