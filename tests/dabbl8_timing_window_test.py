# SPDX-License-Identifier: GPL-3.0-only
"""D8T2 actual C frame, literal window oracle and independently rebuilt CRC."""
import os,struct,zlib,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from dabbl8_timing_report import parse_report
class WindowTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.frame=Path(os.environ['D8_TIMING_TEST_FRAME']).read_bytes().decode('ascii')
 def rewrite(self,change):
  labels=[l.split(' ')[:2] for l in self.frame.splitlines()];data=[[int(w,16) for w in l.split(' ')[2:]] for l in self.frame.splitlines()];change(data)
  words=[0x44385432,*data[0],*data[1],*data[2],*data[3],0x52,*data[4],0x49,*data[5]];data[6]=[*data[0],zlib.crc32(struct.pack('<'+'I'*len(words),*words))]
  return ''.join(' '.join(label+[f'{v:08X}' for v in values])+'\r\n' for label,values in zip(labels,data))
 def test_actual_literal_window_and_percentile_oracle(self):
  r=parse_report(self.frame,1,17);w=r['window'];self.assertEqual(r['format'],'D8T2');self.assertEqual(r['samples'],1024)
  self.assertEqual(w['elapsed_ticks'],62904270);self.assertEqual(w['first_ticks'],0xffffff9b);self.assertEqual(w['last_ticks'],62904169)
  self.assertEqual(w['late_publications'],1024);self.assertEqual(w['uac_underrun_publications'],2048);self.assertEqual(w['uac_overrun_publications'],3072);self.assertEqual(w['alnk_aux_pending_observations'],[512,342,205]);self.assertFalse(r['hardware_qualified'])
  for name,m in [('render',100),('inclusive',120)]:
   for p in [95,99]:
    exact=(((1024*p+99)//100)-1)*m;b=r['metrics'][name][f'p{p}_ticks'];self.assertLessEqual(b['lower'],exact);self.assertGreaterEqual(b['upper'],exact)
 def test_every_truncation_and_mixed_frames(self):
  for i in range(len(self.frame)):
   with self.assertRaises(ValueError):parse_report(self.frame[:i])
  old=Path(__file__).resolve().parents[1]/'docs/dabbl8/evidence/2026-10-11-timing-diagnostic/host/console/normal-flag1.frame'
  for text in [self.frame+self.frame,self.frame+old.read_bytes().decode(),self.frame+'d8t2 stray\n',self.frame.replace('d8t2 window','d8t2 unknown'),self.frame.replace('d8t2','d8t3'),self.frame+'\x85']:
   with self.assertRaises(ValueError):parse_report(text)
 def test_crc_valid_window_refusals(self):
  for index,value in [(11,1),(11,2),(11,4),(11,8),(12,2),(4,18),(2,0),(3,0xffffffff),(5,1025)]:
   with self.subTest(index=index,value=value),self.assertRaises(ValueError):parse_report(self.rewrite(lambda d:d[3].__setitem__(index,value)))
  def absent(d):d[3][12]=0
  with self.assertRaises(ValueError):parse_report(self.rewrite(absent))
 def test_empty_and_accumulated_timer_wrap(self):
  def empty(d):
   d[2]=[0]*4;d[3]=[0]*12+[1];d[4]=[0]*130;d[5]=[0]*130
  r=parse_report(self.rewrite(empty));self.assertEqual(r['measurement_status'],'no_samples');self.assertEqual(r['window']['elapsed_ticks'],0)
  r=parse_report(self.rewrite(lambda d:d[3].__setitem__(3,1)));self.assertEqual(r['window']['elapsed_ticks'],4357871566)
 def test_crc_corruption_and_identity(self):
  for text in [self.frame.replace('00000001','00000002',1),self.frame.replace('00000001','000001',1),self.frame.replace('\r\n','\v')]:
   with self.assertRaises(ValueError):parse_report(text)
  with self.assertRaises(ValueError):parse_report(self.frame,2,17)
  with self.assertRaises(ValueError):parse_report(self.frame,1,18)
if __name__=='__main__':unittest.main()
