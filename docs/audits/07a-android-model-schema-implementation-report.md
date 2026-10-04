# WP-07A — Android Model & Schema Correction

## Status

**Implemented. Source-verified. Runtime Gradle verification pending after the latest changes.**

WP-07A applies the Android model/data-layer corrections required by the approved WP-07 audit and SmartFlow System Contract v1.1.

## Changes Implemented

### 1. Canonical Android domain configuration model

`DeviceConfig` now uses the contract's domain names:

- `pumpStartLevelPct`
- `pumpStopLevelPct`
- `dryRunThresholdLpm`
- `maxPumpRuntimeMin`

Defaults match the current firmware configuration baseline:

- start: 30%
- stop: 100%
- dry-run: 1.0 L/min
- maximum runtime: 120 min

### 2. Dedicated Firebase settings DTO

Added `DeviceConfigDto` to keep RTDB field names separate from the Android domain model.

Canonical Firebase fields:

- `pump_start_level_pct`
- `pump_stop_level_pct`
- `dry_run_threshold_lpm`
- `max_pump_runtime_min`

The repository now reads `DeviceConfigDto` from RTDB and converts it to `DeviceConfig`. Writes convert `DeviceConfig` back to `DeviceConfigDto`.

This prevents the previous invalid direct serialization of the legacy `DeviceConfig` shape.

### 3. Reported fault code propagation

Added `last_fault_code` mapping through:

`ShadowReportedDto → ShadowReported → DashboardUiState`

The human-readable `last_fault_message` remains available as explanatory text.

### 4. Safer desired-state defaults

Android desired-state DTO/domain defaults now use:

- `mode = MANUAL`
- `bypass_level_sensor = false`
- `bypass_flow_sensor = false`

These are model defaults only. They do not automatically overwrite an existing device shadow.

### 5. Minimal presentation-layer compatibility updates

Updated current dashboard/config references to the canonical configuration field names so the domain model change does not leave stale references.

The configuration sheet preserves the existing device's `pumpStopLevelPct` while cross-field editing/validation remains assigned to WP-07C.

## Files Changed

- `app/src/main/java/com/smartflow/domain/Models.kt`
- `app/src/main/java/com/smartflow/data/dto/FirebaseModels.kt`
- `app/src/main/java/com/smartflow/data/repository/FirebaseDeviceRepository.kt`
- `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt`
- `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt`
- `app/src/main/java/com/smartflow/presentation/components/ConfigBottomSheet.kt`

## Verification Performed

Source-level verification confirmed:

- canonical RTDB settings field mappings exist;
- repository reads through `DeviceConfigDto`;
- repository writes through `DeviceConfig.toDto()`;
- `last_fault_code` is mapped into the Android state chain;
- safe desired defaults are present;
- stale legacy `DeviceConfig` property references were removed from the modified Android files.

The branch was advanced from the WP-07 audit commit to the implementation head. The comparison also contains unrelated repository changes made outside WP-07A; those were not modified or interpreted as part of this package.

## Verification Boundary

A fresh `./gradlew.bat compileDebugSources` run has not been executed after the final WP-07A edits in this agent session. The earlier WP-06E Android compilation passed before WP-07A changes.

WP-07A is therefore **source-verified but not yet runtime-verified**.

## Next Package

**WP-07B — Android Command & State Derivation**

Focus:

- emergency-stop precedence,
- fault/interlock representation,
- MAX_RUNTIME semantics,
- command lifecycle semantics,
- reported-vs-desired reconciliation.

Visual redesign remains deferred.
