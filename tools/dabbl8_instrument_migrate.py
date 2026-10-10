#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Strict post-boot capture inputs for simulation-only migration; no device access."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
from itertools import islice
import zlib
from dabbl8_instrument_capture import RAW_MAP, NAMES, CURRENT_LENGTHS

MANIFEST_LIMIT = 16384
ROLE_LIMITS = tuple(a[1] + a[3] for a in RAW_MAP) + (7936, 572, 3080, 3080, 3728)
FILES = tuple('original-' + name + '.bin' for name in NAMES)
HEADER_FIELDS = {'format', 'version', 'role_table', 'mode', 'token', 'post_boot',
                 'current_state_complete', 'raw_bytes', 'hardware_qualified',
                 'restore_qualified', 'migration_authorized', 'ownership_authorized',
                 'full_physical_flash', 'pre_boot_original', 'provenance', 'objects'}
OBJECT_FIELDS = {'role', 'name', 'bytes', 'crc32', 'segments', 'file', 'sha256'}
DENIED_AUTHORITIES = ('hardware_qualified', 'restore_qualified', 'migration_authorized',
                      'ownership_authorized', 'full_physical_flash', 'pre_boot_original')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def integer(value, minimum, maximum):
    return type(value) is int and minimum <= value <= maximum


def strict_json(data):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('Duplicate JSON field')
            result[key] = value
        return result
    def constant(unused):
        raise ValueError('Nonfinite JSON number')
    return json.loads(data, object_pairs_hook=pairs, parse_constant=constant)


def parse_manifest(data):
    if type(data) is not bytes or len(data) > MANIFEST_LIMIT:
        raise ValueError('Capture manifest exceeds bounded size')
    m = strict_json(data.decode('utf-8'))
    if type(m) is not dict or set(m) != HEADER_FIELDS:
        raise ValueError('Unsupported capture manifest fields')
    if m['format'] != 'dabbl8-instrument-capture' or not integer(m['version'], 1, 1) or not integer(m['role_table'], 1, 1):
        raise ValueError('Unsupported capture version or fixed role table')
    if m['mode'] != 'full' or m['post_boot'] is not True or m['current_state_complete'] is not True:
        raise ValueError('Complete post-boot full-current capture required')
    if not integer(m['token'], 0, 0xffffffff) or not integer(m['raw_bytes'], 319488, 319488):
        raise ValueError('Invalid capture token or raw allocation total')
    if any(m[k] is not False for k in DENIED_AUTHORITIES):
        raise ValueError('Capture cannot supply physical, recovery or ownership authority')
    if type(m['provenance']) is not str or len(m['provenance']) > 200:
        raise ValueError('Invalid unverified capture provenance')
    if type(m['objects']) is not list or len(m['objects']) != 17:
        raise ValueError('Exactly seventeen ordered capture roles required')
    for role, item in enumerate(m['objects']):
        if type(item) is not dict or set(item) != OBJECT_FIELDS:
            raise ValueError('Unsupported capture role fields')
        if not integer(item['role'], role, role) or item['name'] != NAMES[role] or item['file'] != FILES[role]:
            raise ValueError('Reordered, duplicate or unsupported capture role identity/path')
        expected = ROLE_LIMITS[role]
        if not integer(item['bytes'], 1 if role == 12 else expected, expected) or not integer(item['crc32'], 0, 0xffffffff):
            raise ValueError('Invalid fixed role size or checksum')
        if type(item['sha256']) is not str or re.fullmatch('[0-9a-f]{64}', item['sha256']) is None:
            raise ValueError('Invalid canonical role SHA-256')
        mapping = RAW_MAP[role] if role < 12 else (0, 0, 0, 0)
        segments = item['segments']
        if type(segments) is not list or len(segments) != 2:
            raise ValueError('Exactly two fixed role segments required')
        for i, segment in enumerate(segments):
            if type(segment) is not dict or set(segment) != {'address', 'bytes'}:
                raise ValueError('Unsupported physical segment fields')
            if not integer(segment['address'], mapping[2 * i], mapping[2 * i]) or not integer(segment['bytes'], mapping[2 * i + 1], mapping[2 * i + 1]):
                raise ValueError('Unsupported fixed capture physical map')
    return m


def bounded_file(path, maximum):
    """Read a regular leaf without following symlinks; oversize inputs stay in place."""
    leaf = Path(path).lstat()
    if not stat.S_ISREG(leaf.st_mode):
        raise ValueError('Input must be a regular nonsymlink file')
    flags = os.O_RDONLY | getattr(os, 'O_NOFOLLOW', 0) | getattr(os, 'O_NONBLOCK', 0)
    fd = os.open(path, flags)
    with os.fdopen(fd, 'rb') as stream:
        before = os.fstat(stream.fileno())
        if not stat.S_ISREG(before.st_mode) or (before.st_dev, before.st_ino) != (leaf.st_dev, leaf.st_ino):
            raise ValueError('Input must be a regular file')
        data = stream.read(maximum + 1)
        after = os.fstat(stream.fileno())
    if len(data) > maximum:
        raise ValueError('Input exceeds bounded size; full source retained in place')
    if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) != (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns) or len(data) != before.st_size:
        raise ValueError('Input changed while being retained')
    return data


def validate_bytes(manifest_data, roles):
    m = parse_manifest(manifest_data)
    if type(roles) is not tuple or len(roles) != 17:
        raise ValueError('All retained roles must validate before execution')
    for role, item in enumerate(m['objects']):
        data = roles[role]
        if type(data) is not bytes or len(data) != item['bytes'] or zlib.crc32(data) != item['crc32'] or digest(data) != item['sha256']:
            raise ValueError('Retained capture role length/CRC/SHA mismatch: ' + NAMES[role])
    # Opaque raw stores and logical banks are deliberately not normalized or
    # rejected for stale internal CRCs. Plan/project semantic checks are separate.
    return {'manifest': m, 'roles': roles, 'manifest_bytes': manifest_data,
            'manifest_sha256': digest(manifest_data)}


def retain_archive(source, destination):
    """Exclusive original retention; no adapter may run before this accepts all roles."""
    source, destination = Path(source), Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    report = {'format': 'dabbl8-instrument-migration-input', 'version': 1,
              'archive_validated': False, 'simulation_only': True, 'device_writes': False,
              'migration_executed': False, 'restore_executed': False,
              'production_ownership': False, 'hardware_qualified': False,
              'pre_boot_original': False, 'source_name': source.name,
              'originals': {}, 'unretained': {}, 'warnings': [
                  'Capture is post-boot, with unverified supplied-input provenance.',
                  'Input validation grants no migration, restore or device ownership authority.',
                  'Oversize, nonregular and unexpected sources remain in place; no subset is an accepted archive.']}
    snapshot = None
    originals = destination / 'retained'
    originals.mkdir()
    errors = []
    try:
        if source.is_symlink() or not source.is_dir():
            raise ValueError('Capture input must be a real directory')
        names = list(islice(source.iterdir(), 65))
        if len(names) > 64 or any(p.name not in {'manifest.json', *FILES} for p in names):
            errors.append('Unexpected capture directory contents')
        for name, maximum in [('manifest.json', MANIFEST_LIMIT), *zip(FILES, ROLE_LIMITS)]:
            try:
                data = bounded_file(source / name, maximum)
                with (originals / name).open('xb') as output:
                    output.write(data)
                if (originals / name).read_bytes() != data:
                    raise ValueError('Retained original copy readback mismatch')
                report['originals'][name] = {'bytes': len(data), 'sha256': digest(data)}
            except (OSError, ValueError) as error:
                report['unretained'][name] = str(error)
                errors.append(name + ': ' + str(error))
        if errors:
            raise ValueError('; '.join(errors))
        manifest_data = (originals / 'manifest.json').read_bytes()
        roles = tuple((originals / name).read_bytes() for name in FILES)
        snapshot = validate_bytes(manifest_data, roles)
        report['archive_validated'] = True
        report['manifest_sha256'] = snapshot['manifest_sha256']
        report['role_count'] = 17
        report['raw_bytes'] = 319488
        report['total_bytes'] = sum(map(len, roles))
    except (OSError, ValueError, KeyError, TypeError, UnicodeError, RecursionError) as error:
        report['reason'] = str(error)
    with (destination / 'input-report.json').open('x') as output:
        json.dump(report, output, indent=2)
        output.write('\n')
    return snapshot, report


RECEIPT_FIELDS = {'format', 'version', 'operation', 'status', 'completed', 'quarantined',
                  'production_ownership', 'hardware_qualified', 'device_writes',
                  'post_boot_originals', 'reads', 'erases', 'programs', 'mutation_calls',
                  'cut_operation', 'partial_bytes', 'source_plan_equivalence', 'plan_review_required'}
STATUSES = {'complete', 'invalid-plan', 'source-changed', 'unsupported-legacy-source',
            'interrupted-or-readback-failed', 'scope-failure'}


def parse_receipt(data, operation, cut, partial, exit_code):
    if len(data) > 4096:
        raise ValueError('Executor receipt exceeds bounded size')
    r = strict_json(data.decode('utf-8'))
    if type(r) is not dict or set(r) != RECEIPT_FIELDS:
        raise ValueError('Unsupported simulation receipt fields')
    if r['format'] != 'dabbl8-instrument-simulation' or not integer(r['version'], 1, 1) or r['operation'] != operation or type(r['status']) is not str or r['status'] not in STATUSES:
        raise ValueError('Wrong simulation receipt version/operation/status')
    if r['quarantined'] is not True or r['post_boot_originals'] is not True or r['plan_review_required'] is not True or any(r[k] is not False for k in ('production_ownership', 'hardware_qualified', 'device_writes', 'source_plan_equivalence')):
        raise ValueError('Simulation receipt claims authority or releases quarantine')
    if type(r['completed']) is not bool or r['completed'] != (r['status'] == 'complete') or exit_code != (0 if r['completed'] else 1):
        raise ValueError('Simulation receipt completion/exit mismatch')
    if any(not integer(r[k], 0, 0xffffffff) for k in ('reads', 'erases', 'programs', 'mutation_calls', 'cut_operation', 'partial_bytes')) or r['mutation_calls'] != r['erases'] + r['programs'] or r['cut_operation'] != cut or r['partial_bytes'] != partial:
        raise ValueError('Simulation receipt counters or cut echo mismatch')
    return r


def expected_image(current, roles, operation, plan):
    result = bytearray(current)
    for role in range(5 if operation == 'apply' else 12):
        data = plan[role * 8192:(role + 1) * 8192] if operation == 'apply' else roles[role]
        mapping = RAW_MAP[role]
        result[mapping[0]:mapping[0] + mapping[1]] = data[:mapping[1]]
        if mapping[3]:
            result[mapping[2]:mapping[2] + mapping[3]] = data[mapping[1]:]
    return bytes(result)


def simulate(source, destination, executor, current, operation, plan=None, cut=0, partial=0, reviewed_plan=False):
    snapshot, inputs = retain_archive(source, destination)
    destination = Path(destination)
    report = {'format': 'dabbl8-instrument-simulation-host', 'version': 1,
              'accepted': False, 'operation': operation, 'simulation_only': True,
              'quarantined': True, 'device_writes': False, 'production_ownership': False,
              'hardware_qualified': False, 'restore_qualified': False,
              'source_to_plan_musical_equivalence': False,
              'input_manifest_sha256': inputs.get('manifest_sha256'), 'warnings': [
                  'This operation changes only an explicitly supplied simulated NOR file.',
                  'A validated reviewed plan is supplied externally; source-to-plan musical equivalence is not established here.',
                  'Raw restoration preserves physical originals only; unsaved current logical roles remain retained separately.',
                  'Quarantine remains active after success; no RAM adoption, persistence polling release or production ownership.']}
    try:
        if snapshot is None:
            raise ValueError(inputs.get('reason', 'Capture input refused'))
        if operation not in ('apply', 'restore') or not integer(cut, 0, 0xffffffff) or not integer(partial, 0, 0xffffffff):
            raise ValueError('Invalid simulation operation or cut')
        if type(reviewed_plan) is not bool or (operation == 'apply' and not reviewed_plan) or (operation == 'restore' and reviewed_plan):
            raise ValueError('APPLY requires an explicit reviewed-plan decision; RESTORE does not adopt a plan')
        if (operation == 'apply') != (plan is not None):
            raise ValueError('APPLY requires a reviewed plan; RESTORE does not accept a plan')
        baseline = bounded_file(Path(current), 1048576)
        if len(baseline) != 1048576:
            raise ValueError('Explicit simulated current image must contain exactly one MiB')
        current_copy = destination / 'original-simulated-current.bin'
        current_copy.write_bytes(baseline)
        report['simulated_current_sha256'] = digest(baseline)
        proposal = None
        if plan is not None:
            proposal = bounded_file(Path(plan), 40960)
            if len(proposal) != 40960:
                raise ValueError('Reviewed proposed pool must contain exactly 40960 bytes')
            plan_copy = destination / 'original-reviewed-plan.bin'
            plan_copy.write_bytes(proposal)
            report['reviewed_plan_sha256'] = digest(proposal)
        executable = Path(executor).resolve()
        executable_data = bounded_file(executable, 32 * 1024 * 1024)
        report['executor_sha256'] = digest(executable_data)
        args = [str(executable), '--simulate', operation, str((destination / 'retained').resolve())]
        if proposal is not None:
            args.append(str(plan_copy.resolve()))
        result_path = destination / 'simulated-result.bin'
        args += [str(current_copy.resolve()), str(result_path.resolve()), str(cut), str(partial)]
        with (destination / 'executor-output.log').open('xb') as stdout, (destination / 'executor-stderr.log').open('xb') as stderr:
            process = subprocess.run(args, stdout=stdout, stderr=stderr, timeout=60)
        report['executor_exit'] = process.returncode
        receipt = parse_receipt(bounded_file(destination / 'executor-output.log', 4096), operation, cut, partial, process.returncode)
        report['executor_receipt'] = receipt
        output = bounded_file(result_path, 1048576)
        if len(output) != 1048576:
            raise ValueError('Simulation executor must retain a complete one-MiB result, including refusal')
        report['simulated_result_sha256'] = digest(output)
        # Verify every externally retained original again after the unqualified
        # supplied executor; never accept an altered source or false completion.
        retained = destination / 'retained'
        if bounded_file(retained / 'manifest.json', MANIFEST_LIMIT) != snapshot['manifest_bytes'] or any(bounded_file(retained / name, ROLE_LIMITS[role]) != snapshot['roles'][role] for role, name in enumerate(FILES)) or bounded_file(current_copy, 1048576) != baseline or digest(bounded_file(executable, 32 * 1024 * 1024)) != report['executor_sha256']:
            raise ValueError('Retained originals or executor changed during simulation')
        if proposal is not None and bounded_file(plan_copy, 40960) != proposal:
            raise ValueError('Retained reviewed plan changed during simulation')
        # Refusal/interruption may change allowed bytes, but never outside its
        # fixed allocation scope. Preserve those uncertain result bytes privately.
        expected = expected_image(baseline, snapshot['roles'], operation, proposal)
        scope = bytearray(1048576)
        for role in range(5 if operation == 'apply' else 12):
            a, n, b, m = RAW_MAP[role]
            scope[a:a + n] = b'\x01' * n
            scope[b:b + m] = b'\x01' * m
        if any(a != b and not scope[i] for i, (a, b) in enumerate(zip(baseline, output))):
            raise ValueError('Simulation result changed bytes outside fixed operation scope')
        if not receipt['completed']:
            raise ValueError('Simulation executor refused or was interrupted: ' + receipt['status'])
        if output != expected:
            raise ValueError('False simulation completion: entire result readback differs from expected fixed-role bytes')
        report['full_image_readback_verified'] = True
        report['accepted'] = True
    except (OSError, ValueError, TypeError, KeyError, UnicodeError, RecursionError, subprocess.TimeoutExpired) as error:
        report['reason'] = str(error)
    with (destination / 'simulation-report.json').open('x') as output:
        json.dump(report, output, indent=2)
        output.write('\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='Produced full-current verified directory')
    parser.add_argument('destination', type=Path, help='New exclusive private evidence directory')
    parser.add_argument('--operation', choices=('apply', 'restore'))
    parser.add_argument('--executor', type=Path, help='Explicitly supplied host-only simulation executable')
    parser.add_argument('--simulate-current', type=Path, help='Exact one-MiB simulated NOR input')
    parser.add_argument('--plan', type=Path, help='Externally reviewed complete 40960-byte proposed native pool')
    parser.add_argument('--reviewed-plan', action='store_true', help='Affirm the externally reviewed plan for simulation only; not production authority')
    parser.add_argument('--cut-operation', type=int, default=0)
    parser.add_argument('--partial-bytes', type=int, default=0)
    args = parser.parse_args()
    if args.operation is None and (any(x is not None for x in (args.executor, args.simulate_current, args.plan)) or args.reviewed_plan or args.cut_operation or args.partial_bytes):
        parser.error('simulation arguments require an explicit operation')
    if args.operation is not None and (args.executor is None or args.simulate_current is None):
        parser.error('simulation requires --executor and --simulate-current; no device transport is provided')
    try:
        if args.operation is None:
            unused, report = retain_archive(args.source, args.destination)
            accepted = report['archive_validated']
        else:
            report = simulate(args.source, args.destination, args.executor, args.simulate_current,
                              args.operation, args.plan, args.cut_operation, args.partial_bytes, args.reviewed_plan)
            accepted = report['accepted']
    except FileExistsError:
        parser.error('destination exists; originals are never overwritten')
    print(json.dumps(report, indent=2))
    return 0 if accepted else 1


if __name__ == '__main__':
    raise SystemExit(main())
