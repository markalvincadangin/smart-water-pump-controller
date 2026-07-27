# Implementation Tasks: Feature 004 — System Integration & Environment Standardization

**Feature**: SmartFlow System Integration & Environment Standardization
**Branch**: `004-system-integration`
**Status**: Active

---

## Phase 1: Setup

**Goal**: Prepare credential scaffolding and gitignore updates before any integration code is written.

- [x] T001 Add `app/google-services.json` to `.gitignore` (append under "Environment and secrets" section)
- [x] T002 Create `app/google-services.json.example` with placeholder structure (projectId, appId, apiKey fields, no real values)
- [x] T003 Create `functions/.env.example` listing required variables: `RESEND_API_KEY`, `RESEND_FROM_EMAIL`

---

## Phase 2: Foundational

**Goal**: Deploy and validate the V2 Firebase RTDB Security Rules — all subsequent integration depends on correct rules.

- [x] T004 Rewrite `database.rules.json` with V2 multi-tenant rules per `data-model.md` and clarifications. Enforce owner-based `/users/{uid}/devices/{id}` ownership. Firmware writes to reported/telemetry/diagnostics/events/status MUST be validated via `auth.uid === data.child('metadata/firmwareUid').val()`.
- [x] T005 Deploy rules to Firebase: `firebase deploy --only database` and confirm `✔ Deploy complete!`
- [x] T006 Validate rules against US3 scenarios using Firebase Emulator UI: (a) owner write to `shadow/desired` → ALLOWED, (b) non-owner write → DENIED, (c) unauthenticated read → DENIED

---

## Phase 3: [US1] First-Time Device Provisioning

**Goal**: Full BLE provisioning flow — firmware advertises, Android app scans, credentials written, firmware signs in anonymously and connects, device appears ONLINE in Firebase.

**Independent test criteria**: Device lifecycle changes from UNCLAIMED → ONLINE in Firebase RTDB, and the firmwareUid is correctly recorded.

- [x] T007 [US1] Update `firmware/master_node/src/network/ble_provisioning.h` and `.cpp` to add a new "Status" GATT characteristic (`CHAR_STATUS_UUID`) for reporting Wi-Fi/Auth connection states back to the Android app.
- [x] T008 [US1] Update `firmware/master_node/src/cloud/cloud_manager.cpp` to implement Firebase Anonymous Authentication on first boot. The returned anonymous UID MUST be saved to NVS and written to `/devices/{deviceId}/metadata/firmwareUid`.
- [x] T009 [US1] Populate `firmware/master_node/src/config/secrets.h` with live Firebase API_KEY and DATABASE_URL (no email/password needed anymore due to Anonymous Auth). Verify firmware compiles: `pio run -e esp32dev`
- [x] T010 [P] [US1] Place real `app/google-services.json` into `app/` directory (downloaded from Firebase Console).
- [x] T011 [P] [US1] Implement runtime BLE permission request in `app/src/main/java/com/smartflow/presentation/ProvisioningScreen.kt` using `rememberLauncherForActivityResult` (BLUETOOTH_SCAN, BLUETOOTH_CONNECT, ACCESS_FINE_LOCATION).
- [x] T012 [US1] Update `app/src/main/java/com/smartflow/data/BleProvisioningClient.kt` to subscribe to the new GATT Status characteristic to provide real-time connection feedback to the user (e.g., "Incorrect Wi-Fi Password").
- [x] T013 [US1] Update `app/src/main/java/com/smartflow/data/FirebaseCloudStore.kt` to write device claim on successful provisioning: set `/users/{uid}/devices/{deviceId}: true` and `/devices/{deviceId}/metadata/claimedByUid: {uid}`.
- [x] T014 [US1] Flash firmware and run full provisioning flow per `quickstart.md`, confirm `/devices/{id}/status/lifecycle = "ONLINE"` and `metadata/firmwareUid` is populated in Firebase.

---

## Phase 4: [US2] Device Control — Shadow Round-Trip

**Goal**: App toggle → Firebase desired → Firmware relay → Firebase reported → App UI reflects actual state.

**Independent test criteria**: Writing `shadow/desired/pumpState: true` from the App causes `shadow/reported/pumpState: true` to appear in Firebase within 3 seconds.

- [x] T015 [US2] Verify `app/src/main/java/com/smartflow/data/FirebaseCloudStore.kt` writes to `/devices/{id}/shadow/desired/pumpState` on pump toggle (match V2 data model).
- [x] T016 [US2] Verify `firmware/master_node/src/cloud/cloud_manager.cpp` reads `shadow/desired` and writes `shadow/reported` after applying safety checks via `SafetyPump`.
- [x] T017 [US2] Validate shadow round-trip: toggle pump ON from app, confirm relay actuation on device, confirm `reported/pumpState = true` in Firebase within 5 seconds.
- [x] T018 [US2] Validate dry-run rejection: with pump dry-run lockout active, send `pumpState: true` from app, confirm pump stays OFF and Firebase `events/` has entry with `severity: ERROR, code: DRY_RUN`.

---

## Phase 5: [US3] Security — Unauthorized Access Rejection

**Goal**: Firebase rules enforce ownership so unauthorized users and unauthenticated clients are rejected.

**Independent test criteria**: All three unauthorized access scenarios from US3 return PERMISSION_DENIED from Firebase.

- [x] T019 [US3] Run Firebase Emulator smoke test (`firebase emulators:start --import=<rtdb-export>.json`) and verify rule scenarios — document pass/fail for each of the 3 scenarios.
- [x] T020 [US3] Create `specs/004-system-integration/checklists/security-rules-validation.md` documenting test results from T019 with ✅/❌ per scenario.

---

## Phase 6: [US4] Observability — Diagnostics & Events in App

**Goal**: DashboardScreen shows live diagnostics (freeHeap, RSSI, restartReason) and the event log from Firebase.

**Independent test criteria**: Dashboard shows `freeHeap`, `wifiRSSI`, and `restartReason` updated within the last 60 seconds.

- [x] T021 [P] [US4] Add `DeviceDiagnostics` data class to `app/src/main/java/com/smartflow/data/Device.kt` with fields: `freeHeap: Long`, `wifiRSSI: Int`, `restartReason: String`.
- [x] T022 [P] [US4] Update `app/src/main/java/com/smartflow/data/FirebaseCloudStore.kt` to add `streamDiagnostics(deviceId)` and `streamEvents(deviceId)` flows.
- [x] T023 [US4] Update `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt` to collect `diagnostics` and `events` StateFlows from `DeviceRepository`.
- [x] T024 [US4] Update `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt` to add a collapsible "Diagnostics" section and an "Events" section.

---

## Phase 7: Factory Reset & Data Privacy (FR-007)

**Goal**: User-initiated factory reset clears NVS on firmware, unclaims in Firebase, and triggers a Cloud Function to wipe historical data.

- [x] T025 [P] Add BLE Factory Reset characteristic `CHAR_RESET_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26ac"` to `firmware/master_node/src/network/ble_provisioning.h` and `.cpp` — on write of `"RESET"`: call `nvs_flash_erase()` then `esp_restart()`.
- [x] T026 [P] Add `unclaimDevice(uid: String, deviceId: String)` to `app/src/main/java/com/smartflow/data/FirebaseCloudStore.kt` — removes `/users/{uid}/devices/{deviceId}` node.
- [x] T027 Add `factoryReset(deviceId: String)` to `app/src/main/java/com/smartflow/viewmodel/DashboardViewModel.kt` — writes "RESET" to BLE then calls `repo.unclaimDevice()`.
- [x] T028 Add "Factory Reset" button to `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt` with confirmation `AlertDialog`.
- [x] T029 Create a new Cloud Function in `functions/src/index.ts`: `onDeviceUnclaimed`. Triggers when `/users/{uid}/devices/{deviceId}` is deleted, and hard-deletes the entire `/devices/{deviceId}` node to ensure data privacy.
- [x] T030 Validate Factory Reset: device reboots and re-advertises via BLE, and the `/devices/{deviceId}` node completely disappears from RTDB.

---

## Phase 8: Cloud Functions V2 Compatibility (FR-009)

**Goal**: Update existing Cloud Functions trigger paths from V1 `pump_system/` to V2 `/devices/{id}/status`.

- [x] T031 Update `functions/src/index.ts` trigger path from `pump_system/status` → `devices/{deviceId}/status` and update data-read paths to use V2 schema (`status.is_running` → `shadow/reported/pumpState`, etc.).
- [x] T032 Update `functions/src/index.ts` notification config read path from `pump_system/config/notifications_by_user/{uid}` → `users/{uid}/notification_prefs`.
- [x] T033 Build and deploy functions: `cd functions && npm run build && firebase deploy --only functions`.

---

## Phase 9: Developer Setup Guide & Smoke Test (FR-004, FR-010)

**Goal**: Document and validate the complete developer onboarding experience.

- [x] T034 Validate `specs/004-system-integration/quickstart.md` end-to-end on a clean machine by following all steps.
- [x] T035 Create `docs/setup/environment-setup.md` as the permanent developer reference including all three credential setup procedures.

---

## Dependencies

```
T001-T003 (Setup) → T004-T006 (Rules) → T007-T014 (US1 Provisioning)
                                       → T015-T018 (US2 Shadow)  [requires US1]
                                       → T019-T020 (US3 Security) [requires T004-T006]
                                       → T021-T024 (US4 Diagnostics) [requires US1]
T025-T030 (Factory Reset) → requires US1
T031-T033 (Functions) → parallelizable with US1-US4
T034-T035 (Docs) → final, after all above
```

---

## Phase 10: Convergence

**Generated by**: `/speckit-converge` — 2026-07-27
**Findings**: 4 gaps (HIGH × 2, MEDIUM × 2). Ordered HIGH first.

- [x] T036 Add `sendFactoryReset(macAddress: String)` method to `app/src/main/java/com/smartflow/data/BleProvisioningClient.kt` — writes `"RESET"` to `RESET_CHAR_UUID` (`beb5483e-36e1-4688-b7f5-ea07361b26ad`); update `DashboardViewModel.factoryReset()` to call BLE write before calling `unclaimDevice()` per FR-007 (partial).
- [x] T037 Add `AlertDialog` confirmation gate to the Factory Reset button in `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt` — present dialog with "Are you sure? This will erase all device data." before calling `viewModel.factoryReset()` per FR-007 / T028 (partial).
- [x] T038 Update event log rows in `app/src/main/java/com/smartflow/presentation/DashboardScreen.kt` to display `code` and `category` fields alongside `severity`, `message`, and `timestamp` — spec US4/AC2 requires all structured event fields to be visible (partial).
- [x] T039 Remove stale Email/Password auth note from `specs/004-system-integration/data-model.md` line 168 — replace with Anonymous Auth description to align with FR-003 clarification and actual implementation (contradicts).
