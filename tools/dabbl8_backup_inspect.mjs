// SPDX-License-Identifier: GPL-3.0-only
// Read-only host adapter to the actual companion archive validator. No device API.
import { readFileSync, statSync } from 'node:fs';
import { readBackup, BACKUP_IDS } from '../web/fm1backup.js';
try {
  if (process.argv.length !== 3) throw new Error('One backup path is required');
  if (statSync(process.argv[2]).size > 2 * 1024 * 1024) throw new Error('Backup exceeds host migration bounds');
  const archive = readBackup(readFileSync(process.argv[2], 'utf8'));
  if (archive.objects.length !== BACKUP_IDS.length) throw new Error('Migration requires a complete current backup including every preset and sample object');
  process.stdout.write(JSON.stringify({ format: 'dabbl8-backup-inspection', version: 1,
    device_writes: false, objects: archive.objects.map(o => ({ id: o.id, size: o.size,
      crc: o.crc, base64: Buffer.from(o.bytes).toString('base64') })) }) + '\n');
} catch (error) {
  process.stderr.write(String(error.message) + '\n'); process.exitCode = 1;
}
