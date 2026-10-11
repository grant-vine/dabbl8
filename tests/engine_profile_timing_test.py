#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
"""Actual curated ISR diagnostic parity; clocks/DMA/UAC are simulated, never target timing."""
import hashlib,json,os,shlex,subprocess,sys,tempfile
from pathlib import Path
S=Path(__file__).resolve().parents[1];parent=S/'build/host/profile-timing';parent.mkdir(parents=True,exist_ok=True)
O=Path(tempfile.mkdtemp(prefix='run-',dir=parent));CC=shlex.split(os.environ.get('CC','cc'))
ENV=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def source_inventory():
 return {str(p.relative_to(S)):h(p) for directory in ('firmware','tests','build/gen') for p in sorted((S/directory).rglob('*')) if p.is_file() and '__pycache__' not in p.parts}
receipt={'scope':'Simulated clock/DMA/UAC; actual ISR and selected DSP sources. No hardware timing or stack proof.','source_hash_scope':'Superset of project-owned firmware/test inputs and actual build/gen headers, excluding Python cache. Compiler/system headers are not included; exact command vectors are retained.','source_hashes':source_inventory(),'result':'FAIL','checks':[],'skips':[]}
def run(name,cmd):
 q=subprocess.run(cmd,cwd=S,env=ENV,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);(O/(name+'.log')).write_bytes(q.stdout)
 receipt['checks'].append({'name':name,'command':cmd,'exit':q.returncode,'log_sha256':hashlib.sha256(q.stdout).hexdigest()})
 if q.returncode:raise RuntimeError(name+' failed: '+q.stdout.decode(errors='replace'))
 return q.stdout
try:
 modes=[('normal',['-O2'])]
 if os.environ.get('SANITIZE','1')!='0':modes.append(('san',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=undefined','-fno-sanitize=signed-integer-overflow,shift,bounds,object-size,pointer-overflow']))
 else:receipt['skips'].append('SANITIZE=0: inherited sanitizer configuration skipped')
 for profile in (0,1,2):
  for mode,flags in modes:
   traces=[]
   for enabled in (0,1):
    name=f'profile{profile}-{mode}-flag{enabled}';b=O/name
    run(name+'-build',[*CC,*flags,'-w','-DNPART=8','-DFELUCCA_FM4=0','-DFELUCCA_SLICE=1',f'-DD8BUILD_PROFILE={profile}','-DD8_TIMING_PROFILE_STRESS=1',f'-DDABBL8_TIMING_DIAGNOSTIC={enabled}','-Ibuild/gen','-Ifirmware/src','tests/audio_timing_isr_test.c','-lm','-o',str(b)])
    for collecting in ([False,True] if enabled else [False]):
     data=run(name+('-collect' if collecting else '-paused'),[str(b)]+(['collect'] if collecting else []))
     traces.append(next(line for line in data.splitlines() if line.startswith(b'actual audio parity:')))
   assert len(set(traces))==1,(profile,mode,traces)
 receipt['result']='PASS'
finally:
 after=source_inventory();receipt['source_unchanged']=after==receipt['source_hashes']
 receipt['source_changes']=[n for n in sorted(set(after)|set(receipt['source_hashes'])) if after.get(n)!=receipt['source_hashes'].get(n)]
 if not receipt['source_unchanged']:receipt['result']='FAIL'
 (O/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
 if not receipt['source_unchanged']:raise RuntimeError('Profile timing source inputs changed during run: '+str(receipt['source_changes']))

print('Profile timing PASS: FULL/Core/Breaks actual8 ISR off/paused/collect parity; selected normal/inheritedSAN scopes and skips recorded; simulated only. Evidence:',O)
