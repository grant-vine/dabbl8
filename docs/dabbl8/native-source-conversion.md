# Legacy conversion in owned source-verification staging

`d8p1_legacy_stage` runs the actual frozen FUN importer and native normalization in one caller-owned state. It publishes a report only after validation; scratch may change on failure and must never be treated as active state. Input and report may not alias scratch or each other. The existing `d8p1_legacy_convert` keeps its transactional destination publication and two-buffer alias/refusal contract.

This makes it possible to retain the full migration plan beside source conversion: the existing 58,432-byte workspace holds one state plus input/output wire and the 40,960-byte plan. Tests decode frozen originals, reuse the wire for canonical encoding and verify the entire plan remains unchanged. Actual source-to-plan comparison and the production migration coordinator are separate required work.

[Exact frozen evidence](evidence/2026-10-11-native-source-conversion/README.md) includes strict focused checks, full suite, hashes, preserved timing failures and explicit coverage limitations. This API is not a write authorization or a completed source-equivalence proof.
