/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual readonly arena/importer/runtime with simulated NOR; no device writes. */
#define D8_INSTRUMENT_CAPTURE_NO_MAIN 1
#include "instrument_capture_test.c"
#include "../firmware/src/native_migration_preflight.c"
#include "../firmware/src/native_source_equivalence.c"
static uint8_t sv_plan[D8POOL_BYTES],sv_before[D8POOL_BYTES],sv_raw[4096];
static unsigned sv_reads,sv_allowed,sv_fault,sv_action,sv_at;
static uint32_t sv_g,sv_crc[5];
static d8p1_project_state sv_state;
static int sv_rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;sv_reads++;CHECK(n==256&&a%256==0);if(sv_fault&&sv_reads==sv_fault)return -1;memcpy(p,nor+a,n);if(sv_action&&sv_reads==sv_at){unsigned action=sv_action;sv_action=0;if(action==1)sv_allowed=0;else if(action==2)d8_capture_store_changed();else if(action==3)usb.resets++;else if(action==4){CHECK(!d8mp_end(sv_g));uint32_t other;CHECK(!d8mp_begin(&other));}else if(action==5)nor[0x97000]^=1;else if(action==6)main_migration_workspace(sv_g)->plan[40959]^=1;else if(action==7)track[0].p[P_LEVEL]^=1;else if(action==8)fm1_ms+=EDC_TIMEOUT+1;}
 return 0;}
static int sv_valid(void *c,uint32_t g){(void)c;return sv_allowed&&g==sv_g;}
static int sv_pr(void *c,uint32_t a,void *p,uint32_t n){(void)c;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;memcpy(p,sv_plan+a,n);return 0;}
static int sv_pe(void *c,uint32_t a){(void)c;memset(sv_plan+a,255,4096);return 0;}
static int sv_pp(void *c,uint32_t a,const void *p,uint32_t n){(void)c;for(unsigned i=0;i<n;i++)sv_plan[a+i]&=((const uint8_t*)p)[i];return 0;}
static size_t sv_load(const char *path){FILE*f=fopen(path,"rb");CHECK(f!=NULL);if(!f)return 0;size_t n=fread(sv_raw,1,sizeof sv_raw,f);CHECK(fgetc(f)==EOF&&!ferror(f));fclose(f);return n;}
static void sv_record(unsigned role,unsigned copy,uint32_t seq,const void *raw,size_t n)
{uint8_t*b=nor+edc_roles[role].a+copy*4096;memset(b,255,4096);memcpy(b+256,raw,n);st_hdr_t h={ST_MAGIC,(uint16_t)(role==4?OBJ_AUTOSAVE:OBJ_PROJECT0+role),(uint16_t)copy,seq,(uint32_t)n,st_crc32(raw,n),{0,0},0};h.hcrc=st_crc32(&h,28);memcpy(b,&h,32);}
static void sv_refresh(void){for(unsigned r=0;r<5;r++)sv_crc[r]=st_crc32(nor+edc_roles[r].a,8192);}
static int sv_build(const char *path,unsigned mask,unsigned choice,int erasedauto)
{
 fixture();project_native_reset();native_as.ready=0;d8sv_cancel();sv_reads=sv_fault=sv_action=sv_at=0;sv_allowed=1;
 for(unsigned r=0;r<5;r++)memset(nor+edc_roles[r].a,255,8192);
 size_t n=sv_load(path);for(unsigned r=0;r<4;r++)if(mask&(1u<<r))sv_record(r,0,1,sv_raw,n);if(!erasedauto)sv_record(4,0,1,sv_raw,n);
 memset(sv_plan,255,sizeof sv_plan);d8pool pool={NULL,sv_pr,sv_pe,sv_pp,stop};d8p1_legacy_report report;unsigned available=mask&7u;
 int converted=d8p1_legacy_stage(&sv_state,&report,sv_raw,n,available);
 if(!converted)return 0;
 size_t z=0;CHECK(d8p1_project_encode(out,sizeof out,&z,&sv_state));for(unsigned r=0;r<3;r++)if(available&(1u<<r))CHECK(!d8pool_save(&pool,r,out,z,available));
 if(choice==D8SV_CURRENT){memset(&sv_state,0,sizeof sv_state);project_capture(&sv_state.project);CHECK(d8p1_project_encode(out,sizeof out,&z,&sv_state));}
 CHECK(!d8pool_save(&pool,3,out,z,available));sv_refresh();memcpy(sv_before,sv_plan,sizeof sv_plan);memcpy(baseline,nor,sizeof nor);
 CHECK(!d8mp_begin(&sv_g));for(unsigned off=0;off<D8POOL_BYTES;off+=256)CHECK(!d8mp_receive(sv_g,off,sv_plan+off,256));return 1;
}
static int sv_run(unsigned choice)
{d8sv_io io={NULL,sv_rd,sv_valid};int rc=d8sv_begin(sv_g,&io,sv_crc,choice);unsigned steps=0;while(rc==D8SV_MORE){unsigned reads=sv_reads;rc=d8sv_step();CHECK(sv_reads-reads<=1&&++steps<1000);}CHECK(!memcmp(main_migration_workspace(sv_g)?main_migration_workspace(sv_g)->plan:sv_plan,sv_before,sizeof sv_plan)||rc==D8SV_CHANGED||rc==D8SV_STALE);return rc;}
static void sv_end(void){uint32_t g=migration_generation;d8sv_cancel();if(migration_owner)CHECK(!d8mp_end(g));CHECK(!writes);}
int main(void)
{
 const char *base="tests/fixtures/projects/fun1.bin";
 for(unsigned choice=0;choice<2;choice++)for(unsigned mask=0;mask<16;mask++){
  CHECK(sv_build(base,mask,choice,choice==D8SV_CURRENT));CHECK(sv_run(choice)==D8SV_COMPLETE);CHECK(!memcmp(nor,baseline,sizeof nor)&&!memcmp(main_migration_workspace(sv_g)->plan,sv_before,sizeof sv_plan));CHECK(d8sv_completed(sv_g)==D8SV_COMPLETE);CHECK(d8mp.phase==1&&d8mp.received==D8POOL_BYTES);d8mp_result sealed;CHECK(d8mp_validate(sv_g,&sealed)==D8MP_OK);CHECK(d8sv_completed(sv_g)==D8SV_STALE);sv_end();
 }
 CHECK(sv_build(base,7,D8SV_PERSISTED,1));CHECK(sv_run(D8SV_PERSISTED)==D8SV_UNSUPPORTED);sv_end();
 for(unsigned action=1;action<=8;action++){
  CHECK(sv_build(base,7,D8SV_CURRENT,0));sv_action=action;sv_at=1;int rc=sv_run(D8SV_CURRENT);CHECK(rc==D8SV_CHANGED||rc==D8SV_STALE);sv_end();
 }
 CHECK(sv_build(base,7,D8SV_PERSISTED,0));sv_fault=1;CHECK(sv_run(D8SV_PERSISTED)==D8SV_IO);sv_end();
 /* All frozen FUN semantics are visited. Existing fourth-slot chains refuse
  * under the real three-manual target context rather than remapping refs. */
 const char*names[]={"fun1.bin","fun1-digital.bin","fun2.bin","fun3.bin","fun4.bin","fun4-phys-drum.bin","fun5.bin","fun6.bin","fun7.bin","fun8.bin","fun8-perc.bin","fun9.bin","fun9-digital.bin"};
 for(unsigned i=0;i<13;i++){char path[128];snprintf(path,sizeof path,"tests/fixtures/projects/%s",names[i]);if(sv_build(path,15,D8SV_PERSISTED,0)){CHECK(sv_run(D8SV_PERSISTED)==D8SV_COMPLETE);sv_end();}else{CHECK(i>=7);CHECK(!migration_owner&&!writes);}}
 CHECK(sv_build(base,7,D8SV_PERSISTED,0));sv_record(0,1,1,sv_raw,sv_load(base));sv_refresh();CHECK(sv_run(D8SV_PERSISTED)==D8SV_UNSUPPORTED);sv_end();
 CHECK(sv_build(base,7,D8SV_PERSISTED,0));sv_record(0,1,0x80000001u,sv_raw,sv_load(base));sv_refresh();CHECK(sv_run(D8SV_PERSISTED)==D8SV_UNSUPPORTED);sv_end();
 CHECK(sv_build(base,7,D8SV_PERSISTED,0));sv_record(0,1,0,sv_raw,sv_load(base));sv_refresh();CHECK(sv_run(D8SV_PERSISTED)==D8SV_COMPLETE);sv_end();
 printf("Native source equivalence: %u checks, %u failures; fixed physical originals, explicit autosave source, canonical byte equality, no retention/consent/authority/device qualification\n",checks,failures);return failures?1:0;
}
