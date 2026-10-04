# WP-07C — Android Client Validation Implementation Report

**Status:** Verified
**Branch:** `docs/smartflow-system-contract`
**Scope:** Android client validation and canonical configuration UI alignment

## Objective

Align the Android client with SmartFlow System Contract v1.1 for device configuration bounds, pump start/stop level controls, dry-run threshold, maximum pump runtime, countdown duration, and reusable client-side validation.

The Android client remains a presentation/request layer. Firebase validation and ESP32 firmware remain the authoritative enforcement layers.

## Changes

### 1. Reusable device configuration validator

Added `app/src/main/java/com/smartflow/domain/DeviceConfigValidator.kt`.

The validator enforces:

| Setting | Valid range / rule |
|---|---|
| Pump start level | 0–100% |
| Pump stop level | 0–100% |
| Start/stop relationship | stop > start |
| Dry-run threshold | 0.1–10.0 L/min |
| Maximum pump runtime | 30–120 minutes |
| Countdown duration | 1–120 minutes |

Validation is pure and independent of Compose, Firebase, and firmware code.

### 2. Configuration UI corrected

Updated `app/src/main/java/com/smartflow/presentation/components/ConfigBottomSheet.kt`.

- Pump Start Level expanded from 0–50% to 0–100%.
- Pump Stop Level added as a separate control.
- Stop level must be greater than start level.
- Dry-run threshold corrected to 0.1–10.0 L/min.
- Maximum runtime corrected to 30–120 minutes.
- “Max Overflow” terminology removed from the client UI.
- Save is disabled whenever a field or cross-field validation rule fails.
- Canonical values are written back into the existing `DeviceConfig` model.

### 3. Countdown UI decoupled from maximum runtime

Updated `app/src/main/java/com/smartflow/presentation/components/ControlPanel.kt`.

The countdown control now uses the contract-defined independent range of 1–120 minutes. It no longer derives the countdown maximum from the configurable maximum pump runtime.

This prevents a maximum-runtime configuration such as 30 minutes from incorrectly limiting a valid 120-minute countdown request.

### 4. ViewModel validation

Updated `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt`.

- `startCountdown()` rejects durations outside 1–120 minutes before any Firebase write.
- `updateConfig()` rejects invalid device configuration before any repository write.

These checks improve user feedback but do not replace Firebase Rules or firmware validation.

### 5. Obsolete UI parameter removed

Updated `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt`.

Removed the obsolete `maxRuntimeLimitMins` parameter from `ControlPanel` in both portrait and landscape layouts now that countdown duration is contract-independent.

### 6. Unit tests

Added `app/src/test/java/com/smartflow/domain/DeviceConfigValidatorTest.kt`.

Tests cover default configuration, boundary values, invalid pump levels, stop-level ordering, dry-run bounds, maximum-runtime bounds, and countdown bounds.

## Verification

Executed in the local Android workspace:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

**Verification Results:**
- `compileDebugSources`: `BUILD SUCCESSFUL` (Both portrait and landscape `ControlPanel` invocations compiled cleanly).
- `testDebugUnitTest`: `BUILD SUCCESSFUL` (24/24 unit tests passed across all 3 test suites):
  - `DeviceConfigValidatorTest`: 10/10 passed
  - `DashboardStateReducerTest`: 11/11 passed
  - `CloudClaimCoordinatorTest`: 3/3 passed

## Scope Boundary

No firmware, Firebase Rules, Cloud Functions, or physical hardware behavior was changed by WP-07C.

The package is limited to client-side validation and UI/schema alignment. Physical safety enforcement remains firmware-owned, while backend validation remains defense-in-depth.

## Acceptance Criteria

- [x] Configuration ranges match System Contract v1.1.
- [x] Pump stop threshold is visible and validated against pump start threshold.
- [x] Countdown UI uses the independent 1–120 minute contract range.
- [x] Countdown ViewModel blocks invalid client requests before writing.
- [x] Configuration ViewModel blocks invalid client configuration before writing.
- [x] Validation logic is pure and unit-testable.
- [x] Android compile verified after WP-07C changes.
- [x] Android unit tests verified after WP-07C changes (24/24 tests passed).