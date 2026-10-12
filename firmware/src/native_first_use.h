/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DABBL8_NATIVE_FIRST_USE_H
#define DABBL8_NATIVE_FIRST_USE_H
#include "d8pool_mapped.h"
/* Pure stopped first-use engine. No boot, panel, physical permission or loader
 * wiring. Caller supplies visible destructive consent and private lifetime
 * checks through stopped; a normal upgrade must never call begin. */
enum { D8FU_FIRST_USE, D8FU_OWN, D8FU_UNCERTAIN, D8FU_IO, D8FU_BUSY,
       D8FU_BAD, D8FU_MORE, D8FU_COMPLETE };
typedef struct {
 d8pool_physical io;
 const void *wire;
 size_t length;
 uint32_t crc,verify_offset;
 unsigned sector,phase,active;
} d8fu;
/* Exact preexisting user-state sector map only. Returns zero out of range. */
uint32_t d8fu_address(unsigned sector);
/* Full five-header probe, never short-circuit past a foreign first block.
 * OWN includes valid-CRC native and future headers; UNCERTAIN protects damaged
 * or torn native signatures. Classification never authorizes erase. */
int d8fu_probe(const d8pool_physical *);
/* reset_uncertain requires separately displayed reset/restart consent. OWN is
 * always refused: preservation/own-schema-upgrade belongs to the normal path.
 * wire is canonical fresh default project 0, immutable until completion.
 * Caller serializes all storage/arena access; callbacks must not reenter. */
int d8fu_begin(d8fu *,const d8pool_physical *,const void *,size_t,int reset_uncertain);
/* Erase then verify each owned sector. Only after ALL user state is erased,
 * reuse d8pool_save payload-first/header-last to commit project 0. A final
 * uncertain write must be independently rescanned; never blindly restart. */
int d8fu_step(d8fu *);
#endif
