/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Robustness against damaged or crafted stored data and editor requests (the 1.0.4 audit's items 5-10): the real
 * C paths of the editor build (editor_test.c is the base: hostsim, stubs, RAM flash).
 *   user sample slots   a SLICE scan bounded by the slot's size; a slot that fails its check leaves no zone behind
 *   projects            an engine number past the last refused (every format, and a RAM restore); a retained
 *                       older-format RAM slot bounded as one from flash
 *   user presets        a bank from flash or a backup: each record's pattern inside its fields
 *   editor              a malformed SMP_END / BACKUP_PUT does not stop the transport */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
#if NTRK > 4
#include "../firmware/src/d8p1_project.h"
#endif

static void zone_set(smp_zone_t *z, uint32_t off, uint32_t n)
{
    memset(z, 0, sizeof *z);
    z->off = off; z->n = n; z->ls = 0; z->le = n - 1u; z->rate = 1u << 16; z->idx = 0; z->lo = 0; z->hi = 127;
}

static int sample_slots(void)
{
    int bad = 0;
    uint32_t i, dl = SMP_USER_SIZE - SMP_USER_DATA;
    smp_user_hdr_t *h;
    reset(); memset(host_samples, 0, sizeof host_samples);
    /* 16 zones over overlapping data, each almost the whole slot (every zone inside the data) */
    h = (smp_user_hdr_t *)host_samples[0];
    h->magic = SMP_USER_MAGIC; h->version = 1; h->nz = 16; h->data_len = dl;
    for (i = 0; i < 16u; i++) zone_set(&h->zone[i], 32u * i, 2u * (dl - 32u * i));
    smp_user_scan(0);
#if FELUCCA_SLICE
    printf("robust: SLICE material of 16 overlapping zones: %u samples (the slot holds %u)\n", slc_usr[0].len,
           2u * dl);
    bad += check("16 overlapping zones: the SLICE scan is at most the slot's size", usr_nz[0] == 16u &&
                 slc_usr[0].len && slc_usr[0].len <= 2u * dl);
#endif
    /* zone 0 good, zone 1 malformed: nothing of the slot is left (usr_zone[1][0] was copied before the check) */
    h = (smp_user_hdr_t *)host_samples[1];
    h->magic = SMP_USER_MAGIC; h->version = 1; h->nz = 2; h->data_len = 4096;
    zone_set(&h->zone[0], 0, 4096);
    zone_set(&h->zone[1], 0, 4096); h->zone[1].idx = 99;
    smp_user_scan(1);
    bad += check("a slot with a malformed zone: unusable, no zone of it left behind",
                 !usr_nz[1] && !usr_zone[1][0].n && !usr_zone[1][1].n);
    h->zone[1].idx = 0;
    smp_user_scan(1);
    bad += check("the same slot made valid scans again", usr_nz[1] == 2u && usr_zone[1][0].n == 4096u);
    return bad;
}

static int projects(void)
{
    static project_t p, q;
    static project_store_t st;
    static project_v5_t v5;
    int bad = 0;
    uint32_t i, pos, sum, e0;
    reset();
    project_capture(&p);
    #if NTRK == PROJ_LEGACY_NTRK
    bad += check("a captured project packs and imports", proj_pack(&st, &p) && proj_import(&q, &st, PROJ_STORE_SIZE));
    pos = 68u + st.raw[66];                                 /* track 1's engine byte (after its parameters) */
    for (e0 = NENGINES; e0 < 256u; e0 += 37u) {
        st.raw[pos] = (uint8_t)e0;
        sum = proj_hash(st.raw, PROJ_STORE_SIZE - 4u);
        memcpy(st.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
        if (proj_import(&q, &st, PROJ_STORE_SIZE)) break;
    }
    bad += check("FUN8 with an engine past the last (a valid hash) is not a project", e0 >= 256u);
    #else
    memset(&st, 0xa5, sizeof st);
    project_store_t before = st;
    bad += check("native capture refuses legacy packing without touching output",
                 !proj_pack(&st, &p) && !memcmp(&st, &before, sizeof st));
    FILE *legacy = fopen("tests/fixtures/projects/fun9.bin","rb");
    if(!legacy) { perror("frozen FUN9 fixture"); return bad+1; }
    size_t length=fread(&st,1,sizeof st,legacy);
    int complete=length==sizeof st && fgetc(legacy)==EOF && !ferror(legacy);
    fclose(legacy);
    bad += check("native importer accepts independently frozen four-track FUN9",complete && proj_import(&q,&st,sizeof st));
    pos=68u+st.raw[66];
    for(e0=NENGINES;e0<256u;e0+=37u) {
        st.raw[pos]=(uint8_t)e0;
        sum=proj_hash(st.raw,PROJ_STORE_SIZE-4u);
        memcpy(st.raw+PROJ_STORE_SIZE-4u,&sum,4);
        if(proj_import(&q,&st,PROJ_STORE_SIZE))break;
    }
    bad += check("native FUN9 rejects out-of-range engine despite valid hash",e0>=256u);
    #endif
    memset(&v5, 0, sizeof v5);                              /* independently constructed FUN5 */
    v5.magic = PROJ_MAGIC_V5; v5.size = sizeof v5;
    for (i = 0; i < G_COUNT; i++) v5.g[i] = GP[i].def;
    for (i = 0; i < PROJ_LEGACY_NTRK; i++) { v5.t[i].engine = trk[i].engine; memcpy(v5.t[i].p, trk[i].p, sizeof v5.t[i].p); }
    v5.t[2].engine = NENGINES;
    v5.sum = proj_hash(&v5, sizeof v5 - 4u);
    bad += check("FUN5 with an engine past the last is not a project", !proj_import(&q, &v5, sizeof v5));
    p.t[1].engine = 200; p.sum = proj_sum(&p);              /* a RAM project_t (valid magic, size and sum) */
    e0 = trk[1].eng_req;
    bad += check("restoring a project with an engine past the last is refused, nothing changes",
                 project_restore_runtime(&p) == 1 && trk[1].eng_req == e0);
    /* a retained RAM slot holding FUN5 (its own size): bounded as one from flash (vel 200 cannot be packed) */
    reset();
    v5.t[2].engine = trk[2].engine;
    v5.t[0].step[0] = (step10_t){{60}, 1, ST_NOTE, 0, 200, 0, 0};
    v5.sum = proj_hash(&v5, sizeof v5 - 4u);
#if NTRK > 4
    static d8p1_project_state staged, scratch;
    d8p1_legacy_report report;
    project_v5_t original = v5;
    project_v5_t valid = v5;
    valid.parts = PROJ_LEGACY_NTRK; valid.phys = PROJ_PHYS;
    valid.t[0].step[0].vel = 100;
    valid.sum = proj_hash(&valid,sizeof valid-4u);
    project_v5_t preserved = valid;
    int accepted = d8p1_legacy_convert(&staged, &scratch, &report, &valid, sizeof valid, 7);
    bad += check("native offline stage accepts independent four-track FUN5 and retains original bytes",
                 accepted && !memcmp(&valid, &preserved, sizeof valid) && report.format == 5);
    int originals = accepted;
    for(unsigned t=0;t<PROJ_LEGACY_NTRK;t++) {
        const proj_trk_t *d=&staged.project.t[t];
        originals &= d->engine==valid.t[t].engine && d->preset==valid.t[t].preset;
        for(unsigned j=0;j<61;j++) originals &= d->p[j]==valid.t[t].p[j];
        for(unsigned j=61;j<P_E0;j++) originals &= d->p[j]==param_desc_of(valid.t[t].engine,j)->def;
        for(unsigned j=0;j<8;j++) originals &= d->p[P_E0+j]==valid.t[t].p[61+j];
        for(unsigned j=0;j<NSTEP;j++) originals &= !memcmp(&d->step[j],&valid.t[t].step[j],sizeof(step10_t));
        originals &= !memcmp(staged.project.fm6[t],FM6_INIT,FM6_PACKED);
    }
    bad += check("native staging retains all original four tracks with independently mapped FUN5 values",originals);
    int added = accepted;
    for (unsigned t = 4; t < 8; t++) {
        const proj_trk_t *d = &staged.project.t[t];
        added &= d->engine == 0 && d->preset == PROJ_DEF_SOUND;
        for (unsigned j = 0; j < P_COUNT; j++) added &= d->p[j] == param_desc_of(0,j)->def;
        for (unsigned j = 0; j < NSTEP; j++) added &= d->step[j].time == ST_REST && !d->step[j].n;
        added &= !memcmp(staged.project.fm6[t], FM6_INIT, FM6_PACKED);
    }
    bad += check("native added tracks5–8 use ANALOG defaults, rest steps and init patches", added);
    v5 = preserved; v5.t[2].engine = NENGINES; v5.sum = proj_hash(&v5,sizeof v5-4u);
    memset(&staged,0xa5,sizeof staged);d8p1_project_state untouched = staged;
    memset(&report,0xa5,sizeof report);d8p1_legacy_report untouched_report=report;
    bad += check("native stage refuses invalid legacy engine without publishing output/report",
                 !d8p1_legacy_convert(&staged,&scratch,&report,&v5,sizeof v5,7) &&
                 !memcmp(&staged,&untouched,sizeof staged) && !memcmp(&report,&untouched_report,sizeof report));
    valid = preserved; valid.sum ^= 1u;
    bad += check("native conversion refuses damaged checksum without publishing output/report",
                 !d8p1_legacy_convert(&staged,&scratch,&report,&valid,sizeof valid,7) &&
                 !memcmp(&staged,&untouched,sizeof staged) && !memcmp(&report,&untouched_report,sizeof report));
    valid = preserved;
    bad += check("native conversion refuses truncated FUN5 without publishing output/report",
                 !d8p1_legacy_convert(&staged,&scratch,&report,&valid,sizeof valid-1u,7) &&
                 !memcmp(&staged,&untouched,sizeof staged) && !memcmp(&report,&untouched_report,sizeof report));
    v5 = original;
#endif
    memset(&proj_slot[2], 0, sizeof proj_slot[2]);
    memcpy(&proj_slot[2], &v5, sizeof v5);
    project_load(2);
    bad += check("a retained FUN5 RAM slot loads bounded (velocity 200 -> inside 0..127)",
                 trk[0].step[0].n == 1u && trk[0].step[0].note[0] == 60u && trk[0].step[0].vel <= 127u);
    return bad;
}

static int user_presets(void)
{
    int bad = 0;
    up_bank_t *bk = &up_bank[0];
    up_rec_t *r;
    reset();
    memset(bk, 0, sizeof *bk);
    bk->magic = UP_BANK_MAGIC; bk->rsize = sizeof(up_rec_t); bk->nslot = UP_PER_BANK;
    r = &bk->r[0];                                          /* a note pattern with a note > 127 and stray flags */
    r->used = UP_USED; r->ver = 2; r->engine = 0; r->np = 8; r->name[0] = 'A';
    r->note[3] = 200; r->flags[3] = 0x43;
    r->note[4] = 0; r->flags[4] = 0x03;                     /* a rest with flags */
    r = &bk->r[1];                                          /* a drum grid: accents where there is no hit */
    r->used = UP_USED; r->ver = UP_VER_GRID; r->engine = ENGI_DRUM; r->np = 8; r->name[0] = 'G';
    r->note[0] = 0x05; r->flags[0] = 0xFF;
    up_bank_check(0, sizeof *bk);
    bad += check("a bank from flash: a note pattern's note inside 0..127, its flags inside their fields",
                 up_used(0) && bk->r[0].note[3] == (200u & 127u) && bk->r[0].flags[3] == 3u && !bk->r[0].flags[4]);
    bad += check("a bank from flash: a drum grid's accents only on its hits", up_used(1) && bk->r[1].note[0] == 5u &&
                 bk->r[1].flags[0] == 5u);
    return bad;
}

#if NTRK > 4
/* editor_test fixes FLASH=0: capture/migration-session subsystem is absent.
 * Snapshot all present music, storage, backup and binding authority below;
 * connected capture/session qualification belongs to its separate fixtures. */
#define ROBUST_STATE(X) __typeof__(X) X
static struct {
    ROBUST_STATE(trk);
    ROBUST_STATE(song);
    ROBUST_STATE(motion);
    ROBUST_STATE(chain);
    ROBUST_STATE(chain_config);
    ROBUST_STATE(fm6_patch);
    ROBUST_STATE(fm6_slot);
    ROBUST_STATE(ui);
    ROBUST_STATE(undo);
    ROBUST_STATE(up_bank);
    ROBUST_STATE(upf);
    ROBUST_STATE(favorites);
    ROBUST_STATE(proj_slot);
    ROBUST_STATE(settings);
    ROBUST_STATE(ed_w);
    ROBUST_STATE(host_samples);
    ROBUST_STATE(transport_req);
    ROBUST_STATE(usr_zone);
    ROBUST_STATE(usr_nz);
    ROBUST_STATE(project_native);
    ROBUST_STATE(proj_cur);
    ROBUST_STATE(proj_wire_u);
    ROBUST_STATE(proj_wire_gen);
    ROBUST_STATE(ed_bk_settings);
    ROBUST_STATE(ed_bk_valid);
    ROBUST_STATE(ed_bk_put);
    ROBUST_STATE(ed_bk_id);
    ROBUST_STATE(ed_bk_gen);
    ROBUST_STATE(ed_bk_len);
    ROBUST_STATE(ed_bk_crc);
    ROBUST_STATE(ed_bk_pos);
    ROBUST_STATE(ed_bk_ms);
    ROBUST_STATE(ed_bk_usb);
    ROBUST_STATE(main_workspace);
    ROBUST_STATE(cv_cpu_active);
    ROBUST_STATE(cv_canvas_valid);
    ROBUST_STATE(migration_owner);
    ROBUST_STATE(migration_generation);
    uint8_t generations[sizeof fm6_pgen]; unsigned writes,erases,age;
    uint8_t panic,sync,depth;
} refused;
#define ROBUST_COPY(X) memcpy(&refused.X,&(X),sizeof(X))
#define ROBUST_SAME(X) (!memcmp(&refused.X,&(X),sizeof(X)))
static void refusal_snapshot(void) {
    ROBUST_COPY(trk);
    ROBUST_COPY(song);
    ROBUST_COPY(motion);
    ROBUST_COPY(chain);
    ROBUST_COPY(chain_config);
    ROBUST_COPY(fm6_patch);
    ROBUST_COPY(fm6_slot);
    ROBUST_COPY(ui);
    ROBUST_COPY(undo);
    ROBUST_COPY(up_bank);
    ROBUST_COPY(upf);
    ROBUST_COPY(favorites);
    ROBUST_COPY(proj_slot);
    ROBUST_COPY(settings);
    ROBUST_COPY(ed_w);
    ROBUST_COPY(host_samples);
    ROBUST_COPY(transport_req);
    ROBUST_COPY(usr_zone);
    ROBUST_COPY(usr_nz);
    ROBUST_COPY(project_native);
    ROBUST_COPY(proj_cur);
    ROBUST_COPY(proj_wire_u);
    ROBUST_COPY(proj_wire_gen);
    ROBUST_COPY(ed_bk_settings);
    ROBUST_COPY(ed_bk_valid);
    ROBUST_COPY(ed_bk_put);
    ROBUST_COPY(ed_bk_id);
    ROBUST_COPY(ed_bk_gen);
    ROBUST_COPY(ed_bk_len);
    ROBUST_COPY(ed_bk_crc);
    ROBUST_COPY(ed_bk_pos);
    ROBUST_COPY(ed_bk_ms);
    ROBUST_COPY(ed_bk_usb);
    ROBUST_COPY(main_workspace);
    ROBUST_COPY(cv_cpu_active);
    ROBUST_COPY(cv_canvas_valid);
    ROBUST_COPY(migration_owner);
    ROBUST_COPY(migration_generation);
    memcpy(refused.generations,(const void *)fm6_pgen,sizeof fm6_pgen);
    refused.writes=host_writes;refused.erases=host_erases;refused.age=vage;
    refused.panic=panic_req;refused.sync=sync_reload;refused.depth=undo_depth;
}
static int refusal_unchanged(void) {
    return ROBUST_SAME(trk)&&
      ROBUST_SAME(song)&&
      ROBUST_SAME(motion)&&
      ROBUST_SAME(chain)&&
      ROBUST_SAME(chain_config)&&
      ROBUST_SAME(fm6_patch)&&
      ROBUST_SAME(fm6_slot)&&
      ROBUST_SAME(ui)&&
      ROBUST_SAME(undo)&&
      ROBUST_SAME(up_bank)&&
      ROBUST_SAME(upf)&&
      ROBUST_SAME(favorites)&&
      ROBUST_SAME(proj_slot)&&
      ROBUST_SAME(settings)&&
      ROBUST_SAME(ed_w)&&
      ROBUST_SAME(host_samples)&&
      ROBUST_SAME(transport_req)&&
      ROBUST_SAME(usr_zone)&&
      ROBUST_SAME(usr_nz)&&
      ROBUST_SAME(project_native)&&
      ROBUST_SAME(proj_cur)&&
      ROBUST_SAME(proj_wire_u)&&
      ROBUST_SAME(proj_wire_gen)&&
      ROBUST_SAME(ed_bk_settings)&&
      ROBUST_SAME(ed_bk_valid)&&
      ROBUST_SAME(ed_bk_put)&&
      ROBUST_SAME(ed_bk_id)&&
      ROBUST_SAME(ed_bk_gen)&&
      ROBUST_SAME(ed_bk_len)&&
      ROBUST_SAME(ed_bk_crc)&&
      ROBUST_SAME(ed_bk_pos)&&
      ROBUST_SAME(ed_bk_ms)&&
      ROBUST_SAME(ed_bk_usb)&&
      ROBUST_SAME(main_workspace)&&
      ROBUST_SAME(cv_cpu_active)&&
      ROBUST_SAME(cv_canvas_valid)&&
      ROBUST_SAME(migration_owner)&&
      ROBUST_SAME(migration_generation)&&!memcmp(refused.generations,(const void *)fm6_pgen,sizeof fm6_pgen)&&
      refused.writes==host_writes&&refused.erases==host_erases&&refused.age==vage&&
      refused.panic==panic_req&&refused.sync==sync_reload&&refused.depth==undo_depth;
}
#endif
static int stop_after_checks(void)
{
    int bad = 0;
    uint8_t a[16] = {0};
    reset();
    song.playing = 1; host_progress = 1;                    /* the transport runs and could be stopped */
#if NTRK > 4
    int32_t audio[2u*CTL];mix_block(audio,CTL);
    for(unsigned t=0;t<NTRK;t++){trk[t].p[P_VOICE]=V_MONO;trk_note_on(&trk[t],60u+t,100);}
    mix_block(audio,CTL);
    bad += check("native refusal controls have actual sounding peer voices",voices_busy()==8);
    panic_req=0;sync_reload=0;refusal_snapshot();
#endif
    a[0] = 0; a[1] = 1; a[2] = 2;
    request(ED_SMP_END, a, 3);                              /* a header far too short */
    bad += check("a malformed SMP_END is refused without stopping the transport", (NTRK > 4 ? host_wire_n == 9 && host_wire[4] == ED_D8_ERROR && host_wire[5] == 1 && host_wire[6] == ED_SMP_END && host_wire[7] == 1 : host_wire[6] == 1) && song.playing);
#if NTRK > 4
    bad += check("native malformed sample request preserves tracked music/storage/undo/session state",refusal_unchanged());
#endif
    a[0] = 0; a[1] = 2;
    request(ED_BACKUP_PUT, a, 2);                           /* BEGIN without its arguments */
    bad += check("a malformed BACKUP_PUT is refused without stopping the transport", (NTRK > 4 ? host_wire_n == 9 && host_wire[4] == ED_D8_ERROR && host_wire[5] == 1 && host_wire[6] == ED_BACKUP_PUT && host_wire[7] == 1 : host_wire[7] == 1) && song.playing);
#if NTRK > 4
    bad += check("native malformed backup begin preserves tracked music/storage/undo/session state",refusal_unchanged());
#endif
    a[0] = 1; a[1] = 2;
    request(ED_BACKUP_PUT, a, 9);                           /* data with no session */
    bad += check("BACKUP_PUT data without a session: refused, the transport runs", (NTRK > 4 ? host_wire_n == 9 && host_wire[4] == ED_D8_ERROR && host_wire[5] == 1 && host_wire[6] == ED_BACKUP_PUT && host_wire[7] == 1 : host_wire[7] == 5) && song.playing);
#if NTRK > 4
    bad += check("native backup chunk without session preserves tracked music/storage/undo/session state",refusal_unchanged());
#endif
    song.playing = 0;
    return bad;
}

int main(void)
{
    int bad = sample_slots() + projects() + user_presets() + stop_after_checks();
    printf("%s\n", bad ? "ROBUSTNESS TEST FAILED" : "robustness test passed");
    return bad != 0;
}
