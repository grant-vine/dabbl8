/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_first_use.h"
#include "d8p1.h"
#ifndef D8_FIRST_USE_AMALGAMATED
#include <string.h>
#endif
uint32_t d8fu_address(unsigned sector)
{
 if(sector<73)return 0x97000u+sector*D8POOL_SECTOR;
 if(sector<75)return 0xe5000u+(sector-73u)*D8POOL_SECTOR;
 if(sector<78)return 0xfc000u+(sector-75u)*D8POOL_SECTOR;
 return 0;
}
static uint32_t fu_get(const uint8_t *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static int fu_ready(const d8pool_physical *io)
{ return io&&io->read&&io->erase&&io->program&&io->stopped; }
int d8fu_probe(const d8pool_physical *io)
{
 if(!fu_ready(io))return D8FU_BAD;
 unsigned own=0,uncertain=0;
 for(unsigned b=0;b<D8POOL_BLOCKS;b++) {
  uint8_t h[32];
  if(!io->stopped(io->context))return D8FU_BUSY;
  if(io->read(io->context,d8pool_mapped_address(b),h,sizeof h))return D8FU_IO;
  if(!io->stopped(io->context))return D8FU_BUSY;
  unsigned erased=1,plausible=1;
  for(unsigned i=0;i<sizeof h;i++)if(h[i]!=255)erased=0;
  for(unsigned i=0;i<4;i++)if((h[i]&(uint8_t)(D8POOL_MAGIC>>(8*i)))!=(uint8_t)(D8POOL_MAGIC>>(8*i)))plausible=0;
  if(fu_get(h)==D8POOL_MAGIC&&fu_get(h+28)==d8p1_crc32(h,28))own=1;
  else if(!erased&&(fu_get(h)==D8POOL_MAGIC||plausible))uncertain=1;
 }
 return own?D8FU_OWN:uncertain?D8FU_UNCERTAIN:D8FU_FIRST_USE;
}
static int fu_overlap(const void *a,size_t an,const void *b,size_t bn)
{
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return x<=y?y-x<an:x-y<bn;
}
int d8fu_begin(d8fu *s,const d8pool_physical *io,const void *wire,size_t n,int reset_uncertain)
{
 d8p1_view v;
 if(!s||s->phase||s->active||!fu_ready(io)||!wire||!n||n>D8P1_LIMIT||
    fu_overlap(s,sizeof *s,wire,n)||fu_overlap(s,sizeof *s,io,sizeof *io)||
    d8p1_read(&v,wire,n)!=1||!d8p1_refs_available(&v,1))return D8FU_BAD;
 s->active=1;int rc=d8fu_probe(io);s->active=0;
 if(s->phase)return D8FU_BAD;
 if(rc!=D8FU_FIRST_USE&&!(rc==D8FU_UNCERTAIN&&reset_uncertain))return rc;
 *s=(d8fu){.io=*io,.wire=wire,.length=n,.crc=d8p1_crc32(wire,n),.phase=1};
 return D8FU_MORE;
}
static int fu_stop(void *c)
{ d8fu *s=c;return s->phase&&s->io.stopped(s->io.context); }
static int fu_read(void *c,uint32_t off,void *out,uint32_t n)
{
 d8fu *s=c;uint8_t *p=out;
 if(!fu_stop(c)||off>D8POOL_BYTES||n>D8POOL_BYTES-off)return -1;
 while(n) {
  unsigned b=off/D8POOL_BLOCK;uint32_t within=off%D8POOL_BLOCK,k=D8POOL_BLOCK-within;
  if(k>n)k=n;
  if(!fu_stop(c)||s->io.read(s->io.context,d8pool_mapped_address(b)+within,p,k)||!fu_stop(c))return -1;
  off+=k;p+=k;n-=k;
 }
 return 0;
}
static int fu_erase(void *c,uint32_t off)
{
 d8fu *s=c;
 if(!fu_stop(c)||off%D8POOL_SECTOR||off>D8POOL_BYTES-D8POOL_SECTOR)return -1;
 int rc=s->io.erase(s->io.context,d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK);
 return fu_stop(c)?rc:-1;
}
static int fu_program(void *c,uint32_t off,const void *data,uint32_t n)
{
 d8fu *s=c;
 if(!fu_stop(c)||!n||n>256||(off&255u)+n>256||off>D8POOL_BYTES||n>D8POOL_BYTES-off)return -1;
 int rc=s->io.program(s->io.context,d8pool_mapped_address(off/D8POOL_BLOCK)+off%D8POOL_BLOCK,data,n);
 return fu_stop(c)?rc:-1;
}
int d8fu_step(d8fu *s)
{
 if(!s||!s->phase||s->active)return D8FU_BAD;
 if(s->phase==4)return D8FU_COMPLETE;
 if(!fu_stop(s)){s->phase=0;return D8FU_BUSY;}
 s->active=1;int rc=D8FU_MORE;
 if(s->phase==1) {
  if(s->io.erase(s->io.context,d8fu_address(s->sector)))rc=D8FU_IO;
  else {s->phase=2;s->verify_offset=0;}
 } else if(s->phase==2) {
  uint8_t page[256];
  if(s->io.read(s->io.context,d8fu_address(s->sector)+s->verify_offset,page,sizeof page))rc=D8FU_IO;
  else {
   for(unsigned i=0;i<sizeof page;i++)if(page[i]!=255)rc=D8FU_IO;
   if(rc==D8FU_MORE) {
    s->verify_offset+=sizeof page;
    if(s->verify_offset==D8POOL_SECTOR){s->sector++;s->phase=s->sector==78?3:1;}
   }
  }
 } else if(s->phase==3) {
  if(d8p1_crc32(s->wire,s->length)!=s->crc)rc=D8FU_BAD;
  else {
   d8pool pool={s,fu_read,fu_erase,fu_program,fu_stop};
   rc=d8pool_save(&pool,0,s->wire,s->length,1)==D8POOL_OK?D8FU_COMPLETE:D8FU_IO;
  }
 } else rc=D8FU_BAD;
 if(!s->io.stopped(s->io.context))rc=D8FU_BUSY;
 s->active=0;
 if(rc==D8FU_COMPLETE)s->phase=4;
 else if(rc!=D8FU_MORE)s->phase=0;
 return rc;
}
