// SPDX-License-Identifier: GPL-3.0-only
import { mountProjectConversion } from './d8project-conversion-page.js';
import { D8MidiSession } from './d8midi.js';
import { CompanionModel, parameterDescriptor, parameterName, noteName } from './d8companion-model.js';
import { descriptors } from './d8descriptors.js';
export function mountCompanion(root,{getAccess=()=>{if(!globalThis.isSecureContext||!globalThis.navigator?.requestMIDIAccess)throw Error('Use Chrome on localhost or HTTPS with Web MIDI support.');return navigator.requestMIDIAccess({sysex:true});},connect=(...args)=>D8MidiSession.connect(...args)}={}){
  const doc=root.ownerDocument;let access=null,session=null,model=null,generation=0,abort=null,working=false,selectedParameter=39;
  const el=(tag,attrs={},...children)=>{const n=doc.createElement(tag);for(const [k,v]of Object.entries(attrs)){if(k.startsWith('on'))n.addEventListener(k.slice(2),v);else if(k==='class')n.className=v;else n.setAttribute(k,v);}for(const c of children.flat())n.append(typeof c==='string'?doc.createTextNode(c):c);return n;};
  const button=(label,fn,attrs={})=>el('button',{type:'button',onclick:fn,...attrs},label);
  const field=(label,input)=>el('label',{class:'field'},el('span',{},label),input);
  const number=(value,min,max)=>{const n=el('input',{type:'number',min,max,step:1});n.value=String(value);return n;};
  const checkbox=(checked,label)=>{const n=el('input',{type:'checkbox'});n.checked=checked;return {input:n,node:el('label',{class:'check'},n,label)};};
  const select=(options,value)=>{const n=el('select',{});for(const [v,label]of options)n.append(el('option',{value:v},label));n.value=String(value);return n;};
  const heading=el('header',{},el('p',{class:'eyebrow'},'Dabbl8 · development companion'),el('h1',{},'Eight tracks. One view.'),el('p',{},'Edit the in-memory sequencer on compatible development firmware. Eight tracks share eight sounding voices.'));
  const notice=el('p',{class:'notice'},'Development tool: host tests have passed; physical MIDI qualification is pending. Changes stay in memory. Device project save, backup and firmware installation are unavailable here. Offline project conversion is available separately below.');
  const status=el('p',{class:'status',role:'status','aria-live':'polite'},'Choose MIDI ports to begin.');
  const identity=el('p',{class:'identity small'}),inputSelect=el('select',{'aria-label':'MIDI input'}),outputSelect=el('select',{'aria-label':'MIDI output'});
  const find=button('Find MIDI ports',findPorts),join=button('Connect',connectPorts,{class:'primary'}),leave=button('Disconnect',disconnect),refresh=button('Refresh from device',()=>act((m)=>m.refresh()));
  const connection=el('section',{class:'connection','aria-label':'Connection'},el('h2',{},'Connection'),el('div',{class:'row'},field('Receive from',inputSelect),field('Send to',outputSelect),find,join,leave),identity,status);
  const controls=el('div',{}),foot=el('footer',{},el('p',{class:'small'},'Built on Felucca by Hügelton Instruments and its contributors. Dabbl8 is GPL-3.0-only.'),el('p',{class:'small'},el('a',{href:'https://github.com/grant-vine/dabbl8'},'Source and development status'),' · ',el('a',{href:'editor.html'},'Legacy four-track editor'),' · ',el('a',{href:'https://github.com/grant-vine/dabbl8/blob/dabbl8/develop/LICENSING.md'},'Licensing')));
  const conversionRoot=el('section',{});
  root.replaceChildren(heading,notice,connection,controls,conversionRoot,foot);
  const conversion=mountProjectConversion(conversionRoot);
  function setStatus(message,bad=false){status.textContent=message;status.className='status'+(bad?' bad':'');}
  function availability(){const busy=working||Boolean(model?.state.busy);find.disabled=busy||Boolean(session);join.disabled=busy||Boolean(session)||!inputSelect.value||!outputSelect.value;inputSelect.disabled=busy||Boolean(session);outputSelect.disabled=inputSelect.disabled;leave.disabled=!working&&!session;refresh.disabled=busy||!model?.editable;}
  function render(){
    controls.replaceChildren();availability();if(!model)return;
    const renderedModel=model,viewGeneration=generation;const actView=(operation)=>{if(model!==renderedModel||generation!==viewGeneration)return;return act(operation);};
    const s=model.state,c=session.tracks;identity.textContent=`${c.info.version} · ${c.caps.tracks} tracks · ${c.caps.voices} shared voices · ${c.caps.steps} steps per track`;
    setStatus(s.message||'Connected.');if(!s.metadata)return;
    const tracks=el('div',{class:'tracks'});
    for(const t of s.metadata.tracks){
      const level=number(t.level,0,127),mute=checkbox(t.mute,'Mute');
      const card=el('article',{class:'track'+(t.track===s.selected?' selected':''),'aria-label':'Track '+(t.track+1)},button('Track '+(t.track+1),()=>actView((m)=>m.select(t.track)),{class:'select-track','aria-pressed':String(t.track===s.selected)}),el('p',{},`${descriptors.engine[t.engine].name} · ${t.armed?'record armed':'not armed'}${t.track===s.metadata.selected?' · device selected':''}`),field('Level',level),el('div',{class:'row'},mute.node,button('Apply mix',()=>{const update={level:level.valueAsNumber,mute:mute.input.checked};actView((m)=>m.mix(t.track,update));})));
      tracks.append(card);
    }
    controls.append(el('section',{'aria-label':'Tracks'},el('div',{class:'row step-head'},el('h2',{},'Tracks'),refresh),el('p',{class:'small'},'Use the FM-1 for playback, recording and engine or preset changes. Refresh after changes on the device; refreshing replaces unapplied form values.'),tracks));
    if(!s.dump)return;
    const opts=Array.from({length:c.caps.params},(_,id)=>[id,parameterName(s.dump.engine,id)]),param=select(opts,selectedParameter);
    const valueBox=el('div',{class:'field'});let valueInput;
    const buildValue=()=>{selectedParameter=Number(param.value);const d=parameterDescriptor(s.dump.engine,selectedParameter),v=s.dump.params[selectedParameter];
      valueInput=d.names?select(d.names.map((name,i)=>[d.min+i,name]),v):number(v,d.min,d.max);
      valueBox.replaceChildren(field(d.names?'Setting':`Value (${d.min} to ${d.max}${d.unit?' '+d.unit:''})`,valueInput));
      if(selectedParameter===39)valueBox.append(el('span',{class:'small'},'Negative = left · 0 = center · positive = right'));
    };
    param.addEventListener('change',buildValue);buildValue();
    controls.append(el('section',{class:'editor-section','aria-label':'Sound parameters'},el('h2',{},`Track ${s.selected+1} · ${descriptors.engine[s.dump.engine].name}`),el('div',{class:'fields'},field('Musical parameter',param),valueBox),el('p',{class:'small'},'Values use the firmware’s native musical range. Sound changes on the device require a refresh before editing. Avoid changing engines or presets while applying an edit.'),button('Apply parameter',()=>{const id=Number(param.value),value=valueInput.tagName==='INPUT'?valueInput.valueAsNumber:Number(valueInput.value);actView((m)=>m.parameter(id,value));},{class:'primary',...(s.reviewRequired?{disabled:''}:{})})));
    const index=select(Array.from({length:c.caps.steps},(_,i)=>[i,'Step '+(i+1)]),s.index);index.addEventListener('change',()=>actView((m)=>m.readStep(Number(index.value))));
    const section=el('section',{class:'editor-section','aria-label':'Step editor'},el('div',{class:'row step-head'},el('h2',{},'Sequencer step'),field('Edit step',index)));
    if(s.step){
      const step=s.step,type=select([[0,'Note'],[1,'Tie'],[2,'Rest']],step.time),count=number(step.n,0,4),velocity=number(step.vel,0,127),chance=number(step.chance,0,100),ratchet=select([[1,'1 ×'],[2,'2 ×'],[3,'3 ×'],[4,'4 ×']],step.ratchet),accent=checkbox(Boolean(step.flags&1),'Accent'),slide=checkbox(Boolean(step.flags&2),'Slide');
      const notes=step.notes.map((n,i)=>{const input=number(n,0,127),label=el('span',{},noteName(n));input.addEventListener('input',()=>{label.textContent=Number.isInteger(input.valueAsNumber)&&input.valueAsNumber>=0&&input.valueAsNumber<=127?noteName(input.valueAsNumber):'Choose a note from 0 to 127';});return {input,node:el('label',{class:'field'},el('span',{},'Note '+(i+1)),input,label)};});
      const drumNames=['Kick','Snare','Clap','Closed hat','Open hat','Tom','Rim','Bell'];
      const drums=drumNames.map((name,i)=>{const hit=checkbox(Boolean(step.hit&(1<<i)),'Hit'),acc=checkbox(Boolean(step.acc&(1<<i)),'Accent');hit.input.addEventListener('change',()=>{if(!hit.input.checked)acc.input.checked=false;});acc.input.addEventListener('change',()=>{if(acc.input.checked)hit.input.checked=true;});return {hit:hit.input,acc:acc.input,node:el('div',{class:'drum',role:'group','aria-label':name},el('strong',{},name),hit.node,acc.node)};});
      section.append(el('div',{class:'fields step-form'},field('Step type',type),field('Active notes',count),field('Velocity',velocity),field('Chance (%)',chance),field('Ratchet',ratchet)),el('div',{class:'fields step-form'},...notes.map((x)=>x.node)),el('div',{class:'row step-form'},accent.node,slide.node),el('div',{class:'drums'},...drums.map((x)=>x.node)),button('Apply step',()=>{const update={n:count.valueAsNumber,notes:notes.map((x)=>x.input.valueAsNumber),time:Number(type.value),flags:Number(accent.input.checked)|(Number(slide.input.checked)<<1),vel:velocity.valueAsNumber,chance:chance.valueAsNumber,ratchet:Number(ratchet.value),hit:drums.reduce((mask,x,i)=>mask|(Number(x.hit.checked)<<i),0),acc:drums.reduce((mask,x,i)=>mask|(Number(x.acc.checked)<<i),0)};actView((m)=>m.applyStep(update));},{class:'primary'}));
    }else section.append(el('p',{},'Refresh and review the changed sound before editing a step.'));
    controls.append(section);
    for(const n of controls.querySelectorAll('button,input,select'))n.disabled=n.disabled||s.busy||!model.editable;
  }
  async function act(operation){const current=model,g=generation;if(!current)return;try{await operation(current);}catch(e){if(g===generation&&model===current)setStatus(e.message,true);}finally{if(g===generation&&model===current)availability();}}
  async function findPorts(){if(working||session)return;working=true;const g=++generation;availability();setStatus('Requesting MIDI access…');try{
    const result=await getAccess();if(g!==generation)return;if(!result.sysexEnabled)throw Error('SysEx access was not enabled.');access=result;
    for(const [select,ports]of [[inputSelect,result.inputs],[outputSelect,result.outputs]]){select.replaceChildren(el('option',{value:''},'Choose a port'));for(const p of ports.values())if(p.state==='connected')select.append(el('option',{value:p.id},p.name||p.id));}
    setStatus('Select the FM-1 input and output, then connect.');
  }catch(e){if(g===generation)setStatus(e.message,true);}finally{if(g===generation){working=false;availability();}}}
  async function connectPorts(){if(working||session)return;const input=access?.inputs.get(inputSelect.value),output=access?.outputs.get(outputSelect.value);if(!input||!output){setStatus('Choose connected MIDI input and output ports.',true);return;}
    working=true;abort=new AbortController();const g=++generation;availability();setStatus('Opening ports and checking firmware…');let opened;
    try{opened=await connect(input,output,{signal:abort.signal,onClose:(reason)=>{if(g!==generation)return;model?.close();model=null;session=null;working=false;generation++;identity.textContent='';controls.replaceChildren();setStatus(reason,true);availability();}});
      if(g!==generation){await opened.close();return;}session=opened;working=false;const client=opened.tracks;
      if(!client.editable){identity.textContent=client.info.version;setStatus(client.reason+' Use the legacy editor for supported four-track firmware.',true);availability();return;}
      model=new CompanionModel(opened,()=>{if(g===generation)render();});await act((m)=>m.refresh());
    }catch(e){if(g===generation)setStatus(e.message,true);if(opened&&g===generation)await opened.close();}
    finally{if(g===generation){working=false;availability();}}
  }
  async function disconnect(){const old=model,oldSession=session;generation++;abort?.abort();model=null;session=null;working=false;identity.textContent='';controls.replaceChildren();setStatus('Disconnected.');availability();await (old?old.close():oldSession?.close());}
  inputSelect.addEventListener('change',availability);outputSelect.addEventListener('change',availability);availability();
  const pagehide=()=>{void disconnect();};doc.defaultView?.addEventListener('pagehide',pagehide);
  return {disconnect,destroy:()=>{conversion.destroy();doc.defaultView?.removeEventListener('pagehide',pagehide);return disconnect();}};
}
