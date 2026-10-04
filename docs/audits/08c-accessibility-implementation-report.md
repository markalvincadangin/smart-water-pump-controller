# WP-08C — Accessibility & Interaction Semantics Implementation Report

**Status:** VERIFIED & CLOSED  
**Work package:** WP-08C — Accessibility & Interaction Semantics  
**Branch:** `docs/smartflow-system-contract`

## 1. Scope

WP-08C addresses the accessibility findings identified after WP-08A/WP-08B:

- Connecting overlay semantics
- Safety-critical E-STOP announcements and state semantics
- Live-region behavior for important status changes
- Minimum interactive target review
- Preservation of existing Material/Compose accessibility semantics

This work does not change pump-control authority, safety rules, Firebase contracts, or command state transitions.

## 2. Findings and implementation

### 2.1 Connecting overlay

The previous overlay used an empty `clickable` handler solely to intercept touches. That exposed the overlay as a meaningless interactive element to accessibility services.

It now:

- consumes pointer input without exposing an empty click action;
- identifies itself as a window-like/pane state with `paneTitle`;
- exposes "Connecting to SmartFlow. Please wait." as its semantic description;
- uses a polite live region.

This preserves the existing behavior of preventing interaction with the underlying dashboard while giving TalkBack meaningful information.

### 2.2 E-STOP semantics

The E-STOP button now exposes explicit accessibility descriptions and state descriptions:

- Ready → "Emergency stop. Stops the pump and activates the safety interlock." / "Ready"
- Pending → "Emergency stop command is being sent." / "Stopping"
- Latched → "Emergency stop is active. Pump is interlocked." / "Active"
- Offline → "Emergency stop is unavailable because the device is offline." / "Unavailable"

The existing safety behavior is unchanged:

- ordinary commands do not disable E-STOP;
- E-STOP is disabled only while its own command is pending or when already latched;
- offline E-STOP remains unavailable because there is no remote connection.

### 2.3 Offline status announcement

The dashboard offline banner is marked as a polite live region so an accessibility service can be notified when the connection state changes without repeatedly announcing rapidly changing telemetry.

### 2.4 Touch-target review

The current dashboard controls were reviewed against the Android/Material 48dp minimum interactive target guidance.

Current implementation:

- Primary/secondary command buttons: **56dp height**
- E-STOP: **56dp height**
- Device configuration IconButton: **48dp minimum**
- Material SegmentedButton: uses Material interactive sizing
- Material Slider: uses its built-in interactive target behavior

No artificial wrapper was added where the Material component already provides the required interaction target.

## 3. Files changed

- `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt`
- `app/src/main/java/com/smartflow/presentation/components/core/EmergencyStopButton.kt`

## 4. Verification performed

Build, unit-test, and live-device accessibility verification were completed.

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

The following runtime checks were performed on the connected POCO device:

1. TalkBack can focus the E-STOP and announces a meaningful action/state.
2. While E-STOP is pending, TalkBack receives the pending state rather than treating it as an ordinary ready action.
3. After E-STOP latches, TalkBack receives the active/interlocked state.
4. When disconnected, E-STOP is announced as unavailable rather than as an actionable remote command.
5. The connecting overlay is announced as status and does not expose a meaningless clickable action.
6. The overlay still blocks interaction with the dashboard underneath.
7. Offline status is announced when the connection changes.
8. Existing WP-08A behavior remains intact: E-STOP isolation, Manual/Countdown-only mode selection, and authoritative reported state.

## 5. Closure result

WP-08C verification passed. Compilation, the full Android unit-test suite, and live-device accessibility-node inspection completed successfully. The implementation is verified and closed.

## 6. Reference basis

The implementation follows Android's Compose accessibility guidance:

- interactive targets should be at least 48dp;
- Material/Compose controls provide built-in semantics for standard interactions;
- custom semantics should add meaningful context where the defaults are insufficient;
- live regions should generally be polite and assertive live regions should be reserved for time-sensitive content;
- frequently changing content such as countdown timers should not be made into live regions.

Source: Android Developers, "API defaults" and "Semantics".
