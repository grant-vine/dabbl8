#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Persistent command-78 capture bridge. No session/migration/update commands.

CoreMIDI dependency and physical endpoint behaviour remain separately qualified.
The collector owns token/phase/chunk/CRC/two-scan verification and publication.
"""
import argparse
import math
import json
import platform
import sys
import threading
import time
from dabbl8_instrument_capture import HEADER, r32


def request_operation(frame):
    """Validate the complete read-only grammar before any send."""
    if type(frame) is not bytes or len(frame) < 8 or not frame.startswith(HEADER) or frame[-1] != 247 or any(x > 127 for x in frame[5:-1]):
        raise ValueError('Not a canonical command-78 request')
    args = frame[5:-1]
    if args[0] != 1 or args[1] > 6:
        raise ValueError('Unsupported capture request version/operation')
    op = args[1]
    sizes = (2, 3, 7, 8, 15, 7, 7)
    if len(args) != sizes[op]:
        raise ValueError('Wrong capture request length')
    if op == 1 and args[2] not in (0, 1):
        raise ValueError('Unsupported completeness mode')
    if op >= 2:
        r32(args[2:7])
    if op in (3, 4) and args[7] >= 17:
        raise ValueError('Unsupported capture role')
    if op == 4:
        offset = r32(args[8:13]); count = args[13] | (args[14] << 7)
        if not 1 <= count <= 256 or offset > 81920 or offset + count > 81920:
            raise ValueError('Unbounded capture chunk')
    return op


class ReadOnlyMidi:
    """One pending request and one bounded reply; any ambiguity poisons this link.

    Callback never closes ports: backend close is not thread-safe. Main-thread
    exchange/close performs cleanup, including errors received between requests.
    Exact endpoint names are selections, not hardware identity attestation.
    """
    def __init__(self, backend, message, input_name, output_name, *, timeout=10, settle=0.25, record=lambda frame: None):
        if any(type(x) is not str or not x or len(x) > 1024 for x in (input_name, output_name)):
            raise ValueError('Explicit input and output names required')
        if type(timeout) not in (int, float) or not math.isfinite(timeout) or not 0 < timeout <= 30 or type(settle) not in (int, float) or not math.isfinite(settle) or not 0 <= settle <= 2:
            raise ValueError('Invalid timeout/open settle interval')
        self.backend, self.message = backend, message
        self.input_name, self.output_name = input_name, output_name
        if not callable(record):
            raise ValueError('Receive journal callback required')
        self.record = record
        self.timeout = timeout
        self.condition = threading.Condition()
        self.operation = None; self.request = None; self.reply = None; self.error = None
        self.closed = False; self.input = None; self.output = None
        self.owner = threading.get_ident()
        # No default port, substring matching, virtual port or broadcast fallback.
        if backend.get_input_names().count(input_name) != 1 or backend.get_output_names().count(output_name) != 1:
            raise ValueError('Selected endpoints absent or ambiguous')
        try:
            self.input = backend.open_input(input_name, callback=self._receive)
            self.output = backend.open_output(output_name)
            time.sleep(settle)
            if self.error:
                raise ValueError(self.error)
        except Exception:
            self.close()
            raise

    def _receive(self, message):
        with self.condition:
            if self.closed or self.error:
                return
            if getattr(message, 'type', None) != 'sysex':
                return  # Notes/clocks are not acknowledgments.
            try:
                data = message.data
                if len(data) > 598 or any(type(x) is not int or not 0 <= x <= 127 for x in data):
                    raise ValueError('Malformed or oversized MIDI reply')
                frame = bytes((240, *data, 247))
                self.record(frame)
                if not frame.startswith(HEADER) or len(frame) < 15 or frame[5] != 1:
                    raise ValueError('Unexpected MIDI SysEx reply')
                if self.operation is None or self.reply is not None or frame[6] != self.operation:
                    raise ValueError('Unsolicited, duplicate or mismatched MIDI reply')
                token = r32(frame[8:13])
                if frame[13] > 4:
                    raise ValueError('Unsupported capture phase')
                if self.operation >= 2 and token != r32(self.request[7:12]):
                    raise ValueError('Capture token does not match request')
                if frame[7] == 0:
                    lengths = (21, 15, 21, 46, None, 15, 15)
                    expected = lengths[self.operation]
                    if self.operation == 4:
                        count = self.request[18] | (self.request[19] << 7)
                        expected = 23 + count + (count + 6) // 7
                    if len(frame) != expected:
                        raise ValueError('Wrong capture reply payload length')
                elif len(frame) != 15:
                    raise ValueError('Malformed capture refusal payload')
                self.reply = frame
            except Exception as error:
                self.error = str(error)
            self.condition.notify_all()

    def exchange(self, frame):
        if threading.get_ident() != self.owner:
            raise ValueError('MIDI operations must use the owning thread')
        try:
            op = request_operation(frame)
            with self.condition:
                if self.closed or self.error or self.operation is not None:
                    raise ValueError(self.error or 'Link closed or request already pending')
                if self.input.closed or self.output.closed:
                    raise ValueError('Selected MIDI port closed')
                if self.backend.get_input_names().count(self.input_name) != 1 or self.backend.get_output_names().count(self.output_name) != 1:
                    raise ValueError('Selected MIDI endpoint disappeared or became ambiguous')
                self.operation = op; self.request = frame
            # Send outside the callback lock; fake and native callbacks may be immediate.
            self.output.send(self.message('sysex', data=frame[1:-1]))
            deadline = time.monotonic() + self.timeout
            with self.condition:
                while self.reply is None and not self.error:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise ValueError('Capture reply timed out; reconnect required')
                    self.condition.wait(remaining)
                if self.error:
                    raise ValueError(self.error)
                reply = self.reply
                self.reply = None; self.operation = None; self.request = None
                return reply
        except Exception:
            self.close()
            raise

    def close(self):
        if threading.get_ident() != self.owner:
            raise ValueError('MIDI close must use the owning thread')
        with self.condition:
            if self.closed:
                return
            self.closed = True; self.operation = None; self.request = None; self.reply = None
            self.condition.notify_all()
        failures = []
        for port in (self.input, self.output):
            if port is not None:
                try:
                    port.close()
                except Exception as error:
                    failures.append(str(error))
        if failures:
            raise ValueError('MIDI port cleanup failed: ' + '; '.join(failures))


def bridge(link, incoming=sys.stdin, outgoing=sys.stdout):
    """Existing capture.Bridge full-frame hex-line interface, bounded before parsing."""
    while True:
        line = incoming.readline(1802)
        if not line:
            break
        if len(line) > 1800 or not line.endswith('\n'):
            raise ValueError('Unbounded or incomplete request line')
        tokens = line[:-1].split(' ')
        if any(len(x) != 2 or any(c not in '0123456789abcdefABCDEF' for c in x) for x in tokens):
            raise ValueError('Request must be canonical space-separated hex bytes')
        reply = link.exchange(bytes.fromhex(line))
        outgoing.write(reply.hex(' ') + '\n'); outgoing.flush()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bridge', action='store_true', required=True)
    parser.add_argument('--input-port', required=True)
    parser.add_argument('--output-port', required=True)
    parser.add_argument('--timeout', type=float, default=10)
    args = parser.parse_args()
    if platform.system() != 'Darwin':
        parser.error('This adapter selects the macOS CoreMIDI backend only')
    # Lazy optional dependency; --help and all offline tests need no MIDI backend.
    try:
        import mido
    except ImportError:
        parser.error('Optional MIDI environment needs mido and python-rtmidi')
    backend = mido.Backend('mido.backends.rtmidi/MACOSX_CORE')
    link = None
    try:
        link = ReadOnlyMidi(backend, mido.Message, args.input_port, args.output_port, timeout=args.timeout,
                            record=lambda frame: print(json.dumps({'received_hex': frame.hex()}), file=sys.stderr, flush=True))
        bridge(link)
    finally:
        if link is not None:
            link.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
