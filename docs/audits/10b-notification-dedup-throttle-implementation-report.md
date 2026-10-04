# WP-10A — N-FIX-04 Safety Notification Deduplication & Throttle Hardening

**Status:** IMPLEMENTED AND LOCALLY VERIFIED.

## Problem addressed

The previous notification flow used:

`canSend() → sendPush() → recordSent()`

That check-and-record sequence was not atomic. Concurrent/retried Cloud Function invocations could both pass `canSend()` before either invocation recorded its timestamp.

## Implementation

### 1. Atomic throttle claims

Added `claimThrottle()`.

It uses an RTDB transaction on:

`users/{uid}/notification_last_sent/{type}`

The transaction itself decides whether the 15-minute throttle slot is available. This closes the check-then-write race.

Transaction retry behavior is handled explicitly so a callback retry cannot leave a stale local "claimed" result.

### 2. Deterministic safety-event deduplication

Added `claimEventDelivery()` and `releaseEventDelivery()`.

Authoritative safety event deliveries are keyed by:

`users/{uid}/notification_delivery/{deviceId}/{eventId}`

This means a retry of the same device event for the same user cannot generate a second notification after a successful delivery claim.

The device ID is part of the key because RTDB event IDs are only unique within their parent device event collection.

### 3. Failed-send rollback

`sendPush()` now reports whether at least one FCM delivery succeeded.

If delivery fails:

- the throttle claim is released;
- the event delivery claim is released for authoritative events.

A later Cloud Function retry can therefore attempt delivery again instead of being permanently suppressed by a failed send.

For multicast FCM, partial success counts as successful delivery and the claim is retained.

### 4. Derived alerts

Low-tank and pump-started notifications now use the atomic throttle claim instead of `canSend()+recordSent()`.

Their claims are also rolled back when FCM delivery fails.

## Scope

N-FIX-04 does **not** introduce:

- notification channels;
- foreground FCM UI;
- deep-link routing;
- notification settings UI;
- alerts/history UI.

Those remain N-FIX-05 through N-FIX-12.

## Tests

Added coverage for:

- atomic throttle claim;
- duplicate authoritative-event suppression;
- retry after event-claim release;
- retry after throttle-claim release;
- existing DND behavior remains covered.

## Verification

All tests and builds have executed and passed:

```bash
cd functions
npm test -- --runInBand
# Result: PASS (4 test suites, 38 / 38 tests passed)
npm run build
# Result: PASS (tsc compilation successful, 0 errors)
```

```powershell
.\gradlew.bat compileDebugSources
# Result: BUILD SUCCESSFUL (18 tasks, 0 errors)
.\gradlew.bat testDebugUnitTest --rerun-tasks
# Result: BUILD SUCCESSFUL (32 / 32 unit tests passed across all 4 suites)
```

**Status:** N-FIX-04 VERIFIED locally.
