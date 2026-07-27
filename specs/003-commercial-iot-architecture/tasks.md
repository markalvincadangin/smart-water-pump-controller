# Implementation Tasks: Epic 2 - Cloud Integration

**Feature**: Commercial IoT Architecture (Epic 2)
**Status**: Active

---

## Phase 1: Setup

**Goal**: Prepare the project for the new dependencies.

- [x] T001 Add `h2zero/NimBLE-Arduino` to `firmware/master_node/platformio.ini` dependencies.

---

## Phase 2: Foundational

**Goal**: Establish the state definitions required for cloud connectivity.

- [x] T002 Update `firmware/master_node/src/state/state.h` to include `UNCLAIMED`, `PROVISIONING`, and `ONLINE` lifecycle states.

---

## Phase 3: [US1] Device Lifecycle & Claiming

**Goal**: Implement BLE Provisioning and robust Wi-Fi management.

- [x] T003 [P] [US1] Create `firmware/master_node/src/network/wifi_manager.h` and `.cpp` to handle Wi-Fi connection, auto-reconnect, and RSSI tracking.
- [x] T004 [P] [US1] Create `firmware/master_node/src/network/ble_provisioning.h` and `.cpp` using NimBLE to expose GATT characteristics for SSID, Password, and ClaimToken.
- [x] T005 [US1] Update `firmware/master_node/src/core/lifecycle/bootloader.cpp` to check NVS for credentials and start `BleProvisioning` if missing.
- [x] T006 [US1] Update `firmware/master_node/src/main.cpp` to loop the `WifiManager` and `BleProvisioning` tasks based on lifecycle state.

---

## Phase 4: [US2] Device Shadow & Telemetry

**Goal**: Implement the new multi-tenant Cloud Data Model contracts.

- [x] T007 [P] [US2] Create `firmware/master_node/src/cloud/device_shadow.h` and `.cpp` to manage `desired` vs `reported` state and evaluate safety constraints.
- [x] T008 [P] [US2] Create `firmware/master_node/src/cloud/cloud_manager.h` and `.cpp` to handle Firebase RTDB init and sync to `/devices/<device_id>/...`.
- [x] T009 [US2] Update `firmware/master_node/src/main.cpp` to call `CloudManager::sync()` instead of legacy connectivity functions.

---

## Phase 5: Polish & Cleanup

**Goal**: Remove deprecated code and verify the system.

- [x] T010 [US2] Delete `firmware/master_node/src/connectivity/connectivity_cloud.h` and `.cpp`.
- [x] T011 [US2] Verify firmware compilation with `pio run` to ensure all legacy dependencies are removed and memory footprint is improved.

---

## Dependencies

- **[US1] Device Lifecycle & Claiming** must be completed before **[US2] Device Shadow & Telemetry** because the Cloud Manager needs an active Wi-Fi connection to sync to Firebase.

## Implementation Strategy

1. **MVP Scope**: Complete Phase 1-3 first to verify that the ESP32 can advertise over BLE and successfully connect to a Wi-Fi network before attempting Firebase integration.
2. **Incremental Delivery**: We will implement the `DeviceShadow` logic in isolation, mock its tests if necessary, and finally wire it up to `CloudManager` and `main.cpp`.

## Phase 6: Convergence

- [x] T012 Implement `pushMetadata` and `readSettings` in `CloudManager` per FR-006 (partial)
- [x] T013 Implement `pushDiagnostics` in `CloudManager` to capture Heap, RSSI, and Restart Reason per FR-009 (missing)
- [x] T014 Implement `pushEventLog` in `CloudManager` and integrate with `app_logger` to report errors to `/devices/<id>/events` per FR-010 (missing)

# Implementation Tasks: Epic 3 - Android Application

**Feature**: Commercial IoT Architecture (Epic 3)
**Status**: Active

---

## Phase 1: Setup & Archivation

**Goal**: Prepare the Android workspace and archive the legacy dashboard.

- [x] T020 Archive `dashboard/` to `archive/dashboard/` by moving the directory.
- [x] T021 Initialize basic Gradle Android Project structure in `app/` directory (build.gradle.kts, src/main/java).
- [x] T022 Configure Android project with Jetpack Compose, Material 3, and Kotlin Coroutines dependencies.
- [x] T023 Add Firebase Android SDK (BOM, auth, database) dependencies to `app/build.gradle.kts`.

---

## Phase 2: Foundational Data & Models

**Goal**: Establish MVVM Repositories and models for Cloud Sync.

- [x] T024 [P] [US1] Create `Device` data class and `DeviceShadow` model in `app/src/main/java/com/smartflow/data/`.
- [x] T025 [P] [US1] Implement `FirebaseCloudStore` to read/write to `/devices/<id>` in RTDB.
- [x] T026 [US1] Implement `DeviceRepository` wrapping `FirebaseCloudStore`.

---

## Phase 3: [US1] Device Lifecycle & Provisioning

**Goal**: Implement BLE Provisioning client flow.

- [x] T027 [US1] Implement `BleProvisioningClient` in `data/` using standard Android `BluetoothLeScanner` or `RxAndroidBle`.
- [x] T028 [US1] Create `ProvisioningViewModel` to orchestrate scanning for `SmartFlow-<ID>` and sending Wi-Fi credentials.
- [x] T029 [US1] Implement `ProvisioningScreen` (Compose UI) to enter SSID/Password and show provisioning progress.

---

## Phase 4: [US2] Device Shadow & Dashboard UI

**Goal**: Implement the App Dashboard to interact with Device Shadow.

- [x] T030 [US2] Create `DashboardViewModel` to stream telemetry and publish desired shadow states (pump state).
- [x] T031 [US2] Implement `DashboardScreen` (Compose UI) displaying telemetry (waterLevel, flowRate) and pump toggle.
- [x] T032 [US2] Implement `DeviceListScreen` to select claimed devices.
- [x] T033 [US2] Implement `LoginScreen` using Firebase Auth (Email/Password or Anonymous) to enter the app.

---

## Phase 5: Integration

**Goal**: Wire navigation and run Android App.

- [x] T034 Setup Compose Navigation in `MainActivity.kt` to route between Login -> DeviceList -> Provisioning/Dashboard.
- [x] T035 Verify Android project builds successfully via Gradle.

---

## Phase 6: Convergence

- [x] T036 [FW] Implement OTA Manager subsystem in `firmware/master_node/src/ota/` per FR-008 and SC-005 (missing)
- [x] T037 [FW] Implement Claim Token cryptographic generation and validation in firmware per FR-005 (partial)
- [x] T038 [DOC] Create `contracts/ble_provisioning.md` to document the BLE GATT UUIDs and payload formats per SC-001 (missing)
