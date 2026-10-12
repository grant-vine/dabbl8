/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared canonical own-pool/residue proof. Read-only, with fresh admission;
 * no cached recognition, authorization or ancillary/firmware writes. */
typedef struct { int (*admit)(void *); } d8own_guard;
static uint32_t d8own_get(const uint8_t *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint32_t d8own_crc(uint32_t c,const uint8_t *p,uint32_t n)
{ while(n--){c^=*p++;for(unsigned bit=0;bit<8;bit++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return c; }
static int d8own_read(void *c,uint32_t a,void *p,uint32_t n)
{ (void)c;return df_read(NULL,a,p,n); }
static int d8own_stopped(void *c)
{ d8own_guard *g=c;return g&&g->admit&&g->admit(NULL); }
static int d8own_noerase(void *c,uint32_t a){(void)c;(void)a;return -1;}
static int d8own_noprogram(void *c,uint32_t a,const void*p,uint32_t n){(void)c;(void)a;(void)p;(void)n;return -1;}
static int d8own_scan(d8pool_index *index,uint32_t crc[5],int (*admit)(void *))
{
 if(!index||!crc||!admit)return D8POOL_INVALID;
 d8own_guard guard={admit};
 d8pool_mapped m;d8pool_physical io={&guard,d8own_read,d8own_noerase,d8own_noprogram,d8own_stopped};
 int rc=d8pool_mapped_open_readonly(&m,&io);d8p1_stage_workspace *stage;
 if(!rc)rc=d8pr_preflight(&m.pool,index,&stage,NULL);
 uint8_t page[256],header[32];

 unsigned used=0;
 for(unsigned object=0;!rc&&object<4;object++)if(index->anchors&(1u<<object))used|=1u<<index->object[object].block;
 for(unsigned block=0;!rc&&block<5;block++) {
  uint32_t base=block*D8POOL_BLOCK,rolling=~0u;unsigned erased=1;
  for(unsigned off=0;off<D8POOL_BLOCK;off+=sizeof page) {
   if(m.pool.read(m.pool.context,base+off,page,sizeof page)){rc=D8POOL_IO;break;}
   rolling=d8own_crc(rolling,page,sizeof page);
   for(unsigned i=0;i<sizeof page;i++)if(page[i]!=255)erased=0;
  }
  crc[block]=rolling^~0u;if(rc||erased)continue;
  if(m.pool.read(m.pool.context,base,header,sizeof header)){rc=D8POOL_IO;break;}
  unsigned object=d8own_get(header+8);uint32_t length=d8own_get(header+16),sequence=d8own_get(header+12);
  unsigned valid=d8own_get(header)==D8POOL_MAGIC&&(header[4]==1||header[4]==D8POOL_TOMBSTONE_VERSION)&&header[5]==block&&!header[6]&&!header[7]&&
    object<4&&((header[4]==1&&length&&length<=D8P1_LIMIT)||(header[4]==D8POOL_TOMBSTONE_VERSION&&!length&&!d8own_get(header+20)))&&d8own_get(header+24)==~0u&&d8own_get(header+28)==d8p1_crc32(header,28);
  /* A full four-object recovered set already proves ownership of its sole
   * noncurrent shared spare. Sparse pools require positively erased absence
   * or a complete older native record: damaged residue never means empty. */
  if(!valid){if(index->anchors==15&&!(used&(1u<<block)))continue;rc=D8POOL_UNSUPPORTED;break;}
  if(!(index->anchors&(1u<<object))){rc=D8POOL_INVALID;break;}
  const d8pool_record *current=&index->object[object];
  if(block!=current->block&&(! (current->sequence-sequence)||current->sequence-sequence>=0x80000000u)){rc=D8POOL_AMBIGUOUS;break;}
  if(header[4]==D8POOL_TOMBSTONE_VERSION)continue; /* inventory proved canonical erased body */
  unsigned mask=block==current->block?index->present&7u:7u;
  d8pool_record record={.block=block,.sequence=sequence,.length=length,.crc=d8own_get(header+20)};
  rc=d8pr_copy_record(&m.pool,&record,stage->wire,mask);if(rc)break;
  if(!d8p1_project_decode(&stage->state,stage->wire,length,mask)){rc=D8POOL_INVALID;break;}
  size_t encoded=0;
  if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&encoded,&stage->state)||encoded!=length){rc=D8POOL_INVALID;break;}
  for(unsigned off=32;!rc&&off<D8POOL_BLOCK;off+=sizeof page) {
   unsigned count=D8POOL_BLOCK-off;if(count>sizeof page)count=sizeof page;
   if(m.pool.read(m.pool.context,base+off,page,count)){rc=D8POOL_IO;break;}
   for(unsigned i=0;i<count;i++) {
    unsigned pos=off+i;uint8_t expected=pos>=D8POOL_HEADER&&pos<D8POOL_HEADER+length?stage->wire[pos-D8POOL_HEADER]:255;
    if(page[i]!=expected){rc=D8POOL_INVALID;break;}
   }
  }
 }
 /* Mapped reads can report IO when a late stopped/revocation guard prevents
  * the read itself. That is BUSY, not an independently observed bad store. */
 if(rc==D8POOL_IO&&!admit(NULL))rc=D8POOL_BUSY;
 d8pool_mapped_close(&m);return rc;
}
