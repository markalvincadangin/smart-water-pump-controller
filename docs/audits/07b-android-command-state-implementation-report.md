# WP-07B — Android Command & State Derivation

## Status

**Implemented. Source-verified. Runtime Gradle verification pending after the latest changes.**

WP-07B corrects Android dashboard state derivation and establishes an observable command lifecycle without introducing a separate device acknowledgment protocol.

## Implementation

### 1. Pure state/command reducer

Added:

`app/src/main/java/com/smartflow/viewmodel/DashboardStateReducer.kt`

Responsibilities:

- map desired/reported operating modes;
- identify active faults;
- identify maximum-runtime faults using canonical `MAX_RUNTIME` with legacy `isOverflowError` fallback;
- derive `PumpState` with explicit precedence;
- determine whether a command is satisfied by reported device state;
- derive `CommandState` for a pending command.

### 2. Emergency-stop precedence

Dashboard pump state now follows:

1. device offline,
2. emergency-stop interlocked,
3. generic safety/fault state,
4. running,
5. idle.

This prevents an E-stop condition that also sets `is_error` from being presented as only a generic fault.

### 3. Safety before normal command completion

For ordinary commands, active E-stop or safety fault is evaluated before matching the requested pump state.

This prevents a pending command such as STOP from being falsely marked complete merely because a safety fault independently stopped the pump.

E-stop itself is considered completed when `emergency_stop_latched = true`.

### 4. Reported-state command reconciliation

Supported command expectations:

- manual power ON/OFF → reported manual mode + matching `is_running`;
- mode change → reported operating mode matches requested mode;
- countdown start → reported COUNTDOWN + remaining seconds > 0;
- countdown stop → reported COUNTDOWN + remaining seconds = 0;
- emergency stop → reported E-stop latch;
- clear errors → reported fault/interlock state cleared.

The application does not treat a successful RTDB write as physical command completion.

### 5. Client-side confirmation lifecycle

Dashboard commands now use:

**Ready → Pending → Reported state transition → Completed**

Failure outcomes:

- `Rejected` for a reported safety/fault rejection;
- `InterlockBlocked` for an active emergency-stop interlock;
- `OfflineBlocked` when Firebase/device connectivity is unavailable;
- `TimedOut` after a 15-second app-side confirmation window.

The 15-second timeout is an application feedback timeout only. It does not replace or alter firmware safety timers.

Completed/failure feedback is displayed transiently and does not become the device's persistent state.

### 6. Maximum-runtime semantics

Android now uses the machine-readable `last_fault_code` for maximum-runtime interpretation:

- canonical: `MAX_RUNTIME`;
- compatibility fallback: `is_overflow_error == true`.

User-facing fault labeling uses "Maximum runtime protection" rather than presenting the legacy overflow terminology as the primary meaning.

### 7. Regression tests

Added:

`app/src/test/java/com/smartflow/viewmodel/DashboardStateReducerTest.kt`

Coverage includes:

- E-stop precedence over generic fault;
- canonical and legacy max-runtime detection;
- manual command completion;
- countdown start/stop completion;
- interlock blocking;
- fault rejection;
- offline blocking;
- E-stop completion;
- regression case preventing a safety-stopped pump from falsely completing a normal stop command.

## Files Changed

- `app/src/main/java/com/smartflow/viewmodel/DashboardStateReducer.kt`
- `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt`
- `app/src/main/java/com/smartflow/presentation/components/ControlPanel.kt`
- `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt`
- `app/src/test/java/com/smartflow/viewmodel/DashboardStateReducerTest.kt`

## Verification Boundary

Source-level integration was checked after the changes, including the dashboard call sites and reducer tests.

A fresh Gradle compile/unit-test run after the final WP-07B commits has **not** been executed in this agent environment. The user should run:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest
```

WP-07B should not be marked fully verified until those commands pass.

## Next Package

**WP-07C — Android Client Validation**

Focus:

- countdown 1–120 minute UI validation;
- maximum runtime 30–120 minute UI validation;
- dry-run threshold 0.1–10.0 L/min;
- pump start/stop level controls;
- stop level > start level cross-field validation.

FCM token cleanup remains WP-07D.
