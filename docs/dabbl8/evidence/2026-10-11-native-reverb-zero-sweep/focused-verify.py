from pathlib import Path
import subprocess,os,json,hashlib,shutil
base=Path('/Users/grantv/Code/dabbl8/.local-baseline')
r=base/'native-reverb-latched-sweep';e=base/'native-reverb-latched-sweep-check'
head='448a4a625731fe2a2aca80445948cf22d9902057'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==head
assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
e.mkdir(exist_ok=True);shutil.copy2(__file__,e/'focused-verify.py')
env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
checks=[]
def run(name,args):
 with (e/(name+'.log')).open('w') as f:p=subprocess.run(args,cwd=r,env=env,stdout=f,stderr=subprocess.STDOUT)
 checks.append({'name':name,'exit':p.returncode});(e/'focused-results.json').write_text(json.dumps(checks,indent=2)+'\n');print(name,p.returncode,flush=True)
 if p.returncode:raise SystemExit(p.returncode)
sources=['firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c']
for mode in ('opt','san'):
 flags=['-O2'] if mode=='opt' else ['-O1','-g','-fsanitize=address,undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow','-fno-sanitize-recover=all']
 for test in ('native_reverb_zero_sweep_test','native_fx_tail_test','native_fx_counter_boundary_test'):
  name=mode+'-'+test;out=e/name
  run(name+'-compile',['clang',*flags,'-w','-Ibuild/gen','-Ifirmware/src','-o',str(out),*sources,'tests/'+test+'.c','-lm'])
  run(name,[str(out)])
probe=e/'mixed-store-probe.c'
run('fixed-mixed-store-compile',['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','-DTREE_RUNTIME="'+str(r/'tests/d8p1_flash_runtime_test.c')+'"','-o',str(e/'fixed-mixed-store'),*sources,str(probe),'-lm'])
run('fixed-mixed-store',[str(e/'fixed-mixed-store')])
wave=e/'wave.c';shutil.copy2(base/'native-reverb-indexed-sweep-check/wave.c',wave)
run('candidate-wave-compile',['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','-DTREE_UI="'+str(r/'tests/ui_test.c')+'"','-DTREE_AUDIO="'+str(r/'firmware/src/audio.c')+'"','-o',str(e/'candidate-wave'),*sources,str(wave),'-lm'])
run('candidate-wave',[str(e/'candidate-wave'),str(e/'candidate-audio.raw')])
parent=base/'native-reverb-zero-sweep-check/parent-audio.raw';audio=e/'candidate-audio.raw'
assert audio.read_bytes()==parent.read_bytes()
(e/'audio-results.json').write_text(json.dumps({'parent':'226440712db0808001abb3ad3dba3cfb5a786f1b','candidate':head,'parent_evidence':str(parent),'bytes':audio.stat().st_size,'sha256':hashlib.sha256(audio.read_bytes()).hexdigest(),'actual_all8tracks_DAC_USB_identity':True},indent=2)+'\n')
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==head
assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
print('Focused paired safety checks, prior counterexample, actual DAC/USB identity PASS',flush=True)
