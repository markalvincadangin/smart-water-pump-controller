# Implementation Plan: SmartFlow Commercial IoT Platform

**Branch**: `003-commercial-iot-architecture` | **Date**: 2026-07-27 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `specs/003-commercial-iot-architecture/spec.md`

**Note**: This plan defines the phased execution roadmap to transition SmartFlow to a commercial-grade embedded IoT architecture.

---

## Summary

This architecture transition moves SmartFlow from a monolithic MVP to a decoupled, multi-layer commercial IoT platform. It introduces a modular firmware HAL, device shadow syncing, BLE provisioning, robust OTA capabilities, a multi-tenant cloud schema, and an MVVM Android application.

---

## Constitution Gate *(must pass before Phase 0)*

| Principle | Status | Evidence |
|-----------|--------|----------|
| I. Fail Toward Pump OFF | ☑ Pass | The new HAL and modular firmware explicitly preserve the safety cutout guarantees regardless of cloud/network states. |
| II. Dry-Run Lockout | ☑ Pass | Device Shadow implementation treats safety locks as authoritative; cloud desired state cannot unlatch dry-run implicitly. |
| III. Overflow Protection | ☑ Pass | Overflow logic remains in the core firmware layer, unaffected by decoupled components. |
| IV. TOR Independence | ☑ Pass | Software cutouts remain secondary to physical TOR hardware. |
| V. Sensor Freshness / E-Stop | ☑ Pass | Provisioning (BLE) and OTA operations will not block E-Stop evaluation. |
| VI. Backward Compatibility | ☑ N/A | Feature explicitly defines a major breaking change (v2.0) dropping V1 backward compatibility. |

---

## Technical Context

**Modules affected**:
- [x] Firmware Master — `firmware/master_node/`
- [ ] Firmware Sensor Node — `firmware/sensor_node/`
- [x] Dashboard / Android App — `app/` (Replacing Dashboard with Android Compose)
- [x] Cloud Functions — `functions/` (TypeScript, Firebase Functions v7)
- [ ] Admin Scripts — `scripts/`
- [x] Docs only — `docs/` (Creating ADRs)

**Cloud data model impact**: Additive schema change (Full schema migration to `/devices/<id>/` structure).

**Hardware required for full validation**: Yes — requires physical ESP32 to validate HAL, BLE provisioning, and OTA partition swapping.

**Performance / timing constraints**: OTA flash writes and TLS handshakes must yield to the RTOS scheduler to avoid watchdog resets.

---

## Project Structure (this feature)

### Spec-Kit Artifacts

```text
specs/003-commercial-iot-architecture/
├── spec.md              # Feature specification
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output (Cloud data model schema)
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output (Device shadow & events contracts)
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code Paths

```text
# Architecture Decision Records
docs/adr/
├── 0001-deprecate-arduino-ide.md
├── 0002-hal.md
├── 0003-layered-firmware.md
├── 0004-device-shadow.md
├── 0005-ble-provisioning.md
├── 0006-compose.md
└── 0007-firebase-selection.md

# Firmware Master
firmware/master_node/src/
├── config/              # Constants, pin maps, compile flags
├── hal/                 # Hardware Abstraction Layer (e.g. PumpHal::enable())
├── drivers/             # Dumb peripherals (Pump, Sensors)
├── services/            # Domain logic (WaterLevelService, FlowService)
├── core/                # App orchestration, State machine, Lifecycle
├── safety/              # E-Stop, Dry-run, Overflow logic
├── network/             # Wi-Fi, BLE Provisioning (Future)
├── cloud/               # Cloud data model sync (Future)
└── ota/                 # OTA Manager (Future)

# Android Application (Future)
app/
├── presentation/        # Jetpack Compose UI
├── viewmodel/           # UI State Management
├── repository/          # Data Access layer
└── data/                # Cloud Sync
```

---

## Phase 0: Research & ADRs

**Purpose**: Document the baseline architecture decisions via ADRs to ensure future context is preserved.

- [ ] Review existing ADR `0001-deprecate-arduino-ide.md`
- [ ] Draft ADR `0002-hal.md`
- [ ] Draft ADR `0003-layered-firmware.md`
- [ ] Draft ADR `0004-device-shadow.md` (Deferred to Epic 2)
- [ ] Draft ADR `0005-ble-provisioning.md` (Deferred to Epic 2)
- [ ] Draft ADR `0006-compose.md` (Deferred to Epic 3)
- [ ] Draft ADR `0007-firebase-selection.md` (Deferred to Epic 2)
- [ ] Output: `research.md` summarizing the ADR conclusions.

---

## Phase 1: Epic 1 - Embedded Platform Foundation

**Purpose**: Refactor the ESP32 firmware into a strict embedded hierarchy without internet dependencies.

### Implementation Tasks
- [ ] Create `config/` and migrate all raw pin definitions and constants.
- [ ] Create `hal/` and migrate raw GPIO accesses into capability interfaces (`PumpHal`, `UltrasonicHal`).
- [ ] Create `drivers/` to house "dumb" drivers that only read sensors and control actuators via HAL.
- [ ] Create `services/` to transform raw driver data into meaningful logic (`WaterLevelService`, `FlowService`).
- [ ] Establish `core/app/` and `core/lifecycle/` to replace the main orchestrator (preventing a new monolith).
- [ ] Refactor `safety/` layer to interface strictly with `drivers/` and `services/`. It MUST NOT touch cloud or network logic.
- [ ] Simplify `main.cpp` to only initialize the hierarchy and loop.

### Definition of Done for Epic 1
- [ ] Firmware compiles successfully (`pio run -e esp32dev`).
- [ ] All hardware behavior matches the previous implementation (offline).
- [ ] Safety logic behaves identically or better.
- [ ] No functional regressions are introduced.
- [ ] `main.cpp` contains only initialization and orchestration.
- [ ] Hardware access occurs only through the HAL.
- [ ] No cloud or networking code exists in the embedded core.

### Architectural Rules (Static Check)
- [ ] `hal/` must not include `core/`, `services/`, `network/`, or `cloud/`.
- [ ] `drivers/` may depend on `hal/` only.
- [ ] `safety/` may depend on `drivers/` and `services/`, but not `cloud/`.
- [ ] `core/` must not directly access GPIO.

---

## Phase 2: Epic 2 - Cloud Integration (Future)

**Purpose**: Connect the firmware to the cloud using the new Device Shadow and Provisioning mechanics.

- [ ] Implement BLE Provisioning flow in `network/`.
- [ ] Implement Device Claiming logic.
- [ ] Implement Device Shadow sync (`desired` / `reported`) in `cloud/`.
- [ ] Deprecate direct RTDB writes for telemetry in favor of structured payloads.

---

## Phase 3: Epic 3 - Android Application (Future)

**Purpose**: Replace the Next.js dashboard with a native Jetpack Compose application.

- [ ] Scaffolding: Setup MVVM Android project.
- [ ] Implement BLE Provisioning client flow.
- [ ] Implement Dashboard UI bound to Device Shadow repository.
- [ ] Implement Device Settings and Claiming UI.

---

## Phase 4: Epic 4 - OTA & Diagnostics (Future)

**Purpose**: Implement fleet management capabilities.

- [ ] Implement OTA Manager in firmware (HTTPS download, SHA256 verification).
- [ ] Implement Dual-partition rollback mechanism.
- [ ] Implement rich diagnostic telemetry (Heap, CPU, RSSI).
- [ ] Implement structured event logging.

---

## Phase 5: Epic 5 - Production Hardening (Future)

**Purpose**: Reserve points for enterprise security features.

- [ ] Document integration points for Secure Boot V2.
- [ ] Document integration points for Flash Encryption.
- [ ] Document implementation requirements for Signed OTA and Hardware Root of Trust.
