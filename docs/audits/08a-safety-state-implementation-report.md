# WP-08A — Safety-Critical Interaction & State Presentation Implementation Report

**Status:** Codebase & Unit Tests Verified (Physical runtime checks pending)

## Scope

WP-08A addresses the three P1 findings confirmed by source inspection and runtime screenshots:

1. E-STOP must not be disabled by an unrelated ordinary command.
2. AUTO must not be presented as a normal selectable MVP mode.
3. Desired/pending/reported command state must be separated in the UI.

## Changes

### E-STOP isolation

`EmergencyStopButton` no longer consumes the global `CommandState` to determine availability.

It now receives:

- connection state,
- whether an E-STOP command itself is pending,
- whether the E-STOP is already latched.

Therefore an ordinary pending command such as pump start, stop, mode change, or countdown operation no longer disables the remote E-STOP.

Behavior:

- connected + no E-STOP pending + no latch → E-STOP enabled
- E-STOP pending → `Stopping...`
- E-STOP latched → `E-STOP ACTIVE`
- disconnected → disabled

Firmware remains the authoritative safety layer.

### AUTO MVP boundary

The mode selector now exposes only:

- Manual
- Countdown

AUTO remains represented internally in the domain model for compatibility/state reporting, but is no longer a selectable production MVP control.

If an existing device state reports or requests AUTO, the UI explicitly states:

> Auto mode is not available in the current MVP.

The initial dashboard fallback state is also MANUAL rather than AUTO.

### Desired / pending / reported separation

Added `PendingCommandType` to `DashboardUiState`.

The view model maps the internal dashboard command to an explicit UI intent type:

- MANUAL_POWER
- MODE_CHANGE
- COUNTDOWN_START
- COUNTDOWN_STOP
- EMERGENCY_STOP
- CLEAR_ERRORS

Controls now use the relevant command type instead of assuming that the global `CommandState` belongs to every control simultaneously.

This prevents, for example, a mode-change operation from being rendered as a pending pump start/stop operation.

Mode selection is also disabled while another command is pending, preventing competing ordinary commands from overwriting the active command lifecycle.

## Files Changed

- `app/src/main/java/com/smartflow/domain/Models.kt`
- `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt`
- `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt`
- `app/src/main/java/com/smartflow/presentation/components/ControlPanel.kt`
- `app/src/main/java/com/smartflow/presentation/components/core/EmergencyStopButton.kt`
- `app/src/main/java/com/smartflow/presentation/components/settings/ModeSelector.kt`

## Verification Results

Executed locally in the Android workspace:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

**Results:**
- `compileDebugSources`: `BUILD SUCCESSFUL` (18 actionable tasks; clean compilation).
- `testDebugUnitTest`: `BUILD SUCCESSFUL` (**32/32 unit tests passed** across all 4 suites: `FirebaseModelsTest` [8], `DeviceConfigValidatorTest` [10], `DashboardStateReducerTest` [11], `CloudClaimCoordinatorTest` [3]).

### Physical / Runtime Verification Checklist

To complete full closure on the physical POCO device:

1. Start pump and confirm E-STOP remains enabled while the ordinary command is pending.
2. Trigger E-STOP and confirm only the E-STOP enters its pending state.
3. Confirm E-STOP becomes `E-STOP ACTIVE` after the device reports the latch.
4. Confirm AUTO is no longer selectable.
5. Confirm Manual/Countdown remain selectable when no ordinary command is pending.
6. Confirm a pending mode change does not appear as `STARTING PUMP...` or `STOPPING PUMP...`.
7. Confirm reported pump state remains authoritative while a command is awaiting confirmation.
8. Test disconnected behavior and confirm E-STOP is unavailable only because there is no connected remote control path.
