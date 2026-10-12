/* SPDX-License-Identifier: GPL-3.0-only */
/* Requested own settings only. Shares the synchronous ancillary ticket/st_buf
 * with own preset saves; never grants generic storage or raw backup writes. */
#if NTRK == 8 && D8_PRESET_SESSION
static int ds_panel(const panel_t *p)
{
 unsigned b=0,e=0;if(p->magic!=PANEL_MAGIC)return 0;
 for(unsigned i=0;i<NB;i++){if(p->btn[i]>=NB||(b&(1u<<p->btn[i])))return 0;b|=1u<<p->btn[i];}
 for(unsigned i=0;i<NE;i++){if(p->enc[i]>=NE||(e&(1u<<p->enc[i]))||(p->dir[i]!=1&&p->dir[i]!=-1))return 0;e|=1u<<p->enc[i];}
 return 1;
}
static int ds_payload(const void *data,uint32_t n)
{
 if(!data||n!=sizeof(persist_t))return 0;const uint8_t*p=data;uint32_t v;panel_t pan;
 memcpy(&v,p,4);if(v!=PERSIST_MAGIC)return 0;
 memcpy(&v,p+offsetof(persist_t,palette),4);if(!palette_stored_ok(v))return 0;
 memcpy(&v,p+offsetof(persist_t,lowcut),4);if(v>2)return 0;
 memcpy(&v,p+offsetof(persist_t,zoom),4);if(!leds_stored_ok(v))return 0;
 memcpy(&v,p+offsetof(persist_t,bold),4);if(!hold_stored_ok(v))return 0;
 memcpy(&pan,p+offsetof(persist_t,panel),sizeof pan);if(!ds_panel(&pan))return 0;
 memcpy(&v,p+offsetof(persist_t,favorites.filter),4);if(v>1)return 0;
 const uint8_t *f=p+offsetof(persist_t,favorites.factory)+15*32;
 if(f[27]>7||f[29]>1)return 0;
 for(unsigned i=0;i<3;i++)if(((f[28]>>(2*i))&3)==3)return 0;
 return 1;
}
static int ds_existing(st_hdr_t *current,int *copy)
{
 *copy=st_current(OBJ_SETTINGS,current);
 if(*copy>=0&&(!ds_payload(st_buf,current->len)||current->rsv[0]!=~0u||current->rsv[1]!=~0u))return D8POOL_UNSUPPORTED;
 uint8_t page[256];uint32_t sequence[2]={0};unsigned valid=0;
 for(unsigned c=0;c<2;c++) {
  st_hdr_t h;if(st_read(st_sector(OBJ_SETTINGS,c),&h,sizeof h))return D8POOL_IO;
  if(h.hcrc==st_crc32(&h,sizeof h-4u)) {
   if(h.magic!=ST_MAGIC)return D8POOL_UNSUPPORTED;
   if(h.type!=OBJ_SETTINGS||h.slot!=c||h.len!=sizeof(persist_t)||h.rsv[0]!=~0u||h.rsv[1]!=~0u)return D8POOL_UNSUPPORTED;
   if(st_body(OBJ_SETTINGS,c,&h)||!ds_payload(st_buf,h.len))return D8POOL_UNSUPPORTED;
   valid|=1u<<c;sequence[c]=h.seq;
  } else if(*copy<0)for(unsigned off=0;off<4096;off+=sizeof page) {
   if(st_read(st_sector(OBJ_SETTINGS,c)+off,page,sizeof page))return D8POOL_IO;
   for(unsigned i=0;i<sizeof page;i++)if(page[i]!=255)return D8POOL_UNSUPPORTED;
  }
 }
 if(valid==3) {
  uint32_t distance=sequence[0]-sequence[1];
  if(distance==0x80000000u)return D8POOL_AMBIGUOUS;
  if(!distance)for(unsigned off=0;off<sizeof(persist_t);off+=sizeof page) {
   unsigned n=sizeof(persist_t)-off;if(n>sizeof page)n=sizeof page;
   if(st_read(st_sector(OBJ_SETTINGS,0)+ST_PAYLOAD_OFF+off,page,n)||
      st_read(st_sector(OBJ_SETTINGS,1)+ST_PAYLOAD_OFF+off,st_buf,n))return D8POOL_IO;
   if(memcmp(page,st_buf,n))return D8POOL_AMBIGUOUS;
  }
 }
 if(*copy>=0) {
  st_hdr_t verify;if(st_head(OBJ_SETTINGS,(unsigned)*copy,&verify)||memcmp(current,&verify,sizeof verify)||st_body(OBJ_SETTINGS,(unsigned)*copy,current))return D8POOL_BUSY;
 }
 return D8POOL_OK;
}
static int d8settings_commit(const void *src,uint32_t n)
{
 /* Only the actual settings_poll request flag enters. Boot normalization and
  * read-only recognition cannot create a request or revive an active ticket. */
 if(!persist_pending||persist_pending==3||d8up.active||d8up.busy)return D8POOL_BUSY;
 if(!dup_quiet(NULL))return D8POOL_BUSY;
 if(project_native_status()!=1)return project_native_status()==2?D8POOL_UNSUPPORTED:D8POOL_BUSY;
 if(!ds_payload(src,n))return D8POOL_INVALID;
 uint32_t proposed_crc=st_crc32(src,n);
 memset(&d8up,0,sizeof d8up);d8up.active=1;d8up.operation=4;
 d8up.epoch=d8_capture_change.epoch;d8up.usb=usb.resets;d8up.usb_config=usb.config;d8up.started=fm1_ms;
 int rc=d8own_scan(&d8up.index,d8up.crc,dup_quiet);st_hdr_t h;int current=-1;
 if(!rc)rc=ds_existing(&h,&current);
 if(!rc&&(!dup_lifetime()||proposed_crc!=st_crc32(src,n)))rc=D8POOL_BUSY;
 if(!rc&&current>=0&&h.len==n&&!memcmp(st_buf,src,n)) {d8up_ui_end();return D8POOL_OK;}
 if(!rc) {
  rc=dup_commit(OBJ_SETTINGS,src,n,current==0?1:0);
  if(rc)rc=dup_lifetime()?D8POOL_IO:D8POOL_BUSY;
 }
 d8up_ui_end();return rc;
}
#endif
