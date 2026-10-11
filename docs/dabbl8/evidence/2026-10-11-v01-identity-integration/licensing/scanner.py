from pathlib import Path
import subprocess,json,hashlib,re
r=Path('/Users/grantv/Code/dabbl8');head='f913ce765e994290554b06b51d6bca6260934cbb';upstream='276f72a4e6ea8a12499a7a6819aadf3165126755';out=r/'.local-baseline/v01-corrected-license-check';out.mkdir(exist_ok=True)
def g(*a):return subprocess.check_output(['git','-C',str(r),*a])
def tree(commit):
 d={}
 for line in g('ls-tree','-r',commit).decode().splitlines():
  meta,p=line.split('\t');mode,kind,blob=meta.split();
  if kind=='blob':d[p]=(mode,blob)
 return d
h=lambda b:hashlib.sha256(b).hexdigest();t=tree(head);u=tree(upstream);rows=[];omissions=[];lost=[]
for p,(mode,blob) in t.items():
 if not (p=='build.sh' or (p.startswith(('firmware/','tools/','tests/','web/')) and Path(p).suffix in {'.c','.h','.inc','.py','.sh','.js','.mjs','.html','.css'})):continue
 b=g('cat-file','blob',blob);s=b.decode();lines=[l for l in s.splitlines() if 'SPDX-License-Identifier:' in l];old=u.get(p)
 if not lines:omissions.append(p)
 if old:
  orig=g('cat-file','blob',old[1]).decode()
  for l in orig.splitlines():
   if ('Copyright' in l or 'SPDX-License-Identifier:' in l) and l.strip() not in {x.strip() for x in s.splitlines()}:lost.append({'path':p,'line':l})
 rows.append({'path':p,'git_mode':mode,'git_blob':blob,'sha256':h(b),'upstream_blob':old[1] if old else None,'relative_to_upstream':('unchanged' if old[1]==blob else 'modified') if old else 'new','literal_spdx_lines':lines})
prior=json.loads(g('show',head+':docs/dabbl8/evidence/2026-10-11-license-provenance/inventory.json'));pins={}
for p,rec in prior['notice_asset_pins'].items():
 b=g('show',head+':'+p)
 if p=='LICENSING.md':
  prior_blob=g('show',prior['source_commit']+':'+p);assert h(prior_blob)==rec['sha256'] and b.startswith(prior_blob),p
 else:assert h(b)==rec['sha256'],p
 pins[p]={'git_blob':t[p][1],'sha256':h(b),'prior_inventory_sha256':rec['sha256'],'prior_notices_byte_preserved':True}
ports={}
for p in ['firmware/src/phys_dsp.c','firmware/src/phys_symp.c','firmware/src/fm6_core.c']:
 a=re.search(rb'/\*.*?\*/',g('show',head+':'+p),re.S).group();b=re.search(rb'/\*.*?\*/',g('show',upstream+':'+p),re.S).group();assert a==b,p;ports[p]=h(a)
assert not omissions and not lost,(omissions,lost)
summary={'code_files':len(rows),'new_files':sum(x['relative_to_upstream']=='new' for x in rows),'modified_files':sum(x['relative_to_upstream']=='modified' for x in rows),'without_literal_spdx':omissions,'lost_upstream_copyright_or_spdx_lines':lost,'retained_notice_asset_pins':len(pins),'three_port_first_comment_headers_byte_preserved':True}
d={'source_commit':head,'upstream_commit':upstream,'scope':'Immutable tracked source blobs plus .inc reference fixture and retained notice/asset pins; external SDK/package notice bundle not audited here','summary':summary,'notice_asset_pins':pins,'port_header_hashes':ports,'code_files':rows,'release_qualification':'NOT_ESTABLISHED'}
(out/'inventory.json').write_text(json.dumps(d,indent=2)+'\n');(out/'scanner.py').write_bytes(Path(__file__).read_bytes());print(json.dumps(summary))
