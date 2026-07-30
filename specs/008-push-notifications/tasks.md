# Tasks: App Push Notifications

**Branch**: `feature/push-notifications` | **Spec**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

**Modules**: Android App (`app/`), Cloud Functions (`functions/`)

---

## Phase 1: Shared Setup

**Purpose**: Any cross-module groundwork (schema design sign-off, new RTDB nodes, shared types)

- [x] T001 Review constitution gate — confirm all applicable principles pass
- [x] T002 [P] Confirm RTDB schema changes for `/devices/{deviceId}/fcmTokens` are purely additive and backward compatible.

**Checkpoint**: Constitution gate ✅ — implementation phases may begin

---

## Phase 3: Android App

**Build**: `.\gradlew.bat assembleDebug` in root directory.

### User Story 3 — Notification Permissions & Setup (P1) — Android

- [x] T030 [APP] [US3] Add `POST_NOTIFICATIONS` permission and ensure `SmartFlowMessagingService` is registered in `app/src/main/AndroidManifest.xml`
- [x] T031 [APP] [US3] Implement runtime permission request for Android 13+ in `app/src/main/java/com/smartflow/MainActivity.kt`
- [x] T032 [APP] [US3] Add `registerFcmToken(token: String)` function in `app/src/main/java/com/smartflow/data/repository/FirebaseDeviceRepository.kt`
- [x] T033 [APP] [US3] Update `onNewToken` to save tokens via proper path in `app/src/main/java/com/smartflow/service/SmartFlowMessagingService.kt`

**Checkpoint**: App compiles, requests permission on Android 13+, and writes FCM token to `/devices/{deviceId}/fcmTokens`.

---

## Phase 4: Cloud Functions

**Build**: `cd functions && npm run build`
**Test**: `cd functions && npm test`

### User Story 1 — Critical Error Alerts (P1) — Functions

- [x] T040 [FN] [US1] Create RTDB trigger `onValueCreate` for `/devices/{deviceId}/events/{eventId}` in `functions/src/index.ts`
- [x] T041 [FN] [US1] Implement payload dispatch for `dry_run_error` and `overflow_error` using `admin.messaging().sendEachForMulticast` in `functions/src/index.ts`

### User Story 2 — Countdown Completion (P2) — Functions

- [x] T042 [FN] [US2] Extend event trigger to dispatch payload for `countdown_finished` events in `functions/src/index.ts`

**Checkpoint**: `tsc` clean; functions correctly parse RTDB events and trigger FCM.

---

## Phase 5: Integration & Documentation

**Purpose**: Cross-module validation and docs updates

- [ ] T050 End-to-end validation: Manually insert mock error and countdown events into RTDB and verify notification on an Android device.
- [ ] T051 Commit: `feat(app,functions): implement push notifications` — Conventional Commits format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 3 (Android App)**: Depends on Phase 1 ✅
- **Phase 4 (Functions)**: Depends on Phase 1 ✅; can run in parallel with Phase 3
- **Phase 5 (Integration)**: Depends on ALL module phases complete

### Within Each Module Phase

- Each user story complete and validated before starting next priority.

### Parallel Opportunities

- `[P]` tasks within a phase have no intra-phase dependencies and can run in parallel.
- Android App and Cloud Functions phases can run in parallel.

---

## Validation Summary

```bash
# Cloud Functions
cd functions && npm run build

# Android App
.\gradlew.bat assembleDebug
```

---

## Notes

- `[P]` = parallel-safe (no dependency on other `[P]` tasks in same phase)
- `[APP]` / `[FN]` = module tag for traceability
- `[US1]` / `[US2]` / `[US3]` = maps task to user story from spec
- Commit after each logical group, not just at phase end
- Stop at each ✅ checkpoint to validate independently before continuing
