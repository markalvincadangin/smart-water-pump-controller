# WP-06E — Backend Runtime Verification & Contract Cleanup

**Status:** Implemented — code/test updates complete; local runtime verification required.

## Scope

WP-06E addresses the remaining backend contract gaps after WP-06D:

1. strengthen automated coverage around RTDB safety validation;
2. publish `last_fault_code` from the ESP32 reported shadow;
3. formally deprecate device-level FCM token storage.

## Changes

### RTDB validation tests

`functions/src/__tests__/rtdb_rules.test.ts` now checks the v1.1 constraints for:

- pump start level: 0–100
- pump stop level: 0–100 and greater than start
- dry-run threshold: 0.1–10.0 L/min
- maximum runtime: 30–120 min
- desired mode: MANUAL, COUNTDOWN, AUTO
- countdown duration: 1–120 min

These are static contract checks against the checked-in rules.

The current Functions test setup does not include the Firebase Emulator or a rules-testing SDK, so these are not live allow/deny emulator tests. Live invalid-write rejection remains required before production deployment.

### Firmware reported shadow

`firmware/master_node/src/cloud/device_shadow.cpp` now publishes:

```json
"last_fault_code": "<machine-readable code>"
```

The value is included in the reported-state cache comparison so a fault-code change refreshes the shadow payload.

### FCM token deprecation

The system contract now formally establishes:

- canonical authority: `users/{uid}/notification_prefs/fcmTokens/{tokenId}`
- deprecated path: `devices/{deviceId}/fcmTokens`
- new Android/backend code must not read or write the deprecated device-level path.

Existing Android writers are intentionally retained until the Android notification cleanup package migrates them.

## Verification boundary

WP-06D established 27 passing Jest tests and a successful TypeScript build.

WP-06E still requires:

```bash
cd functions
npm test -- --runInBand
npm run build
```

Firebase Emulator rules tests should be added/run before production deployment.

The ESP32 firmware also needs a PlatformIO compile and reported-shadow verification.

## Result

WP-06E resolves the known backend contract gaps without prematurely modifying Android notification code. Remaining backend work is runtime/deployment verification, while FCM token migration proceeds with the Android audit.
