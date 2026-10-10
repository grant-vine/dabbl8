# Pre-migration application write quarantine

The eight-track build is **nonpersistent and interim** until a trusted coordinator is implemented. This is a preservation gate for the experimental eight-track build, not a completed persistence implementation or v0.1 release qualification. `NTRK > 4` application writes are refused from cold boot onward. There is no production grant, unlock setter, protocol authorization or permissive test bypass. Recognizing a native pool, binding a catalog, setting `flash_ok`, starting capture or supplying a caller's authorization boolean does not open the actual application write gate. Reads and read-only instrument capture remain available.

The four-track preprocessor path retains its existing behavior. Its target must be compared against actual frozen integration `6f73dba1576ad12bdc3fa30eac1509cd674dc0a5` (application SHA-256 `7e89ae7fd6815748419c60948ac69f0d368f92be8d729608f82218721153ec0b`), independently of the historical audited upstream commit `276f72a4e6ea8a12499a7a6819aadf3165126755`. No loader, OTA handler, flash allocation, golden audio hash or timing threshold is changed by this slice.

## Application coverage

| Actual path | Quarantine result | Preservation/reporting |
|---|---|---|
| `st_save_to`, every legacy object including settings, presets and FM6 | Existing project/autosave refusal or `-10` | Refuses before reading/copying, erasing or programming |
| Hardware `st_erase` / `st_prog`, including native physical adapter callbacks | `-10` inside valid application storage ranges; existing `-8` outside | Refuses before audio silencing, capture invalidation and physical driver callbacks |
| FM6 boot migration from retired bank | Current RAM migration retained; automatic durable marker omitted | Original bank and both generations remain untouched; RAM can still be captured |
| `upf_save` | Existing flash-error result `2`; offline result `3` | Does not claim durable save; inherited caller RAM editing semantics remain |
| Editor sample erase helper | `-10` | Refuses before clearing sample zone counts or live zone extents |
| Editor sample upload/commit | Existing error results | Refuses before transport, sample invalidation, erasure or programming; existing exact already-published no-write END can return success |
| Slice record save | Existing flash-error result `2` | Existing poll reports `SAVE ERROR`, never `SLICES SAVED` for a refused changed record; current manual slice edits remain in RAM |

The inherited eight-track legacy editor family refusal remains in force. Guards on its direct sample helpers cover internal callers too. Settings retain pending/retry error status and do not update their saved mirror. User-preset save retains its existing rollback/error behavior. The application gate intentionally does not change the separate OTA/loader implementation or globally gate low-level flash primitives.

## Test boundaries

The dedicated harness runs actual flash-enabled boot, settings/preset/FM6 storage, sample and slice paths against a simulated NOR, with independent raw fixture injection only. Fixture callbacks are never wired as production write authorization. It checks exact whole-image preservation and zero erase/program callbacks; sample refusal also preserves musical zone metadata. A separately seeded valid native fixture demonstrates that catalog binding/read permission does not grant a physical save. Cold/unbound, failed binding, offline and active read-only capture contexts all remain closed.

The capture tests retain PR86's cold and rotating-autosave fixtures. Former actual application mutation assertions now verify quarantine refusal, unchanged NOR and a still-valid capture. Independent RAM changes, externally injected raw-store changes, faults and transfer resets continue testing capture invalidation. Native component tests that supply their own simulated `st_*` callbacks test a future authorized component and are not proof that the actual application writes are permitted.

This constant-closed gate has no permission transition. Tests do not claim late revocation or a device/hardware safety proof. Runtime sanitizer coverage uses the runner's inherited DSP exclusions; the standalone storage gate has strict sanitizer coverage. Fresh four/eight-track target evidence and the full suite must be recorded after source freeze before considering this slice validated.

## Required next coordinator

A future trusted migration coordinator must be designed and tested separately:

1. Keep application writes quarantined while capturing all originals and current state.
2. Validate a complete retained archive and an explicitly selected, source-derived plan; preserve the original fourth project and shared autosave evidence.
3. Perform bounded migration with cut/readback recovery, verifying complete stored state and its source provenance.
4. Reconcile current RAM, settings, preset/sample and native project ownership; a native magic value or catalog binding is insufficient.
5. Publish an explicit trusted completion decision with precise scope and lifetime, then initialize the intended persistence policy.
6. Revoke permission on failed binding, uncertain readback, interrupted transfer/reset or lost ownership before each actual mutation boundary. Page-level interrupt/callback behavior needs an explicit atomicity contract.

Only that subsequent implementation may introduce an unlock/grant API. The present change must not be presented as final persistence, completed migration, musical/device equivalence or a release-ready permanent read-only instrument.
