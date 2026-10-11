from pathlib import Path
import subprocess,json,hashlib
root=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-sound-selection');out=Path('/private/tmp/d8-engine-sound-selection-evidence/voice-rerun');ctrl=Path('/private/tmp/d8-engine-sound-selection-evidence/controls');(ctrl/'tests/engine_sound_selection_test.c').write_bytes((root/'tests/engine_sound_selection_test.c').read_bytes())
mutants=[('effective-mask','firmware/src/engines.c',' if(!out || requested>=NENGINES',' mask=D8ENGINE_EFFECTIVE_MASK;\n if(!out || requested>=NENGINES',False),('editor-preflight','firmware/src/editor.c','if (factory_ready(a[0], a[1], mask)) { ed_d8_error(cmd, 5u); return; }','/* negative control: no editor preflight */',False),('user-success','firmware/src/upreset.c','if (up_load_mask(k, mask)) return;','(void)up_load_mask(k, mask);',False),('edit-undo','firmware/src/ui_layer.c','    int rc;\n    if (e < NENGINES) {','    int rc;\n    if (!lys.loaded) undo.keep = 0;\n    if (e < NENGINES) {',True)]
rows=[]
for name,file,old,new,ui in mutants:
 p=ctrl/file;original=p.read_text();assert old in original;p.write_text(original.replace(old,new,1));exe=out/('negative_'+name);sha=hashlib.sha256(p.read_bytes()).hexdigest()
 cmd=['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','tests/engine_sound_selection_test.c','-lm','-o',str(exe)]
 if ui:cmd.insert(1,'-DD8_SOUND_UI_TEST')
 with (out/('negative_'+name+'.log')).open('w') as f:
  f.write('Mutation '+file+'\n'+old+'\n -> '+new+'\nsource SHA256 '+sha+'\n');f.flush();compile_rc=subprocess.run(cmd,cwd=ctrl,stdout=f,stderr=subprocess.STDOUT).returncode
  assert compile_rc==0
  run_rc=subprocess.run([str(exe)],cwd=ctrl,stdout=f,stderr=subprocess.STDOUT).returncode
  assert run_rc!=0
 p.write_text(original);rows.append({'name':name,'compile':compile_rc,'run':run_rc,'mutant_source_sha256':sha,'file':file,'before':old,'after':new});print(name+' detected with live voices',flush=True)
(out/'negative-results.json').write_text(json.dumps(rows,indent=2)+'\n')
