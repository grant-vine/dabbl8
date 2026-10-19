#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit Mac CDC timing collection. No flashing, MIDI or storage commands."""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import os
from pathlib import Path
import re
import select
import stat
import sys
import time
from typing import TypedDict
from dabbl8_timing_report import parse_report

COMMANDS = ('status', 'timing start', 'timing stop', 'timing report')
RESOURCE_COMMANDS = ('status', 'resource start', 'resource stop', 'resource report')
CLOCK_COMMANDS = ('status', 'timing clock')


def command_profile(commands):
    if type(commands) is not tuple or commands not in (COMMANDS, RESOURCE_COMMANDS, CLOCK_COMMANDS):
        raise ValueError('Unsupported fixed console command profile')
    return commands
MAX_REPLY = 65536


class TimingProfileMismatch(ValueError):
    def __init__(self, expected_uac: bool) -> None:
        self.expected_uac = expected_uac
        super().__init__(f'Timing profile mismatch: expected USB audio enabled={expected_uac}')


class PackageIdentity(TypedDict):
    source_commit: str
    manifest_sha256: str
    diagnostic_package: str
    package_sha256: str
    device_binary_identity_attested: bool
    clock_source_sha256: str | None


def package_identity(manifest: Path) -> PackageIdentity:
    with manifest.open('rb') as stream:
        raw = stream.read(2_000_001)
    if len(raw) > 2_000_000:
        raise ValueError('Manifest too large')
    m = json.loads(raw)
    source = m['source_commit']
    d = m['candidates']['diagnostic']
    name = d['package']
    if not re.fullmatch(r'[0-9a-f]{40}', source) or not re.fullmatch(r'dabbl8-[0-9.]+-timing-diagnostic\.fwsc', name):
        raise ValueError('Unsupported candidate identity')
    package = manifest.parent / name
    if package.is_symlink() or package.stat().st_size > 2_000_000:
        raise ValueError('Invalid package file')
    digest = hashlib.sha256(package.read_bytes()).hexdigest()
    if digest != d['sha256'] or package.stat().st_size != d['bytes']:
        raise ValueError('Diagnostic package checksum/size mismatch')
    clock_source = m.get('source_inputs', {}).get('firmware/src/audio_clock_console.h')
    if clock_source is not None and (type(clock_source) is not str or not re.fullmatch(r'[0-9a-f]{64}', clock_source)):
        raise ValueError('Invalid clock source checksum')
    return {'source_commit': source, 'manifest_sha256': hashlib.sha256(raw).hexdigest(),
            'diagnostic_package': name, 'package_sha256': digest,
            'device_binary_identity_attested': False,
            'clock_source_sha256': clock_source if m.get('timing_diagnostic') is True else None}


class Console:
    """Bounded request/reply framing; single exact prompt, no automatic retries."""
    def __init__(self, transport, journal, clock=time.monotonic, *, commands=COMMANDS):
        self.commands = command_profile(commands)
        self.transport, self.journal, self.clock = transport, journal, clock

    def receive(self):
        deadline = self.clock() + 10
        raw = bytearray()
        while self.clock() < deadline:
            part = self.transport.read(deadline - self.clock())
            if not part:
                continue
            self.journal('receive', part)
            raw.extend(part)
            if len(raw) > MAX_REPLY or any(c >= 128 for c in raw):
                raise ValueError('Oversized or non-ASCII console reply')
            if raw.endswith(b'\r\n> '):
                if raw.count(b'\r\n> ') != 1:
                    raise ValueError('Multiple console prompts / stale reply')
                return raw.decode('ascii')
        raise TimeoutError('Console response timed out; no retry')

    def request(self, command):
        if command not in self.commands:
            raise ValueError('Command outside selected diagnostic allowlist')
        data = command.encode('ascii') + b'\r'
        self.journal('send', data)
        self.transport.write(data)
        reply = self.receive()
        if not reply.startswith(command + '\r\n'):
            raise ValueError('Missing command echo / stale console reply')
        return reply


def status(reply: str, version: str, uac_enabled: bool = True) -> dict[str, int]:
    lines = reply.splitlines()
    if lines.count('dabbl8 ' + version) != 1:
        raise ValueError('Wrong or missing diagnostic firmware identity')
    result = {'uac_enabled': int(uac_enabled)}
    if not uac_enabled and any(line.startswith('uac_') for line in lines):
        raise TimingProfileMismatch(uac_enabled)
    keys = ('boots', 'usb_resets')
    if uac_enabled:
        keys += ('uac_hz', 'uac_alt', 'uac_rate_sets', 'uac_starts')
    for key in keys:
        rows = [line for line in lines if line.startswith(key + ' ')]
        if len(rows) != 1 or not re.fullmatch(key + r' -?[0-9]+', rows[0]):
            raise ValueError('Missing/ambiguous status field: ' + key)
        value = int(rows[0].split()[1])
        if not -(1 << 31) <= value < (1 << 31):
            raise ValueError('Status field outside signed 32-bit range')
        result[key] = value & 0xffffffff
    return result


def collect(console, output, version, seconds, sleep=time.sleep, *, uac_enabled: bool = True):
    active = False
    stop_attempted = False
    try:
        banner = console.receive()
        if banner.splitlines().count("Dabbl8 " + version + " console - 'help'") != 1:
            raise ValueError('Expected fresh diagnostic console banner absent')
        before = status(console.request('status'), version, uac_enabled)
        start = console.request('timing start')
        if start.splitlines().count('timing started') != 1:
            raise ValueError('Timing start refused; previous run was not overwritten')
        active = True
        sleep(seconds)
        stop_attempted = True  # A missing acknowledgement must not trigger a retry.
        stopped = console.request('timing stop')
        if stopped.splitlines().count('timing stopped') != 1:
            raise ValueError('Timing stop not acknowledged')
        active = False
        reply = console.request('timing report')
        frame = '\r\n'.join(line for line in reply.splitlines() if line.startswith('d8t')) + '\r\n'
        (output / 'report.frame').write_text(frame, encoding='ascii')
        report = parse_report(frame, expected_boots=before['boots'])
        if report['format'] != 'D8T2' or report['tracks'] != 8 or not report['samples']:
            raise ValueError('Expected nonempty eight-track D8T2 run')
        if report['window']['uac_enabled'] != uac_enabled:
            raise TimingProfileMismatch(uac_enabled)
        after = status(console.request('status'), version, uac_enabled)
        if before != after:
            raise ValueError('Boot/USB configuration changed during outer observation window')
        report['host_observation'] = {'before': before, 'after': after, 'requested_seconds': seconds,
            'clock_or_stack_calibration': False, 'device_binary_identity_attested': False,
            'scope': 'Version string and stable status are observations, not cryptographic device identity or physical deadline proof.'}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        return report
    finally:
        if active and not stop_attempted:
            # One cleanup attempt for a run this collector started; never retry.
            try:
                stopped = console.request('timing stop')
                if stopped.splitlines().count('timing stopped') != 1:
                    raise ValueError('Timing cleanup stop not acknowledged')
            except (OSError, RuntimeError, ValueError):
                console.journal('cleanup-failed', b'Collection may still be active; device requires explicit inspection.')
        elif active:
            console.journal('cleanup-failed', b'Timing stop unconfirmed; no retry. Device requires explicit inspection.')


class MacCdc:
    """Exact physical tty only; native modem/termios path remains hardware-unverified."""
    def __init__(self, port, *, commands=COMMANDS):
        self.commands = command_profile(commands)
        if sys.platform != 'darwin' or not re.fullmatch(r'/dev/cu\.usbmodem[A-Za-z0-9_-]+', port):
            raise ValueError('An explicit /dev/cu.usbmodem… port on macOS is required')
        import fcntl
        import termios
        self.fd, self.old, self.dtr = None, None, None
        self.fcntl, self.termios = fcntl, termios
        try:
            self.fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK | os.O_NOFOLLOW)
            if not stat.S_ISCHR(os.fstat(self.fd).st_mode):
                raise ValueError('Selected port is not a character device')
            fcntl.ioctl(self.fd, termios.TIOCEXCL)
            self.old = termios.tcgetattr(self.fd)
            flags = array.array('i', [0]); fcntl.ioctl(self.fd, termios.TIOCMGET, flags, True)
            self.dtr = bool(flags[0] & termios.TIOCM_DTR)
            attrs = termios.tcgetattr(self.fd)
            attrs[0] = attrs[1] = attrs[3] = 0
            attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            attrs[4] = attrs[5] = termios.B115200  # firmware ignores baud rate
            attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
            # A reopen may inherit asserted DTR. The firmware greets only on a
            # main-loop-observed rising edge; establish one without resetting USB.
            # The bounded settling interval is not proof the firmware is healthy:
            # callers still require the exact fresh greeting and status afterward.
            # macOS may assert DTR during open and queue an automatic greeting.
            # Drain that unrequested startup input while the device can transmit,
            # before requesting our greeting. No command has been sent yet.
            fcntl.ioctl(self.fd, termios.TIOCMBIS, array.array('i', [termios.TIOCM_DTR]))
            time.sleep(0.1)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            fcntl.ioctl(self.fd, termios.TIOCMBIC, array.array('i', [termios.TIOCM_DTR]))
            time.sleep(0.1)
            fcntl.ioctl(self.fd, termios.TIOCMBIS, array.array('i', [termios.TIOCM_DTR]))
        except Exception:
            self.close()
            raise

    def read(self, timeout):
        if not select.select([self.fd], [], [], min(timeout, 0.25))[0]:
            return b''
        data = os.read(self.fd, 4096)
        if not data:
            raise OSError('Console disconnected')
        return data

    def write(self, data):
        if data not in tuple(command.encode('ascii') + b'\r' for command in self.commands):
            raise ValueError('Native transport refuses non-allowlisted bytes')
        if os.write(self.fd, data) != len(data):
            raise OSError('Partial console write; no retry')

    def close(self):
        if self.fd is None:
            return
        errors = []
        try:
            if self.dtr is not None:
                try:
                    cmd = self.termios.TIOCMBIS if self.dtr else self.termios.TIOCMBIC
                    self.fcntl.ioctl(self.fd, cmd, array.array('i', [self.termios.TIOCM_DTR]))
                except Exception as error:
                    errors.append(str(error))
            if self.old is not None:
                try:
                    self.termios.tcsetattr(self.fd, self.termios.TCSANOW, self.old)
                except Exception as error:
                    errors.append(str(error))
        finally:
            os.close(self.fd); self.fd = None
        if errors:
            raise OSError('Console state restoration failed: ' + '; '.join(errors))



def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--version', default='v0.1.0-timing')
    p.add_argument('--seconds', type=float, default=10)
    p.add_argument('--workload', required=True)
    p.add_argument('--uac-disabled', action='store_true',
                   help='Explicit UAC-off diagnostic; reject UAC status fields or an enabled report')
    p.add_argument('--port')
    p.add_argument('--output', type=Path)
    p.add_argument('--capture', action='store_true', help='Explicitly open the named CDC port; not install authorization')
    args = p.parse_args()
    if not re.fullmatch(r'v[0-9]\.[0-9]\.[0-9]-timing', args.version) or not math.isfinite(args.seconds) or not 1 <= args.seconds <= 60:
        p.error('Expected vX.Y.Z-timing version and 1..60 seconds')
    identity = package_identity(args.manifest)
    if not args.capture:
        print(json.dumps({'mode': 'plan-only; no device access', 'identity': identity, 'commands': COMMANDS, 'workload': args.workload, 'uac_enabled': not args.uac_disabled}, indent=2))
        return 0
    if not args.port or args.output is None:
        p.error('--capture requires explicit --port and new --output directory')
    args.output.mkdir(parents=True, exist_ok=False)
    receipt = {**identity, 'workload': args.workload, 'port': args.port, 'version': args.version,
               'started_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'accepted': False,
               'hardware_qualified': False, 'device_flashed': False}
    transport = None
    origin = time.monotonic()
    with (args.output / 'journal.jsonl').open('x') as log:
        def journal(direction, data):
            log.write(json.dumps({'host_seconds': time.monotonic() - origin, 'direction': direction, 'hex': data.hex(), 'ascii': data.decode('ascii', errors='backslashreplace')}) + '\n'); log.flush()
        try:
            transport = MacCdc(args.port)
            collect(Console(transport, journal), args.output, args.version, args.seconds,
                    uac_enabled=not args.uac_disabled)
            receipt['accepted'] = True
        except Exception as error:
            receipt['failure'] = str(error)
        finally:
            if transport:
                try:
                    transport.close()
                except Exception as error:
                    receipt['close_failure'] = str(error); receipt['accepted'] = False
            receipt['ended_utc'] = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())
            (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))
    return 0 if receipt['accepted'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
