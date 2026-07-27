# Tasks: Commercial IoT Architecture (Epic 1: Embedded Platform Foundation)

**Branch**: `003-commercial-iot-architecture` | **Spec**: [spec.md](./spec.md) | **Plan**: [plan.md](./plan.md)

**Modules**: Firmware Master (`firmware/master_node/`)

---

## Phase 1: Shared Setup & Documentation

**Purpose**: Document the baseline architecture decisions via ADRs to ensure future context is preserved.

- [x] T001 [P] Move existing ADR `docs/adr/0001-deprecate-arduino-ide.md`
- [x] T002 [P] Create ADR `docs/adr/0002-hal.md` detailing the Hardware Abstraction Layer approach
- [x] T003 [P] Create ADR `docs/adr/0003-layered-firmware.md` detailing the embedded hierarchy rules
- [x] T004 [P] Create ADR `docs/adr/0004-device-shadow.md` (Deferred to Epic 2 implementation)
- [x] T005 [P] Create ADR `docs/adr/0005-ble-provisioning.md` (Deferred to Epic 2 implementation)
- [x] T006 [P] Create ADR `docs/adr/0006-compose.md` (Deferred to Epic 3 implementation)
- [x] T007 [P] Create ADR `docs/adr/0007-firebase-selection.md` (Deferred to Epic 2 implementation)
- [x] T008 Initialize the new directory structure: `firmware/master_node/src/config/`, `hal/`, `drivers/`, `services/`, `core/app/`, `core/lifecycle/`

**Checkpoint**: Architecture Decision Records complete and directory skeleton exists ✅

---

## Phase 2: Firmware Refactoring (Epic 1)

**Build**: `cd firmware/master_node && pio run -e esp32dev`
**Validation**: Hardware serial monitor (verify offline functional parity)

### Foundational Configurations
- [x] T008 [P] Migrate hardware constants (`RELAY_PIN`, `RS485_TX_PIN`, etc.) into `firmware/master_node/src/config/hardware.h`
- [x] T009 [P] Migrate timing and feature constants into `firmware/master_node/src/config/constants.h`

### Hardware Abstraction Layer (HAL)
- [x] T010 [P] Create `firmware/master_node/src/hal/pump_hal.h` and `.cpp` for relay GPIO control
- [x] T011 [P] Create `firmware/master_node/src/hal/ultrasonic_hal.h` and `.cpp`
- [x] T012 [P] Create `firmware/master_node/src/hal/flow_meter_hal.h` and `.cpp`

### Dumb Drivers
- [x] T013 [P] Create `firmware/master_node/src/drivers/pump_driver.h` and `.cpp`
- [x] T014 [P] Create `firmware/master_node/src/drivers/sensor_driver.h` and `.cpp`

### Services & Domain Logic
- [x] T015 [P] Create `firmware/master_node/src/services/water_level_service.h` and `.cpp`

### Safety Layer Isolation
- [x] T016 [P] Migrate safety isolation boundary into `safety/` (ensure no direct cloud checks in `safety_pump.cpp`)

### Core Orchestration
- [x] T017 [P] Extract orchestration logic from `pump_app.cpp` to `firmware/master_node/src/core/app/pump_app.cpp`
- [x] T018 [P] Extract boot/setup sequence from `main.cpp` into `firmware/master_node/src/core/lifecycle/bootloader.cpp`

### Cleanup & Wiring
- [x] T019 [FW] Strip `firmware/master_node/src/main.cpp` to only initialize `hal`, `drivers`, `services`, `core`, and `safety`, and execute the super loop.
- [x] T020 [FW] Delete obsolete legacy files (e.g., `pump_app.cpp` if logic fully migrated to `core/`)

**Checkpoint**: Firmware builds cleanly (`pio run`). Safety guarantees preserved offline ✅

---

## Phase 3: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [ ] T021 Run static/manual dependency check: verify `hal/` does not include `core/`, `safety/`, or `services/` headers.
- [ ] T022 Run static/manual dependency check: verify `drivers/` relies only on `hal/` and standard libraries.
- [ ] T023 Update `docs/specs/firmware.md` to document the new layered architecture model.
- [ ] T024 Commit: `feat(firmware): refactor to strict layered embedded architecture (Epic 1)`

---

## Dependencies & Execution Order

### Phase Dependencies
- **Phase 1 (Setup)**: Start immediately
- **Phase 2 (Firmware)**: Depends on Phase 1
- **Phase 3 (Integration)**: Depends on Phase 2

### Parallel Opportunities
- ADR drafting (T001 - T006) can be done in parallel.

---

## Validation Summary

```bash
# Firmware (Master Node)
cd firmware/master_node && pio run -e esp32dev
```

## Phase 4: Convergence

- [ ] T025 [P] Refactor `safety/safety_pump.cpp` to remove `connectivity_cloud.h` dependency and `pushFirebaseErrorLog` calls per plan architectural rules (`contradicts`).
