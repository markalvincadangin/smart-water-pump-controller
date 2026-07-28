# SmartFlow Android App HMI & System Design (vNext)

This document synthesizes the architectural design and Human-Machine Interface (HMI) principles for the SmartFlow Android Application. It combines the vNext RTDB contract requirements with safety-critical design standards.

## 1. Safety-Critical HMI Principles

The Android application serves as a safety-critical operator interface. The firmware remains the ultimate source of truth and enforces safety lockouts. The app's UX must align with high-performance HMI standards (ISA-101, IEC 62682):

*   **Calm Normal State**: Use a muted baseline color palette (slate/dark theme). Reserve high-saturation colors (amber, red) exclusively for abnormal conditions or active alerts.
*   **Actionable Alarms**: Every warning or error banner must clearly answer:
    1.  *What happened?*
    2.  *What is the risk?*
    3.  *What should I do now?* (e.g., provide a specific recovery action or clear button).
*   **Preventing Mode Errors**: 
    *   Accidental activations of `MANUAL` overrides or `Emergency Stop` must be mitigated through clear visual affordances (e.g., segmented buttons, confirmation dialogs for destructive actions).
    *   Disabled controls must clearly state *why* they are disabled.
*   **Situational Awareness**: Surface critical safety gating signals visually so the operator understands the system's readiness (e.g., indicating if the `remote_sensor_stable` or `level_fresh` gates are unmet).

## 2. RTDB Control Contract (vNext)

The app integrates with Firebase RTDB using the finalized vNext control schema. It writes to `/devices/{deviceId}/shadow/desired` and reads from `reported`.

### 2.1 Mode Management
*   **Allowed Modes**: `AUTO`, `MANUAL`, `COUNTDOWN`.
*   **No Force Modes**: Legacy `FORCE_ON` and `FORCE_OFF` states are strictly prohibited.
*   **Intent-Based Manual Control**: Instead of one-shot `manual_start` commands, the app maintains a persistent `manual_desired` intent (boolean).
    *   If `manual_desired=true` but the pump is stopped by firmware safety (e.g., dry-run), the UI must explicitly show a "Blocked by Safety" state rather than a confusing toggle mismatch.

### 2.2 Emergency Stop & Recovery
*   **E-Stop**: The Emergency Stop control should be contextually visible (highly visible when the pump is running). Activating it writes an `emergency_stop` one-shot trigger.
*   **Latched State**: The UI will reflect a `STOPPED` or latched error state once confirmed by the firmware.
*   **Reset**: A dedicated "Reset Stop" or "Clear Error" (`reset_stop`, `clear_error`) action must be provided to recover from latched faults.

### 2.3 Offline & Stale Data Behavior
*   When the Android device loses connectivity (Firebase disconnected) or the telemetry timestamp (`level_fresh`) becomes stale, the app must:
    *   Show a prominent "Controller Offline" or "Stale Data" banner.
    *   Disable control writes to prevent operators from sending commands based on outdated visual information.

## 3. Google Stitch (Material 3) Design System Compatibility

To ensure compatibility with Google Stitch (Material Theme Builder) and Jetpack Compose's M3 `ColorScheme`, the HMI principles map strictly to the following standard M3 color roles. By adhering to this token mapping, we can import Google Stitch themes directly without breaking our safety-critical color semantics:

*   **`primary` / `primaryContainer`**: Cyan (`#0EA5E9`). Used for brand identity and the fluid water animation fill in the `TankLevelCard`.
*   **`secondary` / `secondaryContainer`**: Emerald/Teal (`#10B981`). Used exclusively to indicate an active, running pump state (e.g., the glowing halo in the `PumpStatusCard`).
*   **`tertiary` / `tertiaryContainer`**: Amber (`#F5A623`). Reserved for warnings, blocked starts, and non-critical fault recovery banners (fulfilling the *Actionable Alarms* principle).
*   **`error` / `errorContainer`**: Red (`#EF4444`). Strictly reserved for the `Emergency Stop` button and hard lockout states (Dry-Run, Overflow).
*   **`surface` / `background`**: Dark Slate (`#0F172A`). Provides the muted, calm baseline environment.
*   **`surfaceVariant`**: Lighter Slate (`#1E293B`). Used for elevated cards (`TankLevelCard`, `PumpStatusCard`) to visually separate them from the calm background without adding saturation noise.

## 4. Jetpack Compose UI Architecture

*   **State Management**: `DashboardViewModel` will use Kotlin `StateFlow` to manage the UI state, merging the RTDB streams (`telemetry` and `shadow/reported`) into a single truth state.
*   **Component Modularity**:
    *   `TankLevelCard`: Renders the water level fluid animation. Muted teal for normal levels, amber/red for thresholds.
    *   `PumpStatusCard`: Central hub for observing pump action and invoking the E-Stop.
    *   `ControlPanel`: Clear segmented controls for mode switching (`AUTO` vs `MANUAL`), respecting the intent-based contract.
    *   `ActivityPanel`: Displays the fault history based on `last_fault_code`, using the actionability principles (What, Risk, Action).
