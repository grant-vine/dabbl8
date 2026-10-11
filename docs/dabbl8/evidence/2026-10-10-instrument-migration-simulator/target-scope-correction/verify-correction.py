from pathlib import Path
import hashlib,json,os,subprocess,shutil
b=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=b/'instrument-migration-executor';e=b/'instrument-migration-target-scope-correction';e.mkdir(exist_ok=False)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
source='381537968d16da8f911f4d1ee0ae579f42ca1c3c';parent='6f73dba1576ad12bdc3fa30eac1509cd674dc0a5'
current=b/'v0.1-integration/build/native-capture-4/linked-disassembly.txt';target=b/'v0.1-integration-target-check'
terminal=json.loads((target/'terminal.json').read_text());assert terminal['source']==parent and terminal['target_builds']=='PASS';record=json.loads((target/'target-results.json').read_text())['4']
assert h(b/'v0.1-integration/build/native-capture-4/felucca.bin')==record['sha256']
old=r/'build/felucca.dis';shutil.copy2(old,e/'initial-baseline-disassembly.txt');shutil.copy2(current,e/'parent-current-disassembly.txt')
assert (e/'initial-baseline-disassembly.txt').read_bytes()!= (e/'parent-current-disassembly.txt').read_bytes()
for name in ['terminal.json','target-results.json']:shutil.copy2(target/name,e/('parent-'+name))
# Source-level sole consumer audit excludes docs, includes all test/tool/web code.
p=subprocess.run(['rg','-n','felucca[.]dis|target_budget[.]py','tests','tools','web','--glob','!*.log','--glob','!*.json'],cwd=r,text=True,stdout=subprocess.PIPE);assert p.returncode==0;(e/'disassembly-consumers.txt').write_text(p.stdout)
assert sum('build/felucca.dis tests/target_budget.txt' in line for line in (r/'tests/run_tests.sh').read_text().splitlines())==1
assert subprocess.check_output(['git','diff','--name-only',source,'HEAD','--','firmware','tests','tools'],cwd=r,text=True).strip()==''
env=os.environ.copy()
for key in list(env):
 if key.startswith(('BUDGET_','FELUCCA_','GOLDEN_')):env.pop(key)
known=json.loads((b/'native-integration-check/unchanged-artifacts.json').read_text());gen=json.loads((b/'native-autosave-combined-check/generated-inputs.json').read_text())
assert all(h(r/p)==v for p,v in known.items());assert all(h(r/p)==v for p,v in gen.items())
with (e/'parent-default4-target-budget.log').open('w') as out:
 run=subprocess.run([str(b/'venv/bin/python'),'tests/target_budget.py',str(e/'parent-current-disassembly.txt'),'tests/target_budget.txt'],cwd=r,env=env,stdout=out,stderr=subprocess.STDOUT)
assert run.returncode==0
assert all(h(r/p)==v for p,v in known.items());assert all(h(r/p)==v for p,v in gen.items())
result={'frozen_executable_source':source,'initial_evidence_head':'3ddb05439c0245eb20754f7dff83dc79d7255e20','full_suite_head':source,'initial_full_suite_exit':0,'initial_full_suite_target_cost_scope':'reference baseline disassembly from a660 capture, not current parent6f73 target','correction':'sole disassembly consumer rerun using actual parent6f73 default4 target; all other host results retained unchanged','sole_consumer':'tests/run_tests.sh:490-491 -> tests/target_budget.py','parent_target_source':parent,'parent_current_default4_image':record,'initial_reference_disassembly_sha256':h(e/'initial-baseline-disassembly.txt'),'correct_parent_disassembly_sha256':h(e/'parent-current-disassembly.txt'),'focused_target_budget_exit':run.returncode,'new_target_build':False,'full_suite_rerun':False,'six_baseline_reference_hashes_unchanged':True,'generated_29_inputs_unchanged':True,'native_budget_qualification':False,'hardware_qualification':False}
(e/'result.json').write_text(json.dumps(result,indent=2)+'\n');shutil.copy2(__file__,e/'verify-correction.py');print(json.dumps(result,indent=2))
