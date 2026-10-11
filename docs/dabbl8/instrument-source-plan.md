# Source-derived instrument pool proposal

`tools/dabbl8_instrument_plan.py` advances the host-only migration gate by deriving a native project pool from a complete seventeen-role post-boot capture. It uses the existing C historical importer, native canonical validator, real shared-pool initializer, and archive adapter for whole-set native readback. It does not activate storage, adopt current RAM, install firmware, access a device, or release persistence quarantine.

The input is the `verified` directory produced by `tools/dabbl8_instrument_capture.py` in full mode. The frozen complete-capture consumer retains the exact manifest and all seventeen raw/current roles before any conversion. Every role must satisfy its fixed identity, map, extent, CRC and SHA-256. Raw settings, both preset pairs, the fragmented FM6 pair, all three complete sample allocations, the fourth original manual pair, and all five current logical roles remain separately retained. They are not replaced by normalized project conversions.

## Explicit source choices

The original project pairs 0–3 and persisted autosave pair use frozen Felucca headers and FUN1–FUN9 frames. Every nonerased sector must have the expected physical slot/type, bounded body, valid header/body CRC and exact frozen frame extent/checksum. A torn or foreign nonerased sector refuses the whole proposal even if another generation survives. Completely erased pairs establish absence. Modular generation ordering permits wrap; equal and half-range generations refuse as ambiguous. Both raw copies remain retained. The actual C importer validates the selected generation of each nonempty pair; older generations receive framing/checksum validation, not semantic import.

Only the original manual identities 0–2 enter the proposed native pool. Original slot 3 is independently converted and validated in its original four-slot reference context for archival review, but receives no native identity or alias. A surviving manual project or persisted autosave referencing that fourth identity refuses the proposal. Missing manual references also refuse. No references are silently redirected or deleted.

Autosave selection is mandatory:

- `--autosave-source persisted` converts the selected persisted legacy autosave. An erased pair refuses; it never implicitly chooses live state.
- `--autosave-source current` uses the retained current native role 12, which must pass the C validator and canonicalize to exactly its original bytes under the available three-slot reference context. Any nonempty persisted pair still receives conservative recovery and selected-generation archival validation. Current settings/preset banks are retained separately and are not adopted by this proposal.

Historical conversion may normalize inactive/reserved bytes and migrate engines or remove incompatible automation. The actual converter metadata is retained in `plan-report.json`; original raw bytes remain unchanged. These changes require review.

## Use

Build the existing host converter, initializer and project archive adapter against the reviewed source and pinned generated inputs. Supply those explicit executables; their SHA-256 identities are recorded, not authenticated by this tool.

```sh
python3 tools/dabbl8_instrument_plan.py CAPTURE_VERIFIED NEW_PRIVATE_DIRECTORY \
  --autosave-source persisted \
  --converter build/host/dabbl8_project_convert \
  --initializer build/host/dabbl8_pool_initialize \
  --adapter build/host/dabbl8_project_archive
```

The output directory must be new. It contains exact originals, extracted selected source records, private intermediate evidence and `plan-report.json`. Only complete successful whole-set conversion/readback publishes `object-N.d8p` and `proposed-native-pool.bin`. The report binds the input manifest, each selected source, actual conversion metadata, converted object hashes, available reference mask, explicit autosave choice, executable identities and exact pool readback. Failure leaves originals and refusal evidence; it publishes no usable derived pool or root-level project subset. Oversize/nonregular inputs remain at the source and are explicitly reported as unretained.

The resulting pool can be passed to the separate `dabbl8_instrument_migrate.py` host simulator with `--operation apply --reviewed-plan` and an explicit one-MiB simulated current image. The simulator still requires all twelve original raw allocations to match that image, validates the actual pool, checks complete resulting bytes, and permanently retains quarantine. Exact raw restoration preserves the captured physical allocations but does not adopt unsaved current logical roles. Neither operation is device transport.

## Verification and limits

Focused tests use the real C converter/canonical validator, real initializer, real pool inventory/readback adapter and mapped APPLY/RESTORE simulator. They cover all frozen FUN frame families, sparse presence, explicit current autosave after proven erased persisted autosave, generation rollover, exact originals restoration, damaged/foreign/torn/ambiguous sources, retained-input tampering, helper protocol type refusals, and fourth-slot reference refusal.

The immutable FUN6–FUN9 test songs contain fourth-slot references and conservatively refuse native survival. Separate temporary synthetic variants remove only those test song chains and recompute their historical checksums, enabling complete conversion/apply/restore tests. Immutable fixtures and golden audio hashes are never changed. Archival fourth-slot references remain independently validated under their original identity.

```sh
python3 tests/instrument_plan_test.py CONVERTER INITIALIZER ARCHIVE_ADAPTER EXECUTOR
```

An accepted proposal proves encoded source derivation and actual C whole-set pool readback using the supplied reviewed helpers. It does not prove source-to-runtime musical equivalence, physical capture/recovery, pre-boot preservation, current RAM adoption, live device write safety or production ownership. Those remain separate release and migration gates. No new firmware capability or automatic bootstrap is enabled.

The initial focused validation passed 192 checks with optimized helpers and 192 with sanitizer helpers. The separately inserted normal and sanitizer runner invocations also passed those checks; runner syntax was checked. This was focused verification, not a new full-suite or target build. The converter retains the repository's inherited DSP exclusions (`signed-integer-overflow`, `shift`, `bounds`, `object-size`, `pointer-overflow`); the initializer, pool archive adapter and executor use strict address/undefined sanitizers. Leak detection was disabled in the sanitizer subprocess environment. Physical device and musical adoption checks remain absent.
