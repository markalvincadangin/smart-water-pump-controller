# WP-10 N-FIX-05 — App-Owned Android Notification Channel

## Status

**IMPLEMENTED — VERIFICATION PENDING**

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

The following must be run locally before this work is marked VERIFIED:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

Runtime verification should also confirm on a physical/test Android device that:

1. the `Pump Alerts` channel exists;
2. its importance is High;
3. vibration is enabled;
4. the channel ID is `pump_alerts`;
5. notifications delivered through the backend's `pump_alerts` channel resolve to this app-owned channel.

## Boundary

N-FIX-05 does **not** claim that foreground FCM handling or notification tap routing is complete. Those remain N-FIX-06 and N-FIX-07 respectively.

**Next:** N-FIX-06 — Foreground FCM Handling.
