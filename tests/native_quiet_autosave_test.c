/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual controller/driver wrappers; virtual physical NOR and IRQ injection. */
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static unsigned disturbance;
enum { ACTIVITY_KINDS=20 };
static void disturb(void)
{
    switch(disturbance) {
    case 0: fm1_in.notes=1;break;
    case 1: fm1_in.buttons=1;break;
    case 2: trk[0].v[0].active=1;break;
    case 3: trk[7].v[7].active=1;break;
    case 4: cv_cpu_active=1;break;
    case 5: transport_req=1;break;
    case 6: song.playing=1;break;
    case 7: case 8: (void)midi_enqueue(0x09u|(0x97u<<8)|(60u<<16)|(100u<<24),disturbance==7?1:2);break;
    case 9: midi_in_overflow=1;break;
    case 10: perf_held=1;break;
    case 11: perf_latched=1;break;
    case 12: perf_act=1;break;
    case 13: pf.busy=1;break;
    case 14: pf.w=1;break;
    case 15: pf.tw=1;break;
    case 16: pf.next=0;break;
    case 17: trk[7].p[P_SLCR]=SL_STUT;break;
    case 18:sl[7].w=1;break;
    default:trk[7].p[P_AMODE]=AM_UP;trk[7].nheld=1;break;
    }
}
static void calm(void)
{
    fm1_in.notes=fm1_in.buttons=0;trk[0].v[0].active=trk[7].v[7].active=0;
    cv_cpu_active=0;transport_req=0;song.playing=0;mi_r=mi_w;midi_in_overflow=0;
    perf_held=perf_latched=perf_act=0;pf.busy=0;pf.w=pf.tw=0;pf.next=PF_N;
    trk[7].p[P_SLCR]=0;sl[7].w=0;
    trk[7].p[P_AMODE]=0;trk[7].nheld=trk[7].arp_phys=trk[7].arp_note=0;
}
int main(void)
{
    ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);blank();flash_ok=1;
    FILE *f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)return 2;
    size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
    CHECK(!d8p1_load_runtime(wire,n,0));mix_block(audio,CTL);
    /* The immutable fixture has stopped STUT tracks: explicitly construct an
     * idle test state, then seed that captured state without changing it. */
    for(unsigned t=0;t<8;t++) {
        trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=0;trk[t].nheld=0;
        sl[t].w=0;sl[t].loop=0;
    }
    calm();CHECK(df_automatic_quiet());
    n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));
    for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();
    d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));memcpy(baseline,nor,sizeof nor);
    proj_cur=2;trk[7].p[P_LEVEL]=109;char name[sizeof proj_name];memcpy(name,proj_name,sizeof name);
    n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));
    unsigned operations=2+(unsigned)((n+255)/256)+1;
    CHECK(d8p1_autosave_flash(0)==D8POOL_UNSUPPORTED&&!reads&&!writes);
    flash_ok=0;CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&!reads&&!writes);flash_ok=1;
    for(disturbance=0;disturbance<ACTIVITY_KINDS;disturbance++) {
        reset();disturb();CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes&&!irq_disabled);
        calm();CHECK(proj_cur==2&&!memcmp(name,proj_name,sizeof name));
    }
    for(unsigned track=0;track<8;track++)for(unsigned voice=0;voice<8;voice++) {
        reset();trk[track].v[voice].active=1;
        CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes&&!irq_disabled);
        CHECK(proj_cur==2&&!memcmp(name,proj_name,sizeof name));trk[track].v[voice].active=0;
    }
    /* Real HOLD key-release path: a stopped arp retains a future note during
     * a gate gap even when there are no sounding voices or physical keys. */
    for(unsigned t=0;t<8;t++) {
        track_t *track=&trk[t];track->p[P_AMODE]=AM_UP;
        int16_t hold=track->p[P_AHOLD];track->p[P_AHOLD]=1;
        arp_add(track,60);arp_remove(track,60);track->arp_pos=0;
        arp_step(track,1,1);
        CHECK(!song.playing&&!track->arp_phys&&track->nheld==1&&!track->arp_note);
        CHECK(autosave_quiet());reset();
        CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes);
        track->p[P_AMODE]=0;track->p[P_AHOLD]=hold;track->nheld=0;
    }
    CHECK(df_automatic_quiet());
    /* Every IRQ-off entry: activity arrives after preceding stopped checks. */
    for(disturbance=0;disturbance<ACTIVITY_KINDS;disturbance++)for(unsigned c=0;c<operations;c++) {
        memcpy(nor,baseline,sizeof nor);calm();reset();inject_at=(int)c;irq_inject=disturb;
        CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&writes==c&&!irq_disabled);
        CHECK(proj_cur==2&&trk[7].p[P_LEVEL]==109&&!memcmp(name,proj_name,sizeof name));
        calm();reset();unchanged(&index);
    }
    /* Physical failures retain every current object and manual identity. */
    for(unsigned c=0;c<operations;c++) {
        memcpy(nor,baseline,sizeof nor);reset();cut=(int)c;
        CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&!irq_disabled&&proj_cur==2);
        reset();unchanged(&index);
    }
    /* A postcommit disturbance can report an error with a valid new autosave. */
    for(disturbance=0;disturbance<ACTIVITY_KINDS;disturbance++) {
        memcpy(nor,baseline,sizeof nor);reset();after_commit=disturb;
        CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&!irq_disabled&&proj_cur==2);
        CHECK(trk[7].p[P_LEVEL]==109&&!memcmp(name,proj_name,sizeof name));
        calm();reset();CHECK(!d8p1_restore_flash_autosave(1,1)&&trk[7].p[P_LEVEL]==109&&proj_cur==PROJ_NO_SLOT);
        mix_block(audio,CTL);proj_cur=2;
    }
    memcpy(nor,baseline,sizeof nor);calm();reset();
    CHECK(!d8p1_autosave_flash(1)&&proj_cur==2&&!memcmp(name,proj_name,sizeof name)&&!irq_disabled);
    size_t got=0;CHECK(!d8pool_load(&seed,3,out,sizeof out,&got,7)&&got==n&&!memcmp(wire,out,n));
    /* Explicit manual save keeps its original policy; no new key guard. */
    memcpy(nor,baseline,sizeof nor);reset();fm1_in.notes=1;
    CHECK(!d8p1_save_flash(0,1)&&proj_cur==0&&!irq_disabled);calm();
    /* No migration is inferred from blank/legacy records. */
    blank();CHECK(d8p1_autosave_flash(1)==D8POOL_EMPTY&&!writes);
    memcpy(nor+0x97000,"FELU",4);reset();CHECK(d8p1_autosave_flash(1)==D8POOL_UNSUPPORTED&&!writes);
    printf("Native quiet autosave: %u checks, %u failures; %u IRQ-entry cuts x %u activity kinds, %u driver cuts, %u postcommit disturbances; simulated hooks only\n",checks,failures,operations,ACTIVITY_KINDS,operations,ACTIVITY_KINDS);
    return failures!=0;
}
