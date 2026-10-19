# SPDX-License-Identifier: GPL-3.0-only
"""One private command-78 snapshot; default plan opens no MIDI connection."""
from __future__ import annotations

from contextlib import closing
import json
from pathlib import Path
import sys
from time import monotonic
from collections.abc import Callable

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.dabbl8_instrument_capture import collect
from tools.dabbl8_native_midi_guard import require_native_qualification
from tools.dabbl8_readonly_midi import ReadOnlyMidi, StrictCoreMidiBackend


class SnapshotRefused(RuntimeError):
    def __init__(self, detail: str) -> None:
        super().__init__('Read-only snapshot refused: ' + detail)


def capture(exchange: Callable[[bytes], bytes], destination: Path) -> None:
    deadline = monotonic() + 120
    released = False
    requests = 0
    pending_operation = 0

    def bounded(frame: bytes) -> bytes:
        nonlocal released, requests, pending_operation
        requests += 1
        if frame[6] != 6:
            pending_operation = frame[6]
        if frame[6] != 6 and monotonic() >= deadline:
            raise SnapshotRefused('120-second capture deadline exceeded')
        reply = exchange(frame)
        if frame[6] == 6:
            released = len(reply) == 15 and reply[5:8] == bytes((1, 6, 0)) and reply[13] == 4
        return reply

    report = collect(bounded, destination, mode='full',
                     provenance='Guarded native read-only snapshot; physical target qualification separate')
    if not report['accepted'] or not report['current_state_complete'] or not released:
        reason = report.get('reason') or report.get('release_error')
        if reason:
            failure = report.get('failed_request', {})
            raise SnapshotRefused(
                f"{reason} (request {failure.get('sequence', requests)}, "
                f"operation {failure.get('operation', pending_operation)}, "
                f"scan role {failure.get('scan_role', 'n/a')}, "
                f"offset {failure.get('scan_offset', 'n/a')})")
        raise SnapshotRefused('capture verification or release acknowledgement failed')
    manifest = json.loads((destination / 'verified/manifest.json').read_text())
    if [item['role'] for item in manifest['objects']] != list(range(17)):
        raise SnapshotRefused('incomplete role inventory')


def main() -> None:
    if not sys.argv[1:]:
        print(json.dumps({'mode': 'plan-only', 'device_access': False, 'roles': 17,
                          'notes': False, 'saved_data_writes': False, 'reset_or_flash': False,
                          'deadline_seconds': 120, 'retries': 0}, indent=2))
        return
    if len(sys.argv) != 3 or sys.argv[1] != '--capture':
        raise SnapshotRefused('expected --capture NEW_PRIVATE_DIRECTORY')
    require_native_qualification()
    import mido
    backend = StrictCoreMidiBackend(mido.Backend('mido.backends.rtmidi/MACOSX_CORE'))
    output = Path(sys.argv[2])
    output.mkdir(mode=0o700, parents=False, exist_ok=False)
    accepted = False
    link = None
    primary_error = None
    try:
        with (output / 'midi-replies.jsonl').open('x') as journal:
            with closing(ReadOnlyMidi(backend, mido.Message, 'Dabbl8', 'Dabbl8',
                         record=lambda frame: journal.write(json.dumps({'hex': frame.hex()}) + '\n'))) as link:
                try:
                    capture(link.exchange, output / 'snapshot')
                except (SnapshotRefused, OSError, ValueError) as failure:
                    primary_error = str(failure)
                    raise
        accepted = True
    finally:
        error = sys.exc_info()[1]
        receipt = {'accepted': accepted, 'native_ports_closed_successfully': bool(link is not None and link.closed and
                       link.cleanup_succeeded),
                   'failure': primary_error or (str(error) if error is not None else None),
                   'cleanup_error': link.cleanup_error if link is not None else None,
                   'notes_sent': 0, 'saved_data_writes': False, 'firmware_flashed': False}
        with (output / 'receipt.json').open('x') as stream:
            json.dump(receipt, stream, indent=2)
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
