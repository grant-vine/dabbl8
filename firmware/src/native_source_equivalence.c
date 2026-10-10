/* SPDX-License-Identifier: GPL-3.0-only */
/* Include after fixed capture roles and readonly migration preflight. Never
 * changes preflight phase, plan bytes, live state, bindings or write policy. */
#include "native_source_equivalence.h"
static struct {
 d8sv_io io; d8pool_index index; st_hdr_t header[5][2];
 uint32_t generation,plan_crc,original[5],crc,offset,epoch,usb,last,live_crc,live_length;
 uint8_t phase,role,copy,present[5],selected[5],original_mask,target_mask,choice;
} d8sv __attribute__((section(".pool")));
enum { SV_IDLE,SV_START,SV_SCAN,SV_SEMANTIC,SV_COMPARE,SV_CURRENT,SV_FINAL,SV_DONE,SV_FAILED };
static void d8sv_cancel(void){memset(&d8sv,0,sizeof d8sv);}
static int d8sv_fail(int rc){d8sv.phase=SV_FAILED;return rc;}
static int d8sv_guard(void)
{
 if(!main_migration_workspace(d8sv.generation)||d8mp.generation!=d8sv.generation||d8mp.phase!=1||d8mp.received!=D8POOL_BYTES)return D8SV_STALE;
 if(!flash_ok||usb.config!=1||cv_cpu_active||transport_busy()||transport_req||seq_counting()||usb.ota_req||usb.uboot_req||usb.resets!=d8sv.usb||d8_capture_change.epoch!=d8sv.epoch||(uint32_t)(fm1_ms-d8sv.last)>EDC_TIMEOUT)return D8SV_CHANGED;
 if(!d8sv.io.valid(d8sv.io.context,d8sv.generation))return D8SV_CHANGED;
 /* Callback reentry may end/start a different lifetime. */
 if(!main_migration_workspace(d8sv.generation)||d8mp.generation!=d8sv.generation||d8mp.phase!=1||d8mp.received!=D8POOL_BYTES)return D8SV_STALE;
 if(!flash_ok||usb.config!=1||cv_cpu_active||transport_busy()||transport_req||seq_counting()||usb.ota_req||usb.uboot_req||usb.resets!=d8sv.usb||d8_capture_change.epoch!=d8sv.epoch||(uint32_t)(fm1_ms-d8sv.last)>EDC_TIMEOUT)return D8SV_CHANGED;
 return D8SV_MORE;
}
static int d8sv_plan_read(void *c,uint32_t off,void *out,uint32_t n)
{(void)c;d8mp_workspace *w=main_migration_workspace(d8sv.generation);if(!w||off>D8POOL_BYTES||n>D8POOL_BYTES-off)return -1;memcpy(out,w->plan+off,n);return 0;}
static int d8sv_plan_stopped(void *c){(void)c;return d8sv_guard()==D8SV_MORE;}
static int d8sv_canonical_plan(void)
{
 d8mp_workspace *w=main_migration_workspace(d8sv.generation);if(!w)return 0;
 d8pool p={NULL,d8sv_plan_read,d8mp_noerase,d8mp_noprogram,d8sv_plan_stopped};
 if(d8pool_inventory(&p,&d8sv.index)||!(d8sv.index.present&8u))return 0;
 unsigned used=0;
 for(unsigned o=0;o<4;o++)if(d8sv.index.present&(1u<<o)){
  const d8pool_record *r=&d8sv.index.object[o];const uint8_t *b=w->plan+r->block*D8POOL_BLOCK;size_t n=0,m=0;used|=1u<<r->block;
  if(!d8mp_erased(b+32,D8POOL_HEADER-32)||!d8mp_erased(b+D8POOL_HEADER+r->length,D8POOL_BLOCK-D8POOL_HEADER-r->length)||d8pool_load(&p,o,w->stage.wire,sizeof w->stage.wire,&n,d8sv.index.present&7u)||!d8p1_project_decode(&w->stage.state,w->stage.wire,n,d8sv.index.present&7u)||!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&m,&w->stage.state)||m!=n||memcmp(w->stage.wire,b+D8POOL_HEADER,n))return 0;
 }
 for(unsigned b=0;b<5;b++)if(!(used&(1u<<b))&&!d8mp_erased(w->plan+b*D8POOL_BLOCK,D8POOL_BLOCK))return 0;
 return 1;
}
static int d8sv_alias(const void *p,size_t n)
{return migration_alias(p,n)||d8mp_metadata_alias(p,n)||d8ps_overlap(p,n,&d8sv,sizeof d8sv);}
static int d8sv_begin(uint32_t g,const d8sv_io *io,const uint32_t source_crc[5],unsigned choice)
{
 if(d8sv.phase!=SV_IDLE||!io||!io->read||!io->valid||!source_crc||choice>D8SV_CURRENT||d8sv_alias(io,sizeof *io)||d8sv_alias(source_crc,5*sizeof *source_crc))return D8SV_BAD;
 d8sv.phase=SV_START; /* Reserve before any callback: recursive begin refuses. */
 d8sv.io=*io;d8sv.generation=g;d8sv.choice=(uint8_t)choice;memcpy(d8sv.original,source_crc,sizeof d8sv.original);
 d8sv.epoch=d8_capture_change.epoch;d8sv.usb=usb.resets;d8sv.last=fm1_ms;int rc=d8sv_guard();if(rc)return d8sv_fail(rc);
 d8mp_workspace *w=main_migration_workspace(g);d8sv.plan_crc=d8p1_crc32(w->plan,D8POOL_BYTES);
 if(!d8sv_canonical_plan())return d8sv_fail(D8SV_UNSUPPORTED);
 if((rc=d8sv_guard()))return d8sv_fail(rc);
 if(d8p1_crc32(w->plan,D8POOL_BYTES)!=d8sv.plan_crc)return d8sv_fail(D8SV_CHANGED);
 d8sv.phase=SV_SCAN;d8sv.crc=~0u;return D8SV_MORE;
}
/* Strict frozen outer/FUN framing. Non-erased torn/foreign sectors refuse;
 * no silent fallback hides an unvalidated original generation. */
static int d8sv_sector(const uint8_t *b,unsigned role,unsigned copy,st_hdr_t *h)
{
 if(d8mp_erased(b,4096))return 0;
 memcpy(h,b,sizeof *h);unsigned obj=role==4?OBJ_AUTOSAVE:OBJ_PROJECT0+role;
 if(h->magic!=ST_MAGIC||h->type!=obj||h->slot!=copy||h->len<8||h->len>ST_PAYLOAD_MAX||h->hcrc!=st_crc32(h,28)||h->crc!=st_crc32(b+256,h->len))return -1;
 const uint8_t *p=b+256;uint32_t magic=d8ps_u16(p)|((uint32_t)d8ps_u16(p+2)<<16),n=d8ps_u16(p+4)|((uint32_t)d8ps_u16(p+6)<<16);
 if(magic<PROJ_MAGIC_V1||magic>PROJ_MAGIC||n!=h->len)return -1;
 /* Actual importer in the second pass validates frozen extent/FNV semantics. */
 return 1;
}
static int d8sv_convert(unsigned role,unsigned copy,unsigned mask,int compare)
{
 d8mp_workspace *w=main_migration_workspace(d8sv.generation);st_hdr_t h;int rc=d8sv_sector(w->stage.wire,role,copy,&h);
 if(rc!=1||memcmp(&h,&d8sv.header[role][copy],sizeof h))return D8SV_CHANGED;
 d8p1_legacy_report report;if(!d8p1_legacy_stage(&w->stage.state,&report,w->stage.wire+256,h.len,mask))return D8SV_UNSUPPORTED;
 if(compare){
  size_t n=0;if(!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&n,&w->stage.state))return D8SV_UNSUPPORTED;
  unsigned object=role==4?3:role;const d8pool_record *r=&d8sv.index.object[object];
  if(!(d8sv.index.present&(1u<<object))||n!=r->length||memcmp(w->stage.wire,w->plan+r->block*D8POOL_BLOCK+D8POOL_HEADER,n))return D8SV_CHANGED;
 }
 return D8SV_MORE;
}
static int d8sv_current(void)
{
 d8mp_workspace *w=main_migration_workspace(d8sv.generation);int rc=d8sv_guard();if(rc)return rc;
 fm1_irq_off();
 if(main_migration_workspace(d8sv.generation)!=w||d8mp.phase!=1||song.playing||chain_busy()||transport_req||seq_counting()||d8_capture_change.epoch!=d8sv.epoch){fm1_irq_on();return D8SV_CHANGED;}
 project_capture(&w->stage.state.project);
 if(d8p1_runtime_cache.valid)memcpy(&w->stage.state.arrangement,&d8p1_runtime_cache.arrangement,sizeof w->stage.state.arrangement);else memset(&w->stage.state.arrangement,0,sizeof w->stage.state.arrangement);
 fm1_irq_on();if((rc=d8sv_guard()))return rc;
 size_t n=0;if(!d8p1_project_encode(w->stage.wire,sizeof w->stage.wire,&n,&w->stage.state))return D8SV_UNSUPPORTED;
 d8p1_view view;if(d8p1_read(&view,w->stage.wire,n)!=1||!d8p1_refs_available(&view,d8sv.target_mask))return D8SV_UNSUPPORTED;
 const d8pool_record *r=&d8sv.index.object[3];if(n!=r->length||memcmp(w->stage.wire,w->plan+r->block*D8POOL_BLOCK+D8POOL_HEADER,n))return D8SV_CHANGED;
 d8sv.live_length=(uint32_t)n;d8sv.live_crc=d8p1_crc32(w->stage.wire,n);return d8sv_guard();
}
static int d8sv_step(void)
{
 if(d8sv.phase==SV_IDLE||d8sv.phase==SV_FAILED)return D8SV_BAD;
 int rc=d8sv_guard();if(rc)return d8sv_fail(rc);
 if(d8sv.phase==SV_DONE)return d8sv_completed(d8sv.generation);
 d8mp_workspace *w=main_migration_workspace(d8sv.generation);
 if(d8sv.phase==SV_CURRENT){if((rc=d8sv_current()))return d8sv_fail(rc);d8sv.phase=SV_FINAL;d8sv.role=d8sv.copy=0;d8sv.offset=0;d8sv.crc=~0u;return D8SV_MORE;}
 unsigned role=d8sv.role,copy=d8sv.copy;uint32_t address=edc_roles[role].a+copy*4096u+d8sv.offset;
 rc=d8sv.io.read(d8sv.io.context,address,w->stage.wire+d8sv.offset,256);int guard=d8sv_guard();if(guard)return d8sv_fail(guard);if(rc)return d8sv_fail(D8SV_IO);
 if(d8sv.phase==SV_SCAN||d8sv.phase==SV_FINAL)d8sv.crc=edc_crc_step(d8sv.crc,w->stage.wire+d8sv.offset,256);
 d8sv.offset+=256;if(d8sv.offset<4096)return D8SV_MORE;d8sv.offset=0;
 if(d8sv.phase==SV_SCAN){st_hdr_t h;rc=d8sv_sector(w->stage.wire,role,copy,&h);if(rc<0)return d8sv_fail(D8SV_UNSUPPORTED);if(rc){d8sv.header[role][copy]=h;d8sv.present[role]|=(uint8_t)(1u<<copy);}}
 else if(d8sv.phase==SV_SEMANTIC){if(d8sv.present[role]&(1u<<copy)){rc=d8sv_convert(role,copy,d8sv.original_mask,0);if(rc)return d8sv_fail(rc);}}
 else if(d8sv.phase==SV_COMPARE){rc=d8sv_convert(role,copy,d8sv.target_mask,1);if(rc)return d8sv_fail(rc);}
 if(d8sv.phase==SV_COMPARE){
  do{d8sv.role++;}while(d8sv.role<5&&(!d8sv.present[d8sv.role]||(d8sv.role==3)||(d8sv.role==4&&d8sv.choice==D8SV_CURRENT)));
  if(d8sv.role<5){d8sv.copy=d8sv.selected[d8sv.role];return D8SV_MORE;}
  d8sv.phase=d8sv.choice==D8SV_CURRENT?SV_CURRENT:SV_FINAL;d8sv.role=d8sv.copy=0;d8sv.crc=~0u;return D8SV_MORE;
 }
 if(++d8sv.copy<2)return D8SV_MORE;d8sv.copy=0;
 if(d8sv.phase==SV_SCAN||d8sv.phase==SV_FINAL){if((d8sv.crc^~0u)!=d8sv.original[role])return d8sv_fail(D8SV_CHANGED);d8sv.crc=~0u;}
 if(++d8sv.role<5)return D8SV_MORE;d8sv.role=0;
 if(d8sv.phase==SV_SCAN){
  for(unsigned r=0;r<5;r++){
   unsigned present=d8sv.present[r];if(present==3){uint32_t delta=d8sv.header[r][1].seq-d8sv.header[r][0].seq;if(!delta||delta==0x80000000u)return d8sv_fail(D8SV_UNSUPPORTED);d8sv.selected[r]=(uint8_t)(delta<0x80000000u);}else d8sv.selected[r]=(uint8_t)(present==2);
   if(r<4&&present)d8sv.original_mask|=(uint8_t)(1u<<r);
  }
  d8sv.target_mask=d8sv.original_mask&7u;if((d8sv.index.present&7u)!=d8sv.target_mask||(!d8sv.present[4]&&d8sv.choice==D8SV_PERSISTED))return d8sv_fail(D8SV_UNSUPPORTED);
  d8sv.phase=SV_SEMANTIC;return D8SV_MORE;
 }
 if(d8sv.phase==SV_SEMANTIC){
  d8sv.phase=SV_COMPARE;while(d8sv.role<3&&!d8sv.present[d8sv.role])d8sv.role++;
  if(d8sv.role==3)d8sv.role=4;
  if(d8sv.role==4&&d8sv.choice==D8SV_CURRENT){d8sv.phase=SV_CURRENT;return D8SV_MORE;}
  d8sv.copy=d8sv.selected[d8sv.role];return D8SV_MORE;
 }
 if(d8p1_crc32(w->plan,D8POOL_BYTES)!=d8sv.plan_crc||!d8sv_canonical_plan())return d8sv_fail(D8SV_CHANGED);
 if((rc=d8sv_guard()))return d8sv_fail(rc);
 if(d8sv.choice==D8SV_CURRENT){uint32_t crc=d8sv.live_crc,n=d8sv.live_length;if((rc=d8sv_current()))return d8sv_fail(rc);if(crc!=d8sv.live_crc||n!=d8sv.live_length)return d8sv_fail(D8SV_CHANGED);}
 d8sv.phase=SV_DONE;return D8SV_COMPLETE;
}
static int d8sv_completed(uint32_t generation)
{
 if(d8sv.phase!=SV_DONE||generation!=d8sv.generation)return D8SV_BAD;
 int rc=d8sv_guard();if(rc)return d8sv_fail(rc);
 d8mp_workspace *w=main_migration_workspace(generation);if(d8p1_crc32(w->plan,D8POOL_BYTES)!=d8sv.plan_crc)return d8sv_fail(D8SV_CHANGED);
 /* CURRENT is re-captured, byte-compared with the sealed proposal and checked
  * against its completed length/CRC. No stage pointer forms a receipt. */
 if(d8sv.choice==D8SV_CURRENT){uint32_t crc=d8sv.live_crc,n=d8sv.live_length;if((rc=d8sv_current()))return d8sv_fail(rc);if(crc!=d8sv.live_crc||n!=d8sv.live_length)return d8sv_fail(D8SV_CHANGED);}
 return D8SV_COMPLETE;
}
