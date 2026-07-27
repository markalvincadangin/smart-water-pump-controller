---
status: draft
version: 1.0
last-reviewed: 2026-07-27
source: hand-authored
---

# Feature Specification: SmartFlow System Integration & Environment Standardization

**Feature Branch**: `004-system-integration`

**Created**: 2026-07-27

**Status**: Draft

**Objective**: Connect the three independently built SmartFlow components — ESP32 Firmware, Native Android App, and Firebase Cloud Backend — into a single, end-to-end functioning system. This includes embedding live credentials, enforcing security rules, validating the full device provisioning lifecycle, and verifying the Device Shadow data flow in a real or emulated environment.

## Quality Attributes

The integration is guided by the following priorities when making trade-offs:
- **Safety First**: Integration work must not weaken or bypass any firmware safety constraints (Dry-Run, Overflow, Fail-to-OFF). All safety mechanisms remain non-negotiable during integration.
- **Security**: Firebase RTDB rules must enforce ownership; no device should accept commands from unauthenticated or unclaiming users.
- **Correctness**: The Device Shadow round-trip (App → Firebase → Firmware → Firebase → App) must be verified as deterministic.
- **Developer Experience**: Any developer with valid Firebase credentials must be able to run the full system locally by following a documented setup procedure.

---

## Clarifications

### Session 2026-07-27
- Q: How should RTDB rules validate that firmware can only write to its own device path, not other devices? → A: **Option C — Anonymous Auth per device**: each ESP32 signs into Firebase anonymously on first boot, stores the returned anonymous UID in NVS, and writes that UID to `/devices/{deviceId}/metadata/firmwareUid`. RTDB rules for `shadow/reported`, `telemetry`, `diagnostics`, `events`, and `status` enforce `auth.uid === data.child('metadata/firmwareUid').val()`. This makes each device independently identifiable and is scalable to N devices. Custom Token minting (Option A) is the designated production-hardening upgrade path.
- Q: Should a Factory Reset also delete the device's historical data from the RTDB, or should it be retained? → A: **Option A — Delete Everything**: To ensure data privacy upon device reassignment, a Cloud Function will trigger on the device unclaim event and hard-delete the entire `/devices/{deviceId}` node.
- Q: If the ESP32 fails to connect to Wi-Fi, how should it communicate this failure back to the Android app? → A: **Option A — BLE Status Characteristic**: The ESP32 maintains the BLE connection while attempting Wi-Fi and updates a "Status" GATT characteristic with success/fail codes. The Android app subscribes to this for real-time feedback (e.g., incorrect password).
- Q: Does the rewrite of `database.rules.json` and Cloud Functions to the new `/devices/` schema violate Constitution Principle VI (Backward Compatibility) by dropping the old `/pump_system/` rules? → A: **Exempted**: The user clarified that the project is still in development with no live production users. The new multi-tenant architecture entirely replaces the legacy V1 code. Maintaining backward compatibility for non-existent legacy hardware is unnecessary. We can safely overwrite the old V1 rules and Cloud Function triggers.

---


| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | Integration of Firebase credentials must not introduce new failure paths that bias toward pump ON. |
| II. Dry-Run Lockout | Yes | Cloud-side command routing must not bypass dry-run latch or issue `clear_error` without explicit user intent. |
| III. Overflow Protection | Yes | No integration change may suppress or shortcut the overflow lockout signal from firmware. |
| IV. TOR Independence | Yes | Physical hardware cutouts are unaffected by this integration work. |
| V. Sensor Freshness / E-Stop | Yes | Cloud Functions and Android App must not send commands that race with or suppress `reset_stop` / `clear_error`. |
| VI. Backward Compatibility | **Exempt** | User clarified no live production devices exist. The old `/pump_system/` schema can be safely hard-replaced by the new `/devices/` schema without migration. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 — First-Time Setup: Device Provisioning (Priority: P1)
A user with a brand-new ESP32 device and the installed Android App completes the full provisioning journey, resulting in the device appearing in their Firebase account and communicating telemetry.

**Acceptance Scenarios**:
1. **Given** a factory-fresh ESP32 with firmware flashed, **When** powered on, **Then** it broadcasts a BLE advertisement named `SmartFlow-<suffix>` within 5 seconds.
2. **Given** the Android App is open, **When** the user taps "Add Device," **Then** the app scans and finds the device within 10 seconds.
3. **Given** the user enters their Wi-Fi credentials and taps "Provision," **When** provisioning completes, **Then** the device connects to Wi-Fi, claims ownership in Firebase under the authenticated user's UID, and reports `lifecycle: ONLINE` within 30 seconds.
4. **Given** a provisioned device, **When** the user opens the Dashboard, **Then** live `waterLevel` and `flowRate` telemetry is visible within 5 seconds of opening the screen.

### User Story 2 — Device Control: Pump Shadow Round-Trip (Priority: P1)
A user toggles the pump state in the Android App and the physical relay responds, with the reported state reflecting the actual hardware outcome.

**Acceptance Scenarios**:
1. **Given** an ONLINE device, **When** the user toggles the pump ON in the app, **Then** the Firebase `shadow/desired/pumpState` is written as `true`, the ESP32 actuates the relay, and `shadow/reported/pumpState` reflects `true` within 3 seconds.
2. **Given** a dry-run lockout is active, **When** the app sends `pumpState: true`, **Then** the firmware rejects the command, the pump remains OFF, and the `events` log contains an entry with `severity: ERROR` and `code: DRY_RUN`.

### User Story 3 — Security: Unauthorized Access Rejection (Priority: P1)
The Firebase RTDB security rules enforce device ownership so that only the claiming user can issue commands to their device.

**Acceptance Scenarios**:
1. **Given** User A has claimed device `SF-000001`, **When** User B (authenticated) attempts to write to `/devices/SF-000001/shadow/desired/`, **Then** Firebase rejects the write with a permission denied error.
2. **Given** an unauthenticated client, **When** it attempts to read or write any `/devices/<id>/` node, **Then** Firebase rejects the request outright.

### User Story 4 — Observability: Diagnostics & Event Logs Visible in App (Priority: P2)
A user can view device health diagnostics and structured error events from the Dashboard.

**Acceptance Scenarios**:
1. **Given** an ONLINE device, **When** the user opens the Diagnostics section, **Then** `freeHeap`, `wifiRSSI`, and `restartReason` are displayed with values updated in the last 60 seconds.
2. **Given** a safety event has occurred (e.g., dry-run), **When** the user views the Events log, **Then** at least one structured event entry is visible with `timestamp`, `severity`, `category`, `code`, and `message` fields.

---

## Requirements *(mandatory)*

### Functional Requirements
- **FR-001**: Firebase RTDB Security Rules MUST enforce that only the authenticated owner UID stored in `/users/<uid>/devices/<deviceId>` may read from or write to `/devices/<deviceId>/shadow/desired/`.
- **FR-002**: The Android App MUST include a valid `google-services.json` configuration so it can authenticate against the live Firebase project.
- **FR-003**: The ESP32 firmware MUST sign into Firebase using Anonymous Authentication on first boot, persist the returned anonymous UID to NVS, and write that UID to `/devices/{deviceId}/metadata/firmwareUid`. The Firebase API key and Database URL MUST be stored in `config/secrets.h` (gitignored). RTDB rules for firmware-writable paths MUST validate `auth.uid === data.child('metadata/firmwareUid').val()`.
- **FR-004**: A developer setup guide MUST document the exact steps to go from a blank environment to a running end-to-end system (Firebase credentials, Android build, firmware flash sequence).
- **FR-005**: The Android App MUST request Bluetooth runtime permissions (BLUETOOTH_SCAN, BLUETOOTH_CONNECT, ACCESS_FINE_LOCATION) at the point of first scan, before initiating BLE discovery.
- **FR-006**: The full provisioning flow (BLE scan → credential write → commit → Wi-Fi connect → Firebase ONLINE) MUST be validated in a documented test procedure with pass/fail criteria. During this flow, the ESP32 MUST maintain the BLE connection and report Wi-Fi connection success or failure codes via a dedicated GATT Status characteristic to the Android App.
- **FR-007**: The Factory Reset flow MUST be implemented — clearing NVS credentials on the firmware side, removing the `/users/<uid>/devices/<deviceId>` claim in Firebase (accessible from the Android Dashboard), and triggering a Cloud Function to hard-delete the `/devices/<deviceId>` node for data privacy.
- **FR-008**: Firebase RTDB Security Rules MUST be deployed and validated against the test scenarios defined in US3 (unauthorized access rejection).
- **FR-009**: The Cloud Functions MUST be compatible with the V2 `/devices/<id>/` schema and not write to any deprecated V1 paths.
- **FR-010**: An integration smoke-test script or checklist MUST exist that verifies the end-to-end data flow from firmware → Firebase → Android App without requiring physical hardware (using the RTDB export JSON as seed data).

### Assumptions
- The Firebase project (`smart-water-pump-system`) is already provisioned and accessible to the developer.
- The ESP32 has the V2 firmware (003 branch) flashed prior to integration testing.
- The Android App will be built and run via Android Studio on a physical Android device (API 31+) for BLE testing.
- Firebase Anonymous Authentication is enabled in the Firebase Console (Authentication → Sign-in providers → Anonymous).
- Each ESP32 device has a unique anonymous Firebase UID stored persistently in NVS after first boot. If NVS is erased (factory reset), the device re-registers with a new anonymous UID.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes
- **SC-001**: A developer following the setup guide can complete first-time provisioning of a new device in under 15 minutes from a clean environment.
- **SC-002**: Firebase RTDB Security Rules pass all three unauthorized access test scenarios (US3) with zero false negatives.
- **SC-003**: The Device Shadow round-trip (app command → relay actuation → reported state) completes within 5 seconds under normal Wi-Fi conditions.
- **SC-004**: The Android App displays live telemetry within 5 seconds of opening the Dashboard on a provisioned device.
- **SC-005**: The developer setup guide is validated by completing a full provisioning cycle end-to-end with zero undocumented steps.
- **SC-006**: Factory Reset successfully clears all device claims from Firebase and returns the device to BLE advertising mode within 10 seconds.
