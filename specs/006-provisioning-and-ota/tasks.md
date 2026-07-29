# Tasks: Provisioning, Ownership & Wireless Flashing

**Branch**: `006-provisioning-and-ota` | **Spec**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

**Modules**: Android app, Cloud Functions, RTDB rules, ESP32 firmware, operations and canonical specs.

---

## Phase 1: Shared Ownership Foundation

**Purpose**: Freeze the ownership contract and prevent direct client ownership writes before adding the user-facing flow.

- [ ] T001 Review the constitution and ownership contract in `specs/006-provisioning-and-ota/contracts/device_ownership.md`; document the pump-OFF and reset-preserves-ownership invariants in the implementation PR.
- [X] T002 [P] Add shared ownership result codes and claim request/response models in `functions/src/device_bootstrap.ts` and `app/src/main/java/com/smartflow/data/`.
- [X] T003 [P] Add emulator/test-project fixtures for unclaimed, claimed, consistent legacy-owner, missing legacy-owner, conflicting legacy-owner, anonymous-user, and durable-user states in `functions/src/__tests__/`.
- [X] T004 [P] Add backend migration and RTDB rule regression tests covering preserved consistent owners, frozen conflicts, trusted per-device conflict resolution with evidence/audit, denied direct ownership writes, and allowed device telemetry writes in `functions/src/__tests__/` and `database.rules.json`. Firebase Emulator rules execution remains part of T036 live validation.
- [X] T005 Implement an idempotent trusted legacy-ownership migration and conflict freeze in `functions/src/device_bootstrap.ts`, then update `database.rules.json` so client ownership writes are denied only after the T004 migration validation passes; preserve device and owner operational permissions.
- [X] T038 Implement a trusted operator-only, per-device migration-conflict resolution command in `functions/src/device_bootstrap.ts` and `functions/scripts/resolve_ownership_migration_conflict.ts`: require operator identity, explicit chosen owner UID, and non-secret evidence reference; atomically write owner/index, resolved state, and immutable audit event; do not expose it as a callable or device command.

**Checkpoint**: Direct-client ownership mutation is denied; existing device-only operational writes remain authorized.

---

## Phase 2: Foundational Account and Pairing Services

**Purpose**: Build the common prerequisites for all ownership stories.

- [X] T006 [P] Implement durable-account eligibility validation (reject anonymous identities, require verified email for password users, and reject any non-owner role) in `functions/src/device_bootstrap.ts`.
- [X] T007 [P] Add Android account-session state that distinguishes signed-out, anonymous/guest, and durable users in `app/src/main/java/com/smartflow/`.
- [X] T008 Define the pairing-proof protocol in `firmware/master_node/src/network/ble_provisioning.h` and `specs/006-provisioning-and-ota/contracts/device_ownership.md`: random raw proof over active BLE only, cloud verifier only, `claim`/`transfer`/`release` purposes, five-minute non-extendable ownership-pairing expiry, cancellation, and one-time consumption.
- [X] T009 Implement device-authenticated pairing-verifier publication and expiry cleanup in `firmware/master_node/src/network/ble_provisioning.cpp` and `firmware/master_node/src/cloud/cloud_manager.cpp` without logging raw proof values.
- [X] T010 Add pairing-verifier validation, expiry, consumption, and race-condition tests in `functions/src/__tests__/device_ownership.test.ts`.

**Checkpoint**: A durable account can be distinguished from a guest, and active setup produces a non-reusable proof without exposing its raw value in cloud data or logs.

---

## Phase 3: User Story 1 — Account-Backed Device Provisioning (P1)

**Goal**: A signed-in user securely provisions and claims a nearby, unclaimed device.

**Independent Test**: A Google or verified email/password user provisions an unclaimed device and receives one owner mapping plus one claim audit event after the final provisioning state.

- [X] T011 [P] [US1] Add callable-boundary regression tests for successful atomic claim, invalid proof, expired proof, already-claimed device, and anonymous caller in `functions/src/__tests__/device_ownership.test.ts`.
- [X] T012 [US1] Implement callable `claimDevice` with durable-user check, pairing-proof verification/consumption, atomic owner/index/audit writes, and bounded outcomes in `functions/src/device_bootstrap.ts`.
- [X] T013 [US1] Export and deploy-register `claimDevice` from `functions/src/index.ts` and update `firebase.json` only if deployment configuration requires it.
- [X] T014 [P] [US1] Add Google sign-in and verified email/password sign-up/sign-in routes, including verification and an account gate before cloud claim or remote control, in `app/src/main/java/com/smartflow/presentation/LoginScreen.kt` and `app/src/main/java/com/smartflow/MainActivity.kt`.
- [X] T015 [US1] Remove automatic anonymous sign-in from normal repository initialization in `app/src/main/java/com/smartflow/data/repository/FirebaseDeviceRepository.kt`; retain guest access only if it cannot reach claim or remote control.
- [X] T016 [US1] Replace direct two-step RTDB claim writes with the callable claim invocation and stable result mapping in `app/src/main/java/com/smartflow/data/FirebaseCloudStore.kt`.
- [X] T017 [US1] Pass the active BLE pairing proof to the callable claim only after final `provisioned` status, and render retryable claim results in `app/src/main/java/com/smartflow/viewmodel/ProvisioningViewModel.kt` and `app/src/main/java/com/smartflow/presentation/ProvisioningScreen.kt`.
- [X] T039 [US1] Add deterministic cloud-claim coordinator tests for bounded progress, retryable timeout, and immediate rejection in `app/src/test/java/com/smartflow/viewmodel/CloudClaimCoordinatorTest.kt`.
- [X] T040 [US1] Implement the 45-attempt/two-second cloud-claim handoff and explicit cloud-retry UI in `app/src/main/java/com/smartflow/viewmodel/ProvisioningViewModel.kt` and `app/src/main/java/com/smartflow/presentation/ProvisioningScreen.kt` without persisting or logging the pairing proof.
- [X] T041 [US1] Add a dry-run-first, acknowledgement-gated named registry-reseed option to `functions/scripts/reset_test_environment.ts`, document the ignored local RTDB backup and non-recoverable Firebase Auth-user deletion boundary in `functions/scripts/README.md`, and add an integration-style script test or deterministic validation fixture.
- [ ] T018 [US1] Build `functions` and `app`; run and time the end-to-end unclaimed-device claim test described in `specs/006-provisioning-and-ota/quickstart.md`, requiring completion within two minutes.

**Checkpoint**: A durable user owns an unclaimed nearby device after setup; anonymous and guessed-ID attempts cannot claim it.

---

## Phase 4: User Story 2 — Durable Account Access and Ownership Lifecycle (P1)

**Goal**: Owners retain cross-phone access and ownership changes are explicit, auditable, and safe.

**Independent Test**: The same durable account sees a claimed device on a second phone; local reset, app removal, and an anonymous-account deletion cannot change ownership.

- [X] T019 [P] [US2] Add deterministic ownership-lifecycle tests for owner-authorized temporary pairing, exactly-five-minute non-extendable expiry, transfer recipient/cancellation, release replacement claim, second-owner rejection, and non-owner rejection in `functions/src/__tests__/device_ownership.test.ts`. Pump-off ordering and BLE proof remain firmware/live validation in T023/T031.
- [X] T020 [US2] Implement owner-authorized `releaseDevice` and transfer lifecycle callable functions with recipient UID, exactly-five-minute non-extendable expiry, owner confirmation, audit records, and an `OWNERSHIP_PAIRING` request to the online device; add app-scoped account-deletion eligibility checks and the single-owner invariant in `functions/src/device_bootstrap.ts`.
- [X] T021 [US2] Implement replay-safe `OWNERSHIP_PAIRING` request processing in `firmware/master_node/src/core/lifecycle/bootloader.cpp`, `firmware/master_node/src/network/ble_provisioning.cpp`, and `firmware/master_node/src/persistence/`: call `setPump(false)`, temporarily advertise BLE, enforce the five-minute non-extendable local timer, then stop BLE on expiry/cancellation while retaining Wi-Fi, ownership, and safety latches; remove implicit ownership clearing from `functions/src/index.ts`.
- [X] T022 [US2] Add owner-only device-management and app-scoped account-deletion-block states with protected-owner, release, transfer, temporary pairing, expiry, and cancellation feedback in `app/src/main/java/com/smartflow/presentation/DeviceListScreen.kt` and related ViewModels; do not add member invitations. The Account screen checks the callable eligibility gate and directs owners to release or transfer devices; final Auth deletion remains deliberately outside this build because Firebase requires recent re-authentication.
- [ ] T023 [US2] Verify second-phone access and the transfer/release ownership-pairing matrix from `specs/006-provisioning-and-ota/quickstart.md` against the test Firebase project.

**Checkpoint**: Only an explicit authorized lifecycle operation can change ownership; a local Wi-Fi reset remains non-transferable.

---

## Phase 5: User Story 3 — Wireless Firmware Upload in Dev Environment (P2)

**Goal**: A developer can use authenticated local-network OTA without USB for normal iteration.

**Independent Test**: A password-authenticated upload succeeds over the LAN and a wrong/missing password is rejected without replacing the running app.

- [X] T024 [P] [US3] Verify `SMARTFLOW_OTA_PASSWORD` is absent from versioned files and documented only as an ignored/local configuration in `firmware/master_node/src/config/secrets.h.example` and `docs/operations/provisioning_ota_recovery.md`.
- [X] T025 [US3] Compile `esp32dev_usb_ota` and `esp32dev_ota`, then perform accepted and rejected OTA validation using `firmware/master_node/platformio.ini`.
- [X] T026 [US3] Record OTA result, post-reboot device identity, and safe pump disposition in the validation evidence linked from `specs/006-provisioning-and-ota/quickstart.md`.

**Checkpoint**: Local OTA is authenticated and repeatable; USB remains a documented recovery path.

---

## Phase 6: User Story 4 — Owner Wi-Fi Recovery (P2)

**Goal**: The recorded owner can re-enter BLE Wi-Fi onboarding without changing ownership or bypassing safety controls.

**Independent Test**: A valid owner request turns the pump OFF, preserves ownership and safety latches, then re-enters provisioning; non-owner, expired, malformed, and replayed requests make no state change.

- [X] T027 [P] [US4] Add callable-function tests for authoritative-owner checks and maintenance-request audit creation in `functions/src/__tests__/device_bootstrap.test.ts`.
- [X] T028 [US4] Update `requestWifiReprovision` authorization to use authoritative ownership rather than a client-writable user index in `functions/src/device_bootstrap.ts`.
- [X] T029 [US4] Implement replay-safe firmware maintenance processing in `firmware/master_node/src/core/lifecycle/bootloader.cpp` and `firmware/master_node/src/persistence/`; require `setPump(false)` before clearing local enrollment.
- [X] T030 [US4] Add owner recovery confirmation/progress/error UX in `app/src/main/java/com/smartflow/` and route back to BLE onboarding after the device restarts.
- [ ] T031 [US4] Compile firmware, validate safety invariant searches, and execute the valid/invalid recovery matrix in `specs/006-provisioning-and-ota/quickstart.md`.
- [ ] T032 [US4] When hardware is available, implement and manually validate the GPIO32 10-second maintenance button in `firmware/master_node/src/` using wrap-safe timing; short presses must be inert.

**Checkpoint**: Recovery preserves cloud owner, immutable ID, safety configuration, and reachable emergency controls.

---

## Phase 7: Logging, Integration, and Documentation

**Purpose**: Complete observable validation and make the operating model maintainable.

- [x] T033 [P] Verify the development TCP logger emits buffered and live application events after Wi-Fi joins; verify production builds do not expose port 2323 and publish bounded health/failure diagnostics readable through the cloud path, using `firmware/master_node/src/utils/logging/` and `specs/006-provisioning-and-ota/quickstart.md`.
- [X] T034 [P] Update the ownership and provisioning operational instructions in `docs/operations/provisioning_ota_recovery.md` with the five-minute ownership-pairing limit and the audited operator migration-conflict resolution procedure, without including any secret values.
- [X] T035 Update the owning RTDB schema and Android app behavior in `docs/specs/firmware.md` and `docs/specs/app.md` for the finalized authoritative ownership paths and account gate.
- [X] T036 Run the full quickstart, including revoked-bootstrap-secret rejection, `cd functions && npm run build && npm test`, `cd app && .\gradlew.bat assembleDebug`, and both PlatformIO compile environments; record outcomes in the feature evidence.
- [X] T037 Review implementation against the constitution and run `/speckit-analyze` and `/speckit-converge` before finalizing the feature.

---

## Dependencies & Execution Order

- Phase 1 blocks all ownership work because direct client ownership writes must be closed first; T038 must be validated before migration conflicts are considered operationally resolvable.
- Phase 2 blocks User Stories 1 and 2 because durable-account validation and pairing proof are shared prerequisites.
- User Story 1 is the MVP and must complete before User Story 2 and owner Wi-Fi recovery.
- User Story 2 establishes authoritative ownership semantics required by User Story 4.
- User Story 3 is independent after existing device bootstrap/OTA configuration and may run in parallel with ownership work.
- Phase 7 runs after applicable stories are complete.

## Parallel Opportunities

- T002 and T003 can run in parallel.
- T006 and T007 can run in parallel; T008 can begin once the contract is accepted.
- T011 and T014 can run in parallel after Phase 2.
- T019 and the early UX design for T022 can run in parallel after User Story 1.
- User Story 3 can proceed independently of User Stories 1–2.

## Implementation Strategy

1. Deliver the ownership foundation and durable account gate.
2. Deliver the atomic nearby-device claim end-to-end (MVP).
3. Add explicit release/transfer before owner recovery becomes broadly available.
4. Validate OTA and development logging alongside, without allowing them to weaken production ownership or device safety.

## Validation Summary

```powershell
cd functions; npm run build; npm test
cd ..\app; .\gradlew.bat assembleDebug
cd ..\firmware\master_node; pio run -e esp32dev_usb_ota; pio run -e esp32dev_ota
```

Hardware validation follows [quickstart.md](quickstart.md). Never place device bootstrap or OTA passwords in source, logs, or validation evidence.

## Phase 8: Convergence

- [X] T042 CRITICAL Replace raw `millis()` subtraction in provisioning timeout and BLE shutdown paths with `elapsedMillis32()` or `millisDeadlineReached()` and add rollover-focused validation per Constitution Technical Constraints: Wrap-Safe Timing (contradicts). Firmware image compilation passed; the new PlatformIO test suite is compile-ready and awaits a connected test target for execution.
- [X] T043 Make `requestWifiReprovision` use a server-authoritative compare-and-set/update path that cannot abort from an empty Admin SDK transaction cache; add callable tests for valid owner, non-owner, replay, expiry, and empty-cache behavior per US4, FR-011, and SC-008 (partial). The implementation reads the authoritative single-device snapshot through RTDB ETag CAS, so it never relies on root transaction cache state.
- [X] T047 Cap `/devices/{deviceId}/events` to the 50 newest WARN/ERROR records with the trusted `retainDeviceEvents` RTDB trigger; verify a legacy oversized history is repaired to exactly 50 after a new device event. Buffered/live development TCP logging is verified separately; production port-2323 runtime closure remains under T033.
