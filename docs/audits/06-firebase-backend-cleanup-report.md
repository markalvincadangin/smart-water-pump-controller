# WP-06E — Backend Runtime Verification & Contract Cleanup

**Status:** Implemented — code/test updates complete; local runtime verification required.

## Scope

WP-06E closes the remaining backend contract gaps identified after WP-06D:

1. strengthen automated coverage around the RTDB safety-validation contract;
2. publish `last_fault_code` from the ESP32 reported shadow;
3. formally deprecate device-level FCM token storage.

## Changes

### RTDB validation tests

`functions/src/__tests__/rtdb_rules.test.ts` now checks the presence and required bounds of:

- pump start level: 0–100
- pump stop level: 0–100 and greater than start
- dry-run threshold: 0.1–10.0 L/min
- maximum runtime: 30–120 min
- desired mode: MANUAL, COUNTDOWN, AUTO
- countdown duration: 1–120 min

These tests verify that the checked-in rules contain the v1.1 contract constraints.

**Important limitation:** the current Functions test setup does not include the Firebase Emulator or a rules-testing SDK. Therefore these are contract/static rule checks, not live allow/deny emulator tests. Live invalid-write rejection must be verified with the Firebase Emulator before production deployment.

### Firmware reported shadow

Updated `firmware/master_node/src/cloud/device_shadow.cpp` so `/shadow/reported` includes:

```json
"last_fault_code": "<machine-readable code>"
```

The value is included in the reported-state cache comparison so changes to the fault code trigger a refreshed shadow payload.

This aligns the firmware with the v1.1 contract without renaming the existing internal `isOverflowError` state.

### FCM token deprecation

The system contract now formally states:

- canonical authority: `users/{uid}/notification_prefs/fcmTokens/{tokenId}`
- deprecated path: `devices/{deviceId}/fcmTokens`
- new Android/backend code must not read or write the deprecated device-level path.

Existing Android writers are intentionally **not removed in WP-06E** because that migration belongs to the Android notification cleanup package. This prevents backend and Android changes from being mixed prematurely.

## Verification boundary

WP-06D already established:

- 27 Jest tests passing;
- TypeScript build passing.

WP-06E adds static coverage and firmware changes, but this environment has not independently compiled the ESP32 firmware or run Firebase Emulator rules tests.

Required local verification before production backend deployment:

```bash
cd functions
npm test -- --runInBand
npm run build
```

Then run Firebase Emulator rules tests once the repository has an emulator-based test harness.

For firmware:

- compile the PlatformIO master-node target;
- verify reported shadow JSON contains `last_fault_code`;
- verify fault-code changes invalidate the reported-state cache.

## Result

WP-06E resolves the known contract gaps without prematurely modifying Android notification code.

The remaining Firebase/backend work is now primarily **runtime/deployment verification**, while the canonical FCM token migration can proceed as part of the Android audit.
