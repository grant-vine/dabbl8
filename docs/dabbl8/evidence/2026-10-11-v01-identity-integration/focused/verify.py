from pathlib import Path
import subprocess,json,hashlib,shutil,os
b=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=b/'v01-session-fixture-fix';e=b/'v01-corrected-identity-focused-check';e.mkdir(exist_ok=True)
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip();assert head=='f913ce765e994290554b06b51d6bca6260934cbb';assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip();(e/'source-head.txt').write_text(head+'\n')
(e/'integration-source.json').write_text(json.dumps({'source':head,'identity_parent':'c1e48711281f6f985c16f7e788e7f48cbff73937','actual_main_parent':'832df855adb1f33c8517ebc5b8e8b11e4fb84c2c','verification':'fresh focused plus fresh target builds; fresh identity plus complete migration and USB optimization composition; no prior whole-suite result is borrowed','hardware':'NOT_RUN'},indent=2)+'\n')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();gen=json.loads((b/'native-autosave-combined-check/generated-inputs.json').read_text());known=json.loads((b/'native-integration-check/unchanged-artifacts.json').read_text())
for p,h in gen.items():
 src=b/'native-autosave-combined'/p;assert sha(src)==h;dst=r/p;assert sha(dst)==h
for p,h in known.items():
 src=b/'main-workspace'/p;assert sha(src)==h;dst=r/p;assert sha(dst)==h
results=[];env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
def run(name,cmd,expected=0):
 with (e/(name+'.log')).open('w') as f:p=subprocess.run(cmd,cwd=r,env=env,stdout=f,stderr=subprocess.STDOUT)
 results.append({'name':name,'exit':p.returncode,'expected_exit':expected});(e/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(name,p.returncode,flush=True)
 assert p.returncode==expected,name
run('profile',['python3','tests/d8identity_test.py'])
for san in [False,True]:
 flags=['-O1','-fsanitize=address,undefined','-fno-sanitize-recover=all'] if san else ['-O2']
 for name,ds in [('upstream',[]),('dabbl8',['-DDABBL8_IDENTITY=1']),('loader',['-DDABBL8_IDENTITY=1','-DFELUCCA_LOADER=1','-DFELUCCA_USB_PID=2'])]:
  n=name+('-san' if san else '-opt');run(n+'-build',['clang',*flags,'-w',*ds,'tests/d8identity_usb_test.c','-o',str(e/n)]);run(n,[str(e/n)])
 for name,ds in [('legacy',[]),('branded',['-DDABBL8_IDENTITY=1'])]:
  n='capture-'+name+('-san' if san else '-opt');capflags=flags+(['-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow'] if san else [])
  run(n+'-build',['clang',*capflags,'-w','-Ibuild/gen','-Ifirmware/src',*ds,'firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c','tests/instrument_capture_test.c','-lm','-o',str(e/n)])
  run(n+'-identity',['node','web/test_d8identity.mjs',str(e/n),'Dabbl8 Editor' if name=='branded' else 'Felucca Editor'])
run('four-bridge-build',['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src','tests/track_client_bridge.c','-lm','-o',str(e/'bridge4')])
run('branded-discovery',['node','web/test_d8info.mjs',str(e/'capture-branded-opt'),str(e/'bridge4'),'build/felucca.fwsc'])
run('installer',['python3','tests/install_test.py'])
run('local-editor-site',['python3','web/make_site.py','build/felucca.fwsc','identity-test',str(e/'site-preview')])
run('experimental-site-refusal',['python3','web/make_site.py',str(r/'build/identity-dabbl8/dabbl8-0.1.0-dev.fwsc'),'0.1.0-dev',str(e/'unapproved-experimental-site')],1)
assert not (e/'unapproved-experimental-site').exists()
assert all(sha(r/p)==h for p,h in gen.items()) and all(sha(r/p)==h for p,h in known.items());assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
print('ALL FINAL IDENTITY FOCUSED CHECKS PASS; actual editor preview built; no publication/install',flush=True)
