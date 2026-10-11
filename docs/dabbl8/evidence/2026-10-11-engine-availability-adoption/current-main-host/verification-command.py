from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import hashlib,json,subprocess
repo=Path('/Users/grantv/Code/dabbl8/.local-baseline/engine-availability-adoption')
old=Path('/private/tmp/d8-engine-availability-adoption/followup-v18otfj1')
out=Path('/private/tmp/d8-engine-adoption-current-main-host');out.mkdir(exist_ok=False)
receipt=json.loads((old/'receipt.json').read_text())
commands={c['name']:c for c in receipt['commands'] if c['name'].startswith('adopt-')}
names=sorted(n for n in commands if not n.endswith('-build'))
assert len(names)==16 and len(commands)==32
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
source={n:sha(repo/n) for n in receipt['source_sha256']}
def run_pair(name):
    rows=[]
    for label in [name+'-build',name]:
        args=[a.replace(str(old),str(out)) for a in commands[label]['command']]
        r=subprocess.run(args,cwd=repo,text=True,capture_output=True)
        log=out/(label+'.log');log.write_text(r.stdout+r.stderr)
        rows.append({'name':label,'command':args,'exit':r.returncode,'log_sha256':sha(log)})
        if r.returncode:break
    return rows
with ThreadPoolExecutor(max_workers=4) as pool:
    rows=[r for group in pool.map(run_pair,names) for r in group]
assert source=={n:sha(repo/n) for n in source},'Source changed during checks'
result={'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'source_sha256':source,'generated_sha256':receipt['generated_sha256'],'commands':rows,'result':'PASS' if len(rows)==32 and all(r['exit']==0 for r in rows) else 'FAIL','scope':'Exact current-main integration host matrix: 4/8 tracks, FM4 and SLICE off/on, normal and inherited DSP sanitizer flags. Earlier guard-removal controls remain separately source-pinned; no target/device/deadline proof.'}
(out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
print(result['result'],len(rows),'compile/run commands; 16 compatibility fixtures')
raise SystemExit(result['result']!='PASS')
