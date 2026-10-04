# WP-10 N-FIX-10 — Android Notification Permission UX

**Status:** IMPLEMENTATION COMPLETE — VERIFICATION PENDING

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

## 4. Verification Required

Run:

```powershell
.gradlew.bat compileDebugSources
.gradlew.bat testDebugUnitTest --rerun-tasks
```

Physical verification should cover Android 13+:

1. fresh/unrequested state → explanation appears before OS dialog;
2. Continue → OS permission dialog appears;
3. Allow → dialog closes and permission status is Allowed;
4. Not now → app continues normally;
5. denied with rationale → explanatory prompt can be shown again;
6. blocked/no-rationale → Open Settings is offered;
7. Notification Settings correctly reflects current OS permission state;
8. returning from Android Settings does not break navigation;
9. existing FCM notification behavior remains unchanged.

Expected automated result: existing Android test suite remains green.

## 5. Files

- `MainActivity.kt`
- `NotificationPermissionPrompt.kt`
- `NotificationPermissionDialog.kt`
- `NotificationSettingsScreen.kt`

N-FIX-10 should only be marked VERIFIED after compilation, unit tests, and physical permission-state verification pass.
