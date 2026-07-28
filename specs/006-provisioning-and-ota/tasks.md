# Tasks: Provisioning & Wireless Flashing

**Branch**: `feature/provisioning-and-ota` | **Spec**: [spec.md](file:///c:/Users/markc/_Projects/micro-controller/smartflow/specs/006-provisioning-and-ota/spec.md) | **Plan**: [plan.md](file:///c:/Users/markc/_Projects/micro-controller/smartflow/specs/006-provisioning-and-ota/plan.md)

**Modules**: Android App, Firmware (master_node)

---

## Phase 1: Shared Setup

**Purpose**: Project initialization and constitution checks.

- [x] T001 Review constitution gate — confirm all applicable principles pass
- [x] T002 [P] Read current behavior in affected files (no code changes)

**Checkpoint**: Constitution gate ✅ — implementation phases may begin

---

## Phase 2: Firmware

**Build**: `pio run` in `firmware/master_node/`
**Validation**: Hardware serial monitor

### User Story 2 — Wireless Firmware Upload in Dev Environment (P2)

- [x] T010 [US2] Update `firmware/master_node/platformio.ini` to add `[env:esp32dev_ota]` environment with `upload_protocol = espota` and `build_flags = -D ENABLE_OTA`.
- [x] T011 [US2] Update `firmware/master_node/src/main.cpp` to include `<ArduinoOTA.h>` wrapped in `#ifdef ENABLE_OTA`.
- [x] T012 [US2] Update `firmware/master_node/src/main.cpp` to call `ArduinoOTA.begin()` in setup phase when Wi-Fi connects.
- [x] T013 [US2] Update `firmware/master_node/src/main.cpp` to call `ArduinoOTA.handle()` in the main `loop()`, wrapped in `#ifdef ENABLE_OTA`.

**Checkpoint**: Firmware builds clean; OTA works in `esp32dev_ota` environment.

---

## Phase 3: Android App

**Build**: `.\gradlew.bat assembleDebug`

### User Story 1 — Route to Provisioning (P1)

- [x] T020 [US1] Update `app/src/main/java/com/smartflow/MainActivity.kt` navigation logic to dynamically fetch devices from Firebase/DeviceRepository instead of the mock list.
- [x] T021 [US1] Update `app/src/main/java/com/smartflow/MainActivity.kt` to route the user automatically to the `provisioning` route if the user's claimed device list is empty.
- [x] T022 [US1] Clean up `app/src/main/java/com/smartflow/presentation/DeviceListScreen.kt` to remove mock references if applicable.

**Checkpoint**: App builds clean, auto-routes to provisioning when no devices are claimed.

---

## Phase 5: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [x] T050 End-to-end validation: Verify firmware OTA config builds and Android App routes correctly.
- [x] T051 Commit: `feat(provisioning): enable dev OTA and dynamic app routing` — Conventional Commits format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Firmware)**: Depends on Phase 1 ✅
- **Phase 3 (Android App)**: Depends on Phase 1 ✅; can run in parallel with Phase 2
- **Phase 5 (Integration)**: Depends on ALL module phases complete

### Parallel Opportunities

- `[P]` tasks within a phase have no intra-phase dependencies and can run in parallel
- App and Firmware phases can run in parallel once Phase 1 is done
