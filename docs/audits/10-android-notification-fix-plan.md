# WP-10 — Android Notification Fix & Polish Plan

**Status:** PLAN COMPLETE — implementation not included  
**Depends on:** WP-09 Notification Audit  
**Purpose:** define the exact notification implementation sequence before changing Android, Cloud Functions, or Firebase notification preferences.

## 1. Design principle

SmartFlow notifications must describe **known system outcomes**, not merely user requests.

The authoritative sequence is:

`request → device validation → reported state/event → notification`

A successful Firebase write is never itself a notification-worthy claim that the pump changed state.

## 2. Target ownership model

| Layer | Responsibility |
|---|---|
| ESP32 | emits canonical event code and actual safety outcome |
| Firebase RTDB | stores event and reported state |
| Cloud Functions | decides whether/when a push is sent |
| FCM | transports the push |
| Android FCM service | handles foreground delivery and notification routing |
| Android EventRegistry | presents canonical events consistently inside the app |
| Android notification UI | presents delivery/read state and navigates to context |

## 3. Canonical notification policy

### Push by default

| Event | Policy |
|---|---|
| `EVT_DRY_RUN_WARN` | Push; throttled |
| `EVT_DRY_RUN_LOCKOUT` | Push |
| `EVT_MAX_RUNTIME_EXCEEDED` | Push |
| `EVT_FAIL_SAFE_STOP` | Push |
| low tank threshold crossing | Push if enabled |
| pump started from reported-state transition | Push if enabled |
| RS-485 timeout | Push if enabled; throttled |
| Wi-Fi disconnected | Push only if explicitly supported and throttled |

### In-app history by default

| Event | Policy |
|---|---|
| `EVT_DRY_RUN_CLEARED` | History |
| `EVT_RS485_INVALID` | History / throttled |
| sensor recovered | History |
| normal pump stop | History |
| boot/config restored | History |

This policy must remain configurable where the current settings UI exposes user preference.

## 4. WP-10 implementation tasks

### N-FIX-01 — Normalize notification preferences

Replace the legacy maximum-runtime preference with:

`maxRuntimeAlert`

Keep backward-compatible reading of `overflowAlert` only during migration.

Android settings must read/write the same canonical field used by Cloud Functions.

Do not remove old data until migration compatibility is confirmed.

### N-FIX-02 — Resolve DND semantics

The existing app persists:

- `dndEnabled`
- `dndStartHour`
- `dndEndHour`

Choose one authoritative behavior:

**Recommended:** DND suppresses non-critical push notifications while allowing safety-critical notifications.

Critical notifications:
- dry-run lockout
- maximum runtime protection
- fail-safe stop
- emergency/safety fault when an authoritative event exists

Non-critical notifications:
- pump started
- low tank
- recovery/informational events

Cloud Functions should enforce this policy because it controls push delivery.

### N-FIX-03 — Define event-to-push policy in one backend registry

Avoid scattered:

`if (code === "...")`

branches as the long-term design.

Create a small backend notification policy registry containing:

- canonical event code
- preference key
- severity
- default enabled state
- DND criticality
- throttle/deduplication category
- title/body generation strategy

The registry should remain independent of Android presentation text.

### N-FIX-04 — Prevent duplicate safety notifications

Use deterministic deduplication based on the RTDB event ID and/or a bounded event key.

Safety event delivery must not be repeated merely because a database write/update is retried.

For recurring events, use an explicit throttle policy.

### N-FIX-05 — Create Android notification channel

Create the `pump_alerts` channel in application startup before notifications can arrive.

Define:
- channel name
- description
- importance
- vibration/sound behavior
- safety notification expectations

Do not attempt to programmatically override a user's existing channel choice.

### N-FIX-06 — Implement foreground notification behavior

`SmartFlowMessagingService.onMessageReceived()` should explicitly handle data needed by the app.

For foreground delivery:

1. parse canonical event/data
2. resolve notification category/severity
3. create an intentional notification when appropriate
4. attach device/event context
5. update in-app notification state if required

Do not depend solely on Android's automatic handling of notification payloads.

### N-FIX-07 — Add notification tap routing

FCM data should contain stable routing information, at minimum:

- `deviceId`
- `eventId` when applicable
- canonical `eventCode`
- notification category

A notification PendingIntent should navigate to the relevant device/event context.

If the device no longer exists or ownership is lost, fall back safely to the notifications screen.

### N-FIX-08 — Include device display name

Push notifications should identify the affected device where practical.

Example:

**Maximum Runtime Protection — Smart Flow**

The backend must use a safe display name from authoritative metadata and must not expose internal device IDs in normal user-facing text.

### N-FIX-09 — Keep Android EventRegistry as local presentation mapping

Do not make Android responsible for backend delivery policy.

EventRegistry should map canonical codes to:

- category
- local title
- local explanation
- severity/presentation metadata

Push content may be separately generated by backend, but the canonical event code remains the stable identity.

### N-FIX-10 — Notification permission UX

Replace the current unconditional permission request flow with an intentional onboarding/settings experience.

The app should explain why notifications matter before requesting permission.

At minimum:
- first-run explanation
- permission request
- denied state
- permanently denied/settings guidance
- notification feature remains understandable when disabled

### N-FIX-11 — Notification settings UI

After schema normalization, polish settings so the user can clearly understand:

- master notification switch
- safety alerts
- pump activity
- low tank
- DND
- maximum-runtime protection
- what DND can/cannot suppress

Safety-critical notifications should not appear as if they are ordinary optional alerts if the implementation intentionally keeps them unsuppressible.

### N-FIX-12 — Notification screen consistency

The Alerts screen should distinguish:

- safety intervention
- warning
- informational
- recovery

Read/unread state must be independent from FCM delivery success.

Deletion/read state remains application history state, not proof that a push was delivered.

## 5. Tests required

### Backend

- notification preference parsing
- canonical event policy mapping
- DND behavior
- critical-event DND bypass
- safety-event deduplication
- throttle behavior
- device ownership filtering
- canonical event code handling
- legacy `overflowAlert` compatibility
- malformed/missing notification data

### Android

- FCM token registration
- foreground message handling
- notification channel creation
- notification data parsing
- notification tap routing
- missing device/event fallback
- permission states
- notification settings persistence
- DND settings
- EventRegistry mapping

### Integration

At minimum verify on a physical Android device:

1. dry-run warning
2. dry-run lockout
3. maximum-runtime protection
4. fail-safe/communication event if physically reproducible
5. low-tank alert
6. pump-started alert
7. foreground app
8. background app
9. terminated app
10. notification tap
11. notification permission denied
12. DND enabled
13. multiple devices

No test should claim physical safety behavior unless the actual ESP32/pump path was exercised.

## 6. Recommended implementation order

1. N-FIX-01 preference schema
2. N-FIX-02 DND semantics
3. N-FIX-03 backend policy registry
4. N-FIX-04 deduplication/throttling
5. N-FIX-05 notification channel
6. N-FIX-06 foreground handling
7. N-FIX-07 routing
8. N-FIX-08 device context
9. N-FIX-09 EventRegistry cleanup
10. N-FIX-10 permission UX
11. N-FIX-11 settings UI
12. N-FIX-12 alerts UI
13. tests
14. physical verification

## 7. Boundary with WP-11

WP-10 fixes **notification behavior and its supporting UX contract**.

WP-11 remains the broader Android UI/UX polish package:

- dashboard visual hierarchy
- cards and spacing
- control presentation
- configuration sheet
- device list
- loading/empty/error states
- animation/motion restraint
- typography
- colors/severity hierarchy
- broader interaction polish
- remaining visual bugs found during runtime review

WP-11 should consume the corrected notification/state semantics rather than redesigning around known incorrect behavior.

## 8. Acceptance criteria

WP-10 can close only when:

- notification preference schema has one canonical maximum-runtime field
- DND behavior is explicitly defined and implemented
- safety notification policy is explicit
- duplicate/retry behavior is bounded
- Android owns the notification channel
- foreground behavior is intentional
- notification taps route to useful context
- device identity is represented safely
- permission UX is intentional
- backend + Android tests pass
- physical-device foreground/background/terminated notification behavior is verified where FCM infrastructure permits

## 9. Scope restriction

This package is a plan only. It does not authorize implementation changes to firmware, Firebase rules, Cloud Functions, or Android notification behavior.

## 10. Next step

Proceed to the approved notification implementation package after reviewing this plan. After WP-10 implementation and verification, proceed to **WP-11 — Android UI/UX implementation & polish**.
