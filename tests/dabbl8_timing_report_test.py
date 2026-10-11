# SPDX-License-Identifier: GPL-3.0-only
import os
from pathlib import Path
import random
import struct
import sys
import unittest
import zlib
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from dabbl8_timing_report import parse_report, report, BINS, SHIFT, U32_MAX


def histogram(samples):
    bins = [0] * BINS
    for value in samples:
        bins[min(value >> SHIFT, BINS-1)] += 1
    return {'bins': bins, 'count': len(samples), 'max_ticks': max(samples, default=0), 'saturated': 0}


class TimingReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.frame = Path(os.environ['D8_TIMING_TEST_FRAME']).read_bytes().decode('ascii')

    def rewrite(self, mutate):
        rows = [line.split(' ') for line in self.frame.splitlines()]
        data = [[int(n,16) for n in row[2:]] for row in rows]
        mutate(data)
        crc_words = [0x44385431,*data[0],*data[1],*data[2],0x52,*data[3],0x49,*data[4]]
        crc = zlib.crc32(struct.pack('<'+'I'*len(crc_words),*crc_words))
        data[5] = [*data[0],crc]
        return ''.join(' '.join(rows[i][:2]+[f'{n:08X}' for n in values])+'\r\n' for i,values in enumerate(data))

    def test_actual_c_frame_sorted_oracle(self):
        r=parse_report(self.frame,1,17)
        self.assertEqual(r['samples'],1024)
        self.assertEqual(r['tracks'],8)
        self.assertFalse(r['hardware_qualified'])
        self.assertFalse(r['clock_calibrated'])
        for metric,multiplier in [('render',100),('inclusive',120)]:
            ordered=[i*multiplier for i in range(1024)]
            for percentile in (95,99):
                bound=r['metrics'][metric][f'p{percentile}_ticks']
                exact=ordered[(len(ordered)*percentile+99)//100-1]
                self.assertLessEqual(bound['lower'],exact)
                self.assertGreaterEqual(bound['upper'],exact)
            self.assertEqual(r['metrics'][metric]['maximum_ticks'],ordered[-1])

    def test_every_truncation_refused(self):
        for length in range(len(self.frame)):
            with self.subTest(length=length),self.assertRaises(ValueError):
                parse_report(self.frame[:length])

    def test_corruption_framing_and_identity(self):
        malformed=[self.frame+self.frame,self.frame.replace('d8t1 render','d8t1 unknown'),
                   self.frame.replace('d8t1 state FFFFFFEB','d8t1 state EFFFFFEB',1),
                   self.frame.replace('00000001','00000002',1),
                   self.frame.replace('00000001','0000001',1),
                   self.frame+'d8t1 stray\n',self.frame.replace('\r\n','\v'),
                   self.frame+'\x85','x'*65537]
        for index,text in enumerate(malformed):
            self.assertNotEqual(text,self.frame)
            with self.subTest(case=index),self.assertRaises(ValueError):parse_report(text)
        with self.assertRaises(ValueError):parse_report(self.frame,2,17)
        with self.assertRaises(ValueError):parse_report(self.frame,1,18)
        with self.assertRaises(ValueError):parse_report(self.frame,True,17)

    def test_crc_valid_semantic_refusals(self):
        mutations=[lambda d:d[1].__setitem__(0,7),lambda d:d[1].__setitem__(3,1),
                   lambda d:d[1].__setitem__(4,9),lambda d:d[2].__setitem__(2,1),
                   lambda d:d[2].__setitem__(3,1),lambda d:d[2].__setitem__(1,1),
                   lambda d:d[3].__setitem__(0,1025),lambda d:d[3].__setitem__(1,0),
                   lambda d:d[3].__setitem__(2,0)]
        for mutate in mutations:
            with self.assertRaises(ValueError):parse_report(self.rewrite(mutate))

    def test_crc_valid_impossible_paired_distribution(self):
        def impossible(d):
            d[3]=[1024,130048]+[0]*127+[1024]
            d[4]=[1024,130048]+[1023]+[0]*126+[1]
        with self.assertRaises(ValueError):parse_report(self.rewrite(impossible))

    def test_empty_run_not_measurement(self):
        def empty(d):
            d[2]=[0,0,0,0];d[3]=[0]*(BINS+2);d[4]=[0]*(BINS+2)
        result=parse_report(self.rewrite(empty))
        self.assertEqual(result['measurement_status'],'no_samples')
        self.assertIsNone(result['metrics']['render']['maximum_ticks'])
        self.assertFalse(result['hardware_qualified'])

    def test_histogram_sorted_oracle_edges_and_overflow(self):
        rng=random.Random(1979)
        for samples in [[],[0],[1023,1024,1025],[0,U32_MAX],list(range(131072)),
                        [rng.randrange(U32_MAX+1) for _ in range(10000)]]:
            result=report(histogram(samples))
            if not samples:
                self.assertIsNone(result['p95_ticks']);continue
            ordered=sorted(samples)
            for percent in (95,99):
                bound=result[f'p{percent}_ticks'];rank=(len(samples)*percent+99)//100
                self.assertEqual(bound['rank'],rank)
                self.assertLessEqual(bound['lower'],ordered[rank-1])
                self.assertGreaterEqual(bound['upper'],ordered[rank-1])
        result=report(histogram([0]*94+[1024]+[U32_MAX]*5))
        self.assertEqual(result['p95_ticks'],{'rank':95,'lower':1024,'upper':2047})
        self.assertEqual(result['p99_ticks'],{'rank':99,'lower':127<<10,'upper':U32_MAX})

    def test_u32_rank_and_raw_histogram_refusals(self):
        h={'bins':[U32_MAX]+[0]*127,'count':U32_MAX,'max_ticks':0,'saturated':0}
        self.assertEqual(report(h)['p99_ticks']['rank'],(U32_MAX*99+99)//100)
        for update in [{'count':True},{'count':-1},{'count':2},{'saturated':1},
                       {'saturated':2},{'bins':[0]*127},{'max_ticks':1024},{'extra':0}]:
            h=histogram([0]);h.update(update)
            with self.assertRaises(ValueError):report(h)


if __name__ == '__main__':
    unittest.main()
