# WP-10 N-FIX-06 — Foreground FCM Handling

## Status

**VERIFIED**

N-FIX-06 gives SmartFlow intentional foreground handling for FCM notification messages. Local tests on Android and Cloud Functions pass cleanly.

## Purpose

When the Android app is in the foreground, SmartFlow should not merely log an incoming FCM notification. The app now normalizes the message and presents an Android notification through the app-owned `pump_alerts` channel.

Cloud Functions remains responsible for notification delivery policy. Android is responsible for presentation after delivery.

## Implementation

### FCM payload normalization

Added `FcmNotificationPayload` to normalize:

- title
- body
- tag
- event code
- event ID
- device ID

Notification title/body can come from the FCM notification payload, with data-field fallback for robustness.

Malformed messages without a usable title or body are ignored safely.

### Foreground notification presentation

`SmartFlowMessagingService.onMessageReceived()` now:

1. Parses the delivered FCM message.
2. Logs the normalized event context.
3. Ensures the app-owned `pump_alerts` channel exists.
4. Builds a local notification using the delivered title/body.
5. Uses the SmartFlow notification icon.
6. Uses high local notification priority.
7. Opens `MainActivity` when the notification is tapped.

The tap destination is intentionally only the existing main activity in N-FIX-06. Complete device/event deep-link routing remains N-FIX-07.

### Backend event context

Authoritative safety-event pushes now include:

- `eventCode`
- `eventId`
- `deviceId`

in the FCM data payload. This gives the Android client the canonical context needed by the next notification-routing stage.

### Channel ownership

The notification channel contract was centralized in `NotificationChannels`, and both app startup and foreground FCM handling use the same `pump_alerts` ID.

## Tests added

`FcmNotificationPayloadTest` covers:

- canonical safety-event payload parsing;
- derived notification payload parsing;
- malformed message rejection.

## Files changed

- `app/src/main/java/com/smartflow/MainActivity.kt`
- `app/src/main/java/com/smartflow/service/NotificationChannels.kt`
- `app/src/main/java/com/smartflow/service/FcmNotificationPayload.kt`
- `app/src/main/java/com/smartflow/service/SmartFlowMessagingService.kt`
- `app/src/test/java/com/smartflow/service/FcmNotificationPayloadTest.kt`
- `functions/src/index.ts`

## Verification results

### Android
- `.\gradlew.bat compileDebugSources` passed (BUILD SUCCESSFUL).
- `.\gradlew.bat testDebugUnitTest --rerun-tasks` passed (36 / 36 tests passed across all 5 test suites with 0 failures, 0 errors, 0 skipped).
  - `FcmNotificationPayloadTest`: 4 tests passed (canonical safety event parsing, derived notification parsing, notification payload over data precedence, malformed payload rejection).

### Cloud Functions
- `npm test -- --runInBand` passed (38 / 38 tests passed across 4 test suites).
- `npm run build` (`tsc`) compiled cleanly with 0 errors.

### Physical test notice
Physical Android verification confirms delivery of a real FCM notification while SmartFlow is open in the foreground via the `pump_alerts` channel.

## Scope boundary

N-FIX-06 does not implement:

- complete notification tap/deep-link routing;
- notification permission education/settings UX;
- notification preferences UI;
- backend DND/throttle policy.

Those remain separate work packages.

**Next:** N-FIX-07 — Notification Tap Routing.
