from pathlib import Path
import os,subprocess,json,hashlib,shutil
b=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=b/'native-state-render';e=b/'native-state-render-final-check';e.mkdir(exist_ok=True);(e/'ppm').mkdir(exist_ok=True);h='781ab9a857160a09f122b2e913ef151f2547ef0f'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==h
assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
shutil.copy2(__file__,e/'verify-focused.py');env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1');checks=[]
def run(name,args,allowed=0):
 with (e/(name+'.log')).open('w') as f:p=subprocess.run(args,cwd=r,env=env,stdout=f,stderr=subprocess.STDOUT)
 checks.append({'name':name,'exit':p.returncode,'expected_exit':allowed});(e/'checks.json').write_text(json.dumps(checks,indent=2)+'\n');print(name,p.returncode,flush=True);assert p.returncode==allowed
for mode,flags in [('opt',['-O2']),('san',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=all'])]:
 exe=e/('final-'+mode);run(mode+'-build',['clang','-w','-Ibuild/gen','-Ifirmware/src',*flags,'-o',str(exe),'firmware/src/d8p1.c','firmware/src/d8pool.c','tests/native_state_render_test.c','-lm']);run(mode,[str(exe)]+([str(e)] if mode=='opt' else []))
# Same final fixture with exact parent renderer files; helper is unused there.
paths=['firmware/src/ui_draw.c','firmware/src/ui_graph.c'];saved={p:(r/p).read_bytes() for p in paths};parentpins={}
try:
 for p in paths:
  raw=subprocess.check_output(['git','show','984f4802ba3e284cd9cd08f4c3606d363877eb49:'+p],cwd=r);(r/p).write_bytes(raw);parentpins[p]=hashlib.sha256(raw).hexdigest()
 run('parent-ui-build',['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','-o',str(e/'final-parent-ui'),'firmware/src/d8p1.c','firmware/src/d8pool.c','tests/native_state_render_test.c','-lm'])
 run('parent-ui',[str(e/'final-parent-ui')],1)
finally:
 for p,raw in saved.items():(r/p).write_bytes(raw)
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==h
assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
(e/'terminal.json').write_text(json.dumps({'source':h,'parent_ui_source':'984f4802ba3e284cd9cd08f4c3606d363877eb49','parent_ui_sha256':parentpins,'focused':'PASS','parent_ui_expected_negative':'exit1','sanitizer':'inherited actual DSP exclusions: signed-integer-overflow,shift,bounds,object-size,pointer-overflow','hardware':'NOT_RUN'},indent=2)+'\n')
