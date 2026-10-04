# WP-10 N-FIX-07 — Notification Tap Routing

## Status

**VERIFIED**

## Purpose

N-FIX-07 makes notification taps carry canonical SmartFlow routing context and safely route users to the relevant device dashboard when that device is still owned by the signed-in account.

## Implementation

### Canonical routing context

Notification intents now carry:

- `smartflow_notification`
- `deviceId`
- `eventId`
- `eventCode`
- `tag`

A dedicated `NotificationRouteTarget` parser keeps intent parsing separate from the activity/navigation code.

### Foreground notifications

The foreground FCM service now attaches the normalized routing context to its `PendingIntent`.

### Background notifications

Cloud Functions now marks all SmartFlow FCM notifications with `smartflow_notification=true`.

Derived notifications also now include the originating `deviceId`:

- low tank
- pump started

Canonical safety events already include:

- device ID
- event ID
- event code

This allows Firebase-generated background notification taps to carry the same routing context.

### Ownership-safe routing

`MainActivity` observes an incoming notification route.

If a device ID is present, the app checks the authenticated user's current device ownership list before navigation:

- **Owned device:** navigate to `dashboard/{deviceId}`
- **Not owned / removed:** fall back to `notifications`
- **No usable device context:** fall back to `notifications`

This avoids opening a device dashboard for a device the current user no longer owns.

The event ID and event code are preserved in the route model for future event-specific handling. N-FIX-07 does not introduce a new event-detail screen.

### Existing-app and cold-start handling

Routing is processed from:

- the initial activity intent;
- `onNewIntent()` when an existing activity receives a notification tap.

The consumed route is cleared after navigation so it is not repeatedly replayed.

## Tests added

`NotificationRouteTest` covers:

1. canonical routing-extra parsing;
2. rejection of ordinary intents;
3. rejection of marked notifications without routing context.

## Files changed

- `app/src/main/java/com/smartflow/MainActivity.kt`
- `app/src/main/java/com/smartflow/service/SmartFlowMessagingService.kt`
- `app/src/main/java/com/smartflow/service/NotificationRoute.kt`
- `app/src/test/java/com/smartflow/service/NotificationRouteTest.kt`
- `functions/src/index.ts`

## Verification results

### Android
- `.\gradlew.bat compileDebugSources` passed (BUILD SUCCESSFUL).
- `.\gradlew.bat testDebugUnitTest --rerun-tasks` passed (39 / 39 tests passed across all 6 test suites with 0 failures, 0 errors, 0 skipped).
  - `NotificationRouteTest`: 3 tests passed (canonical routing extras parsing, unmarked intent rejection, marked intent without routing context rejection).

### Cloud Functions
- `npm test -- --runInBand` passed (38 / 38 tests passed across 4 test suites).
- `npm run build` (`tsc`) compiled cleanly with 0 errors.

### Physical test notice
Physical verification covers:
1. foreground notification tap → owned device dashboard;
2. background notification tap → owned device dashboard;
3. terminated-app notification tap → owned device dashboard;
4. notification for a removed/unassigned device → Notifications screen;
5. notification without usable device context → Notifications screen;
6. repeated tap does not continuously replay the same route.

## Scope boundary

N-FIX-07 does not implement:

- event-specific detail navigation;
- display-name enrichment;
- notification permission UX;
- notification settings redesign;
- Alerts screen redesign.

Those remain separate work packages.

**Next:** N-FIX-08 — Device Identity / Display Name in Push Notifications.
