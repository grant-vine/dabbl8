# Shared-pool preflight evidence

Base: `3174e63e190e44f9905ceddc9911e90696b5009a`. Candidate source hashes and exact build/run commands are in [manifest.json](manifest.json). Generated host inputs were reused from the existing local integration and copied byte-identically to the pristine control archive. No target build or device operation was performed by this lane.

The pristine control has only the test callback hook and new test added. Its production storage/runtime sources remain at the base commit. Control `--cost` exits 0. Control `--negative` exits 1 with 692 expected failed assertions: 679 mutation-epoch boundary refusals, eight changed-already-copied-record cases, two late foreign/future obsolete-spare cases, and three preflight reentry cases. These are deliberately hostile callback scenarios, including violations of the existing no-reentry/concurrent-writer contract; they are not 692 independent deployed-device bugs.

Candidate optimized and inherited sanitizer modes each pass 326,063 checks, including all 274 catalog callback read failures, each callback boundary with late transport/backend/epoch/owner/canvas changes, late header/payload mutations, foreign/future obsolete records and preflight reentry. Torn obsolete payload is still permitted under the existing shared-spare recovery policy, with the selected catalog unchanged.

Seven relevant existing suites pass in both modes: runtime pool (240,366), native menu (4,327), guarded driver (139,254), autosave session (172,202), arrangement (611,130), competing workspace (199), and connected migration (103,314,484). Counts include low-level virtual NOR bounded-access assertions and are not independent use cases. Raw logs are retained. Sanitizers retain upstream DSP exclusions listed in the manifest.

| Fixture | Catalog callbacks, control → candidate | Catalog backend bytes | Autosave snapshot callbacks | Snapshot backend bytes |
| --- | ---: | ---: | ---: | ---: |
| Minimal, one object | 63 → 63 | 20,117 → 20,117 | not present | not present |
| Minimal, four current + obsolete spare | 679 → 274 | 192,171 → 92,706 | 815 → 410 | 231,925 → 132,460 |
| Maximum, four current + obsolete spare | 804 → 324 | 224,245 → 108,190 | 965 → 485 | 270,635 → 154,580 |

Minimum encoded file is 6,599 bytes; maximum fixture is 7,705 bytes. The maximum fixture's fourth-project references are explicitly remapped only in test-owned state to the selected three-slot context. Committed fixtures are unchanged. Full catalog callback traffic falls about 51.8%; full snapshot traffic falls about 42.9%. These are backend byte counts, not CPU, latency or audio deadline improvements.

Preflight retains initial and final complete inventories, CRC/ref checks on copied bodies, selected metadata comparison, epoch/transport/workspace checks and a scoped recursive-preflight exclusion. No trusted index survives the synchronous call. Subsequent operation-specific reads/writes and shipping quarantine remain unchanged. One local second index (68 bytes in the scalar C layout), a four-byte epoch and one persistent byte of exclusion state are added; vendor-linked frame size, actual RAM delta and hardware high-water remain pending.

[Independent source/evidence review](independent-review.json) found no blocker under the documented synchronous backend contract. It pins source before one later comment-only wording clarification. The tested and final preprocessed host fixture are byte-identical; the manifest preserves both source hashes. The reviewer did not build or run tests.
