// SPDX-License-Identifier: GPL-3.0-only
import { ARCHIVE_LIMIT, createOwnArchive, nativeArchiveCodec, validateOwnArchive } from './d8archive.js';

async function defaultCodec() {
  const { default: factory } = await import('../build/d8archive/dabbl8_project_archive.mjs');
  return nativeArchiveCodec(factory);
}

export function mountProjects(root, {
  getSession,
  getCodec = defaultCodec,
  onBusy = () => {},
  onRestored = async () => {},
  saveFile = (bytes, name) => {
    const url = URL.createObjectURL(new Blob([bytes], { type: 'application/json' }));
    const a = root.ownerDocument.createElement('a');
    a.href = url; a.download = name; a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  },
  schedule = (fn) => setTimeout(fn, 400),
  unschedule = clearTimeout,
} = {}) {
  const doc = root.ownerDocument;
  const text = (tag, value) => { const n = doc.createElement(tag); n.textContent = value; return n; };
  const button = (label, action) => {
    const n = text('button', label); n.type = 'button';
    n.addEventListener('click', action); return n;
  };
  const status = text('p', 'Connect compatible Dabbl8 firmware to back up projects.');
  status.setAttribute('role', 'status'); status.setAttribute('aria-live', 'polite');
  const file = doc.createElement('input'); file.type = 'file'; file.accept = '.d8bak,application/json';
  file.setAttribute('aria-label', 'Choose a Dabbl8 project backup');
  const selected = text('p', 'No backup selected.');
  const backup = button('Back up projects', backUp);
  const restore = button('Prepare restore', prepare);
  const cancel = button('Cancel restore', cancelRestore);
  const keep = button('Save selected backup again', () => {
    if (original) saveFile(original.slice(), filename);
  });
  const row = doc.createElement('div'); row.className = 'row';
  row.append(backup, restore, cancel, keep);
  root.setAttribute('aria-label', 'Projects');
  root.append(text('h2', 'Projects'),
    text('p', 'A project backup includes three saved projects, the shared autosave and the current unsaved project. Samples, user presets, settings and recovery firmware are not included.'),
    text('p', 'Restore replaces saved projects and autosave, including empty slots. If interrupted, the set can be partially restored: keep the complete backup and retry. The device must display the request and receive a SAVE press and release before any replacement.'),
    file, selected, row, status);

  let connection = null, epoch = 0, busy = false, staged = false, destroyed = false;
  let original = null, filename = 'dabbl8-projects.d8bak', timer = null, codecPromise = null;
  const codec = () => codecPromise ??= Promise.resolve().then(getCodec).catch(error => {
    codecPromise = null;
    throw Error('Project backup support is unavailable in this local build. Keep your backup. ' + error.message);
  });
  function stopTimer() { if (timer !== null) unschedule(timer); timer = null; }
  function message(value, bad = false) { status.textContent = value; status.className = bad ? 'status bad' : 'status'; }
  function update() {
    const current = getSession();
    if (current !== connection) {
      connection = current; epoch++; staged = false; stopTimer();
    }
    const available = Boolean(current?.connected && current?.projects?.available);
    backup.disabled = busy || staged || !available;
    restore.disabled = busy || staged || !available || !original;
    file.disabled = busy || staged;
    cancel.disabled = busy || !staged;
    keep.disabled = !original;
    if (!busy && !staged && !available)
      message(current?.projectAvailability?.reason || 'Connect compatible Dabbl8 firmware to back up projects.');
  }
  function currentSession(session, generation) {
    if (destroyed || generation !== epoch || session !== getSession() || !session?.connected)
      throw Error('Connection changed. Keep your backup and reconnect before continuing.');
  }
  async function action(task) {
    if (busy || destroyed) return;
    const session = getSession(); update(); const generation = epoch;
    busy = true; onBusy(true); update();
    try { await task(session, generation); }
    catch (error) {
      if (!destroyed && generation === epoch) {
        staged = Boolean(session?.projects?.active); message(error.message, true);
      }
    } finally { busy = false; onBusy(false); if (!destroyed) update(); }
  }
  async function backUp() {
    if (backup.disabled) return;
    await action(async (session, generation) => {
      const c = await codec(); currentSession(session, generation);
      message('Reading projects. Keep playback stopped.');
      const set = await session.projects.exportSet(); currentSession(session, generation);
      const bytes = await createOwnArchive(set, c); currentSession(session, generation);
      saveFile(bytes, 'dabbl8-projects.d8bak');
      message('Project backup verified and saved. Keep it together with any separately backed-up samples and presets.');
    });
  }
  file.addEventListener('change', () => {
    const chosen = file.files?.[0]; if (!chosen || file.disabled) return;
    void action(async () => {
      if (chosen.size > ARCHIVE_LIMIT) throw Error('Choose a project backup no larger than 256 KiB.');
      const bytes = new Uint8Array(await chosen.arrayBuffer());
      if (destroyed) return;
      original = bytes; filename = chosen.name; selected.textContent = chosen.name;
      await validateOwnArchive(bytes, await codec());
      message('Backup validated. Connect the device, stop playback, then prepare restore.');
    });
  });
  async function prepare() {
    if (restore.disabled) return;
    await action(async (session, generation) => {
      const set = await validateOwnArchive(original, await codec()); currentSession(session, generation);
      await session.projects.stage(set); currentSession(session, generation);
      staged = true;
      message('Review the request on the FM-1, then press and release SAVE there. Your backup remains available here.');
      timer = schedule(() => { timer = null; void poll(session, generation); });
    });
  }
  async function poll(session, generation) {
    if (!staged || destroyed || generation !== epoch) return;
    await action(async () => {
      currentSession(session, generation);
      const result = await session.projects.status(); currentSession(session, generation);
      if (result.phase === 5) {
        message('Restoring and verifying the complete project set. Keep the device connected.');
        await session.projects.commit(); currentSession(session, generation);
        staged = false;
        await onRestored(); currentSession(session, generation);
        message('All restored projects, autosave and the current project were read back and verified.');
      } else if (result.phase === 3 || result.phase === 4) {
        message(result.phase === 4 ? 'Release SAVE on the FM-1 to confirm restore.' : 'Review the FM-1 screen, then press and release SAVE on the device.');
        timer = schedule(() => { timer = null; void poll(session, generation); });
      } else throw Error('Restore stopped before completion. Keep the backup and verify the device before retrying.');
    });
  }
  async function cancelRestore() {
    if (cancel.disabled) return;
    stopTimer();
    await action(async (session, generation) => {
      currentSession(session, generation); await session.projects.cancel(); staged = false;
      message('Restore cancelled. Keep the selected backup; verify the device if any replacement had started.');
    });
  }
  update();
  return { update, destroy() { destroyed = true; epoch++; stopTimer(); } };
}
