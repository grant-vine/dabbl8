#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Export the pinned musical schema, without changing firmware or parameter counts."""
import argparse
import json
from pathlib import Path


def export(source: Path, target: Path) -> None:
    data = json.loads(source.read_text())
    if (data['P_COUNT'], data['G_COUNT'], data['NSTEP'], data['P_E0'], len(data['TP']), len(data['ENG'])) != (99, 27, 64, 91, 91, 14):
        raise ValueError('Expected the audited 99-parameter / 14-engine schema')
    if any(len(engine['edit']) != 8 for engine in data['ENG']):
        raise ValueError('Expected eight engine-specific parameters')
    result = {'schema': 1, 'params': 99, 'engines': 14, 'engineStart': 91,
              'common': data['TP'],
              'engine': [{key: engine[key] for key in ['name', 'titles', 'edit']} for engine in data['ENG']]}
    target.write_text('// SPDX-License-Identifier: GPL-3.0-only\n'
                      '// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments\n'
                      '// Generated from the pinned Felucca 99-parameter host descriptor export.\n'
                      'export const descriptors = ' + json.dumps(result, ensure_ascii=True, separators=(',', ':')) + ';\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('target', type=Path)
    args = parser.parse_args()
    export(args.source, args.target)
