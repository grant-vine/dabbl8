/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual NAME confirmation grants one synchronous preset-bank transaction,
 * never a general instrument, project, raw-backup or first-use write session. */
#include "native_preset_session.h"
#if NTRK == 8 && D8_PRESET_SESSION
#ifndef D8_UP_ENCODER_VALUE
#define D8_UP_ENCODER_VALUE(i) fm1_in.enc_steps[i]
#endif
#ifndef D8_UP_ERASE
#define D8_UP_ERASE(a,t) FL_FAR(fl_erase4k_ram)(a,t)
#define D8_UP_PROGRAM(a,p,n,t) FL_FAR(fl_prog_ram)(a,p,n,t)
#endif
#ifndef D8_UP_RANGE_OK
#define D8_UP_RANGE_OK(a,n) FL_STORE_OK(a,n)
#endif
static int dup_quiet(void *c)
{
 (void)c;
 if(!flash_ok||cv_cpu_active||migration_owner||transport_req||!df_automatic_quiet()||
    fm1_in.buttons||fm1_in.notes||usb.ota_req||usb.uboot_req)return 0;
 for(unsigned i=0;i<7;i++)if(D8_UP_ENCODER_VALUE(i))return 0;
#if D8_MIGRATION_SESSION
 if(d8mc_blocked())return 0;
#endif
 return 1;
}
static int dup_lifetime(void)
{ return d8up.active&&!d8up.busy&&dup_quiet(NULL)&&project_native_status()==1&&
         d8_capture_change.epoch==d8up.epoch+d8up.own&&usb.resets==d8up.usb&&
         usb.config==d8up.usb_config&&(uint32_t)(fm1_ms-d8up.started)<2000u; }
static int dup_record(const up_rec_t *r)
{
 if(!r->used){const uint8_t*p=(const uint8_t*)r;for(unsigned i=0;i<sizeof *r;i++)if(p[i])return 0;return 1;}
 if(!up_valid(r)||(r->ver!=UP_VER&&r->ver!=UP_VER_GRID)||r->np>P_COUNT)return 0;
 unsigned zero=0;for(unsigned i=0;i<12;i++){unsigned c=(uint8_t)r->name[i];if(!c)zero=1;else if(zero||c<32||c>126)return 0;}
 return 1;
}
static int dup_payload(unsigned obj,const void *src,uint32_t n)
{
 if(obj==OBJ_UPRESET0||obj==OBJ_UPRESET0+1u){
  if(n!=sizeof(up_bank_t)||!src)return 0;const uint8_t*p=src;
  uint32_t magic;uint16_t rsize,nslot;memcpy(&magic,p,4);memcpy(&rsize,p+4,2);memcpy(&nslot,p+6,2);
  if(magic!=UP_BANK_MAGIC||rsize!=sizeof(up_rec_t)||nslot!=UP_PER_BANK)return 0;
  for(unsigned k=0;k<UP_PER_BANK;k++){up_rec_t r;memcpy(&r,p+8+k*sizeof r,sizeof r);if(!dup_record(&r))return 0;}
  return 1;
 }
 if(obj==OBJ_UPFM6){if(n!=sizeof(upf_t)||!src)return 0;uint32_t magic;uint16_t ver,nslot;memcpy(&magic,src,4);memcpy(&ver,(const uint8_t*)src+4,2);memcpy(&nslot,(const uint8_t*)src+6,2);return magic==UPF_MAGIC&&ver==1&&nslot==UP_SLOTS;}
 return 0;
}
static int dup_existing_shapes(unsigned obj)
{
 st_hdr_t h;int cur=st_current(obj,&h);
 if(cur>=0&&(!dup_payload(obj,st_buf,h.len)||h.rsv[0]!=~0u||h.rsv[1]!=~0u))return 0;
 uint8_t page[256];uint32_t seq[2];unsigned valid=0;for(unsigned c=0;c<2;c++){
  st_hdr_t a;if(st_read(st_sector(obj,c),&a,sizeof a))return 0;
  if(a.magic==ST_MAGIC&&a.hcrc==st_crc32(&a,sizeof a-4u)&&
     (a.type!=obj||a.slot!=c||a.len!=(obj==OBJ_UPFM6?sizeof(upf_t):sizeof(up_bank_t))||a.rsv[0]!=~0u||a.rsv[1]!=~0u))return 0;
  if(!st_head(obj,c,&a)&&!st_body(obj,c,&a)){if(!dup_payload(obj,st_buf,a.len))return 0;seq[c]=a.seq;valid|=1u<<c;}
  if(cur<0)for(unsigned off=0;off<ST_SECTOR;off+=sizeof page){if(st_read(st_sector(obj,c)+off,page,sizeof page))return 0;for(unsigned i=0;i<sizeof page;i++)if(page[i]!=255)return 0;}
 }
 if(valid==3&&(! (seq[0]-seq[1])||seq[0]-seq[1]==0x80000000u))return 0;
 return 1;
}
static int dup_existing(unsigned obj)
{ return dup_existing_shapes(obj)&&dup_quiet(NULL); }
/* A record-bank generation is audible only with matching committed FM6 tags.
 * On an interrupted second-object write, retain the previous whole bank. */
static int dup_coherent_copy(unsigned bank)
{
 unsigned obj=OBJ_UPRESET0+bank;
 if(bank>=2||!dup_existing_shapes(obj))return -2;
 unsigned patch_known=dup_existing_shapes(OBJ_UPFM6);
 uint32_t tags[16]={0},used=0;st_hdr_t patch;
 if(patch_known&&st_current(OBJ_UPFM6,&patch)>=0){memcpy(&used,st_buf+8,4);used>>=16*bank;for(unsigned k=0;k<16;k++)memcpy(&tags[k],st_buf+16+(16*bank+k)*sizeof(upf_ent_t),4);}
 st_hdr_t h[2];unsigned valid=0,coherent=0;
 for(unsigned c=0;c<2;c++)if(!st_head(obj,c,&h[c])&&!st_body(obj,c,&h[c])){
  valid|=1u<<c;unsigned ok=1;
  for(unsigned k=0;k<16;k++){up_rec_t r;memcpy(&r,st_buf+8+k*sizeof r,sizeof r);if(r.used&&r.engine==ENGI_FM6&&(!(used&(1u<<k))||tags[k]!=upf_tag(&r)))ok=0;}
  if(ok)coherent|=1u<<c;
 }
 if(coherent==3)return h[1].seq-h[0].seq<0x80000000u?1:0;
 if(coherent)return coherent==1?0:1;
 if(!valid)return -1; /* both truly erased, already proved above */
 if(patch_known&&(valid==1||valid==2)){unsigned other=valid==1?1:0;uint8_t page[256];for(unsigned off=0;off<ST_SECTOR;off+=sizeof page){if(st_read(st_sector(obj,other)+off,page,sizeof page))return -2;for(unsigned i=0;i<sizeof page;i++)if(page[i]!=255)return -2;}return -1;}
 return -2;
}
static int d8up_recover_banks(void)
{
 int rc=0;for(unsigned bank=0;bank<2;bank++){
  int copy=dup_coherent_copy(bank);if(copy==-2){up_bank_check(bank,-1);rc=-1;continue;}
  if(copy<0)up_bank_check(bank,-1);
  else {st_hdr_t h;if(st_head(OBJ_UPRESET0+bank,(unsigned)copy,&h)||st_body(OBJ_UPRESET0+bank,(unsigned)copy,&h))return -1;memcpy(&up_bank[bank],st_buf,sizeof(up_bank_t));up_bank_check(bank,sizeof(up_bank_t));}
 }
 if(rc)ui_message("PRESET RECOVERY");return rc;
}
/* Idle metadata doubles as a display receipt; acquisition consumes it. */
static uint32_t d8up_display_id(unsigned op,unsigned slot)
{
 uint32_t view[5]={op,slot,ux.gen,d8_capture_change.epoch,up_gen};
 uint32_t id=fnv(2166136261u,view,sizeof view);
 if(op==1)return slot<UP_SLOTS?fnv(id,up_rec(slot),sizeof(up_rec_t)):0;
 uint32_t subject[3]={song.sel,TSEL->eng_req,nm.len};id=fnv(id,subject,sizeof subject);
 id=fnv(id,nm.s,sizeof nm.s);return fnv(id,nm.ph,sizeof nm.ph);
}
static int d8up_display_valid(unsigned op,unsigned slot)
{ return !d8up.active&&d8up.sequence==op&&d8up.offset==slot&&
         d8up.base==d8up_display_id(op,slot)&&d8up.length==D8_UP_LCD_FAULT_COUNT(); }
static void d8up_display_done(unsigned op,unsigned slot,uint32_t id,uint32_t faults)
{
 uint32_t flags=irq_save();
 if(!d8up.active&&!cv_draw_deferred&&!cv_cpu_active&&!migration_owner&&
    id==d8up_display_id(op,slot)&&faults==D8_UP_LCD_FAULT_COUNT()){
  d8up.sequence=op;d8up.offset=slot;d8up.base=id;d8up.length=faults;
 }else d8up_display_clear();irq_restore(flags);
}
static int d8up_ui_begin(unsigned op,unsigned slot,const char *name)
{
 if(d8up.active||op<1||op>3||slot>=UP_SLOTS||fm1_in.buttons||!dup_quiet(NULL)||project_native_status()!=1)return 0;
 if(op==1){if(d8up.confirm!=2||slot!=ui.confirm_trk)return 0;}
 else {
  if(d8up.confirm!=1||!name_on()||nm.slot!=slot||nm.kind!=(op==2?NK_USER_SAVE:NK_USER_RENAME)||!name)return 0;
  char expected[13]={0};unsigned first=0,last=nm.len;if(last>12)return 0;
  while(last&&nm.s[last-1]==' ')last--;while(first<last&&nm.s[first]==' ')first++;
  memcpy(expected,nm.s+first,last-first);unsigned n=0;while(n<13&&name[n])n++;if(n>12||n!=last-first||memcmp(name,expected,n))return 0;
 }
 if(!d8up_display_valid(op,slot))return -2;
 memset(&d8up,0,sizeof d8up);d8up.active=1;d8up.slot=(uint8_t)slot;d8up.operation=(uint8_t)op;
 d8up.epoch=d8_capture_change.epoch;d8up.usb=usb.resets;d8up.usb_config=usb.config;d8up.started=fm1_ms;
 d8pool_index index;uint32_t crc[5];int rc=d8own_scan(&index,crc,dup_quiet);
 if(rc||!dup_existing(OBJ_UPRESET0+slot/UP_PER_BANK)||!dup_existing(OBJ_UPFM6)||!dup_lifetime()){d8up.active=0;return -1;}
 d8up.index=index;memcpy(d8up.crc,crc,sizeof crc);return 1;
}
static int dup_driver(unsigned program,uint32_t a,const void *p,uint32_t n)
{
 uint32_t flags=irq_save(),took=0;int rc=-10;
 if(dup_lifetime()&&d8_capture_change.epoch!=UINT32_MAX&&d8up.own!=UINT32_MAX&&
    D8_UP_RANGE_OK(a,n)&&d8up.payload_crc==st_crc32(st_buf,d8up.length)){
  int ok=!program?d8up.phase==0&&a==d8up.base&&n==4096:
         d8up.phase==1&&p&&((a==d8up.base&&n==32&&d8up.offset==d8up.length)||
         (a==d8up.base+ST_PAYLOAD_OFF+d8up.offset&&p==st_buf+d8up.offset&&
          n==(d8up.length-d8up.offset>256?256:d8up.length-d8up.offset)&&n));
  if(ok&&program&&a==d8up.base){st_hdr_t h;memcpy(&h,p,sizeof h);ok=h.magic==ST_MAGIC&&h.type==d8up.object&&h.slot==(a==st_sector(d8up.object,1))&&h.seq==d8up.sequence&&h.len==d8up.length&&h.crc==d8up.payload_crc&&h.rsv[0]==~0u&&h.rsv[1]==~0u&&h.hcrc==st_crc32(&h,sizeof h-4u);}
  if(ok){d8up.busy=1;d8_capture_store_changed();d8up.own++;rc=program?D8_UP_PROGRAM(a,p,n,&took):D8_UP_ERASE(a,&took);d8up.busy=0;if(!dup_lifetime())rc=-1;if(!rc){if(!program)d8up.phase=1;else if(a==d8up.base)d8up.phase=2;else d8up.offset+=n;}}
 }
 irq_restore(flags);return rc;
}
static int dup_erase(uint32_t a){return dup_driver(0,a,NULL,4096);}
static int dup_program(uint32_t a,const void *p,uint32_t n){return dup_driver(1,a,p,n);}
static int dup_commit(unsigned obj,const void *src,uint32_t len,int to)
{
 if(!dup_lifetime()||!src||!len||len>ST_PAYLOAD_MAX||
    (d8up.operation==4?obj!=OBJ_SETTINGS:
      (obj!=OBJ_UPRESET0+d8up.slot/UP_PER_BANK&&!(obj==OBJ_UPFM6&&d8up.bank_done&&d8up.operation==2))))return -10;
 uint32_t bound_crc=st_crc32(src,len);
 d8pool_index index;uint32_t crc[5];
 if(d8own_scan(&index,crc,dup_quiet)||!d8pr_same_index(&index,&d8up.index)||memcmp(crc,d8up.crc,sizeof crc)||!dup_lifetime())return -10;
 st_hdr_t h;int cur=st_current(obj,&h);if(!dup_lifetime()||st_crc32(src,len)!=bound_crc)return -10;
 if(to<0)to=cur==0?1:0;if(to>1)return -10;
 d8up.object=(uint8_t)obj;d8up.base=st_sector(obj,(unsigned)to);d8up.sequence=(cur<0?0:h.seq)+1;
 d8up.length=len;d8up.payload_crc=bound_crc;d8up.offset=0;d8up.phase=0;
 int rc=st_save_commit(obj,src,len,to,dup_erase,dup_program);
 if(!rc&&(!dup_lifetime()||d8up.phase!=2))rc=-1;
 if(!rc)d8up.bank_done=1;return rc;
}
static int d8up_save(unsigned obj,const void *src,uint32_t len,int to)
{
 if(!dup_lifetime()||!dup_payload(obj,src,len)||
    !((!d8up.bank_done&&obj==OBJ_UPRESET0+d8up.slot/UP_PER_BANK&&src==&up_bank[d8up.slot/UP_PER_BANK])||
      (d8up.bank_done&&d8up.operation==2&&obj==OBJ_UPFM6&&src==&upf&&up_rec(d8up.slot)->engine==ENGI_FM6)))return -10;
 uint32_t bound_crc=st_crc32(src,len);
 if(obj==OBJ_UPRESET0+d8up.slot/UP_PER_BANK){int coherent=dup_coherent_copy(d8up.slot/UP_PER_BANK);if(coherent==-2)return -10;st_hdr_t h;int raw=st_current(obj,&h);to=coherent<0?(raw>=0?raw:0):1-coherent;}
 if(st_crc32(src,len)!=bound_crc)return -10;
 return dup_commit(obj,src,len,to);
}

static void d8up_ui_end(void){memset(&d8up,0,sizeof d8up);}
#endif
