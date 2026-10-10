// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {nativeConverter,prepareProjectBundle,projectBundleZip} from '../web/d8project-conversion.js';
const root=path.resolve(import.meta.dirname,'..'),fixtures=path.join(root,'tests/fixtures/projects');
const factory=(await import(pathToFileURL(path.resolve(process.argv[2])))).default;
const native=path.resolve(process.argv[3]);
let checks=0;const check=(condition,label)=>{assert.ok(condition,label);checks++;};
const entry=(name)=>({name,bytes:new Uint8Array(fs.readFileSync(path.join(fixtures,name)))});
const refs=()=>new Map(Array.from({length:4},(_,slot)=>[slot,entry('fun9.bin')]));
const load=()=>nativeConverter(factory);
const pristine=new Map(fs.readdirSync(fixtures).filter(n=>n.endsWith('.bin')).map(n=>[n,fs.readFileSync(path.join(fixtures,n))]));
const tmp=fs.mkdtempSync(path.join(os.tmpdir(),'dabbl8-browser-convert-'));
try {
  for(const name of [...pristine.keys()].filter(n=>!n.includes('.expected-fun9'))){
    const original=entry(name),bundle=await prepareProjectBundle(original,refs(),load);
    check(bundle.report.accepted,'real legacy conversion accepted');
    check(bundle.files.size===11&&!bundle.report.device_writes&&!bundle.report.firmware_package,'complete offline original/report/project bundle');
    check(Buffer.from(bundle.files.get('original.bin')).equals(pristine.get(name)),'primary original exact');
    const expected=spawnSync(native,['legacy',path.join(fixtures,name),'15']);assert.equal(expected.status,0);
    check(Buffer.from(bundle.files.get('project.d8p')).equals(expected.stdout),'WASM matches actual native output byte exact');
    assert.deepEqual(bundle.report.conversion,JSON.parse(expected.stderr));checks++;
    check(bundle.report.conversion.track_count===8&&bundle.report.conversion.shared_voices===8,'shared voices retained');
    for(let slot=0;slot<4;slot++){
      check(Buffer.from(bundle.files.get(`slot-${slot}-original.bin`)).equals(pristine.get('fun9.bin')),'reference original exact');
      check(bundle.report.references[slot].verification.accepted,'every reference output independently verified');
    }
    fs.writeFileSync(path.join(tmp,'bundle.zip'),Buffer.from(await projectBundleZip(bundle.files).arrayBuffer()));
    const zip=spawnSync(process.env.PYTHON||'python3',['-c',`import zipfile,json,sys,hashlib\nz=zipfile.ZipFile(sys.argv[1]);assert z.testzip() is None;assert len(z.namelist())==11\nr=json.loads(z.read('report.json'));assert r['accepted'];assert z.read('original.bin')==open(sys.argv[2],'rb').read()\nfor row in [r,*r['references'].values()]:\n assert hashlib.sha256(z.read(row['original_file'])).hexdigest()==row['original_sha256']\n assert hashlib.sha256(z.read(row['converted_file'])).hexdigest()==row['converted_sha256']\n assert z.getinfo(row['original_file']).date_time==(1980,1,1,0,0,0)`,path.join(tmp,'bundle.zip'),path.join(fixtures,name)]);
    check(zip.status===0,'independent Python ZIP CRC, originals, report and all output hashes');
  }
  for(const context of [new Map(),new Map([[0,entry('fun9.bin')]]),new Map([[3,entry('fun9.bin')]])]){
    const bundle=await prepareProjectBundle(entry('fun9.bin'),context,load);
    check(!bundle.report.accepted&&![...bundle.files.keys()].some(n=>n.endsWith('.d8p')),'missing saved slots never alias');
    check(Buffer.from(bundle.files.get('original.bin')).equals(pristine.get('fun9.bin')),'refused original retained');
  }
  const corrupted=entry('fun9.bin');corrupted.bytes[100]^=1;
  const context=refs();context.set(3,corrupted);
  const bad=await prepareProjectBundle(entry('fun9.bin'),context,load);
  check(!bad.report.accepted&&bad.files.size===6,'bad reference keeps all five originals and report only');
  check(Buffer.from(bad.files.get('slot-3-original.bin')).equals(Buffer.from(corrupted.bytes)),'corrupted original exact');
  for(const bytes of [new Uint8Array(),new Uint8Array([70,85,78]),entry('fun9.bin').bytes.slice(0,-1),new Uint8Array(9000),new Uint8Array(fs.readFileSync(path.join(root,'tests/fixtures/d8p1/unknown-optional.d8p')))]){
    const bundle=await prepareProjectBundle({name:'unsupported.bin',bytes},refs(),load);
    check(!bundle.report.accepted&&bundle.files.size===6,'unknown damaged oversize refuses output but preserves all originals');
    check(Buffer.from(bundle.files.get('original.bin')).equals(Buffer.from(bytes)),'unknown original exact');
  }
  const unavailable=await prepareProjectBundle(entry('fun9.bin'),refs(),async()=>{throw Error('Converter unavailable');});
  check(!unavailable.report.accepted&&unavailable.files.size===6,'missing compiler module preserves originals and report');
  const actual=await load();
  const late=await prepareProjectBundle(entry('fun9.bin'),refs(),async()=>(mode,bytes,mask)=>{
    const result=actual(mode,bytes,mask);if(mode==='canonical')result.bytes=new Uint8Array([...result.bytes,1]);return result;
  });
  check(!late.report.accepted&&late.files.size===6,'late verification publishes no converted artifacts');
  check([late.report,...Object.values(late.report.references)].every(r=>!r.converted_file),'failure report cannot claim unpublished files');
  const original=entry('fun9.bin'),related=refs();let release;
  const pending=prepareProjectBundle(original,related,async()=>{await new Promise(resolve=>release=resolve);return actual;});
  original.bytes.fill(0);for(const ref of related.values())ref.bytes.fill(0);
  while(!release)await new Promise(resolve=>setTimeout(resolve,0));release();
  const protectedBundle=await pending;
  check(protectedBundle.report.accepted&&Buffer.from(protectedBundle.files.get('original.bin')).equals(pristine.get('fun9.bin')),'asynchronous caller mutation cannot alter copied snapshots');
  for(const [name,bytes]of pristine)assert.deepEqual(fs.readFileSync(path.join(fixtures,name)),bytes);checks++;
} finally {fs.rmSync(tmp,{recursive:true,force:true});}
console.log(`Browser project conversion: ${checks} checks passed; actual C WASM/native parity, independent ZIP verification, no device writes`);
