/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual adopted-native panel sound, pattern and motion undo/redo checks.
 * Simulated panel/storage only; no physical device qualification. */
#include "native_ui_undo_seed.inc"
static struct {
    track_t tracks[NTRK];
    __typeof__(motion) motion_state;
    __typeof__(d8p1_runtime_cache) cache;
    __typeof__(project_native) catalog;
    __typeof__(undo) undo_state;
    __typeof__(song) song_state;
    __typeof__(chain) chain_state;
    uint8_t patches[sizeof fm6_patch];
    uint8_t nor[sizeof native_nor];
    char name[sizeof proj_name];
    uint32_t epoch;
    uint8_t changed, slot, panic, request;
} snapshot_state;
static void take(void)
{
    memcpy(snapshot_state.tracks, trk, sizeof trk);
    snapshot_state.motion_state = motion;
    snapshot_state.cache = d8p1_runtime_cache;
    snapshot_state.catalog = project_native;
    snapshot_state.undo_state = undo;
    snapshot_state.song_state = song;
    snapshot_state.chain_state = chain;
    memcpy(snapshot_state.patches, fm6_patch, sizeof fm6_patch);
    memcpy(snapshot_state.nor, native_nor, sizeof native_nor);
    memcpy(snapshot_state.name, proj_name, sizeof proj_name);
    snapshot_state.slot = proj_cur;
    snapshot_state.epoch = d8_capture_change.epoch;
    snapshot_state.changed = d8_capture_change.changed;
    snapshot_state.panic = panic_req;
    snapshot_state.request = transport_req;
}
static void retained(unsigned selected)
{
    for (unsigned t = 0; t < NTRK; t++)
        if (t != selected)
            proof(!memcmp(&trk[t], &snapshot_state.tracks[t], sizeof trk[t]),
                  "all other complete runtime tracks preserved");
    proof(!memcmp(&d8p1_runtime_cache, &snapshot_state.cache, sizeof d8p1_runtime_cache),
          "complete native arrangement/name cache retained");
    proof(!memcmp(&project_native, &snapshot_state.catalog, sizeof project_native),
          "catalog/authorization/current identity coherent");
    proof(proj_cur == snapshot_state.slot && !memcmp(proj_name, snapshot_state.name, sizeof proj_name),
          "saved identity unchanged by RAM undo");
    proof(!memcmp(native_nor, snapshot_state.nor, sizeof native_nor), "all virtual storage bytes unchanged");
    proof(d8_capture_change.epoch == snapshot_state.epoch &&
              d8_capture_change.changed == snapshot_state.changed,
          "RAM panel changes do not mint or erase persistent store epoch");
    for (unsigned t = 0; t < NTRK; t++)
        if (t != selected)
            proof(
                !memcmp(fm6_patch[t], snapshot_state.patches + t * sizeof fm6_patch[t], sizeof fm6_patch[t]),
                "other-track FM6 patch retained");
}
static uint32_t musical_signature(void)
{
    uint32_t v = 0;
    proof(!d8p1_signature_runtime(&v), "actual stopped canonical signature available");
    return v;
}
static void clean_seed(unsigned t)
{
    seed();
    seq_stop();
    transport_req = 0;
    track_select(t);
    go_home();
    frame();
    cur_name = "native panel undo";
    proof(d8p1_runtime_cache.valid && project_native.ready, "real adopted and bound native state");
}
static void panel_hold(void)
{
    unsigned r = reads, w = writes;
    hold(B_SAVE);
    proof(reads == r && writes == w, "SAVE hold dispatch performs no storage calls");
    proof(ui.home || cur_page()->fam != FAM_SAVE, "hold never becomes SAVE tap");
}
/* Consume actual pending requests before each hold, then model an independent
 * interrupt request for the peer. No direct undo state or dispatch is changed. */
static void pending_peer_after_consumer(unsigned selected, unsigned peer)
{
    events_block(1);
    proof(panic_req == 0, "real events consumer clears all prior panic requests");
    panic_req |= (uint8_t)(1u << peer); /* Fixture ingress: an unrelated pending interrupt request. */
    proof((panic_req & (1u << selected)) == 0, "selected panic bit is clear before held undo/redo");
    take(); /* Compare runtime state after the actual consumer, never before it. */
}
static void stopped_sound(unsigned t)
{
    clean_seed(t);
    unsigned peer = (t + 1u) % NTRK;
    track_select(peer);
    go_home();
    frame();
    turn(EN_PRESET, 1);
    proof((panic_req & (1u << peer)) != 0, "actual other-track sound action establishes pending panic bit");
    track_select(t);
    go_home();
    frame();
    track_t before_track = trk[t];
    uint32_t initial = musical_signature();
    take();
    unsigned r = reads, w = writes;
    turn(EN_PRESET, 1);
    proof(undo.trk == t + 1 && undo.what == UNDO_SOUND, "real PRESET action captures logical sound undo");
    proof(!same_sound(&trk[t], &before_track), "actual sound load changes sound");
    track_t loaded = trk[t];
    uint32_t loaded_sig = musical_signature();
    proof(loaded_sig != initial, "canonical music dirtied by actual sound load");
    pending_peer_after_consumer(t, peer);
    uint8_t before_panic = panic_req;
    panel_hold();
    proof(panic_req == (uint8_t)(before_panic | (1u << t)),
          "sound undo requests selected-track panic and preserves unrelated pending bits");
    proof((panic_req & (1u << peer)) != 0, "sound undo retains actual other-track panic request");
    proof(same_sound(&trk[t], &before_track) &&
              !memcmp(trk[t].step, before_track.step, sizeof before_track.step),
          "actual SAVE-held sound undo restores sound and preserves pattern");
    proof(musical_signature() == initial, "sound undo restores canonical signature");
    retained(t);
    pending_peer_after_consumer(t, peer);
    before_panic = panic_req;
    panel_hold();
    proof(panic_req == (uint8_t)(before_panic | (1u << t)),
          "sound redo requests selected-track panic and preserves unrelated pending bits");
    proof((panic_req & (1u << peer)) != 0, "sound redo retains actual other-track panic request");
    proof(same_sound(&trk[t], &loaded) && !memcmp(trk[t].step, loaded.step, sizeof loaded.step),
          "actual SAVE-held sound redo restores loaded sound");
    proof(musical_signature() == loaded_sig, "sound redo restores dirty signature");
    retained(t);
    proof(reads == r && writes == w, "complete stopped sound flow makes zero backend calls");
    /* Actual source preparation and transport; SAVE held must refuse before undo. */
    start();
    take();
    r = reads;
    w = writes;
    panel_hold();
    proof(msg_is("STOP TO UNDO"), "running native arrangement refuses held undo");
    proof(!memcmp(trk, snapshot_state.tracks, sizeof trk) &&
              !memcmp(&motion, &snapshot_state.motion_state, sizeof motion) &&
              !memcmp(&undo, &snapshot_state.undo_state, sizeof undo),
          "running refusal preserves all tracks/motion/undo bytes");
    proof(!memcmp(&song, &snapshot_state.song_state, sizeof song) &&
              !memcmp(&chain, &snapshot_state.chain_state, sizeof chain) &&
              panic_req == snapshot_state.panic && transport_req == snapshot_state.request,
          "running refusal preserves transport/reference policy and pending state");
    retained(t);
    proof(reads == r && writes == w, "running refusal makes no backend calls");
    seq_stop();
    transport_req = 0;
}
static void stopped_pattern(unsigned t)
{
    clean_seed(t);
    go_page(GR_PATS);
    frame();
    track_t before_track = trk[t];
    uint32_t initial = musical_signature();
    take();
    unsigned r = reads, w = writes;
    turn(EN_K1, 1);
    press(B_OCTUP);
    if (ui.confirm == CF_LOAD_PAT)
        press(B_OCTUP);
    proof(!ui.confirm && undo.trk == t + 1 && (undo.what & UNDO_PAT),
          "actual pattern load/confirmation captures correct track");
    track_t loaded = trk[t];
    uint32_t loaded_sig = musical_signature();
    proof(loaded_sig != initial, "pattern load dirties native musical signature");
    panel_hold();
    proof(!memcmp(trk[t].step, before_track.step, sizeof before_track.step) &&
              same_sound(&trk[t], &before_track),
          "panel pattern undo restores original grid and keeps sound");
    proof(musical_signature() == initial, "pattern undo restores initial canonical signature");
    retained(t);
    panel_hold();
    proof(!memcmp(trk[t].step, loaded.step, sizeof loaded.step) && same_sound(&trk[t], &loaded),
          "panel pattern redo restores new grid and keeps sound");
    proof(musical_signature() == loaded_sig, "pattern redo restores dirty signature");
    retained(t);
    proof(reads == r && writes == w, "complete pattern flow makes zero backend calls");
}
static void stopped_motion(unsigned t)
{
    clean_seed(t);
    open_family(FAM_SEQ);
    go_page(GR_ROLL);
    frame();
    press(B_REC);
    proof(roll_on() && (song.rec & (1u << t)) && !song.playing && !transport_req,
          "actual REC arms stopped piano-roll step entry");
    unsigned r = reads, w = writes;
    unsigned key = key_at(0, 4);
    uint32_t oldlocks = motion_lock_count(&trk[t]);
    key_down(key);
    frame();
    proof(lock_held() != 0, "real keyboard dispatch establishes held step");
    /* The recorded note is not undone by a motion-only undo: capture after entry. */
    uint32_t initial = musical_signature();
    take();
    turn(EN_K1, 3);
    key_up(key);
    frame();
    proof(undo.trk == t + 1 && (undo.what & UNDO_MOT) && motion_lock_count(&trk[t]) == oldlocks + 1,
          "actual held-step knob edit captures native motion undo");
    uint32_t edited = musical_signature();
    proof(edited != initial, "motion edit dirties canonical native signature");
    panel_hold();
    proof(motion_lock_count(&trk[t]) == oldlocks, "SAVE-held motion undo removes only new lock");
    proof(musical_signature() == initial, "motion undo restores canonical signature");
    retained(t);
    panel_hold();
    proof(motion_lock_count(&trk[t]) == oldlocks + 1, "SAVE-held motion redo restores lock");
    proof(musical_signature() == edited, "motion redo restores dirty signature");
    retained(t);
    proof(reads == r && writes == w, "complete motion flow makes zero backend calls");
}
int main(void)
{
    rep = tmpfile();
    if (!rep)
        return 2;
    const unsigned tracks[] = {0, 3, 4, 7};
    for (unsigned i = 0; i < 4; i++) {
        stopped_sound(tracks[i]);
        stopped_pattern(tracks[i]);
        stopped_motion(tracks[i]);
    }
    printf("native_ui_undo: %u checks, %u failures;12 stopped sound/pattern/motion cases and4 actual running "
           "refusals, selected tracks1/4/5/8\n",
           checks, failures);
    return failures ? 1 : 0;
}
