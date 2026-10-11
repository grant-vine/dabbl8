#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exact extracted RtMidi C++ bodies, fake CoreMIDI only; no native extension."""
from pathlib import Path
import subprocess,tempfile,hashlib,json,os,shlex,shutil,re
explicit=os.environ.get('CXX')
if explicit:
 compiler=shlex.split(explicit);selection='explicit CXX'
else:
 cc=shlex.split(os.environ.get('CC','clang'))
 assert len(cc)==1,'Set CXX explicitly for a wrapped or flagged CC'
 name=Path(cc[0]).name
 if re.fullmatch(r'clang(?:-[0-9]+)?',name):paired=name.replace('clang','clang++',1)
 elif re.fullmatch(r'gcc(?:-[0-9]+)?',name):paired=name.replace('gcc','g++',1)
 elif name=='cc':paired='c++'
 else:raise AssertionError('No known C++ pair for CC; set CXX explicitly')
 compiler=[str(Path(cc[0]).with_name(paired))];selection='paired from CC='+cc[0]
assert compiler and shutil.which(compiler[0]),'Selected C++ compiler unavailable; no unrelated fallback'
actual_compiler=shutil.which(compiler[0]);version=subprocess.run(compiler+['--version'],capture_output=True,text=True);assert version.returncode==0,version.stderr
S=Path(__file__).resolve().parent;source=(S/'rtmidi_native_lifecycle_mock.cpp').read_text();before=hashlib.sha256(source.encode()).hexdigest();assert source.count('new CoreMidiData;')==2
fixed=source.replace('new CoreMidiData;','new CoreMidiData();');rows=[]
with tempfile.TemporaryDirectory() as temp:
 D=Path(temp)
 for name,body,expected,exitcode in [('original',source,'2',0),('fixed',fixed,'0',0),('input-reverted',fixed.replace('new CoreMidiData();','new CoreMidiData;',1),'0',1),('output-reverted',fixed.rsplit('new CoreMidiData();',1)[0]+'new CoreMidiData;'+fixed.rsplit('new CoreMidiData();',1)[1],'0',1)]:
  cpp=D/(name+'.cpp');cpp.write_text(body)
  for san in (False,True):
   flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if san else []
   cmd=compiler+['-std=c++11','-O0',*flags,str(cpp),'-o',str(D/(name+str(san)))];c=subprocess.run(cmd,capture_output=True);assert c.returncode==0,c.stderr.decode();q=subprocess.run([str(D/(name+str(san))),expected],capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'});assert q.returncode==exitcode,(name,san,q.stdout,q.stderr)
   assert q.stderr==(b'FAIL unexpected poisoned or valid disposal count\n' if exitcode else b''),(name,san,q.stderr)
   expected_poison=2 if name=='original' else 0 if name=='fixed' else 1
   assert f'poisoned-dispose-attempts={expected_poison} valid-seeded-disposals=2' in q.stdout.decode()
   rows.append({'variant':name,'sanitizer':san,'exit':q.returncode,'expected_exit':exitcode,'stdout':q.stdout.decode().strip(),'stderr':q.stderr.decode().strip(),'compiler_argv':cmd,'compile_exit':c.returncode,'compile_stdout':c.stdout.decode(),'compile_stderr':c.stderr.decode()})
assert hashlib.sha256((S/'rtmidi_native_lifecycle_mock.cpp').read_bytes()).hexdigest()==before
print(json.dumps({'compiler':{'selection':selection,'argv':compiler,'actual_program':actual_compiler,'version_stdout':version.stdout,'version_stderr':version.stderr},'source_sha256':before,'rows':rows,'status':'PASS','scope':'eight actual extracted C++ fake-API builds/runs only; no native MIDI imports, frameworks or OS calls'}))
