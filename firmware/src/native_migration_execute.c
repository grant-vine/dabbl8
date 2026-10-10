/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after readonly preflight and editor fixed-role capture definitions.
 * Not wired into the editor or storage permission policy by this component. */
#include "native_migration_execute.h"
_Static_assert(EDC_RAW==12,"executor uses frozen physical capture roles");
static struct {
 d8mx_io io;
 uint32_t generation,plan_crc,original[12],offset,crc;
 uint8_t phase,role,block,sector,uncertain;
} d8mx __attribute__((section(".pool")));
enum { MX_IDLE, MX_ORIGINALS, MX_ERASE, MX_PAYLOAD, MX_HEADER, MX_VERIFY, MX_FINAL, MX_UNAFFECTED, MX_DONE, MX_FAILED };
static void d8mx_cancel(void) { memset(&d8mx,0,sizeof d8mx); }
static int d8mx_fail(int rc) { d8mx.phase=MX_FAILED;return rc; }
static int d8mx_guard(void)
{
 if(!main_migration_workspace(d8mx.generation)||d8mp.generation!=d8mx.generation||d8mp.phase!=4)return D8MX_STALE;
 if(!d8mx.io.permit(d8mx.io.context,d8mx.generation))return D8MX_REVOKED;
 /* Reentry in a trusted callback cannot resurrect a lost workspace. */
 if(!main_migration_workspace(d8mx.generation)||d8mp.generation!=d8mx.generation||d8mp.phase!=4)return D8MX_STALE;
 return D8MX_MORE;
}
static int d8mx_begin(uint32_t generation,const d8mx_io *io,const uint32_t originals[12])
{
 if(d8mx.phase!=MX_IDLE)return D8MX_BAD;
 if(!io||!originals||!io->read||!io->erase||!io->program||!io->permit||
    migration_alias(io,sizeof *io)||migration_alias(originals,12*sizeof *originals)||
    d8mp_metadata_alias(io,sizeof *io)||d8mp_metadata_alias(originals,12*sizeof *originals)||
    d8ps_overlap(io,sizeof *io,&d8mx,sizeof d8mx)||d8ps_overlap(originals,12*sizeof *originals,&d8mx,sizeof d8mx))return D8MX_BAD;
 /* Consume actual canonical validation, never a caller supplied receipt. */
 d8mp_result result;
 if(d8mp_validate(generation,&result)!=D8MP_OK)return D8MX_BAD;
 d8mx.io=*io;d8mx.generation=generation;d8mx.plan_crc=result.crc;
 memcpy(d8mx.original,originals,sizeof d8mx.original);
 int rc=d8mx_guard();if(rc)return d8mx_fail(rc);
 d8mx.phase=MX_ORIGINALS;d8mx.crc=~0u;return D8MX_MORE;
}
static uint32_t d8mx_address(unsigned role,uint32_t offset)
{
 const uint32_t n=edc_roles[role].n;
 return offset<n?edc_roles[role].a+offset:edc_roles[role].b+offset-n;
}
static uint32_t d8mx_target(void)
{ return d8pool_mapped_address(d8mx.block)+d8mx.sector*4096u; }
static int d8mx_step(void)
{
 if(d8mx.phase==MX_IDLE||d8mx.phase==MX_FAILED)return D8MX_BAD;
 int rc=d8mx_guard();if(rc)return d8mx_fail(rc);
 d8mp_workspace *w=main_migration_workspace(d8mx.generation);
 if(d8mx.phase==MX_DONE)return D8MX_COMPLETE;
 uint8_t *scratch=w->stage.wire;
 if(d8mx.phase==MX_FINAL) {
  unsigned block=d8mx.offset/D8POOL_BLOCK;uint32_t within=d8mx.offset%D8POOL_BLOCK;
  rc=d8mx.io.read(d8mx.io.context,d8pool_mapped_address(block)+within,scratch,256);
  int guard=d8mx_guard();if(guard)return d8mx_fail(guard);
  if(rc)return d8mx_fail(D8MX_IO);
  if(memcmp(scratch,w->plan+d8mx.offset,256))return d8mx_fail(D8MX_CHANGED);
  d8mx.offset+=256;
  if(d8mx.offset==D8POOL_BYTES){d8mx.phase=MX_UNAFFECTED;d8mx.role=5;d8mx.offset=0;d8mx.crc=~0u;}
  return D8MX_MORE;
 }
 if(d8mx.phase==MX_ORIGINALS||d8mx.phase==MX_UNAFFECTED) {
  unsigned role=d8mx.role;uint32_t total=edc_roles[role].n+edc_roles[role].m;
  uint32_t n=total-d8mx.offset;if(n>256)n=256;
  rc=d8mx.io.read(d8mx.io.context,d8mx_address(role,d8mx.offset),scratch,n);
  int guard=d8mx_guard();if(guard)return d8mx_fail(guard);
  if(rc)return d8mx_fail(D8MX_IO);
  d8mx.crc=edc_crc_step(d8mx.crc,scratch,n);d8mx.offset+=n;
  if(d8mx.offset==total) {
   if((d8mx.crc^~0u)!=d8mx.original[role])return d8mx_fail(D8MX_CHANGED);
   d8mx.role++;d8mx.offset=0;d8mx.crc=~0u;
   if(d8mx.role==12) {
    if(d8p1_crc32(w->plan,D8POOL_BYTES)!=d8mx.plan_crc)return d8mx_fail(D8MX_CHANGED);
    if(d8mx.phase==MX_ORIGINALS){d8mx.phase=MX_ERASE;d8mx.sector=1;}
    else {d8mx.phase=MX_DONE;return D8MX_COMPLETE;}
   }
  }
  return D8MX_MORE;
 }
 uint32_t address=d8mx_target(),logical=d8mx.block*D8POOL_BLOCK+d8mx.sector*4096u;
 const uint8_t *source=w->plan+logical;
 if(d8mx.phase==MX_ERASE) {
  rc=d8mx.io.erase(d8mx.io.context,address);
  int guard=d8mx_guard();if(guard)return d8mx_fail(guard);
  /* A failed erase is uncertain; read the exact expected sector before retry.
   * It cannot be treated as erased, even if bytes were partly modified. */
  if(rc){d8mx.uncertain=1;d8mx.phase=MX_VERIFY;d8mx.offset=0;}
  else {d8mx.phase=MX_PAYLOAD;d8mx.offset=32;}
 } else if(d8mx.phase==MX_PAYLOAD||d8mx.phase==MX_HEADER) {
  uint32_t n=d8mx.phase==MX_HEADER?32:256u-(d8mx.offset&255u);
  rc=d8mx.io.program(d8mx.io.context,address+d8mx.offset,source+d8mx.offset,n);
  int guard=d8mx_guard();if(guard)return d8mx_fail(guard);
  if(rc){d8mx.uncertain=1;d8mx.phase=MX_VERIFY;d8mx.offset=0;}
  else if(d8mx.phase==MX_HEADER){d8mx.phase=MX_VERIFY;d8mx.offset=0;}
  else {d8mx.offset+=n;if(d8mx.offset==4096){d8mx.phase=MX_HEADER;d8mx.offset=0;}}
 } else if(d8mx.phase==MX_VERIFY) {
  rc=d8mx.io.read(d8mx.io.context,address+d8mx.offset,scratch,256);
  int guard=d8mx_guard();if(guard)return d8mx_fail(guard);
  if(rc)return d8mx_fail(D8MX_IO);
  if(memcmp(scratch,source+d8mx.offset,256))return d8mx_fail(d8mx.uncertain?D8MX_IO:D8MX_CHANGED);
  d8mx.offset+=256;
  if(d8mx.offset==4096) {
   d8mx.offset=0;d8mx.uncertain=0;
   if(d8mx.sector){d8mx.sector=0;d8mx.phase=MX_ERASE;}
   else if(++d8mx.block<5){d8mx.sector=1;d8mx.phase=MX_ERASE;}
   else {d8mx.phase=MX_FINAL;d8mx.offset=0;}
  }
 } else return d8mx_fail(D8MX_BAD);
 return D8MX_MORE;
}
