#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private simulation orchestration of capture→retention→proposal/resume→session.

No MIDI transport or physical-device CLI is supplied. The shipping coordinator
retains write quarantine. An executable bridge is unqualified supplied code.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import dabbl8_instrument_capture as capture
import dabbl8_instrument_migrate as migration
import dabbl8_instrument_plan as planner

COMMAND = 79
HEADER = bytes((0xf0, 0x7d, 0x46, 0x4c, COMMAND))
READY, FAILED, CONSENT = 9, 10, 4

def pack(data):
    result = bytearray()
    for start in range(0, len(data), 7):
        piece = data[start:start+7]
        result.append(sum((x >> 7) << i for i, x in enumerate(piece)))
        result.extend(x & 127 for x in piece)
    return bytes(result)

class Session:
    def __init__(self, exchange, transcript):
        self.exchange, self.transcript = exchange, transcript
        self.generation, self.request, self.received = 0, 0, 0
        self.own = 0

    def ask(self, op, suffix=b'', begin=False):
        sequence = 0 if begin else self.request + 1
        if sequence > 0xffffffff:
            raise ValueError('Session request sequence exhausted')
        args = bytes((1, op)) + (b'' if begin else capture.u32(self.generation) + capture.u32(sequence)) + suffix
        frame = HEADER + args + b'\xf7'
        reply = self.exchange(frame)
        self.transcript.write(json.dumps({'request': frame.hex(), 'reply': reply.hex()})+'\n')
        self.transcript.flush()
        if len(reply) != 29 or not reply.startswith(HEADER) or reply[-1] != 0xf7 or any(x > 127 for x in reply[5:-1]):
            raise ValueError('Malformed session reply')
        if reply[5] != 1 or reply[6] or reply[7] > FAILED:
            raise ValueError('Session refused request: '+str(reply[6]))
        generation = capture.r32(reply[8:13])
        echoed = capture.r32(reply[13:18])
        received = capture.r32(reply[18:23])
        own = capture.r32(reply[23:28])
        if not generation or echoed != sequence or (not begin and generation != self.generation):
            raise ValueError('Stale session generation/sequence')
        if received > 40960 or (not begin and (received < self.received or own < self.own)):
            raise ValueError('Session progress/counter regressed')
        self.generation, self.request, self.received, self.own = generation, sequence, received, own
        if reply[7] == FAILED:
            raise ValueError('Session failed')
        return reply[7]

    def abort(self):
        if self.generation:
            try:
                self.ask(4)
            except Exception:
                pass


def run(exchange, destination, *, operation, autosave_source, confirm, converter=None, initializer=None, adapter=None):
    """No consent Boolean: confirm invokes an actual private-fixture panel event.

    The ordinary collector still performs its normal cleanup. Only this dedicated
    successful callback consumes the live capture before that cleanup occurs.
    """
    if operation not in ('apply', 'resume') or autosave_source not in ('persisted', 'current'):
        raise ValueError('Explicit operation/source required')
    if operation == 'resume' and autosave_source != 'persisted':
        raise ValueError('Native resume deliberately adopts persisted native autosave')
    destination = Path(destination)
    session = None
    workflow = {'format': 'dabbl8-instrument-session-simulation', 'version': 1,
                'completed': False, 'operation': operation, 'autosave_source': autosave_source,
                'simulation_only': True, 'hardware_qualified': False, 'device_writes': False,
                'production_ownership': False, 'reboot_permission_retained': False}

    def handoff(source, token):
        nonlocal session
        snapshot, retained = migration.retain_archive(source, destination/'independent-originals')
        if snapshot is None or not retained['archive_validated']:
            raise ValueError('Independent complete originals retention refused')
        proposal = None
        if operation == 'apply':
            if any(x is None for x in (converter, initializer, adapter)):
                raise ValueError('Actual converter/initializer/readback adapter required')
            report = planner.prepare(source, destination/'source-derived-plan', converter, initializer, adapter, autosave_source)
            if not report['accepted']:
                raise ValueError('Source-derived plan refused: '+report.get('reason',''))
            if report['source_manifest_sha256'] != retained['manifest_sha256']:
                raise ValueError('Source-derived plan provenance differs from independently retained capture')
            proposal = (destination/'source-derived-plan'/'proposed-native-pool.bin').read_bytes()
            if len(proposal) != 40960:
                raise ValueError('Wrong proposed pool extent')
            workflow['plan_sha256'] = hashlib.sha256(proposal).hexdigest()
        if hashlib.sha256((source/'manifest.json').read_bytes()).hexdigest() != retained['manifest_sha256']:
            raise ValueError('Original capture manifest changed before live handoff')
        for name, data in zip(migration.FILES, snapshot['roles']):
            if migration.bounded_file(destination/'independent-originals'/'retained'/name,len(data)) != data or migration.bounded_file(source/name,len(data)) != data:
                raise ValueError('Originals changed before live handoff')
        with (destination/'session-frames.jsonl').open('x') as transcript:
            session = Session(exchange, transcript)
            try:
                session.ask(1, capture.u32(token)+bytes((operation=='resume', autosave_source=='current', 1)), begin=True)
                if proposal is not None:
                    for off in range(0,40960,256):
                        piece=proposal[off:off+256]
                        phase=session.ask(2,capture.u32(off)+bytes((0,2))+pack(piece))
                        if session.received != off+256:
                            raise ValueError('Ordered proposal acknowledgement differs')
                phase = session.ask(3)
                steps=0
                while phase != CONSENT:
                    if steps >= 4096:
                        raise ValueError('Bounded verification did not reach consent')
                    phase=session.ask(3);steps+=1
                # Caller supplies only a private simulated panel event, never a
                # protocol bit. Firmware checks actual SAVE edge and release.
                confirm()
                deadline=time.monotonic()+120
                while phase != READY:
                    if steps >= 8192 or time.monotonic()>deadline:
                        raise ValueError('Bounded session did not complete')
                    phase=session.ask(3);steps+=1
                workflow.update(completed=True, generation=session.generation,
                                observed_own_operations=session.own,
                                independently_retained_manifest_sha256=retained['manifest_sha256'])
                return dict(workflow)
            except Exception:
                session.abort()
                raise

    report=capture.collect(exchange,destination,'full','private supplied simulation bridge',_verified_handoff=handoff)
    if not workflow['completed']:
        workflow['reason']=report.get('reason','Capture or workflow refused')
    (destination/'session-report.json').write_text(json.dumps(workflow,indent=2)+'\n')
    return workflow


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('destination',type=Path)
    p.add_argument('--simulation-bridge',type=Path,required=True)
    p.add_argument('--operation',choices=('apply','resume'),required=True)
    p.add_argument('--autosave-source',choices=('persisted','current'),required=True)
    for name in ('converter','initializer','adapter'):
        p.add_argument('--'+name,type=Path)
    a=p.parse_args();bridge=capture.Bridge(a.simulation_bridge)
    try:
        def confirm():
            if bridge.line('#consent')!='ok':
                raise ValueError('Private simulated panel event failed')
        report=run(bridge.exchange,a.destination,operation=a.operation,autosave_source=a.autosave_source,
                   confirm=confirm,converter=a.converter,initializer=a.initializer,adapter=a.adapter)
        bridge.retain_diagnostics(a.destination)
    finally:
        bridge.close()
    print(json.dumps(report,indent=2));return 0 if report['completed'] else 1

if __name__=='__main__':
    raise SystemExit(main())
