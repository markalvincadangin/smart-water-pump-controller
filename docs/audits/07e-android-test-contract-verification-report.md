# WP-07E — Android Test Suite & Final Contract Verification

**Status:** Implemented; final local verification pending
**Branch:** `docs/smartflow-system-contract`
**Purpose:** Close the WP-07 Android contract implementation with focused DTO mapping tests and a final source-level verification boundary.

## Scope

WP-07E verifies the Android-side contract work completed in WP-07A through WP-07D:

- Firebase DTO/domain mappings
- canonical settings field names
- reported `last_fault_code`
- safe desired-state defaults
- configuration validation
- command/state derivation
- FCM token authority
- clean Android compilation
- complete Android unit-test suite

## DTO Verification

Added:

`app/src/test/java/com/smartflow/data/dto/FirebaseModelsTest.kt`

The test suite verifies:

1. `DeviceConfig → DeviceConfigDto → DeviceConfig` preserves canonical values.
2. `DeviceConfigDto` uses the canonical RTDB property names:
   - `pump_start_level_pct`
   - `pump_stop_level_pct`
   - `dry_run_threshold_lpm`
   - `max_pump_runtime_min`
3. `ShadowReportedDto` maps `last_fault_code` into the domain model.
4. `last_fault_code` uses the canonical Firebase property name on both getter and setter mappings.
5. `ShadowDesiredDto` defaults to `MANUAL` with both sensor bypasses disabled.
6. `ShadowDesired` and `ShadowReportedDto` preserve their contract fields through domain mapping.
7. `DeviceShadowDto` inherits the safe desired/reported defaults.

Because the unit tests run without a live Firebase backend, the Firebase naming checks verify the explicit `@PropertyName` serialization/deserialization contract while the conversion tests verify application-level mapping.

## WP-07A–D Final Source Status

| Package | Status |
|---|---|
| WP-07A — Android model/schema correction | Implemented and locally verified |
| WP-07B — Command/state derivation | Implemented and locally verified |
| WP-07C — Android client validation | Implemented and locally verified |
| WP-07D — FCM token authority cleanup | Implemented and locally verified |
| WP-07E — DTO tests/final verification | Implemented; verification pending |

## Required Verification

Run from the Android project root:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

Expected result:

- clean debug compilation
- all existing WP-07A–D tests still passing
- all new `FirebaseModelsTest` tests passing

## Acceptance Criteria

- [x] DeviceConfig domain/DTO conversion is covered by unit tests.
- [x] Canonical RTDB settings property names are explicitly tested.
- [x] `last_fault_code` domain mapping is tested.
- [x] `last_fault_code` Firebase getter/setter annotations are tested.
- [x] Safe desired-state defaults are tested.
- [x] Existing WP-07B command/state tests remain part of the full suite.
- [x] Existing WP-07C configuration validation tests remain part of the full suite.
- [x] WP-07D FCM source cleanup has been completed.
- [ ] Final Android compilation after WP-07E changes.
- [ ] Final complete Android unit-test rerun after WP-07E changes.
- [ ] WP-07 series formally closed after those verification results.

## Final Verification Boundary

Even after the Android test suite passes, this package does not establish:

- real Firebase Emulator authorization behavior
- live FCM delivery
- Android foreground/background/terminated notification delivery
- physical ESP32/pump response
- physical E-stop or sensor safety behavior
- production Firebase deployment state

Those remain independent system and commissioning verification activities.