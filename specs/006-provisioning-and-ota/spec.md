---
status: current
version: 0.1
last-reviewed: 2026-07-28
source: auto-generated
---

# Feature Specification: Provisioning & Wireless Flashing

**Feature Branch**: `feature/provisioning-and-ota`

**Created**: 2026-07-28

**Status**: Draft

**Input**: User description: "make the firmware setup (provision) work on our app, just like how the cctv and printers are set up using their respective apps. Next is the wireless uploading/flashing of the firmware to avoid constantly connecting to the usb. we can make it on the android app and this dev environment"

## Clarifications

### Session 2026-07-28
- Q: What is the desired scope of the OTA functionality for this iteration? → A: Limit to local network OTA for dev environment (e.g., via PlatformIO) to support active development. Exclude Android app triggered OTA for now.

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | OTA updates cause device restart. Ensure relay state defaults to OFF during and after boot. |
| II. Dry-Run Lockout | No | Provisioning/OTA does not interact with dry-run paths. |
| III. Overflow Protection | No | Provisioning/OTA does not interact with overflow logic. |
| IV. TOR Independence | No | Hardware independence remains untouched. |
| V. Sensor Freshness / E-Stop | No | Not modifying sensor evaluation or E-Stop reachability. |
| VI. Backward Compatibility | No | No RTDB schema changes for OTA in this iteration. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Unclaimed Device Provisioning (Priority: P1)

As a new user, I want to seamlessly connect my smartphone to a new unprovisioned SmartFlow device via Bluetooth to supply it with my local Wi-Fi credentials so it can connect to the internet and Firebase, mirroring standard IoT onboarding flows (like CCTVs).

**Why this priority**: Without this, users cannot onboard new hardware onto their Wi-Fi networks without hardcoding credentials in the firmware.

**Independent Test**: Can be fully tested by launching the Android App without a configured device, following the BLE setup prompts, and observing the device transition to ONLINE state in Firebase.

**Acceptance Scenarios**:

1. **Given** an unprovisioned ESP32 is powered on and advertising BLE, **When** the user opens the Android app, **Then** they are routed to the Provisioning screen instead of the Dashboard.
2. **Given** the user is on the Provisioning screen, **When** they enter valid Wi-Fi credentials and submit, **Then** the app sends them via BLE, the ESP32 reboots, connects to Wi-Fi, and the app proceeds to the Dashboard.

---

### User Story 2 - Wireless Firmware Upload in Dev Environment (Priority: P2)

As a developer, I want to wirelessly upload newly compiled firmware directly from my IDE (PlatformIO) to the device on my local network.

**Why this priority**: Significantly speeds up the development feedback loop when the device is deployed near the water tank and away from the developer's desk.

**Independent Test**: Tested by running `pio run -t upload -e esp32dev_ota` and verifying the upload succeeds over the local network.

**Acceptance Scenarios**:

1. **Given** the developer has compiled new code, **When** they select the OTA upload target in PlatformIO, **Then** the compiled binary is pushed directly to the ESP32's IP address.

---

### Edge Cases

- What happens when BLE provisioning is interrupted? (App should gracefully time out and allow retry).
- How does system handle a failed local OTA upload? (ArduinoOTA aborts, device resumes normal operation).

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Android App MUST route users to the Provisioning flow if a device is not yet claimed.
- **FR-002**: The Android App MUST successfully transmit SSID, Password, and a Commit command to the ESP32 via BLE.
- **FR-003**: The Dev Environment (PlatformIO) MUST support local network OTA flashes (via ArduinoOTA) configured as a selectable environment.

---

## Firmware Behavior *(if applicable)*

### State Machine Impact
- Boot sequence must initialize `ArduinoOTA` (if enabled/dev mode) after Wi-Fi connects.

### Safety Invariants
- [x] All fault paths exit with `setPump(false)` (OTA restarts reset relays to default OFF hardware state).
- [x] No new `digitalWrite(RELAY_PIN, ...)` calls added outside `setPump()`
- [x] All new timing uses wrap-safe helpers

---

## Dashboard UX *(if applicable)*

### User-Facing Changes
- **MainActivity / Navigation**: New app routing logic to enforce ProvisioningScreen when unclaimed.

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Unprovisioned devices can be successfully claimed and connected to Wi-Fi via the Android app in under 2 minutes.
- **SC-002**: A local network OTA upload from PlatformIO completes successfully without physical interaction.

### Validation Commands

```bash
# Android App
./gradlew assembleDebug

# Firmware Dev OTA
pio run -e esp32dev_ota --target upload
```

---

## Assumptions

- We assume `ArduinoOTA` memory overhead does not exceed the `huge_app.csv` partition limits.
