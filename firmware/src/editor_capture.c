/* SPDX-License-Identifier: GPL-3.0-only */
/* Version 1 fixed-role read-only instrument capture. Main-loop serialized;
 * snapshots are post-boot, not proof of pre-boot originals or recovery.
 * No raw addresses accepted; no native ownership or write authorization. */
#define EDC_RAW 12u
#define EDC_LIVE 12u
#define EDC_TOTAL 17u
#define EDC_TIMEOUT 15000u
/* Physical roles: old project pairs0..3, autosave, settings, user banks0..1,
 * noncontiguous FM6 pair, then full user sample allocations0..2. */
static const struct {uint32_t a,n,b,m;} edc_roles[EDC_RAW]={
 {0x97000,8192,0,0},{0x99000,8192,0,0},{0x9b000,8192,0,0},{0x9d000,8192,0,0},
 {0xe5000,8192,0,0},{0xfc000,8192,0,0},{0xdc000,8192,0,0},{0xde000,8192,0,0},
 {0x9f000,4096,0xfe000,4096},{0xa0000,81920,0,0},{0xb4000,81920,0,0},{0xc8000,81920,0,0}};
static struct {
 uint32_t token,usb,epoch,last,offset,rolling,live_length,crc[EDC_TOTAL];
 uint8_t active,phase,role,full;
} ed_capture __attribute__((section(".pool")));
enum {EDC_OK,EDC_BAD,EDC_BUSY,EDC_STALE,EDC_CHANGED,EDC_IO};
static int edc_stopped(void)
{ return flash_ok&&usb.config==1&&!usb.ota_req&&!usb.uboot_req&&!cv_cpu_active&&!transport_busy()&&!transport_req&&!seq_counting(); }
static int edc_valid(void)
{
 return ed_capture.active&&edc_stopped()&&ed_capture.usb==usb.resets&&
  !d8_capture_change.changed&&ed_capture.epoch==d8_capture_change.epoch&&
  (uint32_t)(fm1_ms-ed_capture.last)<=EDC_TIMEOUT;
}
static uint32_t edc_crc_step(uint32_t c,const uint8_t *p,uint32_t n)
{ while(n--){c^=*p++;for(unsigned b=0;b<8;b++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return c; }
static uint32_t edc_length(unsigned role)
{ if(role<EDC_RAW)return edc_roles[role].n+edc_roles[role].m;
 if(role==EDC_LIVE)return ed_capture.live_length;
 return role==13?sizeof(persist_t):role<16?sizeof(up_bank[0]):sizeof(upf); }
static int edc_read(unsigned role,uint32_t off,uint32_t n)
{
 if(role>=EDC_RAW)return EDC_BAD;
 uint32_t first=edc_roles[role].n,k=off<first?first-off:0;
 if(k>n)k=n;
 if(k&&st_read(edc_roles[role].a+off,ed_smp_buf,k))return EDC_IO;
 if(n>k&&st_read(edc_roles[role].b+off+k-first,ed_smp_buf+k,n-k))return EDC_IO;
 return EDC_OK;
}
static int edc_live(uint32_t off,uint32_t n,int initial)
{
 size_t length=0;int rc=d8p1_capture_runtime(main_workspace.d8p1.wire,D8P1_LIMIT,&length);
 if(rc)return rc==D8RT_BUSY?EDC_BUSY:EDC_BAD;
 uint32_t crc=st_crc32(main_workspace.d8p1.wire,(uint32_t)length);
 if(initial){ed_capture.live_length=(uint32_t)length;ed_capture.crc[EDC_LIVE]=crc;}
 else if(length!=ed_capture.live_length||crc!=ed_capture.crc[EDC_LIVE])return EDC_CHANGED;
 if(n)memcpy(ed_smp_buf,main_workspace.d8p1.wire+off,n);
 return edc_stopped()?EDC_OK:EDC_BUSY;
}
/* Use the inherited PER4 scratch, never mutate current settings while exporting.
 * A new BEGIN revokes the old legacy backup before this scratch is reused. */
static const uint8_t *edc_current(unsigned role)
{
 if(role==13){
#if FELUCCA_FLASH
  ed_bk_settings=persist_saved;
#else
  memset(&ed_bk_settings,0,sizeof ed_bk_settings);
#endif
  ed_bk_settings.magic=PERSIST_MAGIC;
  ed_bk_settings.palette=palette_to_stored(settings.palette);
  ed_bk_settings.lowcut=settings.lowcut;
  ed_bk_settings.zoom=leds_to_stored(settings.zoom,settings_leds);
  ed_bk_settings.panel=panel;ed_bk_settings.bold=hold_to_stored(ed_bk_settings.bold,settings_hold);
#ifdef FELUCCA_FAVORITES
  memcpy(&ed_bk_settings.favorites,&favorites,sizeof favorites);
#endif
  return (const uint8_t *)&ed_bk_settings;
 }
 return role<16?(const uint8_t *)&up_bank[role-14]:(const uint8_t *)&upf;
}
static int edc_ram(unsigned role,uint32_t off,uint32_t count,int initial)
{
 if(role==EDC_LIVE)return edc_live(off,count,initial);
 const uint8_t *bytes=edc_current(role);uint32_t crc=st_crc32(bytes,edc_length(role));
 if(initial)ed_capture.crc[role]=crc;
 else if(crc!=ed_capture.crc[role])return EDC_CHANGED;
 if(count)memcpy(ed_smp_buf,bytes+off,count);
 return edc_stopped()?EDC_OK:EDC_BUSY;
}
/* Common bounded reply; operation-specific descriptor/chunk follows. */
static void edc_reply(unsigned op,unsigned rc)
{ ed_b(1);ed_b(op);ed_b(rc);ed_bk_u32(ed_capture.token);ed_b(ed_capture.phase); }
static void edc_error(unsigned op,unsigned rc)
{ ed_capture.active=0;edc_reply(op,rc); }
static void ed_capture_handle(const uint8_t *a,uint32_t n)
{
 unsigned op=n>1?a[1]:127;
 /* Validate all framing first; even direct host dispatch cannot mask high bits. */
 for(uint32_t i=0;i<n;i++)if(a[i]>127){edc_error(op,EDC_BAD);return;}
 if(n<2||a[0]!=1||op>6){edc_error(op,EDC_BAD);return;}
 if(op==0){if(n!=2){edc_error(op,EDC_BAD);return;}edc_reply(op,0);ed_b(1);ed_b(EDC_RAW);ed_b(EDC_TOTAL);ed_b(0);ed_b(2);ed_b(1);return;}
 if(op==1){
  if(n!=3||a[2]>1){edc_error(op,EDC_BAD);return;}
  if(ed_capture.active&&edc_valid()){edc_reply(op,EDC_BUSY);return;}
  ed_capture.active=0;
  if(!edc_stopped()){edc_reply(op,EDC_BUSY);return;}
  ed_capture.token++;ed_capture.usb=usb.resets;ed_capture.epoch=d8_capture_change.epoch;
  d8_capture_change.changed=0;ed_capture.last=fm1_ms;ed_capture.full=a[2];
  ed_capture.phase=1;ed_capture.role=0;ed_capture.offset=0;ed_capture.rolling=0xffffffffu;
  ed_bk_valid=ed_bk_put=0;
  if(ed_capture.full)for(unsigned role=EDC_RAW;role<EDC_TOTAL;role++){
   int rc=edc_ram(role,0,0,1);if(rc){edc_reply(op,rc);return;}
  }
  ed_capture.active=1;
  if(!edc_valid()){edc_error(op,EDC_CHANGED);return;}
  autosave_hold();edc_reply(op,0);return;
 }
 if(n<7||a[6]>15||ed_bk_r32(a+2)!=ed_capture.token){edc_error(op,EDC_STALE);return;}
 if(op==6){if(n!=7){edc_error(op,EDC_BAD);return;}ed_capture.active=0;edc_reply(op,0);return;}
 if(!edc_valid()){edc_error(op,EDC_CHANGED);return;}
 if((op==2||op==5)&&n!=7){edc_error(op,EDC_BAD);return;}
 if(op==3&&(n!=8||a[7]>=(ed_capture.full?EDC_TOTAL:EDC_RAW))){edc_error(op,EDC_BAD);return;}
 if(op==4&&(n!=15||a[7]>=(ed_capture.full?EDC_TOTAL:EDC_RAW)||a[12]>15)){edc_error(op,EDC_BAD);return;}
 /* Only valid frames refresh idle time. No hold creates ownership. */
 ed_capture.last=fm1_ms;autosave_hold();
 if(op==5){
  if(ed_capture.phase!=2){edc_error(op,EDC_BAD);return;}
  ed_capture.phase=3;ed_capture.role=0;ed_capture.offset=0;ed_capture.rolling=0xffffffffu;edc_reply(op,0);return;
 }
 if(op==2){
  if(ed_capture.phase!=1&&ed_capture.phase!=3){edc_error(op,EDC_BAD);return;}
  unsigned role=ed_capture.role;uint32_t left=edc_length(role)-ed_capture.offset,k=left>256?256:left;
  int rc=edc_read(role,ed_capture.offset,k);if(rc){edc_error(op,rc);return;}
  ed_capture.rolling=edc_crc_step(ed_capture.rolling,ed_smp_buf,k);ed_capture.offset+=k;
  if(!edc_valid()){edc_error(op,EDC_CHANGED);return;}
  if(ed_capture.offset==edc_length(role)){
   uint32_t crc=~ed_capture.rolling;
   if(ed_capture.phase==1)ed_capture.crc[role]=crc;
   else if(crc!=ed_capture.crc[role]){edc_error(op,EDC_CHANGED);return;}
   ed_capture.role++;ed_capture.offset=0;ed_capture.rolling=0xffffffffu;
   if(ed_capture.role==EDC_RAW){
    if(ed_capture.full)for(unsigned role=EDC_RAW;role<EDC_TOTAL;role++){
     rc=edc_ram(role,0,0,0);if(rc){edc_error(op,rc);return;}
    }
    if(!edc_valid()){edc_error(op,EDC_CHANGED);return;}
    ed_capture.phase=ed_capture.phase==1?2:4;
   }
  }
  edc_reply(op,0);ed_b(ed_capture.role);ed_bk_u32(ed_capture.offset);return;
 }
 if(op==3){
  if(ed_capture.phase!=2&&ed_capture.phase!=4){edc_error(op,EDC_BAD);return;}
  unsigned role=a[7];edc_reply(op,0);ed_b(role);ed_bk_u32(edc_length(role));ed_bk_u32(ed_capture.crc[role]);
  ed_bk_u32(role<EDC_RAW?edc_roles[role].a:0);ed_bk_u32(role<EDC_RAW?edc_roles[role].n:0);
  ed_bk_u32(role<EDC_RAW?edc_roles[role].b:0);ed_bk_u32(role<EDC_RAW?edc_roles[role].m:0);return;
 }
 /* GET requires the initial complete manifest. Bound before any I/O or arena borrow. */
 if(ed_capture.phase!=2){edc_error(op,EDC_BAD);return;}
 unsigned role=a[7];uint32_t off=ed_bk_r32(a+8),count=(uint32_t)a[13]|(uint32_t)a[14]<<7;
 if(!count||count>256||off>edc_length(role)||count>edc_length(role)-off){edc_error(op,EDC_BAD);return;}
 int rc=role>=EDC_RAW?edc_ram(role,off,count,0):edc_read(role,off,count);
 if(rc){edc_error(op,rc);return;}
 if(!edc_valid()){edc_error(op,EDC_CHANGED);return;}
 edc_reply(op,0);ed_b(role);ed_bk_u32(off);ed_b(count&127);ed_b(count>>7);ed_bk_pack(ed_smp_buf,count);
}
