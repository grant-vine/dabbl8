from pathlib import Path
import json,hashlib,shutil,subprocess
r=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-capability-capture');out=Path('/private/tmp/d8-engine-capture-matrix');sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs=json.loads((out/'inputs.json').read_text());results=json.loads((out/'results.json').read_text());negative=json.loads((out/'negative-results.json').read_text());assert len(results)==32 and all(x['exit']==0 for x in results)
for row in results:assert sha(out/(row['name']+'.log'))==row['log_sha256']
for name,h in inputs['source_hashes'].items():assert sha(r/name)==h
for name,h in inputs['gen_hashes'].items():assert sha(r/'build/gen'/name)==h
for row in results:
 if row['name'].endswith('-run'):
  text=(out/(row['name']+'.log')).read_text();assert ('8775 checks 0 failures tracks=8' if '-8-' in row['name'] else '1943 assertions PASS; host only') in text
for row in negative['rows']:assert sha(out/(row['name']+'.log'))==row['log_sha256']
sh=subprocess.run(['sh','-n','tests/run_tests.sh'],cwd=r,capture_output=True,text=True);assert not sh.returncode
receipt={'dependency_head':inputs['head'],'source_hashes':inputs['source_hashes'],'generated_header_hashes':inputs['gen_hashes'],'generated_header_source':'../uac-array-bounds/build/gen','compiler':inputs['compiler'],'python':subprocess.check_output(['python3','--version'],text=True).strip(),'positive':results,'negative_controls':negative,'result':'16 host executions PASS; expected late-dispatch runtime and incorrect-four-track compile refusals observed','coverage':{'native8_capture_and_owned_arena':{'executions':8,'checks_each':8775,'FM4':[0,1],'SLICE':[0,1],'contexts':['active full capture initial phase','active full capture after one status/poll','exclusive main migration workspace owner'],'queries_per_execution':1152,'snapshots':['caller input bytes','entire capture object','entire capture-change state','entire workspace bytes','migration owner and generation','hardware read counter','NOR bytes and write counter'],'responses':'Exact schema2 mask/flags0/frame or version2/bounds3 error; all128schemas with lengths1,2,3 in each context'},'legacy4':{'executions':8,'checks_each':1943,'scope':'Existing actual C capability/schema1/refusal runtime-state fixture. Native capture/owned-arena protocol does not exist at four tracks; NOT APPLICABLE, never overridden to8.'}},'runner_syntax_exit':0,'production_delta':subprocess.check_output(['git','diff','HEAD','--','firmware','tools','web','ci','.github'],cwd=r,text=True),'limitations':['Apple clang21 host, not hosted clang18 or targetcompiler qualification.','Inherited sanitizer exclusions signed-integer-overflow,shift,bounds,object-size,pointer-overflow unchanged.','Focused tests only, no full-suite rerun, targetbuild, package, physical MIDI/capture/recovery/device.','No reduced selectable profiles, writes, authority flags, budgets or golden changes.','Prior CI123 had fixture source but no invocation; PR127 source already restored paired default-native8 invocation. This strengthens that existing fixture/matrix, not a duplicate new protocol.']}
assert receipt['production_delta']==''
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');(out/'candidate.diff').write_text(subprocess.check_output(['git','diff','HEAD','--','tests'],cwd=r,text=True))
dest=r/'docs/dabbl8/evidence/2026-10-11-engine-capability-capture-matrix';dest.mkdir(parents=True,exist_ok=False)
for p in out.iterdir():
 if p.is_file() and (p.suffix in ['.json','.py','.log','.diff']):shutil.copy2(p,dest/p.name)
manifest={p.name:sha(p) for p in sorted(dest.iterdir()) if p.is_file()};(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('receipt',sha(dest/'receipt.json'),'manifest',sha(dest/'manifest.json'))
