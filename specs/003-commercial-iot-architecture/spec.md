---
status: current
version: 1.0
last-reviewed: 2026-07-27
source: hand-authored
---

# Feature Specification: SmartFlow Commercial IoT Platform

**Feature Branch**: `003-commercial-iot-architecture`

**Created**: 2026-07-27

**Status**: Current

**Objective**: Develop SmartFlow as a single-device IoT platform using production-quality software engineering practices, modular architecture, and industry-standard embedded design. The implementation should remain intentionally lightweight while preserving a clear migration path toward future commercial deployment without requiring major architectural redesign.

## Quality Attributes
The architecture is guided by the following core quality attributes when making technical trade-offs:
- **Safety**: Physical fail-safes (Dry-Run, Overflow) always dominate software intents.
- **Offline Capability**: The device must operate its core pump automation without any cloud connectivity.
- **Maintainability**: Strict separation of concerns (HAL -> Drivers -> Services -> Cloud -> App).
- **Scalability**: Data models and provisioning flows must support N > 1 devices natively.
- **Observability**: Rich diagnostics and structured event logs for fleet health monitoring.
- **Security**: Device claiming, encrypted transport, and a clear migration path to hardware security.

---

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | The restructured firmware safety layer MUST retain its ability to fail-safe regardless of cloud, network, or OTA state. |
| II. Dry-Run Lockout | Yes | Cloud command refactoring and device shadow synchronization MUST keep dry-run unlatching strictly gated. |
| III. Overflow Protection | Yes | Retained strictly within the firmware independent of cloud/app intervention. |
| IV. TOR Independence | Yes | Firmware refactoring MUST keep software gates from bypassing physical cutouts. |
| V. Sensor Freshness / E-Stop | Yes | OTA, Diagnostics, and Provisioning tasks MUST NOT block safety interrupts or watchdogs. |
| VI. Backward Compatibility | No | This is a major version (v2.0) architectural break deprecating the V1 schema and dashboard. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Device Lifecycle & Claiming (Priority: P1)
The device follows a strict state machine lifecycle. A new device boots as UNCLAIMED, moves to PROVISIONING via a mobile app scan (QR code/Device ID), and is securely assigned to a user account before entering the ONLINE state where remote commands are permitted.
**Acceptance Scenarios**:
1. **Given** an unprovisioned ESP32, **When** powered on, **Then** it broadcasts a BLE beacon and enters the `PROVISIONING` state.
2. **Given** a provisioned but unclaimed device on Wi-Fi, **When** it receives a cloud command, **Then** it rejects the command until a user scans the QR token and claims ownership in the cloud registry.
3. **Given** an `ONLINE` device, **When** the user initiates a Factory Reset, **Then** the device clears NVS credentials, detaches from the user account, and reverts to `UNCLAIMED`.

### User Story 2 - Device Shadow & Telemetry (Priority: P1)
Users interact with the device through an Android App built with MVVM architecture, manipulating a "desired" state in the cloud which the device independently syncs and reports back as the "reported" state to eliminate race conditions.
**Acceptance Scenarios**:
1. **Given** the app requests the pump to turn ON, **When** it writes `desired: { pump: ON }` to the Cloud Shadow, **Then** the ESP32 detects the change, actuates the relay, and writes `reported: { pump: ON }`.

### User Story 3 - Robust OTA Firmware Updates (Priority: P2)
Firmware is updated wirelessly via a dedicated OTA Manager subsystem that verifies the binary integrity before swapping partitions.
**Acceptance Scenarios**:
1. **Given** an OTA request, **When** the OTA Manager downloads the file via HTTPS, **Then** it verifies the SHA256 hash and cryptographic signature before flashing to the inactive partition.
2. **Given** a corrupt OTA update, **When** the device reboots, **Then** the bootloader rolls back to the previous known-good partition and reports an `ERROR` lifecycle event to the cloud.

---

## Requirements *(mandatory)*

### Functional Requirements
- **FR-001**: System MUST decouple into five independent layers: Hardware, Firmware, Cloud, Android App, and (Optional) Backend.
- **FR-002**: Firmware MUST be modularized into a strict embedded hierarchy (`hal/`, `drivers/`, `network/`, `cloud/`, `ota/`, `diagnostics/`, `safety/`, `app/`). Higher layers depend on lower layers; drivers MUST NOT know about the cloud.
- **FR-003**: System MUST utilize a Hardware Abstraction Layer (HAL) to isolate GPIO and physical dependencies from business logic.
- **FR-004**: System MUST implement a Device Shadow pattern (`desired` vs `reported` state) for robust cloud synchronization.
- **FR-005**: System MUST implement secure Device Claiming. Devices MUST be claimed by a user account before accepting remote commands.
- **FR-006**: Cloud data model MUST separate concerns explicitly: `/devices/<id>/` split into `metadata/`, `status/`, `telemetry/`, `settings/`, `shadow/`, `diagnostics/`, and `events/`. *(Current Implementation: Firebase Realtime Database)*.
- **FR-007**: Architecture MUST implement ubiquitous versioning (`FirmwareVersion`, `HardwareVersion`, `ConfigVersion`, `ProtocolVersion`, `CloudSchemaVersion`, `AndroidAPIVersion`).
- **FR-008**: System MUST implement an OTA Manager subsystem featuring HTTPS download, SHA256 validation, and dual-partition rollback.
- **FR-009**: Device MUST capture and report rich diagnostics to the cloud (e.g., Free Heap, CPU Usage, Flash Usage, RSSI, Watchdog counts, Restart reason).
- **FR-010**: System MUST define a structured Event Logging model (`eventId`, `timestamp`, `severity`, `category`, `code`, `message`).
- **FR-011**: NVS Persistence MUST be explicitly mapped (e.g., WiFi, Device ID, settings, thresholds, claim token) ensuring the ESP32 operates autonomously offline.
- **FR-012**: Android Client MUST be built natively using Jetpack Compose and adhere to the MVVM architectural pattern (Presentation → ViewModel → Repository → Cloud Store).
- **FR-013**: Architecture MUST define Architectural Decision Records (ADRs) to document why specific technologies (e.g., ESP-IDF, Firebase, Jetpack Compose) were chosen.

### Future Production Hardening
The architecture SHALL reserve extension points and design headroom for the following future production security features (not required for MVP):
- Secure Boot V2
- Flash Encryption
- Cryptographically Signed OTA
- Device Certificates and Hardware Root of Trust
- Rate Limiting & JWT verification

---

## Firmware Behavior

### State Machine Impact
- **Decoupled Architecture**: Firmware behaves like an OS. The `app` layer orchestrates input from `cloud` and `sensors`, passing intents to `safety`, which actuates `drivers` via `hal`. 
- **Offline Autonomy**: Device relies completely on NVS config. If the cloud drops, `network` handles backoff reconnects while `app` and `safety` maintain 100% operational flow.

---

## Cloud Contract (Vendor-Agnostic Model)

### Data Schema Changes (Currently Firebase RTDB)
```json
{
  "users": {
    "user_id_123": {
      "devices": {
        "SF-000001": true
      }
    }
  },
  "devices": {
    "SF-000001": {
      "metadata": {
        "firmwareVersion": "2.0.0",
        "hardwareVersion": "ESP32-WROOM-32",
        "protocolVersion": "1.0",
        "serialNumber": "SF-000001"
      },
      "status": {
        "lifecycle": "ONLINE",
        "uptimeSeconds": 3600
      },
      "telemetry": {
        "waterLevel": 85.0,
        "flowRate": 12.5
      },
      "settings": {
        "configVersion": 2,
        "tankHeight": 200,
        "lowThreshold": 20
      },
      "shadow": {
        "desired": {
          "pumpState": true,
          "mode": "AUTO"
        },
        "reported": {
          "pumpState": true,
          "mode": "AUTO"
        }
      },
      "diagnostics": {
        "freeHeap": 150000,
        "wifiRSSI": -65,
        "restartReason": "POWERON_RESET"
      },
      "events": {
        "evt_1715629199": {
          "timestamp": 1715629199,
          "severity": "ERROR",
          "category": "SAFETY",
          "code": "DRY_RUN",
          "message": "Dry run detected."
        }
      }
    }
  },
  "firmware": {
    "stable": {
      "2.0.0": "https://storage.googleapis.com/.../fw.bin"
    }
  }
}
```

---

## Success Criteria *(mandatory)*

### Measurable Outcomes
- **SC-001**: A user can provision and claim a new device securely via the Android app, establishing unique ownership.
- **SC-002**: Firmware successfully compiles cleanly with the new isolated directory structure (`hal/`, `core/`, `drivers/`, etc.) with zero cyclic dependencies.
- **SC-003**: The device operates continuously and enforces safety cutouts even if the internet connection is physically severed.
- **SC-004**: Command propagation uses Device Shadowing; race conditions between the Android app and device state are eliminated.
- **SC-005**: Device successfully downloads an OTA update, validates the hash, and commits the partition swap autonomously.

### Validation Commands
```bash
# Validate firmware embedded structure compilation
pio run -e esp32dev
```
