/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* A song uses the sequences of four saved projects, with the current sounds.
 * Sources are copied in the main loop before PLAY. The ISR changes only their
 * index and four timing parameters; the editable steps stay untouched. */
typedef struct {
    step_t step[NTRK][NSTEP];
    int16_t timing[NTRK][4];
    motion_store_t motion;
} chain_pattern_t;

#if NTRK == 8
/* Native references and immutable source provenance use the otherwise unused
 * fourth legacy source extent. Never keep display/staging pointers here. */
enum { D8ARR_APPLY_MUTE=1, D8ARR_APPLY_MIX=2, D8ARR_APPLY_TRANSPOSE=4, D8ARR_NONE=255 };
typedef struct {
    uint8_t project[4][8], track[4][8], scene_bank[16];
    uint8_t row_scene[16], row_repeat[16], source_engine[3][8];
    uint8_t banks, scenes, rows, valid, suspended, scene, bank, pending, preparing;
    uint32_t generation, pending_generation, store_epoch;
    uint32_t sequence[3], length[3], crc[3];
    uint32_t held[8][4],input_generation;
} d8arr_state;
_Static_assert(sizeof(d8arr_state)<=sizeof(chain_pattern_t), "native policy fits unused source");
static uint32_t motion_guard(void);
static void motion_unguard(uint32_t);
static unsigned d8arr_ui_rows(void);
static void d8arr_held_build(uint32_t [8][4]);
static void d8arr_held_refresh(track_t *,unsigned);
static void d8arr_held_rebuild(unsigned);
static int d8arr_ui_row(unsigned,char [13],unsigned *);
#endif

static chain_config_t chain_config;
static struct {
#if NTRK == 8
    union {
        chain_pattern_t source[4];
        struct { chain_pattern_t manual[3]; d8arr_state policy; } native;
    };
#else
    chain_pattern_t source[4];
#endif
    chain_config_t config;
    int16_t timing[NTRK][4];
    volatile uint8_t armed, running, row, remaining;
    uint8_t slot, rec;
#if NTRK == 8
    uint8_t native_mode; /* 0 editable, 1 native, 2 invalid-resume tombstone */
#endif
    uint32_t carry;
} chain;

static void seq_release(track_t *t);
static void seq_stop(void);
static uint32_t div_samples(uint32_t div);
static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx);

static void chain_defaults(chain_config_t *c)
{
    uint32_t i;
    memset(c, 0, sizeof *c);
    for (i = 0; i < CHAIN_ROWS; i++)
        c->row[i].repeat = 1;
}
static int chain_valid(const chain_config_t *c)
{
    uint32_t i;
    if (c->count > CHAIN_ROWS)
        return 0;
    for (i = 0; i < c->count; i++)
        if (c->row[i].slot >= 4u || !c->row[i].repeat || c->row[i].repeat > 16u)
            return 0;
    return 1;
}
static void motion_restore(track_t *t);

#if NTRK == 8
static int d8arr_running(void) { return chain.native_mode==1 && chain.running; }
static unsigned d8arr_source_project(const track_t *t)
{ return chain.native.policy.project[chain.native.policy.bank][t-trk]; }
static unsigned d8arr_source_track(const track_t *t)
{ return d8arr_running()?chain.native.policy.track[chain.native.policy.bank][t-trk]:(unsigned)(t-trk); }
static const motion_store_t *d8arr_motion(const track_t *t)
{ return &chain.source[d8arr_source_project(t)].motion; }
static int d8arr_motion_compatible(const track_t *t,unsigned id)
{ return !d8arr_running() || id<P_FM1_ATK || chain.native.policy.source_engine[d8arr_source_project(t)][d8arr_source_track(t)]==t->eng_req; }
/* Stopped source-changing entry points invalidate suspension; never confer
 * storage ownership and never tear a running source out from under the ISR. */
static void d8arr_invalidate(void)
{ if(chain.native_mode&&!chain.running&&!chain.armed){chain.native_mode=2;chain.native.policy.valid=0;chain.native.policy.pending=D8ARR_NONE;} }
static int d8arr_request_row(unsigned row)
{
    uint32_t f=motion_guard();
    int rc=2;
    if(d8arr_running()&&chain.native.policy.valid&&row<chain.native.policy.rows){
        chain.native.policy.pending=(uint8_t)row;
        chain.native.policy.pending_generation=chain.native.policy.generation;
        RING_PUBLISH();rc=0;
    }
    motion_unguard(f);return rc;
}
static void d8arr_timing(void)
{
    for(unsigned i=0;i<NTRK;i++){
        unsigned o=d8arr_source_project(&trk[i]),k=d8arr_source_track(&trk[i]);
        memcpy(&trk[i].p[P_SLEN],chain.source[o].timing[k],sizeof chain.timing[i]);
    }
}
static void d8arr_apply(unsigned row,uint32_t carry)
{
    d8arr_state *a=&chain.native.policy;
    chain.row=(uint8_t)row;chain.remaining=a->row_repeat[row];
    a->scene=a->row_scene[row];a->bank=a->scene_bank[a->scene];chain.carry=carry;
    /* This is bounded existing musical work, not merely O(1) publication:
     * <=96 note-offs and 8xP_COUNT motion restoration scans. No engines/patches. */
    for(unsigned i=0;i<NTRK;i++) {seq_release(&trk[i]);motion_restore(&trk[i]);}
    d8arr_timing();
    for(unsigned i=0;i<NTRK;i++){
        track_t *t=&trk[i];t->seq_idx=(uint16_t)(t->p[P_SLEN]-1);t->seq_pos=0x7fffffffu;
        t->rh_n=t->rskip_n=t->rat_left=0;
    }
}
static void d8arr_start(void)
{
    d8arr_state *a=&chain.native.policy;
    if(!chain.running&&!a->suspended){
        chain.rec=song.rec;
        for(unsigned i=0;i<NTRK;i++)memcpy(chain.timing[i],&trk[i].p[P_SLEN],sizeof chain.timing[i]);
    }
    a->generation++;a->pending=D8ARR_NONE;a->suspended=0;
    song.rec=0;chain.running=1;chain.armed=0;d8arr_apply(0,0);
}
static void d8arr_stop(void)
{
    d8arr_state *a=&chain.native.policy;
    chain.armed=0;a->preparing=0;a->generation++;a->pending=D8ARR_NONE;
    if(!chain.running)return;
    for(unsigned i=0;i<NTRK;i++)memcpy(&trk[i].p[P_SLEN],chain.timing[i],sizeof chain.timing[i]);
    song.rec=chain.rec;chain.running=0;a->suspended=1;
}
static int d8arr_continue(void)
{
    if(!chain.native_mode)return 1;
    if(chain.native_mode==2)return 0;
    d8arr_state *a=&chain.native.policy;
    if(!a->valid||!a->suspended||a->store_epoch!=d8_capture_change.epoch){
        if(a->valid&&a->store_epoch!=d8_capture_change.epoch){chain.native_mode=2;a->valid=0;}
        a->preparing=0;return 0;
    }
    a->generation++;a->pending=D8ARR_NONE;a->suspended=0;
    chain.running=1;chain.armed=0;song.rec=0;d8arr_timing();return 1;
}
static void d8arr_tick(uint32_t n,uint32_t pos,uint32_t step,uint32_t period)
{
    d8arr_state *a=&chain.native.policy;
    if(!d8arr_running()||!a->valid||pos==0xffffffffu)return;
    uint32_t total=step*period+pos+n,bar=16u*period;
    if(total<bar)return;
    if(a->pending!=D8ARR_NONE&&a->pending_generation==a->generation){
        unsigned row=a->pending;a->pending=D8ARR_NONE;d8arr_apply(row,total-bar);return;
    }
    a->pending=D8ARR_NONE;
    if(chain.remaining>1){chain.remaining--;return;}
    if((unsigned)chain.row+1>=a->rows){seq_stop();return;}
    d8arr_apply((unsigned)chain.row+1,total-bar);
}
#endif

static const step_t *seq_steps(const track_t *t)
{
#if NTRK == 8
    if(d8arr_running())return chain.source[d8arr_source_project(t)].step[d8arr_source_track(t)];
#endif
    return chain.running ? chain.source[chain.slot].step[t - trk] : t->step;
}
static void motion_restore(track_t *t);
static void chain_apply(void)
{
    uint32_t i;
    chain.slot = chain.config.row[chain.row].slot;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        seq_release(t);
        motion_restore(t);
        memcpy(&t->p[P_SLEN], chain.source[chain.slot].timing[i], sizeof chain.timing[i]);
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFFu;
        t->rh_n = t->rskip_n = 0;
    }
}
static void chain_start(void)
{
    uint32_t i;
#if NTRK == 8
    if(chain.native_mode&&chain.native.policy.valid&&(chain.armed||chain.running||chain.native.policy.suspended)){d8arr_start();return;}
#endif
    if (!chain.armed)
        return;
    chain.rec = song.rec;
    song.rec = 0;
    for (i = 0; i < NTRK; i++)
        memcpy(chain.timing[i], &trk[i].p[P_SLEN], sizeof chain.timing[i]);
    chain.row = 0;
    chain.remaining = chain.config.row[0].repeat;
    chain.carry = 0;
    chain.running = 1;
    chain.armed = 0;
    chain_apply();
}
static void chain_stop(void)
{
    uint32_t i;
#if NTRK == 8
    if(chain.native_mode){d8arr_stop();return;}
#endif
    chain.armed = 0;
    if (!chain.running)
        return;
    for (i = 0; i < NTRK; i++)
        memcpy(&trk[i].p[P_SLEN], chain.timing[i], sizeof chain.timing[i]);
    song.rec = chain.rec;
    chain.running = 0;
}
#if NTRK == 8
/* Historical row changes are rare boundaries. Keep their actual work separate
 * from the audio fragment loop; helper cost/stack remain part of qualification. */
static __attribute__((noinline)) void chain_advance_legacy(uint32_t carry)
{
    if (chain.row + 1u >= chain.config.count) { seq_stop(); return; }
    chain.carry = carry;
    chain.row++;
    chain.remaining = chain.config.row[chain.row].repeat;
    chain_apply();
}
#endif
static void chain_tick(uint32_t n)
{
#if NTRK == 8
    if(chain.native_mode)return; /* native master-bar hook is after MIDI advancement */
#endif
    const track_t *t = &trk[0];
    uint32_t length;
    if (!chain.running || t->seq_pos >= 0x7FFFFFFFu || t->seq_idx + 1u != (uint32_t)t->p[P_SLEN])
        return;
    length = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx);
    if (t->seq_pos + n < length)
        return;
    if (chain.remaining > 1u) {
        chain.remaining--;
        return;
    }
#if NTRK == 8
    chain_advance_legacy(t->seq_pos + n - length);
#else
    if (chain.row + 1u >= chain.config.count) {
        seq_stop();
        return;
    }
    chain.carry = t->seq_pos + n - length;
    chain.row++;
    chain.remaining = chain.config.row[chain.row].repeat;
    chain_apply();
#endif
}
