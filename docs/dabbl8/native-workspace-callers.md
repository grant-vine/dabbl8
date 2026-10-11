# Native workspace ownership refusal

Native main-loop runtime and pool APIs use checked canvas/staging accessors. An active migration lifetime returns BUSY without reading a backend or changing caller outputs. The ownership check repeats after LCD synchronization, before switching union members. Frontend operations and raw/full capture BEGIN share the refusal policy.

Unchecked legacy helpers remain invariant assertions. They are not an operational BUSY contract; historical and one-shot UI callers must be converted before migration activation. Source `8a25353` does not enable the migration coordinator or application storage writes.

[Exact local verification](evidence/2026-10-11-native-workspace-callers/README.md) includes actual held/late-owner fixtures, complete upstream suite, unchanged golden hashes and fresh pinned4/8 links. Native static timing failures and physical qualification remain open.
