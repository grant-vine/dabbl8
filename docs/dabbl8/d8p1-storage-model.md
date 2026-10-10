# D8-005: bounded D8P1 storage model

Status: arithmetic and format proposal only. No firmware storage code, sample region, loader or flash boundary is changed. The map decision remains open; #7 must not close until the selected map and its remaining gates are recorded. Reproduce the arithmetic with `python3 tools/dabbl8_storage_model.py`; [machine-readable results](evidence/2026-10-09-storage-model/model.json).

## Worst-case file

Compression is not assumed. D8P1 schema 1 has a 32-byte header and at most eight chunks, each with an eight-byte header. Values are explicit bytes/LE integers, never native runtime structs. The full bounded payload is:

| Chunk | Bound and representation | Maximum bytes |
|---|---|---:|
| Globals | 27 signed LE16 values | 54 |
| Tracks | 8 × (99 biased byte params + engine/preset bytes + 64 nine-byte packed steps) | 5,416 |
| Patches | 8 × 128-byte FM6 patches, bytes 0–127 | 1,024 |
| Motion | D8M1 header plus 64 five-byte records | 328 |
| Chain | Count plus 16 (scene reference, repeat) pairs | 33 |
| Metadata | 12-byte ASCII name, selected track, PHYS schema, flags, reserved zero | 16 |
| Banks | Count plus four (12-byte name + eight project/track reference pairs) | 113 |
| Scenes | Count plus 16 (12-byte name + bank + apply flags + mute mask + eight level/pan/transpose triples) | 625 |
| Framing | 32-byte file header + eight eight-byte chunk headers | 96 |
| **Total** | All maxima present simultaneously | **7,705** |

Banks reference patterns in stored projects; scenes reference banks and carry bounded mix/transpose state. They do not duplicate whole projects or engine states. The proposed bank count is four and scene count sixteen. These are explicit proposed first-release resource bounds, not claims of implemented arrangement features. #15 must test their actual workflow and worst case without silently enlarging the format. Levels 0–127, pan -64–63, transpose -24–24; scene apply bits select mute, mix and transpose (other bits refused). Names stop at zero and are padded; nonprintable characters refused. Source project indices 0–3 and source track indices 0–7 require available storage capabilities before play; a two-project map cannot silently alias references 2/3.

Motion retains 64 shared records across 512 possible addresses. Step records retain the validated FUN9 musical fields, with no change to historical FUN formats. Every parameter is in -64–127 before biasing; range failure is refused, never truncated. Unknown schema/IDs cannot grant editing.

## Header, chunks and validation

The 32-byte file header proposal is: bytes 0–3 ASCII D8P1; 4–5 LE16 schema 1; 6–7 LE16 header size 32; 8–11 LE32 total size; 12–15 LE32 CRC; bytes 16–20 track count (8), steps (64), parameter count (99), globals (27), chunk count (at most 8); bytes 21–22 parameter/engine ID schemas (1/1); bytes 23–31 zero reserved.

File CRC is standard reflected CRC-32 (polynomial 0xEDB88320, initial/final XOR 0xFFFFFFFF), over the entire file with its CRC field zeroed. Each chunk header is LE16 type, LE16 length, LE32 payload CRC using the same algorithm. Known type IDs 1–8 follow the table order; bit 15 means required. Mandatory chunks are globals, tracks, patches, motion, chain and metadata. Bank and scene chunks are optional, with count-zero defaults when absent. Enforce unique base type IDs and canonical increasing order on writes. No old magic or format ID is reused.

Reject a wrong magic/version/header size, mismatched exact file length, bad CRC, nonzero reserved bytes, missing/duplicate mandatory chunks, invalid counts or IDs, and any chunk beyond its bound or remaining file bytes. Check lengths by subtraction before addition to avoid overflow. The total parser bound is 7,936 bytes, independently of any claimed length. Known variable chunks must have exact count-derived lengths, and all references/field ranges must validate before adopting state.

Unknown required chunks are unsupported and refused. Unknown optional chunks may be bounded/CRC-checked for inspection, but make the import read-only unless the implementation retains their raw bytes unchanged. A writer must never silently drop them. Eight is the total chunk-count bound, including unknown chunks; no unbounded trailing extensions. Added fields or limits need a deliberate new schema and maximum recalculation.

This section is a specification for later codec/storage work, not an implemented verifier. The arithmetic model proves resource totals; it does not prove a parser's safety or atomic writes. Those require #13's real codec, malformed-input and power-cut checks.

## Flash and autosave are separate

The current sector is 4,096 bytes with a 256-byte payload offset: single-sector maximum 3,840 bytes. Eight-track steps alone are 4,608 bytes. A two-sector copy with one 256-byte header region gives 7,936 bytes; D8P1 leaves 231 bytes spare. A/B copies consume four sectors (16 KiB) per project. Full independent A/B autosave also needs four sectors, not the existing two. Atomic publication must commit the full-copy header last after validating every sector; do not confuse data-object A/B with firmware recovery.

Existing protected regions and all addresses remain unchanged in code: project area 0x97000–0x9EFFF, FM6 copies 0x9F000/0xFE000, user samples 0xA0000–0xDBFFF, presets 0xDC000–0xDFFFF, OTA 0xE0000–0xE4FFF, autosave 0xE5000–0xE6FFF, settings 0xFC000–0xFDFFF. Unmapped addresses and the stock update margin are not assumed available.

| Alternative | Project allocation | Samples | Autosave consequence |
|---|---:|---|---|
| Two projects, preserve three samples | 32 KiB, fits current project area | All three 80 KiB slots retained | Another approved 8 KiB needed in addition to current autosave area; no safe allocation is established |
| Four projects, reduce to two samples | 64 KiB | Keep the existing second/third physical sample areas | Retiring one 80 KiB sample area can supply extra project copies and full autosave; internal remap/migration is required |

A concrete four-project candidate for review uses projects 0/1 in 0x97000–0x9EFFF (16 KiB each), project 2 at 0xA0000–0xA3FFF, project 3 at 0xA4000–0xA7FFF and new A/B autosave at 0xA8000–0xABFFF. 0xAC000–0xB3FFF remains unallocated. The two remaining sample areas stay physically at 0xB4000–0xC7FFF and 0xC8000–0xDBFFF; old sample slot numbering cannot be reinterpreted without a verified migration. Original autosave sectors remain reserved for legacy import. Outer firmware, boot, loader, OTA, settings and preset addresses are untouched by this candidate.

**No map is selected or approved yet.** The initial planning decision asks to preserve sample regions and present this tradeoff before implementation. Three-sample/two-project support currently has an unresolved autosave allocation. Four-project/two-sample support has a concrete candidate, but retires sample capacity and needs backup/migration evidence and a deliberate map decision. The user's prohibition on flash-boundary changes remains in force. Do not implement a remap under an inferred approval.

## Retained RAM and live state

Baseline .noinit capacity is 15,696 bytes. Current use 14,796 includes four 3,648-byte project caches and 204 bytes of other retained state. A maximum D8P1 file aligned to four bytes occupies 7,708 bytes:

- One cache plus existing other state: 7,912 bytes, 7,784 spare.
- Two caches plus existing other state: 15,620 bytes, only 76 spare before new metadata.
- Four caches plus existing other state: 31,036 bytes, exceeds capacity by 15,340.

Recommend a single active retained cache with other slots loaded from flash in the main loop. Do not double every retained object or borrow stack/boot areas. Exact cache metadata and target linker evidence must be established before implementation. General RAM, engine pool and stack usage are separate from this retained-file calculation; eight-track runtime/chain/engine arrays still need #12 measurement. The current verified four-track capability build uses 92,116/98,304 general RAM and 331,332/344,064 pool bytes. D8P1 model bytes are not evidence of target runtime fit.
