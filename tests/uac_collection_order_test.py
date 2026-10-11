#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual descriptor matrix; byte deltas, not physical host enumeration proof."""
import argparse, itertools, os, subprocess, tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--parent-source', type=Path, help='Optional privately retained parent usb.c for exact opt-out comparison')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    compiler = os.environ.get('CC', 'clang').split()
    flags = ['-Ifirmware/src'] + (['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=undefined'] if args.sanitize else ['-O2'])
    checks = variants = 0
    fresh, toggled = {}, {}
    def check(ok, description):
        nonlocal checks
        checks += 1
        if not ok: raise AssertionError(description)
    with tempfile.TemporaryDirectory(prefix='d8-uac-order-') as temp:
        target = Path(temp) / 'descriptors'
        for cdc, layout, rate, on in itertools.product([0, 1], range(4), [0, 1, 2], [0, 1]):
            uac, k48 = int(rate != 0), int(rate == 2)
            common = [f'-DT_CDC={cdc}', f'-DT_UAC={uac}', f'-DT_LAYOUT={layout}', f'-DT_ON={on}', f'-DT_48K={k48}']
            outputs = []
            for order in [0, 1]:
                cmd = compiler + flags + common + [f'-DT_AS_FIRST={order}', '-o', str(target), 'tests/usb_desc_test.c']
                subprocess.run(cmd, cwd=root, check=True, capture_output=True)
                run = subprocess.run([str(target)], cwd=root, capture_output=True)
                check(run.returncode == 0, (cmd, run.stdout.decode(), run.stderr.decode()))
                check(not run.stderr, 'no sanitizer diagnostics')
                dump = subprocess.run([str(target), '--dump'], check=True, capture_output=True).stdout
                check(len(dump) >= 27 and int.from_bytes(dump[20:22], 'little') == len(dump)-18, 'complete GET_DESCRIPTOR bytes')
                outputs.append(dump); variants += 1
                key=(cdc,layout,rate,on,order); fresh[key]=dump
                toggled[key]=subprocess.run([str(target), "--dump-other"], check=True, capture_output=True).stdout
            expected = bytearray(outputs[0])
            if uac:
                minor = 10 * (expected[12] >> 4) + (expected[12] & 15) + 40
                expected[12] = 16 * (minor // 10) + minor % 10
                off = 18
                while off < len(expected):
                    if expected[off:off+3] == bytes([10, 0x24, 1]):
                        expected[off+8], expected[off+9] = expected[off+9], expected[off+8]
                    off += expected[off]
            check(outputs[1] == expected, 'modern differs only cache version and AC collection order; no endpoints/interfaces/PID changes')
            if args.parent_source:
                cmd = compiler + flags + common + ['-DT_AS_FIRST=0', '-DUSB_DESC_SOURCE="'+str(args.parent_source.resolve())+'"', '-o', str(target), 'tests/usb_desc_test.c']
                subprocess.run(cmd, cwd=root, check=True, capture_output=True)
                original = subprocess.run([str(target), '--dump'], check=True, capture_output=True).stdout
                check(original == outputs[0], 'legacy opt-out exact parent descriptors in every variant')
    identities = {}
    for data in fresh.values():
        identity = data[12:14]
        # Collapse compile flags whose complete emitted descriptors are identical.
        check(identity not in identities or identities[identity] == data, "different complete descriptor profiles never share a cache version")
        identities[identity] = data
    for (cdc,layout,rate,on,order), data in toggled.items():
        check(data==fresh[(cdc,layout,rate,1-on if cdc else on,order)], "runtime console selection equals the complete separately compiled descriptor view")
    print(f'UAC collection order: {variants} variants, {checks} checks, zero failures; '+('strict ASan/UBSan' if args.sanitize else 'normal')+'; no physical enumeration proof')


if __name__ == '__main__':
    main()
