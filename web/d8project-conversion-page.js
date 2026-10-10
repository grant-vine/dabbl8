// SPDX-License-Identifier: GPL-3.0-only
import {nativeConverter,prepareProjectBundle,projectBundleZip} from './d8project-conversion.js';
import {descriptors} from './d8descriptors.js';
async function defaultConverter(){const {default:factory}=await import('../build/d8converter/dabbl8_project_convert.mjs');return nativeConverter(factory);}
export function mountProjectConversion(root,{loadConverter=defaultConverter,onSettled=()=>{}}={}) {
  const doc=root.ownerDocument,view=doc.defaultView;let generation=0,alive=true,url=null;
  const el=(tag,attrs={},text='')=>{const n=doc.createElement(tag);for(const [key,value]of Object.entries(attrs))n.setAttribute(key,value);n.textContent=text;return n;};
  const file=(label)=>{const input=el('input',{type:'file','aria-label':label}),field=el('label',{class:'field'});field.append(el('span',{},label),input);return {input,field};};
  const primary=file('Legacy project file'),refs=Array.from({length:4},(_,i)=>file(`Saved slot ${i+1} (optional)`));
  const prepare=el('button',{type:'button',class:'primary'},'Prepare offline bundle'),status=el('p',{class:'status',role:'status','aria-live':'polite'},'Choose a standalone Felucca project. No device connection is needed.'),results=el('div',{});
  const fields=el('div',{class:'fields'});fields.append(...refs.map(x=>x.field));
  root.className='editor-section';root.setAttribute('aria-label','Offline project conversion');
  root.replaceChildren(el('h2',{},'Prepare a project offline'),el('p',{},'Convert legacy tracks 1–4 and start tracks 5–8 empty. Download the complete original files, a migration report and experimental project data together.'),el('p',{class:'small'},'This does not save or upload to an FM-1. Device restore and arrangement playback are not qualified. Keep independent originals. If the project has a chain, choose the actual files for its saved slots below. A complete original snapshot is included when conversion is refused. Files must be 16 MiB or smaller; unreadable or larger files cannot be bundled.'),primary.field,fields,prepare,status,results);
  const inputs=[primary.input,...refs.map(x=>x.input)];
  function clear(){generation++;if(url){view.URL.revokeObjectURL(url);url=null;}results.replaceChildren();status.className='status';prepare.disabled=!primary.input.files?.length;status.textContent='Choose the project and any referenced saved slots, then prepare the bundle.';}
  for(const input of inputs)input.addEventListener('change',clear);
  prepare.disabled=true;
  const read=async(file)=>{if(file.size>16777216)throw Error('Each file must be 16 MiB or smaller. Keep this original separately; no bundle was prepared.');return {name:file.name,bytes:new Uint8Array(await file.arrayBuffer())};};
  prepare.addEventListener('click',async()=>{
    if(prepare.disabled||!alive)return;const selected=primary.input.files?.[0];if(!selected)return;
    const selectedRefs=refs.map(x=>x.input.files?.[0]);clear();const g=generation;prepare.disabled=true;status.textContent='Copying originals and checking the complete project bundle…';
    try {
      const source=await read(selected),references=new Map();
      for(let slot=0;slot<4;slot++)if(selectedRefs[slot])references.set(slot,await read(selectedRefs[slot]));
      const bundle=await prepareProjectBundle(source,references,loadConverter);
      if(!alive||g!==generation)return;
      const report=bundle.report;
      const blob=projectBundleZip(bundle.files);url=view.URL.createObjectURL(blob);
      const download=el('a',{href:url,download:report.accepted?'dabbl8-project-bundle.zip':'dabbl8-originals-and-report.zip',class:'download-bundle'},report.accepted?'Download verified offline bundle':'Download originals and refusal report');
      status.textContent=report.accepted?'Bundle verified. Originals and the report are included. No device data was changed.':`Conversion refused: ${report.reason} The download retains originals and the report, without converted projects.`;
      status.className='status'+(report.accepted?'':' bad');results.append(download);
      if(report.accepted){
        results.append(el('p',{},`${report.conversion.source_format} imported into eight tracks with eight shared sounding voices. Tracks 5–8 begin empty.`));
        for(const change of report.conversion.engine_changes)results.append(el('p',{},`Track ${change.track}: legacy sound migrated to ${descriptors.engine[change.after]?.name||'a supported engine'}.`));
        if(report.conversion.removed_motion_records)results.append(el('p',{class:'notice'},`${report.conversion.removed_motion_records} incompatible automation record(s) were removed from the converted project. The complete originals still contain them.`));
        if(report.conversion.chain_converted)results.append(el('p',{class:'small'},'Saved-slot chain order and repeats were translated into explicit references. Arrangement playback is still pending.'));
      }
      results.append(el('p',{class:'small'},'The ZIP includes SHA-256 hashes and migration details in report.json. This is project data, not firmware. Read the report and keep the original files.'));
    } catch(error){if(alive&&g===generation){status.textContent=error.message||String(error);status.className='status bad';}}
    finally {if(alive&&g===generation)prepare.disabled=!primary.input.files?.length;onSettled();}
  });
  function destroy(){alive=false;clear();for(const input of inputs)input.disabled=true;prepare.disabled=true;view?.removeEventListener('pagehide',hide);}
  const hide=()=>clear();view?.addEventListener('pagehide',hide);return {destroy};
}
