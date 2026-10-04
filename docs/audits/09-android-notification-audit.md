# WP-09 — Android Notification Audit

**Status:** AUDIT COMPLETE — implementation not authorized by this work package  
**Branch:** `docs/smartflow-system-contract`

## 1. Purpose

Audit the notification path from firmware events through Firebase Cloud Functions and FCM to Android, against the SmartFlow system contract.

## 2. Current notification architecture

The current path is:

Firmware/device event → RTDB `/devices/{deviceId}/events/{eventId}` → Cloud Function `onDeviceEventCreated` → FCM → Android `SmartFlowMessagingService`.

There is also a separate `onDeviceUpdated` path for low tank and pump-started notifications based on reported/telemetry state.

Android registers FCM tokens under the canonical user preference path.

## 3. Findings

### N-01 — P1: Cloud Functions only push two canonical safety events

`onDeviceEventCreated` currently sends push notifications only for:

- `EVT_DRY_RUN_LOCKOUT`
- `EVT_MAX_RUNTIME_EXCEEDED`

The canonical event registry contains additional safety/system events, including:

- `EVT_DRY_RUN_WARN`
- `EVT_DRY_RUN_CLEARED`
- `EVT_FAIL_SAFE_STOP`
- `EVT_RS485_TIMEOUT`
- `EVT_RS485_INVALID`

Not every registry event necessarily needs a push notification, but the notification policy is currently implicit rather than explicitly defined.

**Recommendation:** define which canonical events are push-worthy, which are in-app history only, and which are intentionally silent.

### N-02 — P1: EventRegistry and backend notification policy are separate sources of truth

Android `EventRegistry` contains user-facing notification messages for many event codes, while Cloud Functions independently constructs push titles/bodies.

This creates a drift risk: an event can have one message in Android's registry and a different push message from the backend.

**Recommendation:** establish a clear ownership rule. The backend should remain authoritative for push content, while Android's registry should be responsible for local/in-app presentation of the same canonical event code.

### N-03 — P1: Some EventRegistry events are no longer aligned with the current safety contract

Examples include:

- `EVT_AUTO_BYPASS_ENABLED` — AUTO is not part of the MVP contract and the current firmware safety direction does not make this a normal MVP notification.
- Legacy/generic system events are present without an explicit current notification policy.

This does not prove the firmware emits all of these events today.

**Recommendation:** audit firmware event emission against EventRegistry before deciding notification behavior.

### N-04 — P1: Notification preferences are incomplete relative to current backend behavior

Android writes:

- enabled
- DND fields
- pumpStartedAlert
- lowLevelAlert
- dryRunAlert
- overflowAlert

The backend uses:

- enabled
- lowLevelAlert
- pumpStartedAlert
- dryRunAlert
- maxRuntimeAlert, with legacy overflowAlert fallback

The Android update path does not currently write a canonical `maxRuntimeAlert` field.

This means the backend's canonical preference and Android's settings model are not fully aligned.

**Recommendation:** normalize the preference schema before UI polish.

### N-05 — P1: DND preferences are persisted but not enforced in the audited Cloud Function path

Android writes `dndEnabled`, `dndStartHour`, and `dndEndHour`, but the audited backend notification functions do not use these fields when deciding whether to send.

This is a functional mismatch if the UI presents DND as active notification suppression.

**Recommendation:** determine whether DND is intended to suppress push delivery. If yes, implement and test it in the backend; if no, remove or clearly redefine the setting.

### N-06 — P1: Foreground Android handling does not create an application notification

`SmartFlowMessagingService.onMessageReceived()` currently logs notification payloads when a message reaches the service. It does not create an Android notification itself.

With FCM notification messages, Android may display notifications automatically when the app is backgrounded, while foreground behavior differs. Therefore foreground and background presentation are not currently guaranteed to be consistent.

**Recommendation:** explicitly define foreground behavior. For a SmartFlow safety app, important safety notifications should remain visible in-app and/or produce an intentional notification presentation rather than relying on implicit FCM behavior.

### N-07 — P1: Notification tap/deep-link behavior is not explicitly implemented in the audited service

The backend sends a `tag` in FCM data, but the Android service does not currently map notification data to a destination or device/event context.

**Recommendation:** define notification tap behavior, preferably opening the affected device/event context rather than merely launching the default screen.

### N-08 — P1: Backend notification throttling is inconsistent

`lowLevel` and `pumpStarted` notifications use `canSend()` / `recordSent()`.

The event-created safety notifications do not use the throttle helper.

Therefore dry-run lockout and maximum-runtime notifications can potentially repeat if duplicate/repeated events are created, while other notification classes are throttled.

**Recommendation:** define per-event deduplication/throttling policy, especially for safety events.

### N-09 — P1: Device event ownership is checked, but notification content does not identify the device

The backend verifies that the user owns the device before sending. However, the notification title/body does not include the device's user-facing name.

For users with multiple SmartFlow devices, a generic "Maximum Runtime Protection" notification is ambiguous.

**Recommendation:** include a safe device display name in notification content or deep-link metadata.

### N-10 — P1: Push content generally describes physical outcomes correctly, but this needs an explicit contract

The strongest existing messages are outcome-oriented:

- "The pump ran longer than the safety limit and was stopped automatically."
- "Dry run detected! The pump was stopped automatically..."

These correctly describe safety intervention rather than merely claiming that a command succeeded.

However, pump-started notification is inferred from reported `is_running`, while other notifications come from event creation. This distinction should be made explicit.

**Recommendation:** classify notification sources as:
- reported-state transition
- safety event
- informational/system event

### N-11 — P2: FCM Android notification channel is assumed by backend but not audited as an application-owned channel

Backend sends `channelId: "pump_alerts"`.

The Android manifest/service does not show channel creation in the audited files.

Android should explicitly create and configure the channel on supported versions, including importance appropriate to safety notifications.

### N-12 — P2: POST_NOTIFICATIONS permission exists, but permission-state UX was not established by this audit

The manifest declares `POST_NOTIFICATIONS`, but this audit did not establish where/when Android requests permission or how notification-disabled state is presented.

This requires source/runtime verification before implementation.

### N-13 — P2: Push and in-app event presentation need clearer separation

Android's `EventRegistry` is already useful for event history presentation. Push notification content should not become another uncontrolled copy of the same policy.

Recommended ownership:

- Firmware: canonical event code + actual event facts
- Cloud Functions: delivery policy + push envelope
- Android EventRegistry: local human-readable event presentation
- Android notification handler: routing/deep link + foreground presentation

## 4. Notification policy proposed for next implementation package

Do not implement blindly from this audit. First approve/validate the policy.

| Event/source | Push candidate | Reason |
|---|---:|---|
| Dry-run warning | Yes, configurable | Immediate risk warning |
| Dry-run lockout | Yes | Safety intervention |
| Dry-run cleared | Usually no | Lower urgency; activity history sufficient |
| Maximum runtime exceeded | Yes | Safety intervention |
| Fail-safe stop | Yes | Safety intervention |
| RS-485 timeout | Yes/configurable | Sensor/control reliability issue |
| RS-485 invalid | Usually no / throttled | Avoid noise |
| Low tank | Yes/configurable | User alert |
| Pump started | Configurable | Informational |
| Pump stopped normally | Usually no | Avoid notification noise |
| Sensor recovered | Usually no | Activity/history sufficient |
| Wi-Fi disconnected | Configurable | Connectivity status |
| Boot/config restored | Usually no | Informational |

This is a policy proposal, not yet an implementation decision.

## 5. Priority order

### P0
No confirmed P0 safety defect was established by source audit.

### P1
- N-01 event notification policy undefined
- N-02 duplicate message sources
- N-04 preference schema mismatch
- N-05 DND not enforced
- N-06 foreground handling
- N-07 notification routing
- N-08 safety-event dedup/throttle
- N-09 device identification
- N-10 source/outcome classification

### P2
- N-03 EventRegistry cleanup
- N-11 notification channel ownership
- N-12 notification permission UX
- N-13 in-app/push responsibility boundary

## 6. Boundary

WP-09 is an audit only. No notification behavior, preference schema, Firebase event schema, or Android service behavior should be changed until the notification policy and implementation plan are reviewed.

## 7. Next package

WP-10 should convert this audit into an implementation/fix plan covering:

1. canonical notification policy
2. preference/schema correction
3. DND semantics
4. FCM channel ownership
5. foreground notification behavior
6. notification tap/deep-link routing
7. deduplication/throttling
8. device-name context
9. notification permission UX
10. tests and physical-device verification

After WP-10 approval, WP-11 can implement the notification fixes and polish.
