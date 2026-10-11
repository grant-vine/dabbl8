from pathlib import Path
import subprocess,json,hashlib
root=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-ui-availability');out=Path('/private/tmp/d8-engine-ui-availability-evidence');ctrl=out/'controls';ctrl.mkdir(exist_ok=True)
for f in subprocess.check_output(['git','ls-files'],cwd=root,text=True).splitlines()+['tests/engine_ui_availability_test.c']:
 p=root/f
 if p.is_file():q=ctrl/f;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(p.read_bytes())
(ctrl/'build').mkdir(exist_ok=True);(ctrl/'build/gen').symlink_to(root/'build/gen',target_is_directory=True)
mutants=[('engine-count','firmware/src/engines.c','static unsigned eng_count_mask(unsigned mask)\n{','static unsigned eng_count_mask(unsigned mask)\n{\n    mask=D8ENGINE_EFFECTIVE_MASK;'),('user-filter','firmware/src/upreset.c','n += !up_ready(k, mask);','n += up_used(k);'),('startup-fallback','firmware/src/engines.c','if (rc == D8ENGINE_UNAVAILABLE) {','if (rc == D8ENGINE_UNAVAILABLE) { return rc;'),('init-key','firmware/src/ui_layer.c','unsigned count = eng_count_mask(engine_mask);','unsigned count = NENG_SHOWN;'),('led-count','firmware/src/ui_layer.c','            unsigned count = eng_count_mask(engine_mask);','            unsigned count = NENG_SHOWN;'),('draw-identity','firmware/src/ui_layer.c','unsigned e = eng_vis_mask(i, mask);','unsigned e = ENGINE_ORDER[i];')]
rows=[]
for name,file,old,new in mutants:
 p=ctrl/file;original=p.read_text();assert old in original;p.write_text(original.replace(old,new,1));exe=out/('negative_'+name);sha=hashlib.sha256(p.read_bytes()).hexdigest()
 cmd=['clang','-O2','-w','-DNPART=8','-Ibuild/gen','-Ifirmware/src','firmware/src/d8p1.c','tests/engine_ui_availability_test.c','-lm','-o',str(exe)]
 with (out/('negative_'+name+'.log')).open('w') as f:
  f.write('Mutation '+file+'\n'+old+'\n -> '+new+'\nsource SHA256 '+sha+'\n');f.flush();compile_rc=subprocess.run(cmd,cwd=ctrl,stdout=f,stderr=subprocess.STDOUT).returncode
  assert compile_rc==0
  run_rc=subprocess.run([str(exe)],cwd=ctrl,stdout=f,stderr=subprocess.STDOUT).returncode
  assert run_rc!=0,(name,'not detected')
 p.write_text(original);rows.append({'name':name,'compile':compile_rc,'run':run_rc,'mutant_source_sha256':sha,'file':file,'before':old,'after':new});print(name+' detected',flush=True)
(out/'negative-results.json').write_text(json.dumps(rows,indent=2)+'\n')
