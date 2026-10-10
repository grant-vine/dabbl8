/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_D8P1_H
#define DABBL8_D8P1_H
#include <stddef.h>
#include <stdint.h>
#define D8P1_LIMIT 7936u
#define D8P1_MAX_FILE 7705u
#define D8P1_CHUNKS 8u
#define D8P1_REQUIRED 0x8000u
/* Borrowed payload views. Original bytes must remain alive and immutable. */
typedef struct { const uint8_t *data; uint16_t type, length; } d8p1_chunk;
typedef struct {
    const uint8_t *original;
    size_t length;
    uint8_t count, readonly;
    d8p1_chunk chunk[D8P1_CHUNKS];
} d8p1_view;
uint32_t d8p1_crc32(const void *data, size_t length);
/* 0 refuses without changing *out; 1 known writable format; 2 unknown optional,
 * inspect-only. Neither result adopts runtime state or authorizes flash writes. */
int d8p1_read(d8p1_view *out, const void *data, size_t length);
/* Canonical increasing chunk order. Refusal preserves output and *written.
 * Unknown optional data cannot be silently discarded by writing its view. */
int d8p1_write(uint8_t *out, size_t capacity, size_t *written, const d8p1_view *view);
/* Available project bits 0..3. No aliasing of absent referenced slots. */
int d8p1_refs_available(const d8p1_view *view, unsigned project_mask);
#endif
