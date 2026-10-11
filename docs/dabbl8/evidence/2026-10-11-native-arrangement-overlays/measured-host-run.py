from pathlib import Path
import subprocess,json,hashlib,statistics,time,shutil
b=Path('/Users/grantv/Code/dabbl8/.local-baseline');e=b/'native-arrangement-overlay-host-cost';rows=[];start=time.time()
repos={'parent':b/'native-arrangement-final','candidate':b/'native-arrangement-overlays'}
for tag,r in repos.items():
 shutil.copy2('/private/tmp/d8_overlay_irq_'+tag,e/tag/'probe')
 for v in range(1,5):
  p=Path('/private/tmp/d8_overlay_irq_'+tag+('-build.log' if v==1 else '-build-v'+str(v)+'.log'))
  if p.exists():shutil.copy2(p,e/tag/p.name)
shutil.copy2('/private/tmp/d8_overlay_irq_probe.c',e/'probe.c');shutil.copy2(__file__,e/'run.py')
for round in range(6):
 for kind in range(3):
  for apply in range(8):
   for tag in (['parent','candidate'] if round%2==0 else ['candidate','parent']):
    p=subprocess.run([str(e/tag/'probe'),str(kind),str(apply),'512'],cwd=repos[tag],text=True,capture_output=True)
    rec={'round':round,'kind':kind,'apply':apply,'tag':tag,'exit':p.returncode,'stdout':p.stdout,'stderr':p.stderr};rows.append(rec)
    (e/'raw-batches.json').write_text(json.dumps(rows,indent=2)+'\n')
    if p.returncode:raise SystemExit(p.returncode)
 print('round',round,'batches',len(rows),flush=True)
summary=[]
for kind in range(3):
 for apply in range(8):
  vals={}
  for tag in repos:
   batches=[list(map(int,x['stdout'].strip().split(','))) for x in rows if x['kind']==kind and x['apply']==apply and x['tag']==tag]
   vals[tag]={'ns_per_irq':[x[3]/x[2] for x in batches],'instructions_per_irq':[x[4]/x[2] for x in batches]}
  d={'kind':kind,'apply':apply}
  for metric in ['ns_per_irq','instructions_per_irq']:
   p=vals['parent'][metric];c=vals['candidate'][metric];d[metric]={'parent_median':statistics.median(p),'candidate_median':statistics.median(c),'ratio':statistics.median(c)/statistics.median(p),'parent_range':[min(p),max(p)],'candidate_range':[min(c),max(c)]}
  summary.append(d)
report={'parent_source':'bf9ddbab103563fd7066618e4a51824c440e5f07','candidate_source':'048df569ca6035490cc2199fc76d3078e1e093cc','scope':'Actual fm1_alnk0_irq including synchronous event/sequencer/engines/FX/master/USB48-resampler/click/output paths; simulated DMA/registers/timers. Full host fixture uses8initial gatedsharedvoices, worst boundary96sequence-releases,99parameter motion restoration each8tracks,96localstepzero noteons. Parent fill case is equivalent explicit selectedrow transition; parent cannot implement overlays/fill. Parent apply0 throughout; candidate all8 combinations. Fixture reset and simulated USB-ring read-index draining outside measured window. CLOCK_MONOTONIC measured IRQ only; instruction delta includes clock/rusage wrapper overhead, no subtraction. 16warmups then512calls perbatch,6alternatingorder rounds. No Mac→pi32/device deadline or complete hardware IRQ/context timing inference.','elapsed_seconds':time.time()-start,'batches':len(rows),'summary':summary,'pins':{str(p.relative_to(e)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [e/'probe.c',e/'parent/probe',e/'candidate/probe',e/'parent/arrangement_fixture.h',e/'candidate/arrangement_fixture.h']}}
(e/'results.json').write_text(json.dumps(report,indent=2)+'\n');print('terminal batches',len(rows),flush=True)
