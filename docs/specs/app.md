---
status: current
last-reviewed: 2026-10-04
source: hand-authored
---

# SmartFlow Android App Specification

The overarching cross-system contract governing interactions between the Android client, ESP32 Firmware, and Firebase RTDB is defined in [`docs/specifications/smartflow-system-contract.md`](../specifications/smartflow-system-contract.md) (v1.1).

## Account and device gate

The app is available only after Firebase Authentication establishes a durable account. Google accounts are eligible immediately; email/password accounts must verify their email before they can claim or control a device. Before device access or an ownership-sensitive callable, the app refreshes the Firebase session and rejects a locally cached user that has been deleted or revoked. Anonymous or guest sessions may not create, own, transfer, release, or control hardware. Cloud Functions independently resolve the caller from the current Firebase Auth record and enforce the same rule.

When a signed-in durable user has no claimed devices, the app routes to provisioning instead of rendering a controllable device dashboard. The app may read a user's device index for navigation, but all ownership changes occur through authenticated Cloud Functions; it must never write an authoritative owner marker directly.

## Provisioning handoff

After the ESP32 emits terminal BLE `provisioned`, the app shows cloud-registration progress for up to 90 seconds while retrying the callable claim with the in-memory pairing proof. It must not restart BLE scanning automatically during this period. If the window expires, the app offers a cloud-claim retry that preserves the in-memory proof and a separate explicit action to start a new BLE provisioning session.

## Owner management

Each device has exactly one owner. Owner management offers explicit Wi-Fi recovery, release, transfer, and cancellation actions; it does not offer sharing, technician roles, or lost-owner takeover. Wi-Fi recovery communicates that the pump is stopped and only local Wi-Fi/enrollment is reset. Release and transfer communicate the five-minute nearby BLE pairing requirement and preserve ownership until a valid replacement claim completes.

The Account screen checks server-authoritative deletion eligibility before any account-deletion action. Self-service account deletion is blocked while the signed-in account owns one or more devices; the app directs the owner to release or transfer every device first. A final account-delete action must require Firebase re-authentication and a separate explicit confirmation.

## Notification Authority and FCM

Canonical push notification token authority is established strictly at:

`users/{uid}/notification_prefs/fcmTokens/{tokenId}`

All client token registrations, updates, and removals must route through this user-level path. Device-level token writes (`devices/{deviceId}/fcmTokens`) are formally deprecated and must not be used as an authoritative recipient registry. Cloud Functions fan out alerts only to tokens registered under the owning user's profile.

## Production diagnostics

The app presents the device health snapshot (`freeHeap`, `wifiRSSI`, `restartReason`) and at most 50 WARN/ERROR cloud events. It must not depend on or expose the development TCP log console.

## UI/UX and Design System

The app strictly adheres to the centralized brand design system via Material 3 composition. Dynamic color extraction from user wallpapers is explicitly disabled to ensure uncompromised brand identity across light and dark modes. All semantic styling relies exclusively on `MaterialTheme.colorScheme` tokens; hardcoded colors are forbidden. The core dashboard layout is intrinsically responsive, enforcing a scrolling single-column hierarchy in portrait and pivoting to a side-by-side grid in landscape to maximize telemetry visibility without obscuring controls.

## HMI, Command Lifecycle, and State Derivation

Human-machine interface (HMI) controls require tactile confirmation and strict state idempotency. Critical hardware actions invoke device haptic feedback (`LongPress`).

### 1. Observable Command Model

Commands adhere to an explicit, observable four-stage lifecycle:

$$\text{Ready} \longrightarrow \text{Pending} \longrightarrow \text{Reported Device Transition} \longrightarrow \text{Completed}$$

- **Write Acknowledgment Is Not Execution**: Successful RTDB write to `/shadow/desired` indicates only that cloud intent was recorded. A command completes **only** when the device reflects the corresponding change in `/shadow/reported`.
- **Command Outcomes**:
  - `Completed`: Hardware transitioned to match requested state.
  - `InterlockBlocked`: Command blocked because emergency stop is actively latched.
  - `Rejected`: Command rejected because an active safety fault exists.
  - `OfflineBlocked`: Command cannot be executed because device is disconnected.
  - `TimedOut`: Hardware failed to report expected transition within timeout window (default 10s).
- **Fault Isolation**: Safety faults/interlocks are evaluated prior to normal command completion. An active fault stopping the pump while a STOP command is pending produces a fault/interlock state rather than normal completion.
- **E-Stop Exemption**: The Emergency Stop (E-STOP) button remains perpetually enabled for redundant abort signaling.

### 2. State Presentation Precedence

The dashboard resolves conflicting device conditions in strict priority order:

$$\text{Offline} \longrightarrow \text{E-STOP Interlocked} \longrightarrow \text{Fault (Lockout)} \longrightarrow \text{Running} \longrightarrow \text{Idle}$$

- **E-Stop Precedence**: An active emergency stop (`emergency_stop_latched = true`) takes immediate visual precedence over generic faults.
- **Fault Semantics**: Machine-readable `last_fault_code` is authoritative for fault representation (`MAX_RUNTIME`, `DRY_RUN`, `LEVEL_SENSOR_TIMEOUT`). `last_fault_message` is purely explanatory. `is_overflow_error` is supported solely as a legacy fallback for `MAX_RUNTIME`.

## Client Input Validation

Client forms and controls must validate inputs before sending updates to RTDB, preventing rejected writes:

- **Countdown Timer Duration**: Valid 1–120 minutes. The countdown upper bound is decoupled from maximum pump runtime. Running timers cannot be extended; they must expire or be stopped before a new countdown can start.
- **Pump Start Level (`pump_start_level_pct`)**: 0–100%.
- **Pump Stop Level (`pump_stop_level_pct`)**: 0–100%, and strictly `stop > start`.
- **Dry-Run Threshold (`dry_run_threshold_lpm`)**: 0.1–10.0 L/min.
- **Maximum Pump Runtime (`max_pump_runtime_min`)**: 30–120 minutes.
