# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dabbl8 contributors
"""Bounded source notice inventory from immutable tracked Git blobs only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REFERENCE = Path(__file__).with_name('license_notice_baseline.json')
EXTENSIONS = {'.c', '.h', '.inc', '.py', '.sh', '.js', '.mjs', '.html', '.css'}


def scoped(path):
    return path == 'build.sh' or (path.startswith(('firmware/', 'tools/', 'tests/', 'web/'))
                                 and Path(path).suffix in EXTENSIONS)



def header_spdx(path, text):
    """Declarations must be standalone lines in opening language comments."""
    suffix = Path(path).suffix
    kinds = (['hash'] if suffix in {'.py', '.sh'} else
             ['html'] if suffix == '.html' else
             ['block'] if suffix == '.css' else ['block', 'slash'])
    position = 1 if text.startswith('\ufeff') else 0
    comments = []
    if 'hash' in kinds and text[position:].startswith('#!'):
        end = text.find('\n', position)
        position = len(text) if end < 0 else end + 1
    while position < len(text):
        while position < len(text) and text[position].isspace():
            position += 1
        if 'html' in kinds:
            doctype = re.match(r'<!doctype\s+html\s*>', text[position:], re.I)
            if doctype:
                position += doctype.end()
                continue
        if 'block' in kinds and text.startswith('/*', position):
            end = text.find('*/', position + 2)
            if end < 0:
                break
            comments.append(text[position + 2:end])
            position = end + 2
        elif 'html' in kinds and text.startswith('<!--', position):
            end = text.find('-->', position + 4)
            if end < 0:
                break
            comments.append(text[position + 4:end])
            position = end + 3
        elif (('slash' in kinds and text.startswith('//', position)) or
              ('hash' in kinds and text.startswith('#', position))):
            width = 2 if text.startswith('//', position) else 1
            end = text.find('\n', position)
            end = len(text) if end < 0 else end
            comments.append(text[position + width:end])
            position = end
        else:
            break
    identifiers = []
    for comment in comments:
        for line in comment.splitlines():
            declaration = re.fullmatch(r'\s*\*?\s*SPDX-License-Identifier:\s*(\S(?:.*\S)?)\s*', line)
            if declaration:
                identifiers.append(declaration[1])
    return identifiers


def digest(blob):
    return hashlib.sha256(blob).hexdigest()


def inventory(repo, source, reference):
    def git(*args):
        return subprocess.check_output(['git', '-C', str(repo), *args])
    commit = git('rev-parse', '--verify', source + '^{commit}').decode().strip()
    tree = git('rev-parse', commit + '^{tree}').decode().strip()
    blobs = {}
    for entry in git('ls-tree', '-r', '-z', commit).split(b'\0'):
        if not entry:
            continue
        meta, path = entry.split(b'\t', 1)
        mode, kind, oid = meta.decode().split()
        if kind == 'blob':
            blobs[path.decode()] = (mode, oid)
    def read(path):
        return git('cat-file', 'blob', blobs[path][1])
    failures, rows = [], []
    for path, (mode, oid) in sorted(blobs.items()):
        if not scoped(path):
            continue
        data = read(path)
        try:
            text = data.decode('utf-8')
        except UnicodeDecodeError:
            failures.append({'path': path, 'reason': 'source_not_utf8'})
            text = ''
        lines = text.splitlines()
        spdx = header_spdx(path, text)
        expected = reference['code_spdx_identifiers'].get(path, ['GPL-3.0-only'])
        if not spdx:
            failures.append({'path': path, 'reason': 'missing_comment_header_spdx'})
        elif sorted(spdx) != sorted(expected):
            failures.append({'path': path, 'reason': 'unexpected_spdx_identifier',
                             'expected': expected, 'actual': spdx})
        present = {line.strip() for line in lines}
        for required in reference['code_notice_lines'].get(path, []):
            if required not in present:
                failures.append({'path': path, 'reason': 'lost_upstream_notice', 'line': required})
        rows.append({'path': path, 'git_mode': mode, 'git_blob': oid,
                     'sha256': digest(data), 'comment_header_spdx_identifiers': spdx})
    pins = {}
    for path, expected in reference['notice_asset_pins'].items():
        if path not in blobs:
            failures.append({'path': path, 'reason': 'missing_notice_or_asset'})
            continue
        data = read(path)
        compared = data[:expected['bytes']] if expected['comparison'] == 'prefix' else data
        valid = digest(compared) == expected['sha256']
        pins[path] = {'sha256': digest(data), 'git_blob': blobs[path][1],
                      'comparison': expected['comparison'], 'preserved': valid}
        if not valid:
            failures.append({'path': path, 'reason': 'changed_notice_or_asset'})
    ports = {}
    for path, expected in reference['port_first_comment_sha256'].items():
        header = re.search(rb'/\*.*?\*/', read(path), re.S) if path in blobs else None
        actual = digest(header.group()) if header else None
        ports[path] = {'first_comment_sha256': actual, 'preserved': actual == expected}
        if actual != expected:
            failures.append({'path': path, 'reason': 'changed_port_notice_header'})
    return {'schema': 2, 'source_commit': commit, 'source_tree': tree,
            'upstream_commit': reference['upstream_commit'],
            'notice_pin_commit': reference['notice_pin_commit'],
            'reference_sha256': digest(json.dumps(reference, sort_keys=True, separators=(',', ':')).encode()),
            'reference_hash_encoding': 'Canonical sorted compact UTF-8 JSON of the passed reference',
            'scope': 'Tracked Git C/H/inc/Python/shell/JS/MJS/HTML/CSS under firmware/tools/tests/web, build.sh, 16 pinned notice/assets and three retained port headers; no untracked/private files or external SDK/package inputs.',
            'code_files': rows, 'notice_asset_pins': pins, 'port_headers': ports,
            'failures': failures,
            'summary': {'code_files': len(rows), 'notice_asset_pins': len(pins),
                        'port_headers': len(ports), 'failures': len(failures)},
            'release_qualification': 'NOT_ESTABLISHED'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--source', default='HEAD', help='Immutable Git commit/ref; working-tree files are never scanned')
    parser.add_argument('--output', type=Path, help='Write a new inventory JSON; existing files are never overwritten')
    args = parser.parse_args()
    if args.output and args.output.exists():
        parser.error('output exists; historical inventories must not be overwritten')
    reference = json.loads(REFERENCE.read_text())
    report = inventory(args.repo, args.source, reference)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with args.output.open('x') as stream:
            stream.write(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'source_commit': report['source_commit'], **report['summary']}))
    if report['failures']:
        print(json.dumps(report['failures'], indent=2))
    return bool(report['failures'])


if __name__ == '__main__':
    raise SystemExit(main())
