from pathlib import Path
import os,sys,subprocess,json,hashlib,shutil,importlib.util,time
base=Path('/Users/grantv/Code/dabbl8/.local-baseline');r=base/'native-identity-profile';e=base/'native-identity-profile-target-check';e.mkdir(exist_ok=True)
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip();assert head.startswith('d86711a');assert not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
gen=json.loads((base/'native-autosave-combined-check/generated-inputs.json').read_text());known=json.loads((base/'native-integration-check/unchanged-artifacts.json').read_text())
for p,h in gen.items():
 src=base/'native-autosave-combined'/p;assert sha(src)==h;p2=r/p;p2.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,p2)
for p,h in known.items():
 src=base/'main-workspace'/p;assert sha(src)==h;p2=r/p;p2.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,p2)
(e/'before-pins.json').write_text(json.dumps({'source':head,'generated29':gen,'six_baseline':known},indent=2)+'\n')
env=os.environ.copy();env.update(JIELI_TOOLCHAIN=str(base/'deps/jieli-linux-toolchains-20250324.1'),AC79_SDK=str(base/'deps/ac79-sdk'),JIELI_DOCKER_IMAGE='debian@sha256:7c7b2c966bc9ee8cedfeef67e0e279108992c77681fa595db4a9d65c06ccc587',PATH=str(base/'venv/bin')+':'+env['PATH'])
for k in list(env):
 if k.startswith(('FELUCCA_','GOLDEN_','BUDGET_')):env.pop(k)
os.chdir(r);os.environ.update(env);sys.path.insert(0,str(r/'tools'));import build as b;import fm1_install as install
shutil.copy2(__file__,e/'verify.py');(e/'environment.json').write_text(json.dumps({k:env[k] for k in ['JIELI_TOOLCHAIN','AC79_SDK','JIELI_DOCKER_IMAGE']},indent=2)+'\n')
# Fresh actual editor/capture discovery test under branded app profile.
for n,flags in [('capture-branded',['-DDABBL8_IDENTITY=1']),('bridge4',[])]:
 srcs=['firmware/src/d8p1.c','firmware/src/d8pool.c','firmware/src/d8pool_mapped.c','tests/instrument_capture_test.c'] if n.startswith('capture') else ['tests/track_client_bridge.c']
 with (e/(n+'-compile.log')).open('w') as f:p=subprocess.run(['clang','-O2','-w','-Ibuild/gen','-Ifirmware/src',*flags,*srcs,'-o',str(e/n),'-lm'],stdout=f,stderr=subprocess.STDOUT)
 assert p.returncode==0,n
with (e/'branded-discovery.log').open('w') as f:p=subprocess.run(['node','web/test_d8info.mjs',str(e/'capture-branded'),str(e/'bridge4'),'build/felucca.fwsc'],stdout=f,stderr=subprocess.STDOUT)
assert p.returncode==0,'actual branded INFO discovery';print('Actual branded INFO discovery PASS',flush=True)
res={};baseflags=list(b.CFLAGS)
for n in ['4','8','dabbl8']:
 b.OUT=r/('build/identity-'+n);b.OUT.mkdir(parents=True,exist_ok=True);b.CFLAGS=baseflags+(['-DNPART=8'] if n=='8' else [])
 b.PRODUCT='FM-1_900';b.VERSION=None;b.DABBL8_VERSION=None
 if n=='dabbl8':
  i=b.dabbl8_identity('0.1-dev');b.PRODUCT=i.product;b.VERSION=i.version;b.DABBL8_VERSION=i.version
 with (e/('target-'+n+'.log')).open('w') as f:
  old=sys.stdout;sys.stdout=f
  try:
   image,syms,dis,rt=b.build_app();errors,notes=b.check(image,syms,dis,rt);errors+=b.mmio_check()
   (b.OUT/'linked-symbols.txt').write_text(syms);(b.OUT/'linked-disassembly.txt').write_text(dis)
  finally:sys.stdout=old
 rec={'source':head,'bytes':len(image),'sha256':hashlib.sha256(image).hexdigest(),'errors':errors,'notes':notes,'hardware':'NOT_RUN'};res[n]=rec
 assert not errors,rec
 if n=='4':
  assert rec['sha256']=='3ced057d74785ecd0cc2ccc8f274a1c9e94caa4a6b1203c846014a890d4720fc';shutil.copy2(b.OUT/'linked-disassembly.txt',r/'build/felucca.dis')
 if n=='8':assert rec['sha256']=='4ca641484ac85d3ba7181c62ea96e4e09b52714147ce924f5bf366a1a251f764'
 (e/'target-results.json').write_text(json.dumps(res,indent=2)+'\n');print('Target',n,rec['bytes'],rec['sha256'],flush=True)
 if n=='dabbl8':
  b.LDR=r/'build/identity-loader';ota=b.build_loader();assert hashlib.sha256(ota).hexdigest()==known['build/loader/ota.bin']
  pkg=b.fm1pkg_make.ufw(b.fm1pkg_make.flash_image(image,b.fm1pkg_make.KEY),ota,b.PRODUCT)
  p=b.OUT/i.package;p.write_bytes(pkg);assert install.product_of(pkg)==i.product and install.LOADER_MARK in pkg
  web=subprocess.check_output(['node','--input-type=module','-e','import {readFileSync} from "node:fs"; import {productOf} from "./web/fm1pkg.js"; console.log(productOf(readFileSync(process.argv[1])));',str(p)],text=True).strip();assert web==i.product
  res['package']={'bytes':len(pkg),'sha256':hashlib.sha256(pkg).hexdigest(),'identity':i.product,'version':i.version,'loader_sha256':hashlib.sha256(ota).hexdigest(),'python_browser_identity_equal':True,'hardware':'NOT_RUN','release_qualified':False};(e/'target-results.json').write_text(json.dumps(res,indent=2)+'\n');print('Package identity and unchanged loader PASS',flush=True)
assert all(sha(r/p)==h for p,h in known.items()) and all(sha(r/p)==h for p,h in gen.items())
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip()==head and not subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).strip()
print('ALL IDENTITY TARGET CHECKS PASS; baseline/generator pins preserved',flush=True)
