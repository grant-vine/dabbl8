# Version 1 read-only instrument capture

This implements a stopped, configured-USB observation protocol and an offline host collector for issue #11. It neither restores stores nor establishes migration ownership. The collector preserves the exact bytes received and publishes a verified manifest only after a complete initial scan, every role checksum, a complete final scan, and matching final descriptors.

These are **post-boot** musical allocations and declared current logical state. They are not a pre-upgrade dump, a complete physical flash image, authenticated device provenance, vendor recovery firmware, or a qualified restore path. Boot-time normalization/writes may already have occurred. Native activation and destructive migration remain gated on independently qualified original retention, restore and authorization.

## Scope and existing memory

Raw-store mode has 12 fixed roles totaling 319,488 bytes. Full-current mode additionally has five current logical roles. Caller-selected raw addresses are refused. Original physical project pairs remain separate from native object identities: the existing native virtual pool concatenates raw roles 0–4.

| ID | Role | Exact physical segments or logical size |
|---|---|---|
| 0–3 | Original project pairs 0–3 | 0x97000, 0x99000, 0x9B000, 0x9D000; 8,192 bytes each |
| 4 | Original persisted autosave pair | 0xE5000; 8,192 bytes |
| 5 | Persisted settings pair | 0xFC000; 8,192 bytes |
| 6–7 | Persisted preset bank pairs | 0xDC000, 0xDE000; 8,192 bytes each |
| 8 | Persisted FM6 pair | 0x9F000 and 0xFE000; 4,096 bytes each, concatenated in that order |
| 9–11 | Complete user sample allocations | 0xA0000, 0xB4000, 0xC8000; 81,920 bytes each |
| 12 | Exact native live D8P1 | Valid runtime encoding, at most 7,936 bytes, selection and name included |
| 13 | Current PER4 settings | 572 bytes, pure serialization of current panel/preferences/favorites merged with saved feature-absent fields |
| 14–15 | Current preset bank mirrors | 3,080 bytes each, including inert/unknown records |
| 16 | Current FM6 preset mirror | 3,728 bytes |

Raw roles include old generations, invalid/torn content, padding, unused sample tails and the original fourth project/autosave allocations without decoding or normalization. Current logical mirrors can differ from persisted raw stores, including after boot migration or a failed FM6 save. Both versions are retained. Raw-store mode deliberately omits unsaved logical state; full-current mode refuses if live native capture is unavailable instead of silently omitting it.

No full-instrument snapshot array is added. Each raw scan request reads/checks at most 256 bytes. Existing hardware `st_read` has 256-byte IRQ-off windows; the outer scan and CRC do not hold interrupts off. Native live capture reuses the existing fenced workspace, then copies a chunk into existing editor sample scratch before replying. PER4 uses the inherited backup scratch after revoking the old backup session; banks stream existing mirrors. No pointer to borrowed native workspace survives between requests.

## Protocol

Frames are `F0 7D 46 4C 78 args F7`. Version-prefixed arguments and replies are strictly seven-bit. Unsigned 32-bit integers use five least-significant-first seven-bit bytes; the final byte must be at most 15. The inherited sample seven-bit pack groups a high-bit mask followed by at most seven low-bit bytes. Maximum chunk is 256 bytes, within the existing 600-byte reply buffer.

Common reply payload: `[version=1, operation, status, token:u32, phase]`. Status: 0 OK, 1 invalid, 2 busy, 3 stale token, 4 changed/interrupted, 5 read failure. Phases: 1 initial scan, 2 manifest/data ready, 3 final scan, 4 complete. Operation-specific suffixes follow the nine-byte common payload.

| Operation | Request arguments after version/op | Successful reply suffix |
|---|---|---|
| 0 CAPS | none | map version 1, raw count 12, full count 17, max chunk `[0,2]`, read-only flags 1 |
| 1 BEGIN | mode 0 raw-store or 1 full-current | none; new token and phase 1 |
| 2 SCAN | token | next raw role, offset:u32; exactly one bounded chunk per request |
| 3 DESCRIPTOR | token, role | role, length:u32, CRC32:u32, address0:u32, size0:u32, address1:u32, size1:u32 |
| 4 GET | token, role, offset:u32, count:two seven-bit bytes | echoed role/offset/count, packed bytes |
| 5 END | token | phase 3; repeat SCAN until phase 4, then re-read all descriptors |
| 6 ABORT | token | none; session revoked |

The fixed-role scans each take 1,248 requests. Descriptor physical fields are zero for current logical roles. The INFO `C8` version/command extension only advertises this read-only family; inherited backup/write flags remain zero on eight tracks.

## Coherence and interruption

Attempted validated storage writes/erases, direct editor sample writes/erases, app OTA writes and recognized update requests invalidate the session. A sticky changed flag prevents mutation-and-revert or epoch rollover from reviving it. USB reset, configuration loss/reconfiguration, transport start, canvas activity and 15 seconds without a valid session request also refuse continuation. Any competing ordinary editor transaction revokes capture. Configuration/reset observation requires a configured USB connection but does not authenticate the incoming editor transport; DIN is not qualified.

Every current role is fingerprinted at BEGIN and revalidated after both raw scans; each current-role GET revalidates its entire role before copying. Live selection/name/preferences changes refuse. Raw scan consistency additionally depends on all known write paths using the mutation observer. The collector verifies every assembled file CRC and the final manifest, rejecting missing/reordered/replayed/truncated/high-bit/mutated responses. CRC32 detects ordinary corruption; it is not a cryptographic proof against deliberate collisions or concurrent hostile mutation. SHA-256 records host output identity after transfer, not a firmware-supplied authenticated signature.

## Host collector and limits

`tools/dabbl8_instrument_capture.py --adapter PATH --mode full NEW_PRIVATE_DIRECTORY` uses an explicitly supplied persistent transport executable: one full SysEx frame as space-separated two-digit hex bytes per input line, one response frame per output line; diagnostics go to stderr. Its default adapter argument is `--bridge`. It introduces no MIDI library, opens no device itself and makes no restore/write request.

Existing destinations refuse. Exact response/request transcripts and received original files, including partial originals after refusal, remain available. Only complete verified captures get a `verified/manifest.json` and exact verified copies. Receipts expressly deny physical recovery, migration/ownership authorization and pre-boot provenance. Preserve these archives privately; settings/project/preset/sample data can be personal and must not be committed.

The actual editor/USB/native capture and storage observer are exercised against a virtual NOR test bridge, with independent physical-address byte oracles and current-role references. This validates implementation behavior on the host, not device timing, physical USB delivery, power-loss recovery, loader compatibility or safe native production activation. Target memory/direct-stack evidence is recorded separately; host timing is not a target deadline qualification.
