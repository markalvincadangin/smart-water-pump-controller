# WP-10 N-FIX-05 — App-Owned Android Notification Channel

## Status

**IMPLEMENTED AND LOCALLY VERIFIED**

N-FIX-05 establishes the Android app as the owner of the `pump_alerts` notification channel used by the SmartFlow backend.

## Scope

This work is intentionally limited to notification-channel ownership. It does not implement foreground FCM handling, notification tap routing, permission UX, or notification-settings behavior.

## Implementation

### Android notification channel

`MainActivity` now creates the `pump_alerts` channel during application startup on Android O/API 26 and newer.

Channel configuration:

- Channel ID: `pump_alerts`
- User-visible name: **Pump Alerts**
- Importance: `IMPORTANCE_HIGH`
- Vibration: enabled
- Notification badge: enabled
- Description: **Safety alerts and important pump activity notifications.**

Android's `NotificationManager.createNotificationChannel()` is idempotent, so startup creation is safe across repeated launches.

### Backend compatibility

The channel ID remains `pump_alerts`, matching the existing Cloud Functions notification payload. No backend notification contract was changed in this work.

### User-facing strings

Channel name and description are stored in Android resources rather than hard-coded in the channel creation logic.

## Files changed

- `app/src/main/java/com/smartflow/MainActivity.kt`
- `app/src/main/res/values/strings.xml`

## Verification

The following have been run and verified locally:

```powershell
.\gradlew.bat compileDebugSources
# Result: BUILD SUCCESSFUL (18 tasks, 0 errors)
.\gradlew.bat testDebugUnitTest --rerun-tasks
# Result: BUILD SUCCESSFUL (32 / 32 unit tests passed across all 4 suites)
```

Runtime verification on physical hardware confirms:

1. The `Pump Alerts` channel exists (`pump_alerts`);
2. Importance is `IMPORTANCE_HIGH`;
3. Vibration is enabled (`enableVibration(true)`);
4. Notification badge is enabled (`setShowBadge(true)`);
5. Matches backend payload `channelId: "pump_alerts"`.

**Status:** N-FIX-05 VERIFIED locally.

## Boundary

N-FIX-05 does **not** claim that foreground FCM handling or notification tap routing is complete. Those remain N-FIX-06 and N-FIX-07 respectively.

**Next:** N-FIX-06 — Foreground FCM Handling.
