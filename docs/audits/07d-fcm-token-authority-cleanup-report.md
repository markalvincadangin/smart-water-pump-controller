# WP-07D — FCM Token Authority Cleanup

**Status:** Implemented; runtime verification pending
**Branch:** `docs/smartflow-system-contract`
**Scope:** Android FCM token ownership and registration-path cleanup

## Contract

The canonical FCM token authority is:

`users/{uid}/notification_prefs/fcmTokens/{tokenId}`

The deprecated device-level path is:

`devices/{deviceId}/fcmTokens`

New Android code must not read or write the deprecated path.

## Implementation

### 1. Centralized token registration

Added `app/src/main/java/com/smartflow/service/FcmTokenRegistrar.kt`.

The helper resolves the currently signed-in Firebase user and writes the FCM token only beneath that user's `notification_prefs/fcmTokens` collection.

Token registration is deliberately separate from notification preference state. Registering or refreshing an FCM token does not force `notification_prefs/enabled` back to `true`.

### 2. Token refresh handling

Updated `SmartFlowMessagingService.onNewToken()` to delegate exclusively to `FcmTokenRegistrar`.

There is no device-level token write or enumeration of the user's devices.

### 3. Authentication lifecycle handling

Updated `MainActivity` to register the existing FCM token whenever an eligible Firebase auth state becomes signed in.

The auth listener is explicitly removed in `onDestroy()` to avoid retaining the activity after its lifecycle ends.

This covers the case where the FCM token already exists but the user signs in after application startup.

### 4. Deprecated repository API removed

Removed `registerFcmToken()` from `DeviceRepository` and `FirebaseDeviceRepository`.

The repository no longer contains a device-level FCM token writer.

## Source Audit

Before cleanup, three Android locations were writing the deprecated device path:

- `FirebaseDeviceRepository.kt`
- `MainActivity.kt`
- `SmartFlowMessagingService.kt`

After cleanup:

- `FcmTokenRegistrar.kt` writes only the canonical user path.
- `MainActivity.kt` delegates token registration to the helper.
- `SmartFlowMessagingService.kt` delegates token refresh to the helper.
- `FirebaseDeviceRepository.kt` contains no FCM token registration API.

Cloud Functions already consume `users/{uid}/notification_prefs/fcmTokens`, so the Android write authority now matches the backend read authority.

## Preference Semantics

Notification preference `enabled` remains controlled by notification settings rather than being treated as part of token registration. This prevents an FCM token refresh from silently overriding a user's notification preference.

## Verification

Run locally after pulling the WP-07D commits:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

Also perform the following behavioral checks on a signed-in Android build:

1. Sign in with an eligible account and confirm a token appears only under the user's `notification_prefs/fcmTokens` path.
2. Refresh/rotate the FCM token and confirm the new token is written only to the same user-level collection.
3. Disable notifications, then trigger token refresh; confirm `notification_prefs/enabled` is not silently changed.
4. Sign out and sign in as another account; confirm the current token is registered under the newly signed-in user's collection.

## Acceptance Criteria

- [x] Canonical user-level token path is the only Android registration path.
- [x] Deprecated device-level token writes removed from `FirebaseDeviceRepository`.
- [x] Deprecated device-level token writes removed from `MainActivity`.
- [x] Deprecated device-level token writes removed from `SmartFlowMessagingService`.
- [x] Token registration centralized in one helper.
- [x] Authentication lifecycle registration added for post-startup sign-in.
- [x] Token registration does not mutate notification preference enablement.
- [ ] Android compilation verified after WP-07D changes.
- [ ] Android unit tests verified after WP-07D changes.
- [ ] Live RTDB token-path behavior verified on a running signed-in Android build.