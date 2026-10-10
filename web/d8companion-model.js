// SPDX-License-Identifier: GPL-3.0-only
import { descriptors } from './d8descriptors.js';
export function parameterDescriptor(engine, id) {
  if (!Number.isInteger(engine) || engine < 0 || engine >= descriptors.engines || !Number.isInteger(id) || id < 0 || id >= descriptors.params) throw Error('Unknown musical parameter.');
  return id < descriptors.engineStart ? descriptors.common[id] : descriptors.engine[engine].edit[id - descriptors.engineStart];
}
const names = { LVL:'Level', ATK:'Attack', DEC:'Decay', SUS:'Sustain', REL:'Release', FLT:'Filter', PIT:'Pitch', SHP:'Shape', FX:'Effects', RATE:'Rate', WAVE:'Waveform', PHS:'Phase', FADE:'Fade', AMP:'Amplitude', MODE:'Mode', OCT:'Octaves', GATE:'Gate', SWG:'Swing', PROB:'Probability', HOLD:'Hold', ORD:'Order', ROOT:'Root note', SCL:'Scale', QNT:'Quantize', TRN:'Transpose', LEN:'Length', DIV:'Division', DST:'Distortion', CHO:'Chorus', DLY:'Delay', REV:'Reverb', VCE:'Voice mode', GLD:'Glide', PAN:'Pan', MUTE:'Mute', GLMOD:'Glide mode', PRIO:'Note priority', ALLOC:'Voice allocation', DTUNE:'Detune', SLCR:'Slicer', PAT:'Pattern', DEPTH:'Depth', CHRD:'Chord', VOIC:'Voicing', HATCL:'Closed hat', HATOP:'Open hat', DTN:'Detune', MIX:'Mix', NOIS:'Noise', CUT:'Cutoff', RES:'Resonance', DRV:'Drive', KTR:'Key tracking', KIT:'Kit', TUNE:'Tune', TONE:'Tone', DECY:'Decay', SNAP:'Snap', ACC:'Accent' };
export function parameterName(engine, id) {
  const d = parameterDescriptor(engine,id); let group = 'Sound';
  if(id >= 91) group = descriptors.engine[engine].name;
  else if(id>=83) group='Drum'; else if(id>=81) group='Chord';
  else if(id>=61) group='Envelope '+(1+Math.floor((id-61)/5));
  else if(id>=49) group='Modulation'; else if(id>=45) group='Slicer';
  else if(id>=37) group='Voice'; else if(id>=33) group='Effects';
  else if(id>=29) group='Sequence'; else if(id>=25) group='Scale';
  else if(id>=17) group='Arpeggiator'; else if(id>=13) group='Envelope modulation';
  else if(id>=9) group='LFO'; else if(id>=5) group='LFO modulation'; else if(id>=1) group='Amplitude envelope';
  const label = names[d.label] || d.label.replace(/^(SRC|DST|AMT)(\d)$/,(_,kind,n)=>({SRC:'Source',DST:'Destination',AMT:'Amount'}[kind])+' '+n);
  return group+' · '+label;
}
export const noteName = (n) => ['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'][n%12]+(Math.floor(n/12)-1);
export class CompanionModel {
  #session; #open = true; #generation=0; #notify;
  state = { busy:false, selected:0, index:0, metadata:null, dump:null, step:null, reviewRequired:false, message:'' };
  constructor(session,onChange=()=>{}) { if(!session?.tracks?.editable)throw Error('Track editing is unavailable.');this.#session=session;this.#notify=onChange; }
  get editable(){return this.#open && this.#session.tracks.editable;}
  get reason(){return this.#session.tracks.reason;}
  #emit(){if(this.#open)this.#notify(this.state);}
  async #run(task,message){
    if(!this.editable)throw Error(this.reason||'Disconnected.');if(this.state.busy)throw Error('Wait for the pending edit.');
    const generation=this.#generation;this.state.busy=true;this.state.message='Reading device…';this.#emit();
    try{const patch=await task();if(!this.#open||generation!==this.#generation)throw Error('Disconnected.');Object.assign(this.state,patch);this.state.message=message;return patch;}
    catch(e){if(this.#open&&generation===this.#generation)this.state.message=e.message;throw e;}
    finally{if(this.#open&&generation===this.#generation){this.state.busy=false;this.#emit();}}
  }
  async #load(track,index){const c=this.#session.tracks;const metadata=await c.readTracks();if(!this.#open)throw Error('Disconnected.');const dump=await c.dump(track);if(!this.#open)throw Error('Disconnected.');const step=await c.step(track,index);if(!this.#open)throw Error('Disconnected.');return {metadata,dump,step,selected:track,index,reviewRequired:false};}
  refresh(){return this.#run(()=>this.#load(this.state.selected,this.state.index),'Refreshed from device.');}
  select(track){return this.#run(async()=>{await this.#session.tracks.select(track);return this.#load(track,this.state.index);},'Track '+(track+1)+' selected.');}
  readStep(index){return this.#run(()=>this.#load(this.state.selected,index),'Step '+(index+1)+' loaded.');}
  mix(track,update){const copy={...update};return this.#run(async()=>{await this.#session.tracks.mix(track,copy);return this.#load(this.state.selected,this.state.index);},'Track '+(track+1)+' mix updated.');}
  async #sameSound(track){
    if(this.state.reviewRequired)throw Error('Refresh and review the changed sound before editing.');
    const current=await this.#session.tracks.dump(track),previous=this.state.dump;if(!this.#open)throw Error('Disconnected.');
    if(!previous||current.engine!==previous.engine||current.preset!==previous.preset){
      this.state.dump=current;this.state.step=null;this.state.reviewRequired=true;throw Error('The device sound changed. Refresh, review the controls, then apply again.');
    }
  }
  parameter(id,value){
    const track=this.state.selected,d=this.state.dump?parameterDescriptor(this.state.dump.engine,id):null;
    if(!d||!Number.isInteger(value)||value<d.min||value>d.max)return Promise.reject(Error('Choose a value within the displayed musical range.'));
    return this.#run(async()=>{await this.#sameSound(track);await this.#session.tracks.parameter(track,id,value);return this.#load(track,this.state.index);},'Parameter updated.');
  }
  applyStep(update){const track=this.state.selected,index=this.state.index,copy={...update,notes:[...update.notes]};return this.#run(async()=>{await this.#sameSound(track);await this.#session.tracks.step(track,index,copy);return this.#load(track,index);},'Step '+(index+1)+' updated.');}
  close(){this.#open=false;this.#generation++;return this.#session.close();}
}
