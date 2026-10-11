/* SPDX-License-Identifier: GPL-3.0-only */
#include "native_arp_arrangement_fixture.c"
static void next(track_t*t){t->arp_pos=0xFFFFFFF;arp_step(t,1,1);}
static int mapping_cases(void){
 for(unsigned ti=0;ti<8;ti++)for(unsigned hold=0;hold<2;hold++)for(unsigned kind=0;kind<4;kind++){
  ui_power_on();song.sel=(uint8_t)ti;song.g[G_CLOCK]=0;song.octave=0;song.grid=0;track_t*t=&trk[ti];
  t->eng_req=0;t->p[P_AMODE]=AM_UP;t->p[P_AHOLD]=(int16_t)hold;t->p[P_AOCT]=1;t->p[P_APROB]=127;t->p[P_ROOT]=0;t->p[P_TRANS]=0;t->p[P_SCALE]=1;t->p[P_QUANT]=(int16_t)(kind==2?QN_OFF:QN_WHITE);t->p[P_CHRD]=0;
  events_block(1);unsigned k=kind==1?12:kind==2?8:7;fm1_in.notes=1u<<k;keyboard_block();next(t);unsigned original=t->arp_note;
  proof(t->nheld==1&&original==kb_map(t,k),"actual keyboard starts mapped arp note");
  if(hold){fm1_in.notes=0;keyboard_block();proof(t->nheld==1&&!t->arp_phys,"real release leaves HOLD latched");}
  go_page(GR_SCALE);if(kind==0)edit_param(0,2);if(kind==1)edit_param(1,4);if(kind==2)edit_param(2,2);if(kind==3)edit_param(3,12);
  unsigned expected=kb_map(t,k);next(t);
  printf("track=%u hold=%u change=%u original=%u expected=%u actual=%u held=%u\n",ti+1,hold,kind,original,expected,t->arp_note,t->nheld);
  proof(expected==KB_SILENT?!t->arp_note:t->arp_note==expected,"ARP follows actual current SCL control mapping");
  unsigned before_oct=t->arp_note;press(B_OCTUP);next(t);proof(song.octave==1&&t->arp_note==before_oct,"actual octave button preserves captured octave of held/latched key");press(B_OCTDN);
  fm1_in.notes=0;keyboard_block();t->p[P_AHOLD]=0;events_block(1);next(t);proof(!t->nheld&&!t->arp_note,"final clear releases sounding arp");
 }
 for(unsigned ti=0;ti<8;ti++){
  ui_power_on();song.sel=(uint8_t)ti;song.g[G_ROUTE]=0;track_t*t=&trk[ti];t->eng_req=0;t->p[P_AMODE]=AM_UP;t->p[P_AHOLD]=0;t->p[P_AOCT]=1;t->p[P_APROB]=127;t->p[P_CHRD]=0;t->p[P_QUANT]=QN_WHITE;t->p[P_SCALE]=1;
  midi_note_event(ti,61,100);next(t);proof(t->nheld==1&&t->arp_note==61,"actual MIDI note starts as absolute pitch");
  go_page(GR_SCALE);edit_param(0,2);edit_param(1,4);edit_param(3,12);next(t);proof(t->arp_note==61,"mapped-key controls do not reinterpret actual MIDI pitch");
  midi_note_event(ti,61,0);next(t);proof(!t->nheld&&!t->arp_note,"actual MIDI release clears mapped provenance and sound");
 }
 for(unsigned ti=0;ti<8;ti++){
  ui_power_on();track_t*t=&trk[ti];song.sel=(uint8_t)ti;t->eng_req=0;t->p[P_AMODE]=AM_UP;t->p[P_AHOLD]=0;t->p[P_AOCT]=1;t->p[P_APROB]=127;t->p[P_QUANT]=QN_OFF;events_block(1);
  for(unsigned i=0;i<16;i++){song.octave=(int)(i%7)-3;in_key=(uint8_t)(i+1);kb_note[i]=kb_map(t,i);arp_add(t,kb_note[i]);in_key=0;}
  /* Different octaves may alias; rebuild capacity with explicit distinct absolute MIDI pitches. */
  t->nheld=t->arp_phys=0;
  for(unsigned i=0;i<16;i++)arp_add(t,40+i);
  proof(t->nheld==16&&t->arp_phys==16,"full sixteen distinct notes fit");arp_add(t,90);proof(t->nheld==16,"seventeenth note cannot overflow metadata");
  arp_add(t,40);proof(t->nheld==16,"repeated pitch does not append metadata");arp_remove(t,45);proof(t->nheld==15,"release compacts full chord");
  for(unsigned i=0;i<t->nheld;i++)proof(!held_key[ti][i]&&!held_dt[ti][i],"MIDI insertion/compaction excludes stale panel provenance");
  t->p[P_AHOLD]=1;t->arp_phys=0;in_key=8;song.octave=-3;kb_note[7]=kb_map(t,7);arp_add(t,kb_note[7]);in_key=0;
  proof(t->nheld==1&&held_key[ti][0]==8,"HOLD replaces chord and records minus-three octave");arp_remove(t,kb_note[7]);
  song.octave=3;in_key=8;kb_note[7]=kb_map(t,7);arp_add(t,kb_note[7]);in_key=0;proof(t->nheld==1&&held_key[ti][0]==200,"HOLD replaces chord and records plus-three octave");
  fm1_in.notes=0;arp_remove(t,kb_note[7]);t->aholdp=1;t->p[P_AHOLD]=0;events_block(1);proof(!t->nheld&&!t->arp_note,"events path drops released HOLD chord");
  midi_note_event(ti,61,100);proof(t->nheld==1&&!held_key[ti][0],"fresh MIDI after latch uses absolute provenance");midi_silence_track(ti);proof(!t->nheld&&!t->arp_phys,"MIDI track silence invalidates live history");
  t->p[P_AMODE]=AM_UP;arp_add(t,60);apply_preset_to(t,0);events_block(1);proof(!t->nheld&&!t->arp_phys,"sound preset panic resets live history");
 }
 printf("metadata_state_bytes=%zu key_octave_range=-3..3\n",sizeof held_key+sizeof held_dt+sizeof in_key);
 printf("arp_mapping_probe: %u checks %u failures\n",checks,failures);return failures?1:0;
}

static void panel_arp(unsigned ti,unsigned hold,unsigned keys)
{
 song.sel=(uint8_t)ti;song.g[G_CLOCK]=0;song.octave=0;song.grid=0;
 track_t*t=&trk[ti];t->eng_req=0;t->p[P_AMODE]=AM_UP;t->p[P_AHOLD]=(int16_t)hold;t->p[P_AOCT]=1;t->p[P_APROB]=127;t->p[P_ROOT]=0;t->p[P_TRANS]=0;t->p[P_SCALE]=1;t->p[P_QUANT]=QN_OFF;t->p[P_CHRD]=0;
 events_block(1);fm1_in.notes=keys;keyboard_block();next(t);
}
static void native_cases(void)
{
 for(unsigned ti=0;ti<8;ti++)for(unsigned hold=0;hold<2;hold++){
  seed();panel_arp(ti,hold,1u<<7);track_t*t=&trk[ti];proof(t->nheld==1&&t->arp_note==60,"native fixture real panel ARP");
  if(hold){fm1_in.notes=0;keyboard_block();}
  unsigned key=held_key[ti][0];int dt=held_dt[ti][0];unsigned prior_writes=writes;
  size_t n=0;proof(!d8p1_capture_runtime(overlay_wire,sizeof overlay_wire,&n),"actual native capture with held ARP");
  proof(!d8p1_load_runtime(overlay_wire,n,7),"actual native full adoption with held ARP");events_block(1);
  proof(!t->nheld&&!t->arp_note,"full native adoption invalidates live metadata before ARP");
  fm1_in.notes=0;keyboard_block();t->p[P_AMODE]=AM_UP;t->p[P_AHOLD]=0;events_block(1);midi_note_event(ti,61,100);next(t);
  proof(t->nheld==1&&!held_key[ti][0]&&t->arp_note==61,"MIDI after full adoption overwrites stale panel metadata");midi_note_event(ti,61,0);
  panel_arp(ti,hold,1u<<7);key=held_key[ti][0];dt=held_dt[ti][0];
  prepare_start();boundary(1);boundary(2);
  proof(chain.row==1&&d8arr_running()&&t->nheld==1,"actual native row transition retains logical held ARP");
  proof(held_key[ti][0]==key&&held_dt[ti][0]==dt,"scene/source transition retains captured key provenance");next(t);proof(t->arp_note==60,"actual row transition preserves base ARP pitch");
  seq_stop();transport_req=0;fm1_in.notes=0;keyboard_block();t->p[P_AHOLD]=0;events_block(1);
  proof(!t->nheld&&!t->arp_note&&writes==prior_writes,"native scene/adoption checks write no storage and release normally");
 }
}
static void compatibility_cases(void)
{
 for(unsigned ti=0;ti<8;ti++){
  ui_power_on();panel_arp(ti,0,(1u<<7)|(1u<<8));track_t*t=&trk[ti];t->p[P_QUANT]=QN_SNAP;
  unsigned list[64],cnt=0,len=arp_key_list(t,list,&cnt);proof(len==2&&cnt==2&&list[0]==60&&list[1]==60,"distinct mapped keys converging keep duplicate rhythm slots");
  t->p[P_AMODE]=AM_CHORD;next(t);proof(t->arp_note==60&&t->arp_ch[0]==60,"mapped CHORD retains duplicate note semantics");
  t->p[P_AMODE]=AM_REPEAT;next(t);proof(t->arp_note==60,"mapped REPEAT uses last surviving mapped key");
  t->p[P_QUANT]=QN_WHITE;t->p[P_TRANS]=12;next(t);proof(t->arp_note==72,"REPEAT skips silent final black key and maps last surviving key");
  fm1_in.notes=0;keyboard_block();panel_arp(ti,0,1u<<8);t->p[P_QUANT]=QN_WHITE;next(t);proof(!t->arp_note&&!t->arp_ch[0],"all-silent remapping releases prior voice before list indexing");
  fm1_in.notes=0;keyboard_block();t->p[P_QUANT]=QN_OFF;t->p[P_TRANS]=0;t->p[P_AMODE]=AM_ORD;
  midi_note_event(ti,72,100);midi_note_event(ti,60,100);next(t);proof(t->arp_note==60,"ORD retains original UP sorting when AORDER off");
  t->p[P_AORDER]=1;t->arp_idx=0xffffffffu;next(t);proof(t->arp_note==72,"AORDER retains actual insertion ordering for ORD");midi_silence_track(ti);
 }
}

static void captured_chord_cases(void)
{
 for(unsigned ti=0;ti<8;ti++)for(unsigned hold=0;hold<2;hold++)for(unsigned mode=0;mode<2;mode++)for(unsigned change=0;change<4;change++){
  ui_power_on();panel_arp(ti,hold,0);track_t*t=&trk[ti];t->p[P_VOICE]=V_POLY;t->p[P_CHRD]=(int16_t)(mode?CH_DIA3:CH_MAJ);t->p[P_VOIC]=0;
  unsigned k=change==1?12:change==2?8:9;t->p[P_QUANT]=(int16_t)(change==2?QN_OFF:QN_WHITE);
  fm1_in.notes=1u<<k;keyboard_block();next(t);
  unsigned captured[CHORD_MAX],count=kb_chn[k],base=kb_note[k];
  proof(count==3&&t->nheld==3,"actual CHRD panel key captures a polyphonic triad");
  for(unsigned j=0;j<count;j++){
   captured[j]=kb_chord[k][j];
   proof(t->held[j]==captured[j]&&held_dt[ti][j]==(int)captured[j]-(int)base,"actual chord interval stored with physical key provenance");
  }
  if(change==0)proof(captured[1]-base==(mode?3u:4u),"DIA3 minor versus fixed MAJ interval is genuinely exercised");
  if(hold){fm1_in.notes=0;keyboard_block();proof(t->nheld==3&&!t->arp_phys,"actual chord HOLD release retains all captured intervals");}
  go_page(GR_SCALE);if(change==0)edit_param(0,2);if(change==1)edit_param(1,4);if(change==2)edit_param(2,2);if(change==3)edit_param(3,12);
  unsigned mapped=kb_map(t,k),list[64],cnt=0,len=arp_key_list(t,list,&cnt);
  if(mapped==KB_SILENT){proof(!cnt&&!len,"QNT silence excludes whole captured chord");next(t);proof(!t->arp_note&&!t->arp_ch[0]&&!t->arp_ch[1],"QNT silence releases previously sounding chord input");}
  else{
   proof(cnt==count&&len==count,"remapped chord retains triad rhythm slots");
   for(unsigned j=0;j<count;j++)proof(list[j]==mapped+captured[j]-base,"ROOT SCALE TRN remap key while preserving captured chord intervals");
   t->arp_idx=0xffffffffu;next(t);proof(t->arp_note==list[0],"actual ARP voice emits first remapped captured chord pitch");
  }
  fm1_in.notes=0;keyboard_block();t->p[P_AHOLD]=0;events_block(1);proof(!t->nheld&&!t->arp_note,"actual chord release leaves no stale sounding/provenance slots");
 }
}
int main(void){mapping_cases();native_cases();compatibility_cases();captured_chord_cases();printf("native_arp_key_mapping: %u checks %u failures; host only\n",checks,failures);return failures?1:0;}
