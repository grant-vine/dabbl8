/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual runtime/preflight, virtual NOR, callback byte counts and late faults. */
static void scan_read_hook(unsigned,void *,unsigned);
#define D8POOL_RUNTIME_READ_HOOK scan_read_hook
#define D8POOL_RUNTIME_NO_MAIN 1
#include "d8p1_pool_runtime_test.c"
static unsigned scan_bytes,scan_at,scan_action,scan_reentered,scan_body;
static int scan_enabled;
static d8p1_project_catalog scan_catalog;
static void scan_read_hook(unsigned address,void *out,unsigned n)
{
 (void)address;(void)out;
 if(!scan_enabled)return;
 scan_bytes+=n;
 if(scan_action==7&&n>256&&!scan_reentered){
  scan_reentered=1;d8p1_project_catalog c;memset(&c,0x5a,sizeof c);d8p1_project_catalog prior=c;
  size_t before_writes=writes;uint32_t sig=123;d8pool_record record={0};
  proof(d8p1_catalog_pool(&pool,&c)==D8POOL_BUSY&&!memcmp(&c,&prior,sizeof c),"recursive catalog refuses without publication");
  proof(d8p1_load_pool(&pool,0)==D8POOL_BUSY&&d8p1_save_pool(&pool,0)==D8POOL_BUSY&&
        d8p1_rename_pool(&pool,0,"REENTER")==D8POOL_BUSY&&d8p1_autosave_snapshot_pool(&pool,&sig,&record)==D8POOL_BUSY&&
        sig==123&&writes==before_writes,"recursive preflight entrants cannot borrow, adopt or write");
 }
 if(scan_action>=8&&n>256&&++scan_body==scan_at){
  /* Modify an already copied object's header or body without using callbacks.
   * This hostile-backend negative control must not become a published catalog. */
  uint8_t *h=nor+256+(address-D8POOL_HEADER);
  if(scan_action==8){put(h+12,((uint32_t)h[12]|(uint32_t)h[13]<<8|(uint32_t)h[14]<<16|(uint32_t)h[15]<<24)+1);put(h+28,d8p1_crc32(h,28));}
  else if(scan_action==9)h[D8POOL_HEADER+100]^=1;
  else {
   /* Full fixture has an obsolete object-0 record in block 0; current
    * object 0 moved to block 4. Unknown versions/magic remain a refusal,
    * while a torn obsolete payload retains the pool's recovery policy. */
   h=nor+256;
   if(scan_action==10){h[4]=2;put(h+28,d8p1_crc32(h,28));}
   if(scan_action==11){memcpy(h,"FELU",4);put(h+28,d8p1_crc32(h,28));}
   if(scan_action==12)h[D8POOL_HEADER+100]^=1;
  }
 }
 if(reads!=scan_at)return;
 if(scan_action==1)transport_req=1;
 if(scan_action==2)backend_busy=1;
 if(scan_action==3)d8_capture_change.epoch++;
 if(scan_action==4)migration_owner=1;
 if(scan_action==5)cv_cpu_active=1;
}
static size_t scan_seed(const char *fixture_name,unsigned present)
{
 scan_enabled=0;blank();size_t n=fixture(fixture_name);
 d8p1_project_state state;proof(d8p1_project_decode(&state,wire,n,15),"decode cost fixture");
 for(unsigned b=0;b<state.arrangement.banks;b++)for(unsigned t=0;t<8;t++)if(state.arrangement.bank[b].project[t]==3)state.arrangement.bank[b].project[t]=2;
 proof(d8p1_project_encode(wire,sizeof wire,&n,&state),"encode explicit test-only three-slot maximum remap");
 for(unsigned o=0;o<4;o++)if(present&(1u<<o))proof(!d8pool_save(&pool,o,wire,n,7),"seed selected shared pool");
 if(present==15)proof(!d8pool_save(&pool,0,wire,n,7),"retain one valid obsolete shared-spare record");
 memcpy(baseline,nor,sizeof nor);reset();return n;
}
static void scan_reset(void)
{
 scan_enabled=0;memcpy(nor,baseline,sizeof nor);reset();scan_bytes=scan_at=scan_action=scan_reentered=scan_body=0;
 migration_owner=0;cv_cpu_active=0;d8_capture_change.epoch=0;scan_enabled=1;
}
static void cost_case(const char *name,unsigned present)
{
 size_t n=scan_seed(name,present);scan_reset();snapshot();
 proof(!d8p1_catalog_pool(&pool,&scan_catalog)&&scan_catalog.present==(present&7u)&&unchanged()&&!writes,"catalog cost preserves music and inventory");
 unsigned catalog_calls=reads,catalog_bytes=scan_bytes;
 uint32_t signature=0;d8pool_record record;
 scan_reset();snapshot();
 if(present&8u)proof(!d8p1_autosave_snapshot_pool(&pool,&signature,&record)&&unchanged()&&!writes,"autosave snapshot cost preserves music");
 printf("cost fixture=%s present=%u bytes=%zu catalog_calls=%u catalog_bytes=%u snapshot_calls=%u snapshot_bytes=%u\n",name,present,n,catalog_calls,catalog_bytes,reads,scan_bytes);
}
static void faults(void)
{
 scan_seed("tests/fixtures/d8p1/minimal.d8p",15);scan_reset();snapshot();
 proof(!d8p1_catalog_pool(&pool,&scan_catalog),"count candidate read boundaries");unsigned total=reads;d8p1_project_catalog prior=scan_catalog;
 /* Every backend read may fail. Output and active musical state stay intact. */
 for(unsigned at=0;at<total;at++){
  scan_reset();read_error=(int)at;snapshot();
  proof(d8p1_catalog_pool(&pool,&scan_catalog)==D8POOL_IO&&!memcmp(&scan_catalog,&prior,sizeof prior)&&unchanged()&&!writes,"every read failure preserves catalog and active state");
 }
 for(unsigned action=1;action<=5;action++)for(unsigned at=1;at<=total;at++){
  scan_reset();scan_at=at;scan_action=action;snapshot();
  proof(d8p1_catalog_pool(&pool,&scan_catalog)==D8POOL_BUSY&&!memcmp(&scan_catalog,&prior,sizeof prior)&&unchanged()&&!writes,"every read-boundary late transport/backend/epoch/owner/canvas change refuses");
 }
 for(unsigned action=8;action<=9;action++)for(unsigned body=1;body<=4;body++){
  scan_reset();scan_at=body;scan_action=action;snapshot();int rc=d8p1_catalog_pool(&pool,&scan_catalog);
  proof(rc!=D8POOL_OK&&!memcmp(&scan_catalog,&prior,sizeof prior)&&unchanged()&&!writes,"changed already copied object cannot publish catalog");
 }
 for(unsigned action=10;action<=12;action++){
  scan_reset();scan_at=4;scan_action=action;snapshot();int rc=d8p1_catalog_pool(&pool,&scan_catalog);
  proof((action==12?rc==D8POOL_OK:rc==D8POOL_UNSUPPORTED)&&!memcmp(&scan_catalog,&prior,sizeof prior)&&unchanged()&&!writes,
        "late obsolete spare: foreign/future refuses, torn obsolete payload retains safe recovery policy");
 }
 scan_reset();scan_action=7;snapshot();proof(!d8p1_catalog_pool(&pool,&scan_catalog)&&scan_reentered&&unchanged()&&!writes,"outer preflight survives refused recursive operations");
 scan_reset();proof(!d8p1_catalog_pool(&pool,&scan_catalog),"all refusal paths release private preflight exclusion");
 printf("safety callback_reads=%u checks=%u failures=%u; virtual NOR only\n",total,checks,failures);
}
int main(int argc,char **argv)
{
 ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);
 cost_case("tests/fixtures/d8p1/minimal.d8p",1);
 cost_case("tests/fixtures/d8p1/minimal.d8p",15);
 cost_case("tests/fixtures/d8p1/maximum.d8p",15);
 if(argc==1||(argc==2&&!strcmp(argv[1],"--negative")))faults();else proof(argc==2&&!strcmp(argv[1],"--cost"),"only explicit cost mode skips candidate-specific safety cases");
 return !!failures;
}
