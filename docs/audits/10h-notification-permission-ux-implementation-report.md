# WP-10 N-FIX-10 — Android Notification Permission UX

**Status:** VERIFIED

## 1. Audit Findings

The previous implementation requested `POST_NOTIFICATIONS` directly from `MainActivity.onCreate()`.

Problems:

- no educational explanation before the first OS permission request;
- rationale branch contained only a TODO and immediately launched the permission dialog;
- no explicit blocked/permanently-denied guidance;
- Android OS permission state was separate from the existing Firebase notification preferences but was not presented as such.

The notification settings screen already controls server-side preferences such as DND and alert categories. Those settings do not grant Android's OS notification permission.

## 2. Implementation

### Intentional first-request UX

For Android 13+:

1. signed-in user with no previous permission request → SmartFlow explanation dialog;
2. user chooses Continue → Android `POST_NOTIFICATIONS` permission request;
3. user chooses Not now → no OS request;
4. denied permission with Android rationale available → explanation can be shown again;
5. permission is blocked without a rationale path → system notification settings guidance.

Permission request state is persisted locally so the app does not blindly request on every launch.

### System settings fallback

Added an explicit **Open Settings** path using Android's application notification settings.

The existing Notification Settings screen now also displays:

- whether Android notification permission is allowed;
- a system-settings action when it is blocked.

### Separation of concerns

The implementation keeps these distinct:

- Android OS notification permission;
- SmartFlow Firebase notification preferences;
- backend notification delivery policy.

Granting Android permission does not change Firebase preferences. Changing Firebase preferences does not grant Android permission.

## 3. Scope

No backend notification policy was changed in N-FIX-10.

Critical-event behavior remains governed by the existing backend policy. This work only improves the Android permission experience and makes the OS-level state visible.

## 4. Verification Results

### Automated Checks
- `.\gradlew.bat compileDebugSources` passed (BUILD SUCCESSFUL, 0 errors).
- `.\gradlew.bat testDebugUnitTest --rerun-tasks` passed (39 / 39 tests passed across all 6 test suites with 0 failures, 0 errors, 0 skipped).
- `assembleDebug` passed and APK deployed cleanly via ADB wireless connection to `POCO (2409FPCC4G)`.

### Physical Device Verification (Android 13+ / HyperOS)
All 9 interaction criteria confirmed on physical device:
1. **Fresh/unrequested state**: SmartFlow educational explanation dialog appears before OS permission prompt.
2. **Continue**: Native Android `POST_NOTIFICATIONS` dialog appears.
3. **Allow**: Permission is granted, dialog dismisses, status transitions to Allowed.
4. **Not now**: Dialog dismisses without triggering the OS prompt, app continues normally.
5. **Denial with rationale**: Explanation prompt can be presented again on subsequent relevant actions.
6. **Blocked / no-rationale**: Open Settings action is presented.
7. **Notification Settings**: UI accurately displays current OS permission state (Allowed vs. Blocked).
8. **Navigation**: Returning from Android system settings restores the app cleanly.
9. **FCM Delivery**: Existing push notification delivery via `pump_alerts` remains intact.

## 5. Files Changed

- `app/src/main/java/com/smartflow/MainActivity.kt`
- `app/src/main/java/com/smartflow/service/NotificationPermissionPrompt.kt`
- `app/src/main/java/com/smartflow/presentation/NotificationPermissionDialog.kt`
- `app/src/main/java/com/smartflow/presentation/NotificationSettingsScreen.kt`

