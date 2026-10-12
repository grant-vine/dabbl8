/* SPDX-License-Identifier: GPL-3.0-only */
/* Test-only historical input construction and native persistence quarantine.
 * Native eight-track music is never represented as a successful FUN9 save. */
#include "legacy_wire_fixture.h"
static int motion_fixture_bad;
#if NPART == 4
#define MOTION_HIST(native,legacy) legacy
#define motion_fixture_pack proj_pack
#define motion_fixture_same(a,b) (!memcmp((a),(b),sizeof *(a)))
#define motion_fixture_save project_save
#else
#define MOTION_HIST(native,legacy) native
static int motion_fixture_same(const project_t *a,const project_t *b)
{
    return b->parts==PROJ_LEGACY_NTRK && (a->parts==PROJ_LEGACY_NTRK||a->parts==NPART) && a->magic==PROJ_MAGIC && b->magic==PROJ_MAGIC && a->size==sizeof *a && b->size==sizeof *b && !memcmp(a->g,b->g,sizeof a->g) && a->sel==b->sel && a->phys==b->phys &&
        !memcmp(a->t,b->t,PROJ_LEGACY_NTRK*sizeof a->t[0]) &&
        !memcmp(a->fm6,b->fm6,PROJ_LEGACY_NTRK*FM6_PACKED) &&
        !memcmp(&a->chain,&b->chain,sizeof a->chain) &&
        !memcmp(&a->motion,&b->motion,sizeof a->motion) &&
        !memcmp(a->name,b->name,sizeof a->name);
}
static int motion_fixture_pack(project_store_t *out,const project_t *q)
{
    project_store_t sentinel,before;
    memset(&sentinel,0xA5,sizeof sentinel);before=sentinel;
    motion_fixture_bad += check("native FUN9 encoder refuses before touching output",!proj_pack(&sentinel,q)&&!memcmp(&sentinel,&before,sizeof sentinel));
    if(q->sel>=PROJ_LEGACY_NTRK)return 0;
    project_t historical=*q;historical.parts=PROJ_LEGACY_NTRK;
    return fixture_fun9(out,&historical);
}
static int motion_fixture_save(unsigned slot)
{
    static struct {
        __typeof__(fx) effects;__typeof__(dly_buf) delay;__typeof__(cho_buf) chorus;__typeof__(rev_comb) comb;__typeof__(rev_u) reverb;project_t captured;
        track_t tracks[NTRK];__typeof__(song) music;motion_store_t events;
        __typeof__(chain) playback;chain_config_t config;
        project_store_t retained[4],wire;
        __typeof__(main_workspace) arena;
        __typeof__(heavy_pool) heavy;__typeof__(heavy_owner) owners;
        __typeof__(host_slots) samples;
        uint32_t transport,panic,wire_gen;uint8_t current;
        char name[sizeof proj_name];
    } s;
    project_capture(&s.captured);s.effects=fx;memcpy(s.delay,dly_buf,sizeof dly_buf);memcpy(s.chorus,cho_buf,sizeof cho_buf);memcpy(s.comb,rev_comb,sizeof rev_comb);memcpy(&s.reverb,&rev_u,sizeof rev_u);
    memcpy(s.tracks,trk,sizeof trk);s.music=song;s.events=motion;s.playback=chain;s.config=chain_config;
    memcpy(s.retained,proj_slot,sizeof proj_slot);memcpy(&s.wire,&proj_wire,sizeof proj_wire);
    memcpy(&s.arena,&main_workspace,sizeof main_workspace);memcpy(s.heavy,heavy_pool,sizeof heavy_pool);memcpy(s.owners,heavy_owner,sizeof heavy_owner);
    memcpy(s.samples,host_slots,sizeof host_slots);s.transport=transport_req;s.panic=panic_req;s.wire_gen=proj_wire_gen;s.current=proj_cur;memcpy(s.name,proj_name,sizeof proj_name);
    int rc=project_save(slot);
    motion_fixture_bad += check("native save explicitly refuses without a migration binding",rc==1&&str_eq(ui.msg,"MIGRATION REQUIRED"));
    project_t after;project_capture(&after);
    motion_fixture_bad += check("native save refusal preserves tracks, transport, motion, FX, mirrors and arena",
        !memcmp(&s.captured,&after,sizeof after)&&!memcmp(&s.effects,&fx,sizeof fx)&&!memcmp(s.delay,dly_buf,sizeof dly_buf)&&!memcmp(s.chorus,cho_buf,sizeof cho_buf)&&!memcmp(s.comb,rev_comb,sizeof rev_comb)&&!memcmp(&s.reverb,&rev_u,sizeof rev_u)&&
        !memcmp(s.tracks,trk,sizeof trk)&&!memcmp(&s.music,&song,sizeof song)&&!memcmp(&s.events,&motion,sizeof motion)&&
        !memcmp(&s.playback,&chain,sizeof chain)&&!memcmp(&s.config,&chain_config,sizeof chain_config)&&
        !memcmp(s.retained,proj_slot,sizeof proj_slot)&&!memcmp(&s.wire,&proj_wire,sizeof proj_wire)&&
        !memcmp(&s.arena,&main_workspace,sizeof main_workspace)&&!memcmp(s.heavy,heavy_pool,sizeof heavy_pool)&&!memcmp(s.owners,heavy_owner,sizeof heavy_owner)&&
        !memcmp(s.samples,host_slots,sizeof host_slots)&&s.transport==transport_req&&s.panic==panic_req&&s.wire_gen==proj_wire_gen&&s.current==proj_cur&&!memcmp(s.name,proj_name,sizeof proj_name));
    /* Subsequent read/playback cases need an independently valid historical input.
     * This is test fixture seeding, after explicit refusal; no firmware write. */
    project_t captured,decoded;project_store_t input;project_capture(&captured);
    int valid=motion_fixture_pack(&input,&captured)&&proj_import(&decoded,&input,sizeof input)&&decoded.parts==PROJ_LEGACY_NTRK&&motion_fixture_same(&captured,&decoded);
    motion_fixture_bad += check("independent historical read fixture validates before seeding",valid);
    if(valid)memcpy(&proj_slot[slot],&input,sizeof input);
    return rc;
}
#endif
