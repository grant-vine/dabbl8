// SPDX-License-Identifier: GPL-3.0-only
// Explicitly selected ports; only the typed D8Tracks client can send requests.
import { D8Tracks, TrackProtocolError } from './d8tracks.js';
const owners = new WeakMap();
const pushes = new Set([23, 24, 26, 32]);
const byte = (n) => Number.isInteger(n) && n >= 0 && n <= 127;
export class D8MidiSession {
  #input; #output; #timeout; #alive = true; #ready = false; #pending = null;
  #client = null; #opening; #cleanup = null; #signal; #notify;
  #receive = (e) => this.#message(e.data);
  #state = () => {
    if (this.#input.state !== 'connected' || this.#output.state !== 'connected' ||
        (this.#ready && (this.#input.connection !== 'open' || this.#output.connection !== 'open')))
      this.close('MIDI port disconnected or closed.');
  };
  #abort = () => { this.close('Connection cancelled.'); };
  constructor(input, output, { timeout = 1500, signal, onClose = () => {} } = {}) {
    if (input?.type !== 'input' || output?.type !== 'output' || input === output ||
        [input, output].some((p) => !['open', 'close', 'addEventListener', 'removeEventListener'].every((k) => typeof p[k] === 'function')) ||
        typeof output.send !== 'function' || !Number.isInteger(timeout) || timeout < 1 || timeout > 30000 ||
        typeof onClose !== 'function' || (signal !== undefined &&
        (typeof signal?.aborted !== 'boolean' || typeof signal?.addEventListener !== 'function' || typeof signal?.removeEventListener !== 'function'))) throw Error('Invalid MIDI ports or connection options.');
    if (owners.has(input) || owners.has(output)) throw Error('MIDI ports are already in use by a Dabbl8 session.');
    if (signal?.aborted) throw Error('Connection cancelled.');
    this.#input = input; this.#output = output; this.#timeout = timeout; this.#signal = signal; this.#notify = onClose;
    owners.set(input, this); owners.set(output, this);
    input.addEventListener('midimessage', this.#receive);
    input.addEventListener('statechange', this.#state); output.addEventListener('statechange', this.#state);
    signal?.addEventListener('abort', this.#abort, { once: true });
    // Defer open calls until #opening has been installed, including synchronous failures.
    this.#opening = Promise.allSettled([input, output].map((p) => Promise.resolve().then(() => p.open())));
  }
  static async connect(input, output, options) {
    const session = new D8MidiSession(input, output, options);
    try {
      const opened = await session.#opening;
      if (opened.some((r) => r.status === 'rejected')) throw Error('Could not open the selected MIDI ports.');
      if (!session.#alive) throw Error('Connection cancelled or disconnected.');
      session.#ready = true; session.#state();
      if (!session.#alive) throw Error('MIDI ports are unavailable.');
      session.#client = await D8Tracks.connect((r) => session.#request(r));
      if (!session.#alive) { session.#client.close(); throw Error('Connection cancelled or disconnected.'); }
      return session;
    } catch (error) { await session.close(error.message); throw error; }
  }
  get tracks() { return this.#client; }
  get connected() { return this.#alive && this.#ready; }
  #request([cmd, args]) {
    if (!this.connected) return Promise.reject(Error('Disconnected.'));
    if (![1, 75, 77].includes(cmd) || !Array.isArray(args) || args.length > 594 || !args.every(byte))
      return Promise.reject(Error('Unsupported MIDI request.'));
    if (this.#pending) return Promise.reject(Error('Another MIDI request is pending.'));
    return new Promise((resolve, reject) => {
      const pending = { cmd, resolve, reject, timer: null };
      this.#pending = pending;
      pending.timer = setTimeout(() => { this.close('MIDI reply timed out; reconnect before editing.'); }, this.#timeout);
      // No retries: a missing acknowledgment cannot prove a mutation was not applied.
      try { this.#output.send([240, 125, 70, 76, cmd, ...args, 247]); }
      catch (error) { this.close(`MIDI send failed: ${error.message}`); }
    });
  }
  #message(data) {
    if (!this.#alive || !data || data[0] !== 240 || data[1] !== 125 || data[2] !== 70 || data[3] !== 76) return;
    if (data.length < 6 || data.length > 600 || data[data.length - 1] !== 247 ||
        !Array.from(data).slice(4, -1).every(byte)) { this.close('Malformed MIDI reply.'); return; }
    const cmd = data[4], args = Array.from(data).slice(5, -1);
    if (pushes.has(cmd)) return; // Previously enabled WATCH notifications are not acknowledgments.
    const pending = this.#pending;
    if (!pending) { this.close('Unexpected MIDI reply; reconnect before editing.'); return; }
    if (cmd === 76) {
      if (args.length !== 3 || args[0] !== 1 || args[1] !== pending.cmd || args[2] < 1 || args[2] > 4) {
        this.close('Malformed or mismatched firmware refusal.'); return;
      }
      this.#finish(null, new TrackProtocolError(args[2])); return;
    }
    if (cmd !== pending.cmd) { this.close('MIDI reply targets another request.'); return; }
    this.#finish(args);
  }
  #finish(value, error) {
    const pending = this.#pending; if (!pending) return;
    clearTimeout(pending.timer); this.#pending = null;
    if (error) pending.reject(error); else pending.resolve(value);
  }
  close(reason = 'Disconnected.') {
    if (this.#cleanup) return this.#cleanup;
    this.#alive = false; this.#client?.close(); this.#finish(null, Error(reason));
    this.#input.removeEventListener('midimessage', this.#receive);
    this.#input.removeEventListener('statechange', this.#state); this.#output.removeEventListener('statechange', this.#state);
    this.#signal?.removeEventListener('abort', this.#abort);
    this.#cleanup = (async () => {
      await this.#opening;
      const results = await Promise.allSettled([this.#input, this.#output].map((p) => Promise.resolve().then(() => p.close())));
      owners.delete(this.#input); owners.delete(this.#output);
      return results; // Failed browser closes remain observable to the caller.
    })();
    try { this.#notify(reason); } catch { /* UI callbacks cannot prevent port cleanup. */ }
    return this.#cleanup;
  }
}
