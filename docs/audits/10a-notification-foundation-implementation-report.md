# WP-10 — Notification Fix & Polish Implementation Report

**Status:** N-FIX-01 / N-FIX-02 / N-FIX-03 implemented; local runtime verification pending.

## Scope

This implementation covers only the first three WP-10 items:

1. canonical notification preference normalization;
2. backend-enforced DND boundaries;
3. backend-owned notification delivery policy.

N-FIX-04 onward remain unchanged.

## N-FIX-01 — Preference normalization

### Changes

- Android `NotificationPrefs` now uses canonical `maxRuntimeAlert`.
- Android writes `maxRuntimeAlert` to `users/{uid}/notification_prefs/maxRuntimeAlert`.
- The previous `overflowAlert` field is no longer written by the Android client.
- Cloud Functions accepts canonical `maxRuntimeAlert` and retains `overflowAlert` only as a backward-compatible read fallback.

This keeps existing user preferences readable while making new writes canonical.

## N-FIX-02 — DND enforcement

Cloud Functions now evaluates DND before sending non-critical notifications.

- low-tank alerts are suppressed during DND;
- pump-started alerts are suppressed during DND;
- canonical safety events marked critical bypass DND;
- DND hours are interpreted using a stored IANA timezone;
- Android persists the device timezone alongside DND settings;
- invalid/missing timezones safely fall back to UTC.

The current default remains DND disabled.

## N-FIX-03 — Backend notification policy registry

Added `functions/src/notificationPolicy.ts`.

The backend now owns delivery policy for:

- `EVT_DRY_RUN_LOCKOUT`
- `EVT_MAX_RUNTIME_EXCEEDED`
- derived low-tank alerts
- derived pump-started alerts

Each policy records its preference key, severity, DND criticality, and throttle category.

Android EventRegistry remains presentation-oriented; it is not used to decide whether an FCM push should be delivered.

Unknown event codes therefore do not become push notifications automatically.

## Tests added

- DND cross-midnight behavior
- DND all-day behavior
- invalid timezone fallback
- canonical safety policy mapping
- derived alert policy mapping

## Verification status

The branch diff from WP-10 plan commit `7a3ab655e01f963066788fb8262af7279eebbb04` contains only the seven expected notification implementation/test files.

Local execution could not be performed in this environment because outbound GitHub access from the execution container is unavailable.

Run locally:

```bash
cd functions
npm test -- --runInBand
npm run build
```

Then from the repository root:

```powershell
.\gradlew.bat compileDebugSources
.\gradlew.bat testDebugUnitTest --rerun-tasks
```

## Remaining WP-10 work

Do not close WP-10 yet. Continue with:

- N-FIX-04 safety notification dedup/throttle hardening
- N-FIX-05 app-owned notification channel
- N-FIX-06 intentional foreground FCM handling
- N-FIX-07 notification tap/deep-link routing
- N-FIX-08 user-facing device identity
- N-FIX-09 Android EventRegistry boundary cleanup
- N-FIX-10 permission UX
- N-FIX-11 notification settings UI
- N-FIX-12 alerts/history presentation
- backend + Android automated verification
- physical FCM foreground/background/terminated verification
