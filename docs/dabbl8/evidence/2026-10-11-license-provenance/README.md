# License provenance snapshot — 2026-10-11

`inventory.py` reads immutable Git blobs from integrated main `b2466bf9bc1c675ed8b8f667d37af8f5e4e4b377` and the unchanged Felucca v1.1.5 baseline. Run it from a checkout containing both commits; it never reads private local archives, external SDK contents or untracked files. It writes only the adjacent inventory JSON.

Result: 296 code files, 114 new and 45 modified relative to the baseline; every scanned file declares SPDX. Every baseline SPDX/copyright line in each scanned file's first 8 KiB is retained. The complete first notice blocks of the three MIT/Apache engine ports are byte-preserved. LICENSE and all seven LICENSES texts retain their baseline Git blobs; sixteen notice/attribution/asset pins are recorded. No donor rights or release qualification are inferred.

The code-file scope is firmware/tools/tests/web C, headers, Python, shell, JavaScript, HTML and CSS, plus build.sh; it excludes generated headers and YAML workflows. The source snapshot precedes this documentation-only PR and later firmware work. The scanner is evidence tooling, not a firmware/build input or a complete corresponding-source/release bundle verifier.

Validation: inventory regenerated after updating to the immutable integrated source; all notice paths exist and SDK hash values match tools/build.py. LICENSING.md retains all preceding bytes with an appended Dabbl8 ledger link. `git diff --check` passes. No firmware, loader, golden, budget or package input changed. Hardware and release qualification remain unperformed.
