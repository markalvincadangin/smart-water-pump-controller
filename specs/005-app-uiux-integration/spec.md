---
status: draft
version: 0.1
last-reviewed:
source: auto-generated
---

# Feature Specification: App UI/UX Redesign & System Integration

**Feature Branch**: `feature/app-uiux-integration`

**Created**: 2026-07-28

**Status**: Draft

**Input**: User description: "update our app UI/UX and layouts, we can use the archived web app 'archive/dashboard' as our baseline or inspo. We will do the UI/UX enhancement and the app integration to the cloud and cloud to firmware."

## Clarifications

### Session 2026-07-28
- Q: What is the source of the logo and branding assets to be used in the Android App? → A: Use the Google Stitch generated placeholders/logo.
- Q: The firmware spec documents its cloud contract as `/pump_system/status` and `/pump_system/control`, but `database.rules.json` defines a v2 schema (`devices/$deviceId/...`). How should the Android app communicate with the firmware? → A: Option A: App writes directly to the v2 `devices/$deviceId/shadow/desired` node; Cloud Functions will translate this to the legacy firmware nodes. (REVISED 2026-07-28: See below)
- Q: In the firmware operational rules, MANUAL pumping is controlled by `mode: "MANUAL"` and `manual_desired: true/false`. Should the app adopt the firmware's explicit fields? → A: Option A: Yes, update the app data model to match the firmware's exact control fields (`mode`, `manual_desired`, `emergency_stop`).
- Q: The firmware defines a `COUNTDOWN` mode. Should the App UI support and control it? → A: Option A: Yes, include COUNTDOWN mode. Add duration selection and a timer display to the App UI, updating the spec and schema accordingly.
- Q: Should the App adopt the exact field names used by the firmware instead of `waterLevel`, `flowRate`, and `distanceCm`? → A: Option A: Yes, update the App Data Contract to use the exact firmware field names (`water_level_percent`, `flow_rate_lpm`, `ultrasonic_last_good_cm`) for 1:1 mapping.
- Q: Should we add `clear_error` to the schema and UI to recover from dry-run/overflow states? → A: Option A: Yes, add `clear_error` to the schema and ensure the UI can trigger it.
- Q: Should the App include toggles for maintenance bypasses (`bypass_level_sensor`, `bypass_flow_sensor`)? → A: Option A: Yes, expose maintenance bypass toggles in the App UI (Configuration Screen) and schema.
- Q: `firmware.md` currently documents the legacy `/pump_system/` schema. How would you like to handle `firmware.md`? → A: Option B: Update `firmware.md` to document the v2 `devices/$deviceId/...` schema, meaning we will also update the firmware codebase to natively support v2 instead of relying on Cloud Functions.

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | App must safely command pump off without assuming success; requires feedback from RTDB `shadow/reported`. |
| II. Dry-Run Lockout | Yes | UI must clearly reflect dry-run lockout state and provide a clear, intentional reset mechanism. |
| III. Overflow Protection | Yes | UI must display tank level safely, show overflow limits, and respect auto mode constraints. |
| IV. TOR Independence | No | Hardware cutout handles itself; app only displays status. |
| V. Sensor Freshness / E-Stop | Yes | App must indicate stale sensor data and provide emergency stop UI. |
| VI. Backward Compatibility | Yes | App must integrate with existing RTDB v2 schema gracefully. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Real-time Dashboard Monitoring (Priority: P1)

Users open the app and instantly see the real-time status of their water pump, flow rate, and water tank level using rich, fluid animations inspired by the legacy web dashboard.

**Why this priority**: Core value of an IoT dashboard is immediate, clear visibility of critical metrics.

**Independent Test**: Can be tested by mocking RTDB `telemetry` updates and verifying that the UI updates in real-time (water wave animations, flow rate numbers, pump running halos).

**Acceptance Scenarios**:

1. **Given** the pump is actively running, **When** the user views the dashboard, **Then** they see a glowing halo around the pump status card and a live L/min flow rate.
2. **Given** the tank is at 50%, **When** the user views the tank card, **Then** they see a fluid wave animation filled to the middle of the container with an amber/teal color based on thresholds.

---

### User Story 2 - Remote Pump Control & Overrides (Priority: P1)

Users can toggle the pump mode (AUTO/MANUAL/COUNTDOWN) and manually override the pump state directly from the app, communicating via Firebase RTDB to the ESP32 firmware.

**Why this priority**: Essential functionality to control the hardware remotely.

**Independent Test**: Can be tested by tapping the UI controls and verifying that the correct `shadow/desired` state is written to the Firebase RTDB.

**Acceptance Scenarios**:

1. **Given** the system is in AUTO mode, **When** the user selects MANUAL and taps the Pump Power toggle, **Then** the app writes `mode: "MANUAL"` and `manual_desired: true` to the RTDB `shadow/desired` node.
2. **Given** the user selects COUNTDOWN mode, **When** they select a 30-minute duration and tap Start, **Then** the app writes `mode: "COUNTDOWN"`, `countdown_duration_min: 30`, and `countdown_start: true` to the RTDB.
3. **Given** a Dry-Run Lockout state, **When** the user attempts to start the pump, **Then** the UI prevents the action or shows a warning, requiring a formal error reset first (`clear_error: true`).

---

### User Story 3 - Device Configuration & Thresholds (Priority: P2)

Users can configure critical safety thresholds (low water level, dry-run flow threshold, max overflow timeout) via an intuitive bottom sheet or settings screen.

**Why this priority**: Customizes safety constraints to the user's specific tank size and pump capacity.

**Independent Test**: Can be tested by sliding the threshold controls and ensuring the new config values are persisted to Firebase RTDB.

**Acceptance Scenarios**:

1. **Given** the user opens the Device Config sheet, **When** they adjust the Low Level threshold slider to 30%, **Then** the app updates the Firebase `config/lowLevelThreshold` value to 30.

---

### Edge Cases

- What happens when the Android device loses network connectivity? (App should display a "Disconnected" or "Offline" banner, graying out controls).
- How does the system handle Firebase Auth token expiration? (Should silently refresh or prompt for re-login).
- What happens if the firmware takes too long to respond to a command? (App should timeout the loading state and revert the toggle switch to the actual `reported` state).

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST implement a high-performance HMI dark-themed interface based on the Google Stitch Material 3 design system.
- **FR-002**: The system MUST strictly map colors to safety-critical roles: Primary/Cyan (`#0EA5E9`) for neutral visualization, Secondary/Emerald (`#10B981`) for active states, Tertiary/Amber (`#F5A623`) for warnings, Error/Red (`#EF4444`) for emergency actions.
- **FR-003**: The system MUST use typography suited for industrial legibility: Hanken Grotesk (Headlines), Inter (Body), and JetBrains Mono (Telemetry/Labels).
- **FR-004**: The system MUST feature an animated visual gauge (`TankLevelCard`) displaying the water tank percentage, absolute distance, and upper/lower threshold markers.
- **FR-005**: The system MUST feature a prominent control card (`PumpStatusCard`) indicating the pump's operating mode, with an Emerald glowing halo animation when active.
- **FR-006**: The system MUST display real-time sensor metrics (flow rate, water level) immediately as they are reported by the hardware, utilizing monospaced fonts to prevent visual jumping.
- **FR-007**: The system MUST securely transmit user commands (start/stop, mode changes, clear errors) to the connected hardware via a `ControlPanel` with distinct segmented controls supporting AUTO, MANUAL, and COUNTDOWN modes.
- **FR-008**: The system MUST present a countdown timer UI when in COUNTDOWN mode, allowing users to select a duration and observe the `countdown_remaining_sec` ticking down.
- **FR-009**: The system MUST present a structured historical event log with severity color coding for user auditing via an `ActivityPanel`.
- **FR-010**: The system MUST maintain a minimum 48x48dp touch target for all interactive elements to prevent accidental activations.
- **FR-011**: The system MUST provide toggles for maintenance bypasses (`bypass_level_sensor`, `bypass_flow_sensor`) in the Configuration screen.

### Key Entities

- **Device State**: The conceptual pairing of what the user wants the hardware to do vs. what the hardware is currently doing.
- **Telemetry**: Continuous real-time readings from the sensors (flow rate, distance, tank level).
- **Event Log**: Structured array of system events (errors, warnings, state changes) for user auditing.

---

## Data Contract *(if applicable)*

### Schema Changes
```json
{
  "devices": {
    "[device_id]": {
      "shadow": {
        "desired": {
          "mode": "string (AUTO/MANUAL/COUNTDOWN)",
          "manual_desired": "boolean",
          "countdown_start": "boolean",
          "countdown_duration_min": "number",
          "emergency_stop": "boolean",
          "reset_stop": "boolean",
          "clear_error": "boolean",
          "bypass_level_sensor": "boolean",
          "bypass_flow_sensor": "boolean"
        },
        "reported": {
          "run_mode": "string",
          "is_running": "boolean",
          "countdown_remaining_sec": "number",
          "is_error": "boolean",
          "is_overflow_error": "boolean",
          "emergency_stop_latched": "boolean"
        }
      },
      "telemetry": {
        "water_level_percent": "number",
        "flow_rate_lpm": "number",
        "ultrasonic_last_good_cm": "number"
      },
      "settings": {
        "pump_start_level_pct": "number",
        "pump_stop_level_pct": "number",
        "dry_run_threshold_lpm": "number",
        "max_pump_runtime_min": "number"
      }
    }
  }
}
```

---

## App Interface UX

### User-Facing Changes
- **Branding & Assets**: The application will utilize the Google Stitch generated logo and placeholder assets for the App Icon and Splash Screen.
- **Dashboard Screen**: Complete visual overhaul utilizing Google Stitch design tokens. Replaces basic text readouts with rich UI components:
  - **TankLevelCard**: Level 1 elevation (Surface-Variant #1E293B) containing the fluid animation. Uses Cyan fill for normal operation, transitioning to desaturated/gray when telemetry is stale.
  - **PumpStatusCard**: Central status hub. Uses an Emerald halo to indicate active pumping. Includes live Countdown Timer when in COUNTDOWN mode.
  - **ControlPanel**: Segmented controls (24px radius) for mode switching (AUTO/MANUAL/COUNTDOWN), duration selectors, and a prominent red Emergency Stop pill button.
  - **ActivityPanel**: Streamlined event log at the bottom.
- **Configuration Modal**: New bottom-sheet layout for threshold sliders and maintenance bypass toggles (`bypass_level_sensor`, `bypass_flow_sensor`).

### Offline / Connectivity Behavior
- The interface will use local caching for read-only viewing of the last known state.
- Control actions will immediately fail or warn if the network is disconnected, and a prominent banner will instruct the user of the stale state.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: UI transitions and animations (like the fluid tank wave) run smoothly without visible stuttering or dropped frames on supported mobile devices.
- **SC-002**: Tapping the pump toggle switch successfully transmits the command to the cloud platform in under 200ms on a typical Wi-Fi connection.
- **SC-003**: The application successfully authenticates and connects to the backend services securely without crashing.
- **SC-004**: Real-time telemetry updates from the hardware are reflected on the UI within 100ms of reaching the cloud platform.

### Validation Commands

```bash
# Android App Verification
cd app && ./gradlew assembleDebug

# Firebase RTDB Rule Validation (if needed)
firebase emulators:exec "npm test"
```

---

## Assumptions

- The ESP32 firmware will be updated to natively support reading and writing to the v2 schema (`devices/$deviceId/shadow`, `devices/$deviceId/telemetry`, `devices/$deviceId/settings`).
- Firebase Anonymous Auth is sufficient for app-to-cloud connections for this phase, matching the firmware approach.
- Jetpack Compose Canvas APIs are sufficient to recreate the Next.js/CSS `tankWave` fluid animations.
