<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Private resource diagnostic build

`--resource-diagnostic` explicitly adds pool and stack observation capability
to a private Dabbl8 eight-track timing build. It requires `--dabbl8` and
`--timing-diagnostic`; release mode, four-track/upstream selection and explicit
CDC-off configuration are refused before dependency, SDK, container or output
work. Existing private profile/environment validation also applies. This
option does not enable owner initialization or authorize installation.

For an isolated application, after providing already available build inputs:

```sh
python3 tools/build.py --dabbl8 0.1 --timing-diagnostic \
  --resource-diagnostic --app-only
```

The option also applies to the existing private timing-kit route when
`--app-only` is omitted. That route includes the ordinary loader/package
orchestration; the resource option adds no loader flags or loader behavior.
Artifact creation is separate from permission to install, reset or capture a
connected instrument. Fresh isolated run directories preserve ordinary products
and earlier diagnostic runs.

Application C compilation receives both `D8_HEAVY_POOL_DIAGNOSTIC=1` and
`D8_STACK_WATERMARK_DIAGNOSTIC=1`. The actual application `crt0.S` invocation
receives only the stack flag. Omitting the option adds neither flag, and
default startup compiler arguments stay unchanged. The stack helper's existing
link-time capability binding rejects a C-on/startup-off mismatch.

Both `app-only-manifest.json` and `diagnostic-manifest.json` record:

| Field | Meaning when enabled |
| --- | --- |
| `resource_diagnostic` | Explicit build capability, `true` |
| `resource_report_schema` | `D8R1` wire format |
| `resource_collection_initially_enabled` | `false`; pool collection requires explicit start |
| `resource_engine_profile` | Actual profile ID: Full 0, Core 1, Breaks 2 |
| `resource_engine_mask` | Effective compiled engine availability, including Full switches |
| `resource_uac_enabled` | Actual application UAC default/override |
| `startup_flags` | Exact forwarded application startup flag list |
| `resource_pool_scope` | Observed live-owned occupancy since explicit resource start |
| `resource_stack_scope` | Surviving written depth since application startup |
| `resource_scope` | Paused observations; no workload, timing, hardware or complete stack proof |

The existing profile label, compiler flags, product/version and artifact fields
remain available for capture provenance. Disabled manifests explicitly report
`resource_diagnostic: false`, no resource schema/collection capability and an
empty startup flag list. Readout does not claim a true stack-pointer peak:
pattern collisions, restored pattern contents and unwritten frames can hide
deeper use. Collection and hardware qualification are separate from build
capability; instrumentation changes load.

Inspect bounded local plans before executing them:

```sh
python3 ci/local_checks.py --paths tools/build.py \
  tests/resource_diagnostic_build_test.py --check resource-build
python3 ci/local_checks.py --paths firmware/src/resource_diagnostic_console.h \
  tools/dabbl8_resource_capture.py --check resource-capture
```

Use the existing isolated Python runtime when required by other build test
imports. The resource build test invokes actual parser/build/manifest functions
with stopped compiler adapters or temporary mock tool/package fixtures. It
retains compiler arguments, copied manifest fixtures and refusal diagnostics.
The actual C engine-mask header is preprocessed for profile consistency.
These host checks execute no real firmware/package build, dependency download,
SDK read, container or device operation. Other local recipes cover the actual
console fixture, scripted capture transport, heavy-pool fixture and host-only
stack runner. Target assembly/package qualification remains a separate step.
