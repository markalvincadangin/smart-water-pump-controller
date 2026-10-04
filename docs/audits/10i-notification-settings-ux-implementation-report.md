# WP-10 N-FIX-11 — Notification Settings UX

**Status:** IMPLEMENTATION COMPLETE — VERIFICATION PENDING

## 1. Purpose

Polish the Android notification settings experience so users can distinguish:
- Android OS notification permission;
- SmartFlow's global push-notification preference;
- optional informational alerts;
- critical safety alert categories;
- Quiet Hours / DND behavior.

This change is presentation and interaction polish only. It does not change the backend notification policy or Firebase schema.

## 2. Changes

### Terminology normalization

Replaced the legacy **Overflow Protection** label with the canonical:

**Maximum Runtime Protection**

This matches the normalized `maxRuntimeAlert` preference and the backend event `EVT_MAX_RUNTIME_EXCEEDED`.

### Notification hierarchy

Settings are now grouped into:

1. **Notifications** — Enable Push Notifications and Android Notification Permission.
2. **Quiet Hours** — Quiet Hours switch, schedule, and explicit DND behavior.
3. **Informational Alerts** — Pump Started and Low Tank Level.
4. **Critical Safety Alerts** — Dry-Run Protection and Maximum Runtime Protection.

Informational alert switches remain user-configurable.

Critical safety alert category switches are displayed as fixed enabled controls so they do not appear to be ordinary optional notifications.

### Quiet Hours explanation

The UI now explicitly states that:
- Pump Started and Low Tank alerts are suppressed during Quiet Hours.
- Dry-Run Lockout and Maximum Runtime Protection bypass Quiet Hours.

This matches the currently implemented backend DND policy.

### Android permission distinction

The existing OS permission card remains separate from SmartFlow's Firebase-backed notification preferences.

The UI therefore distinguishes:
- **Enable Push Notifications** — SmartFlow/backend delivery preference.
- **Android Notification Permission** — device-level OS permission.

## 3. Important behavior boundary

The global **Enable Push Notifications** switch currently controls whether the backend considers the user an active notification recipient.

The critical safety alert switches themselves are not user-configurable, but disabling the global push-notification switch still prevents backend push delivery. N-FIX-11 does not change this backend behavior.

Quiet Hours are different: they suppress only non-critical notification categories while the critical policies bypass DND.

## 4. Files Changed

- `app/src/main/java/com/smartflow/presentation/NotificationSettingsScreen.kt`
- `docs/audits/10i-notification-settings-ux-implementation-report.md`

## 5. Verification Required

Run:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

Then verify on the physical Android device:
1. Notification Settings opens and scrolls cleanly.
2. Global notification switch still persists correctly.
3. Android permission state remains accurate.
4. Quiet Hours can be enabled/disabled.
5. Quiet Hours schedule can be changed and saved.
6. Informational alert switches persist independently.
7. **Maximum Runtime Protection** uses canonical terminology.
8. Critical safety alerts appear clearly non-optional.
9. Quiet Hours explanation is visible and understandable.
10. No notification behavior/regression is introduced.

N-FIX-11 should be marked **VERIFIED** only after the automated and physical checks pass.