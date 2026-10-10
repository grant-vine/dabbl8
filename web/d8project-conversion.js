// SPDX-License-Identifier: GPL-3.0-only
// Actual C importer via isolated Emscripten MEMFS. No MIDI or device writes.
export async function nativeConverter(factory) {
  let stdout=[],stderr=[];
  const instance=await factory({stdout:(byte)=>{if(byte!==null)stdout.push(byte);},stderr:(byte)=>{if(byte!==null)stderr.push(byte);}});
  return (mode,bytes,mask)=>{
    stdout=[];stderr=[];
    instance.FS.writeFile('/input.bin',bytes);
    try {
      instance.callMain([mode,'/input.bin',String(mask)]);
      const meta=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(Uint8Array.from(mode==='check'?stdout:stderr)));
      if(meta.format!=='dabbl8-project-conversion'||meta.version!==1||meta.accepted!==true)throw Error(meta.reason||'The converter refused this file.');
      return {bytes:Uint8Array.from(stdout),meta};
    } finally {instance.FS.unlink('/input.bin');}
  };
}
const hash=async(bytes)=>Array.from(new Uint8Array(await globalThis.crypto.subtle.digest('SHA-256',bytes)),x=>x.toString(16).padStart(2,'0')).join('');
const equal=(a,b)=>a.length===b.length&&a.every((x,i)=>x===b[i]);
const snapshot=(entry)=>{
  if(!entry||!(entry.bytes instanceof Uint8Array)||typeof entry.name!=='string')throw Error('Choose a project file.');
  return {name:entry.name,bytes:entry.bytes.slice()};
};
export async function prepareProjectBundle(primary,references,loadConverter) {
  primary=snapshot(primary);
  const refs=new Map();
  for(const [slot,entry] of references){
    if(!Number.isInteger(slot)||slot<0||slot>3||refs.has(slot))throw Error('Saved slots must be distinct slots 1 through 4.');
    refs.set(slot,snapshot(entry));
  }
  const files=new Map();
  const report={format:'dabbl8-project-bundle-report',version:1,accepted:false,device_writes:false,firmware_package:false,references:{},warnings:[
    'Experimental project data; no qualified firmware or device upload.',
    'Keep the complete original files. Engine migration can remove incompatible automation and normalize inactive data.',
    'No flash map, runtime arrangement playback, device backup or hardware recovery is qualified.'
  ]};
  const preserve=async(entry,name)=>{files.set(name,entry.bytes);return {source_name:entry.name,original_file:name,original_bytes:entry.bytes.length,original_sha256:await hash(entry.bytes)};};
  Object.assign(report,await preserve(primary,'original.bin'));
  for(const [slot,entry] of [...refs].sort((a,b)=>a[0]-b[0]))report.references[slot]=await preserve(entry,`slot-${slot}-original.bin`);
  const mask=[...refs.keys()].reduce((m,slot)=>m|(1<<slot),0);report.available_project_mask=mask;
  try {
    const convert=await loadConverter();const pending=[];
    for(const [slot,entry] of [...refs].sort((a,b)=>a[0]-b[0])){
      const result=convert('legacy',entry.bytes,mask);report.references[slot].conversion=result.meta;
      pending.push({name:`slot-${slot}.d8p`,...result,entry:report.references[slot]});
    }
    const result=convert('legacy',primary.bytes,mask);pending.push({name:'project.d8p',...result,entry:report});
    for(const output of pending){
      if(output.bytes.length<32||output.bytes.length>7936||String.fromCharCode(...output.bytes.subarray(0,4))!=='D8P1')throw Error('Invalid bounded converted project.');
      const checked=convert('check',output.bytes,mask),canonical=convert('canonical',output.bytes,mask);
      if(!equal(output.bytes,canonical.bytes))throw Error('Converted project failed canonical verification.');
      Object.assign(output.entry,{converted_file:output.name,converted_bytes:output.bytes.length,converted_sha256:await hash(output.bytes),verification:checked.meta});
    }
    for(const output of pending)files.set(output.name,output.bytes);
    report.conversion=result.meta;report.accepted=true;
  } catch(error){
    report.reason=error.message||String(error);
    for(const entry of [report,...Object.values(report.references)])for(const key of ['converted_file','converted_bytes','converted_sha256','verification'])delete entry[key];
  }
  files.set('report.json',new TextEncoder().encode(JSON.stringify(report,null,2)+'\n'));
  return {report,files};
}
// Uncompressed ZIP: complete original files and report in one local download.
export function projectBundleZip(files) {
  const crc=(bytes)=>{let c=0xffffffff;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^0xffffffff)>>>0;};
  const chunks=[],central=[];let offset=0,centralBytes=0;
  const header=(size)=>{const bytes=new Uint8Array(size);return {bytes,v:new DataView(bytes.buffer)};};
  for(const [name,data] of files){
    if(!/^(original\.bin|slot-[0-3]-original\.bin|slot-[0-3]\.d8p|project\.d8p|report\.json)$/.test(name)||!(data instanceof Uint8Array))throw Error('Invalid bundle entry.');
    if(data.length>16777216||files.size>11)throw Error('Bundle exceeds the offline size limit.');
    const encoded=new TextEncoder().encode(name),checksum=crc(data),h=header(30),c=header(46);
    h.v.setUint32(0,0x04034b50,true);h.v.setUint16(4,20,true);h.v.setUint16(12,33,true);h.v.setUint32(14,checksum,true);h.v.setUint32(18,data.length,true);h.v.setUint32(22,data.length,true);h.v.setUint16(26,encoded.length,true);
    c.v.setUint32(0,0x02014b50,true);c.v.setUint16(4,20,true);c.v.setUint16(6,20,true);c.v.setUint16(14,33,true);c.v.setUint32(16,checksum,true);c.v.setUint32(20,data.length,true);c.v.setUint32(24,data.length,true);c.v.setUint16(28,encoded.length,true);c.v.setUint32(42,offset,true);
    chunks.push(h.bytes,encoded,data);central.push(c.bytes,encoded);offset+=30+encoded.length+data.length;centralBytes+=46+encoded.length;
  }
  const end=header(22);end.v.setUint32(0,0x06054b50,true);end.v.setUint16(8,files.size,true);end.v.setUint16(10,files.size,true);end.v.setUint32(12,centralBytes,true);end.v.setUint32(16,offset,true);
  return new Blob([...chunks,...central,end.bytes],{type:'application/zip'});
}
