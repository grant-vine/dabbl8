/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual fixed-role capture/UI/native preflight with virtual NOR callbacks.
 * The callback permit is simulated, not a production authorization grant. */
#define D8_INSTRUMENT_CAPTURE_NO_MAIN 1
#include "instrument_capture_test.c"
#include "../firmware/src/native_migration_preflight.c"
#include "../firmware/src/native_migration_execute.c"
static uint8_t mx_proposal[D8POOL_BYTES];
static unsigned mx_calls,mx_reads,mx_erases,mx_programs,mx_mutations,mx_cut,mx_allowed,mx_late_revoke,mx_partial,mx_read_cut,mx_corrupt_final;
static uint32_t mx_generation;
static int mx_rd(void *c,uint32_t a,void *p,uint32_t n){(void)c;mx_calls++;mx_reads++;if(mx_reads==mx_read_cut)return -1;if(mx_corrupt_final&&d8mx.phase==MX_FINAL){nor[0x97000]^=1;mx_corrupt_final=0;}CHECK(n<=256&&a<=sizeof nor&&n<=sizeof nor-a);memcpy(p,nor+a,n);return 0;}
static int mx_er(void *c,uint32_t a){(void)c;mx_calls++;mx_erases++;mx_mutations++;CHECK(mx_allowed&&a%4096==0);unsigned n=mx_mutations==mx_cut&&mx_partial<4096?mx_partial:4096;memset(nor+a,255,n);if(mx_late_revoke)mx_allowed=0;return mx_mutations==mx_cut?-1:0;}
static int mx_pg(void *c,uint32_t a,const void *p,uint32_t n){(void)c;mx_calls++;mx_programs++;mx_mutations++;CHECK(mx_allowed&&n<=256&&(a&255)+n<=256);unsigned k=mx_mutations==mx_cut&&mx_partial<n?mx_partial:n;for(unsigned i=0;i<k;i++)nor[a+i]&=((const uint8_t*)p)[i];if(mx_late_revoke)mx_allowed=0;return mx_mutations==mx_cut?-1:0;}
static int mx_permit(void *c,uint32_t g){(void)c;return mx_allowed&&g==mx_generation&&!transport_busy()&&!transport_req;}
static int mx_plan_read(void *c,uint32_t a,void *p,uint32_t n){(void)c;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;memcpy(p,mx_proposal+a,n);return 0;}
static int mx_plan_erase(void *c,uint32_t a){(void)c;if(a>D8POOL_BYTES-4096)return -1;memset(mx_proposal+a,255,4096);return 0;}
static int mx_plan_prog(void *c,uint32_t a,const void *p,uint32_t n){(void)c;if(a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;for(unsigned i=0;i<n;i++)mx_proposal[a+i]&=((const uint8_t*)p)[i];return 0;}
static void mx_setup(void)
{
 fixture();project_native_reset();native_as.ready=0;d8mx_cancel();memset(mx_proposal,255,sizeof mx_proposal);
 d8p1_project_state state={0};project_capture(&state.project);strcpy(state.project.name,"MIGRATION");
 size_t n=0;CHECK(d8p1_project_encode(out,sizeof out,&n,&state));
 d8pool pool={NULL,mx_plan_read,mx_plan_erase,mx_plan_prog,stop};
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&pool,o,out,n,7));
 uint32_t originals[12];for(unsigned r=0;r<12;r++){
  uint32_t crc=~0u;for(unsigned off=0;off<edc_roles[r].n+edc_roles[r].m;off+=256)crc=edc_crc_step(crc,nor+d8mx_address(r,off),256);
  originals[r]=crc^~0u;
 }
 CHECK(!d8mp_begin(&mx_generation));for(unsigned off=0;off<D8POOL_BYTES;off+=256)CHECK(!d8mp_receive(mx_generation,off,mx_proposal+off,256));
 mx_calls=mx_reads=mx_erases=mx_programs=mx_mutations=mx_cut=mx_late_revoke=mx_read_cut=mx_corrupt_final=0;mx_partial=4096;mx_allowed=1;
 d8mx_io io={NULL,mx_rd,mx_er,mx_pg,mx_permit};CHECK(d8mx_begin(mx_generation,&io,originals)==D8MX_MORE);
}
static int mx_run(void)
{
 int rc;unsigned steps=0;
 do {unsigned calls=mx_calls;rc=d8mx_step();CHECK(mx_calls-calls<=1);CHECK(++steps<4000);}while(rc==D8MX_MORE);
 return rc;
}
static void mx_outside(void)
{
 unsigned bad=0;for(unsigned a=0;a<sizeof nor;a++)if(!((a>=0x97000&&a<0x9f000)||(a>=0xe5000&&a<0xe7000))&&nor[a]!=baseline[a])bad=1;CHECK(!bad);
}
static void mx_end(void){d8mx_cancel();CHECK(!d8mp_end(mx_generation));}
int main(void)
{
 mx_setup();CHECK(mx_run()==D8MX_COMPLETE&&mx_erases==10&&mx_programs==170);
 for(unsigned b=0;b<5;b++)CHECK(!memcmp(nor+d8pool_mapped_address(b),mx_proposal+b*8192,8192));mx_outside();mx_end();
 /* Every physical failure is either exact readback-reconciled or terminal;
  * it never grants native storage ownership or writes outside mapped pairs. */
 const unsigned partials[]={0,1,31,255,4096};
 for(unsigned cut_at=1;cut_at<=180;cut_at++)for(unsigned prefix=0;prefix<5;prefix++) {
  mx_setup();mx_cut=cut_at;mx_partial=partials[prefix];int rc=mx_run();CHECK(rc==D8MX_IO||rc==D8MX_COMPLETE);
  CHECK(mx_mutations<=180);mx_outside();CHECK(!project_native_status()&&!d8_instrument_write_allowed());mx_end();
 }
 mx_setup();mx_read_cut=1;CHECK(mx_run()==D8MX_IO&&!mx_mutations);mx_end();
 mx_setup();mx_corrupt_final=1;CHECK(mx_run()==D8MX_CHANGED&&mx_mutations==180);mx_end();
 mx_setup();nor[0xa0000]^=1;CHECK(mx_run()==D8MX_CHANGED&&!mx_mutations);mx_end();
 mx_setup();mx_allowed=0;CHECK(mx_run()==D8MX_REVOKED&&!mx_calls);mx_end();
 mx_setup();mx_late_revoke=1;CHECK(mx_run()==D8MX_REVOKED&&mx_mutations==1);mx_outside();mx_end();
 mx_setup();CHECK(!d8mp_end(mx_generation));CHECK(mx_run()==D8MX_STALE&&!mx_calls);d8mx_cancel();
 printf("Bounded native migration: %u checks, %u failures; actual canonical arena, 180 mutation cuts x 5 selected prefixes, fixed physical roles, no production grant/adoption/device qualification\n",checks,failures);
 return failures?1:0;
}
