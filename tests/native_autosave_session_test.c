/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual native scheduler/driver/queue code; virtual NOR and synthetic time. */
static void read_inject(void);
#define D8FLASH_READ_HOOK read_inject
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include "native_output_queue_test.c"
#include "../firmware/src/project_native_flash.c"
#include "../firmware/src/native_autosave_session.c"
static unsigned change_during_read;
static void read_inject(void){if(change_during_read&&native_as.pending){change_during_read=0;trk[7].p[P_LEVEL]=113;}}
static void late_current(void){trk[7].p[P_LEVEL]=114;}
static uint32_t current_signature(void){uint32_t s=0;CHECK(d8p1_signature_runtime(&s)==D8RT_OK);return s;}
static int tick(uint32_t delta){fm1_ms+=delta;return d8p1_autosave_session_poll();}
static void idle_fixture(void)
{
 cold();blank();flash_ok=1;native_as.ready=0;project_native_reset();ui_prefs&=~PREF_RESTORE_OFF;fm1_ms=0;
 FILE*f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)exit(2);size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(!d8p1_load_runtime(wire,n,0));int32_t a[CTL*2];mix_block(a,CTL);
 for(unsigned t=0;t<8;t++){trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=0;trk[t].nheld=0;sl[t].w=sl[t].loop=0;}
 strcpy(proj_name,"POLICY");proj_cur=2;CHECK(df_automatic_quiet());n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));
 for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));
 CHECK(!project_native_bind_flash(1));reset();
}
/* Test-only valid committed NOR fixture, not a mocked sequence comparison. */
static void sequence_max_fixture(void)
{
 d8pool_record record;CHECK(!d8pool_current(&seed,3,&record));
 uint8_t *header=nor+d8pool_mapped_address(record.block);
 for(unsigned i=0;i<4;i++)header[12+i]=0xff;
 uint32_t crc=d8p1_crc32(header,28);for(unsigned i=0;i<4;i++)header[28+i]=(uint8_t)(crc>>(i*8));
 CHECK(!d8pool_current(&seed,3,&record)&&record.sequence==UINT32_MAX);
}
static void begin(void){CHECK(!d8p1_autosave_session_begin(1,0));reset();}
static void dirty(void){trk[7].p[P_LEVEL]=100;CHECK(!tick(AS_POLL_MS));CHECK(native_as.seen==current_signature());}
static void due(void){CHECK(!tick(AS_IDLE_MS));}
static void identity(void){CHECK(proj_cur==2&&!strcmp(proj_name,"POLICY"));}
#ifndef D8_AUTOSAVE_SESSION_NO_MAIN
int main(void)
{
 idle_fixture();CHECK(d8p1_autosave_session_begin(0,1)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 project_native_reset();CHECK(d8p1_autosave_session_begin(1,1)==D8POOL_UNSUPPORTED&&!reads&&!writes);CHECK(!project_native_bind_flash(1));begin();
 CHECK(!tick(AS_IDLE_MS+AS_GAP_MS)&&!writes&&!native_as.writes);identity();
 /* Poll/quiet/gap boundary policy, with actual canonical state and writer. */
 dirty();CHECK(!tick(AS_IDLE_MS-AS_POLL_MS)&&!writes);CHECK(!tick(AS_POLL_MS-1)&&!writes);CHECK(!tick(1)&&writes&&native_as.writes==1&&!native_as.pending);identity();
 unsigned first=writes;uint32_t saved=native_as.saved;CHECK(saved==current_signature());
 trk[7].p[P_LEVEL]=101;CHECK(!tick(AS_POLL_MS));CHECK(!tick(AS_IDLE_MS)&&writes==first);
 fm1_ms=native_as.last+AS_GAP_MS-AS_POLL_MS;CHECK(!d8p1_autosave_session_poll()&&writes==first);
 CHECK(!tick(AS_POLL_MS)&&writes>first&&native_as.writes==2);identity();
 /* OFF/ON and transfer hold restart a full quiet window without clearing gap. */
 idle_fixture();begin();dirty();ui_prefs|=PREF_RESTORE_OFF;CHECK(!tick(AS_IDLE_MS+AS_GAP_MS)&&!writes);
 ui_prefs&=~PREF_RESTORE_OFF;CHECK(!tick(AS_POLL_MS)&&!writes);CHECK(!tick(AS_IDLE_MS-AS_POLL_MS)&&!writes);CHECK(!tick(AS_POLL_MS)&&writes);identity();
 idle_fixture();begin();dirty();CHECK(!tick(AS_IDLE_MS-AS_POLL_MS));d8p1_autosave_session_hold();CHECK(!tick(AS_POLL_MS)&&!writes);CHECK(!tick(AS_IDLE_MS)&&writes);
 /* Failed signatures never turn invalid native selection into clean state. */
 idle_fixture();begin();uint32_t old=native_as.saved;song.sel=8;CHECK(tick(AS_POLL_MS)==D8POOL_INVALID&&!writes&&native_as.saved==old);song.sel=0;dirty();due();CHECK(native_as.writes==1);
 /* Failure before commit: reconcile prior record; retry only after30seconds. */
 idle_fixture();begin();dirty();cut=0;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&!writes&&!native_as.pending&&native_as.err);cut=-1;
 CHECK(!tick(AS_RETRY_MS-AS_POLL_MS)&&!writes);CHECK(!tick(AS_POLL_MS)&&writes&&native_as.writes==1);
 /* Postcommit read uncertainty: no physical retry until a successful rescan. */
 idle_fixture();begin();dirty();post_commit_io=1;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&writes&&native_as.pending&&!native_as.writes);
 unsigned committed=writes;post_commit_io=0;CHECK(!tick(AS_RETRY_MS-AS_POLL_MS)&&writes==committed&&native_as.pending);
 CHECK(!tick(AS_POLL_MS)&&writes==committed&&!native_as.pending&&native_as.writes==1&&!native_as.err&&native_as.saved==current_signature());identity();
 /* Persistent read failure remains uncertain; never repeats physical writes. */
 idle_fixture();begin();dirty();post_commit_io=1;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&native_as.pending);committed=writes;
 CHECK(tick(AS_RETRY_MS)==D8POOL_IO&&native_as.pending&&writes==committed);post_commit_io=0;
 CHECK(!tick(AS_RETRY_MS-AS_POLL_MS)&&native_as.pending&&writes==committed);CHECK(!tick(AS_POLL_MS)&&!native_as.pending&&native_as.writes==1&&writes==committed);
 /* Every physical erase/program failure resolves the old committed record. */
 size_t bytes=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&bytes));unsigned operations=3u+(unsigned)((bytes+255)/256);
 for(unsigned c=0;c<operations;c++){idle_fixture();begin();dirty();cut=(int)c;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&!native_as.pending&&native_as.err&&!native_as.writes);cut=-1;CHECK(!tick(AS_RETRY_MS)&&native_as.writes==1);identity();}
 /* The committed capture can differ from the signature read before save. */
 idle_fixture();begin();dirty();uint32_t presave=native_as.seen;change_during_read=1;due();CHECK(!change_during_read&&native_as.saved==current_signature()&&native_as.saved!=presave&&native_as.writes==1);identity();
 /* Current RAM can differ AFTER commit; persisted state alone becomes clean. */
 idle_fixture();begin();dirty();presave=native_as.seen;after_commit=late_current;due();after_commit=NULL;CHECK(native_as.saved==presave&&native_as.saved!=current_signature());CHECK(!tick(AS_POLL_MS)&&native_as.seen==current_signature());identity();
 /* A busy postcommit return retains uncertainty until actual activity clears. */
 idle_fixture();begin();dirty();post_commit_start=1;CHECK(tick(AS_IDLE_MS)==D8POOL_BUSY&&native_as.pending&&transport_req);unsigned count=writes;post_commit_start=0;transport_req=0;
 CHECK(!tick(AS_RETRY_MS)&&!native_as.pending&&native_as.writes==1&&writes==count);
 /* Real logical output histories and held notes defer signature/write work. */
 for(queue_kind=0;queue_kind<7;queue_kind++){idle_fixture();begin();dirty();queue_disturb();CHECK(tick(AS_IDLE_MS)==D8POOL_BUSY&&!reads&&!writes);}
 idle_fixture();begin();dirty();fm1_in.notes=1;CHECK(tick(AS_IDLE_MS)==D8POOL_BUSY&&!writes);fm1_in.notes=0;CHECK(!tick(AS_IDLE_MS)&&writes);
 /* Actual committed object3 rollover is distinct from clock rollover. */
 idle_fixture();sequence_max_fixture();begin();CHECK(native_as.sequence==UINT32_MAX);dirty();due();
 d8pool_record wrapped;uint32_t wrapped_sig=0;CHECK(!d8p1_autosave_snapshot_flash(&wrapped_sig,&wrapped,1)&&wrapped.sequence==0);
 CHECK(native_as.sequence==0&&native_as.writes==1&&!native_as.pending&&!native_as.err&&native_as.saved==wrapped_sig);identity();
 /* Unchanged MAX record after failed write cannot count as replacement. */
 idle_fixture();sequence_max_fixture();begin();dirty();cut=0;CHECK(tick(AS_IDLE_MS)==D8POOL_IO);
 CHECK(native_as.sequence==UINT32_MAX&&!native_as.pending&&native_as.err&&!native_as.writes);cut=-1;
 CHECK(!tick(AS_RETRY_MS)&&native_as.sequence==0&&native_as.writes==1&&!native_as.err);identity();
 /* Physically committed zero remains uncertain until actual readback. */
 idle_fixture();sequence_max_fixture();begin();dirty();post_commit_io=1;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&native_as.pending&&native_as.sequence==UINT32_MAX&&!native_as.writes);
 unsigned wrapped_count=writes;post_commit_io=0;CHECK(!tick(AS_RETRY_MS)&&writes==wrapped_count&&!native_as.pending&&native_as.sequence==0&&native_as.writes==1&&!native_as.err);
 CHECK(!d8p1_autosave_snapshot_flash(&wrapped_sig,&wrapped,1)&&wrapped.sequence==0&&native_as.saved==wrapped_sig);identity();
 /* Time subtraction is unsigned through wrap for poll, idle and write gap. */
 idle_fixture();fm1_ms=UINT32_MAX-5000u;begin();dirty();due();CHECK(native_as.writes==1&&fm1_ms<AS_IDLE_MS);
 trk[7].p[P_LEVEL]=101;CHECK(!tick(AS_POLL_MS));CHECK(!tick(AS_GAP_MS)&&native_as.writes==2);
 idle_fixture();fm1_ms=UINT32_MAX-15000u;begin();dirty();cut=0;CHECK(tick(AS_IDLE_MS)==D8POOL_IO&&!writes);cut=-1;CHECK(!tick(AS_RETRY_MS)&&native_as.writes==1&&fm1_ms<AS_RETRY_MS);
 /* Clean boot restores; unclean/off boot baselines current RAM, no overwrite. */
 idle_fixture();trk[7].p[P_LEVEL]=100;CHECK(!d8p1_autosave_session_begin(1,1)&&trk[7].p[P_LEVEL]!=100&&proj_cur==PROJ_NO_SLOT&&!writes);
 CHECK(!tick(AS_IDLE_MS+AS_GAP_MS)&&!writes);
 idle_fixture();trk[7].p[P_LEVEL]=100;CHECK(!d8p1_autosave_session_begin(1,0)&&trk[7].p[P_LEVEL]==100);CHECK(!tick(AS_IDLE_MS+AS_GAP_MS)&&!writes);identity();
 idle_fixture();trk[7].p[P_LEVEL]=100;ui_prefs|=PREF_RESTORE_OFF;CHECK(!d8p1_autosave_session_begin(1,1)&&trk[7].p[P_LEVEL]==100&&!writes);CHECK(!tick(AS_IDLE_MS+AS_GAP_MS)&&!writes);
 /* Empty/offline storage cannot establish a session or initialize sectors. */
 idle_fixture();blank();CHECK(project_native_bind_flash(1)!=D8POOL_OK);reset();CHECK(d8p1_autosave_session_begin(1,1)==D8POOL_UNSUPPORTED&&!reads&&!writes);
 idle_fixture();begin();d8p1_autosave_session_end();reset();CHECK(tick(AS_IDLE_MS)==D8POOL_UNSUPPORTED&&!reads&&!writes);CHECK(project_native_owns_storage());
 /* Snapshot outputs publish only on success and cannot overlap stage/caller. */
 idle_fixture();uint32_t actual=current_signature(),output=0x12345678u;d8pool_record rec;memset(&rec,0x5a,sizeof rec);d8pool_record prior=rec;
 CHECK(!d8p1_autosave_snapshot_flash(&output,&rec,1)&&output==actual&&!writes);identity();
 reset();output=0x12345678u;rec=prior;
 CHECK(d8p1_autosave_snapshot_pool(&seed,NULL,&rec)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(d8p1_autosave_snapshot_pool(&seed,&output,(d8pool_record *)&output)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(d8p1_autosave_snapshot_pool(&seed,(uint32_t *)&main_workspace,&rec)==D8POOL_INVALID&&!reads&&!writes);
 CHECK(output==0x12345678u&&!memcmp(&rec,&prior,sizeof rec));identity();
 post_commit_io=2;CHECK(d8p1_autosave_snapshot_flash(&output,&rec,1)==D8POOL_IO&&output==0x12345678u&&!memcmp(&rec,&prior,sizeof rec)&&!writes);post_commit_io=0;
 CHECK(!irq_disabled&&!dma_errors);
 printf("Native autosave session: %u checks, %u failures; virtual NOR, canonical committed reconciliation, policy/wrap/boot/late-state/actual queue guards; no production ownership grant\n",checks,failures);
 return failures!=0;
}

#endif /* D8_AUTOSAVE_SESSION_NO_MAIN */
