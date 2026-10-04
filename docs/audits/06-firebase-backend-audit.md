---
status: completed
date: 2026-10-04
branch: docs/smartflow-system-contract
scope: WP-06A reconnaissance and WP-06B Firebase/backend audit
source-of-truth: docs/specifications/smartflow-system-contract.md
---

# WP-06 — Firebase & Backend Audit

## 1. Purpose

This audit traces the SmartFlow Firebase Realtime Database contract across Firebase RTDB security rules, Cloud Functions, firmware event publication, firmware reported-shadow and telemetry publication, Android Firebase DTOs, Android notification token registration, and the countdown desired-state lifecycle.

The goal is to identify concrete cross-system mismatches before changing backend behavior.

This is a repository/code audit. It does not prove production Firebase deployment, live RTDB rules, FCM delivery, or physical-device behavior.

## 2. Executive Result

WP-06A/B result: NOT READY FOR BACKEND NORMALIZATION WITHOUT CONTRACT CORRECTION.

| Priority | Finding | Impact |
|---|---|---|
| P0 | Cloud Functions match legacy event codes (DRY_RUN, OVERFLOW, COUNTDOWN_FINISHED) while firmware publishes structured EVT_* codes | Safety-event push notifications can be silently skipped |
| P0 | database.rules.json authorizes writes but does not validate safety-sensitive field values | Structurally authorized but invalid settings/commands can reach the database; firmware remains the final safety boundary |
| P1 | Firmware reported shadow does not serialize last_fault_code even though the contract/spec expect it | Android cannot reliably consume a machine-readable active fault from shadow/reported |
| P1 | FCM token registration exists in both users/uid/notification_prefs/fcmTokens and devices/deviceId/fcmTokens | Two token locations exist; Cloud Functions currently use the user-level location |
| P1 | Cloud Function telemetry consumer reads telemetry.waterLevel / telemetry.flowRate while firmware publishes water_level_percent / flow_rate_lpm | Notification messages can use default/incorrect telemetry values |
| P1 | Android event registry uses EVT_* identifiers while the Cloud Function notification trigger uses a different vocabulary | Activity-log and push-notification semantics are inconsistent |
| P1 | Countdown completion clears desired state, but no matching COUNTDOWN_FINISHED event source exists | Countdown completion notification path is currently unconnected |
| P2 | Older firmware documentation still contains /pump_system/* paths and legacy terminology | Future clients/integrators can follow stale paths |

## 3. Event Pipeline

### 3.1 Firmware event producer

The firmware event registry in firmware/master_node/src/utils/log_events.h defines structured identifiers including:

- EVT_DRY_RUN_WARN
- EVT_DRY_RUN_LOCKOUT
- EVT_DRY_RUN_CLEARED
- EVT_MAX_RUNTIME_EXCEEDED
- EVT_FAIL_SAFE_STOP
- EVT_RS485_TIMEOUT
- EVT_RS485_INVALID

app_logger.cpp converts the enum to the exact string returned by getEventCodeString(), then calls CloudManager::pushCloudEvent().

cloud_manager.cpp writes that code to devices/{deviceId}/events/{eventId}/code.

Therefore current firmware events contain values such as EVT_DRY_RUN_LOCKOUT and EVT_MAX_RUNTIME_EXCEEDED.

### 3.2 Cloud Function consumer

functions/src/index.ts currently filters for:

- DRY_RUN
- OVERFLOW
- COUNTDOWN_FINISHED

These do not match the firmware event vocabulary.

### 3.3 Consequence

The event-triggered notification branch returns before sending FCM for the firmware's current structured dry-run and maximum-runtime events.

This is a real integration defect, not merely a documentation mismatch.

### 3.4 Required correction

Do not patch the Cloud Function by guessing aliases.

First establish one canonical event vocabulary in the system contract. Then update firmware event codes if necessary, Android EventRegistry, Cloud Functions filtering, notification types/throttling, and tests.

Recommended direction: preserve the existing structured EVT_* identifiers because Android already consumes them and the firmware introduced them as stable machine-readable events.

## 4. Countdown Completion

The firmware contains explicit countdown cleanup behavior.

main.cpp detects countdown expiry, calls CloudManager::clearCountdownDesiredState(), calls CloudManager::setErrorFallbackDesiredState(), then updates local mode to MANUAL and manual intent to false.

The repository search did not find a COUNTDOWN_FINISHED event identifier in the firmware event registry or event-producing code.

Therefore countdown completion is currently represented through state transition, while the Cloud Function's COUNTDOWN_FINISHED notification branch has no matching firmware event source.

### Contract decision required

Choose one:

A. Event-based notification:
- define a canonical EVT_COUNTDOWN_FINISHED;
- publish it exactly once on completion;
- consume it in Android and Cloud Functions.

B. State-transition-only behavior:
- remove the unused Cloud Function notification branch;
- rely on reported state/event-log semantics;
- document that countdown completion is not a push-notification event.

For the current MVP, B is the lower-complexity option unless completion push notifications are an explicit requirement.

## 5. Reported Shadow

Current firmware serialization in firmware/master_node/src/cloud/device_shadow.cpp publishes:

- run_mode
- is_running
- is_error
- is_overflow_error
- emergency_stop_latched
- countdown_remaining_sec
- last_fault_message

It does not publish last_fault_code.

However:
- the system contract specifies last_fault_code;
- docs/specs/firmware.md documents last_fault_code;
- app/FirebaseModels.kt currently does not consume it either.

### Required correction

Treat last_fault_code as a cross-system contract change, then update the ESP32 reported shadow and Android DTO/domain mapping together.

Do not add the field to only one side.

## 6. Telemetry Naming

Firmware publishes:

- devices/{deviceId}/telemetry/water_level_percent
- devices/{deviceId}/telemetry/flow_rate_lpm
- devices/{deviceId}/telemetry/ultrasonic_last_good_cm

Android DTOs correctly map these names.

However, functions/src/index.ts currently reads after.telemetry?.waterLevel and after.telemetry?.flowRate.

Those are not the firmware field names.

### Consequence

The notification function's low-level and pump-started messages can fall back to water level 0 and flow rate 0 even when valid telemetry exists.

### Required correction

Update Cloud Functions to consume the canonical telemetry names already used by firmware and Android.

## 7. FCM Token Storage

The repository has two token-writing patterns.

### User-level path

Android SmartFlowMessagingService.kt and MainActivity.kt write:

users/{uid}/notification_prefs/fcmTokens/{tokenId}

Cloud Functions also read this location.

### Device-level path

FirebaseDeviceRepository.kt writes:

devices/{deviceId}/fcmTokens/{tokenId}

MainActivity.kt also contains a device-level token write.

### Consequence

The backend currently has two token authorities.

The user-level location is the stronger candidate for canonical ownership because notification preferences are user-scoped and one user may own multiple SmartFlow devices.

The device-level token path should be removed or explicitly deprecated only after all readers and writers are migrated.

## 8. Firebase Rules Audit

database.rules.json has useful authorization boundaries:

- users can access their own user node;
- device principals are restricted by auth.token.deviceId;
- ownership is not directly writable;
- reported state is device-write-only;
- telemetry is device-write-only;
- settings require owner/claim authorization;
- event writes are device-only.

These are positive structural controls.

### Missing validation layer

The rules primarily use .write authorization expressions. They do not define .validate constraints for the safety-sensitive values identified by the system contract.

Examples requiring validation:

Device settings:
- pump_start_level_pct: 0–100
- pump_stop_level_pct: 0–100
- pump_stop_level_pct greater than pump_start_level_pct
- dry-run threshold within firmware-supported range
- maximum runtime within firmware-supported range

Desired shadow:
- mode must be an allowed value;
- countdown_duration_min must be 1–120 when supplied for a countdown request;
- command fields must have expected primitive types.

Firebase validation is defense in depth, not the final safety authority. Firmware must continue validating all safety-sensitive settings and commands.

## 9. Security Rule Observation

The current authorization expression for shadow/desired distinguishes device principals from user principals and checks ownership/claim state.

The settings rule also checks ownership/claim state.

No evidence in this audit authorizes clients to directly mutate ownership/ownerUid or ownershipAudit, which is consistent with the system contract.

The rules should nevertheless be strengthened with explicit .validate constraints before the backend contract is considered complete.

## 10. Countdown Command Semantics

The firmware treats countdown_start as an edge-triggered command:

- a rising edge is detected;
- duration is validated at the firmware boundary;
- the timer is local to the ESP32;
- countdown completion clears the desired start flag;
- completion transitions desired state toward MANUAL/OFF.

This resolves the earlier ambiguity about stale countdown_start remaining indefinitely: the firmware has explicit cleanup.

### Remaining concern

The current desired-state contract should explicitly document what happens if a new countdown request arrives while a countdown is already active.

The existing edge-detection model means changing duration alone does not constitute a new countdown_start rising edge.

Possible intended behaviors:
- reject/ignore a new start while active;
- restart the countdown;
- require countdown_stop then a new start;
- use a separate request ID.

Do not change this behavior in WP-06 without deciding the contract first.

## 11. Notification Throttling

functions/src/notifications.ts has a 15-minute throttle for:

- dryRun
- lowLevel
- pumpStarted
- overflow

The backend event vocabulary should be normalized before renaming throttle keys.

Maximum-runtime protection should eventually use a distinct name such as maxRuntime rather than overflow if the system contract separates those conditions.

## 12. SpecKit Observation

docs/specs/README.md still states that active feature development should use the Spec Kit lifecycle and that feature artifacts live under specs/[###-feature-name]/.

The repository contains numbered specifications such as:
- specs/010-portfolio-readiness/
- specs/011-publication-security/

No deletion is recommended during WP-06. Later documentation cleanup should classify active workflow, shipped/read-only architectural plans, and historical specifications.

## 13. Contract Changes Required Before Implementation

Before backend edits, update smartflow-system-contract.md with these evidence-backed decisions:

1. Event codes: adopt one canonical EVT_* event namespace. Keep event code separate from fault code, e.g. event EVT_MAX_RUNTIME_EXCEEDED and fault MAX_RUNTIME.
2. Countdown completion: decide whether completion is an event/notification or only a reported state transition. Recommendation for MVP: state transition only unless push notification is explicitly required.
3. Reported fault code: add last_fault_code to reported shadow.
4. Token authority: make users/{uid}/notification_prefs/fcmTokens/{tokenId} the canonical notification token location, subject to final reader/writer verification.
5. Telemetry names: make water_level_percent, flow_rate_lpm, and ultrasonic_last_good_cm authoritative.
6. Firebase validation: require RTDB .validate rules for safety-sensitive values while preserving firmware-side validation.
7. Concurrent countdown: explicitly document behavior for a start request received while COUNTDOWN is already active.

## 14. Recommended Implementation Order

### WP-06C — Contract amendment

1. Amend smartflow-system-contract.md.
2. Record canonical event vocabulary.
3. Record token authority.
4. Record telemetry names.
5. Add reported last_fault_code.
6. Decide countdown completion notification semantics.
7. Decide concurrent countdown behavior.

### WP-06D — Backend correction

After WP-06C approval:
1. Add Firebase .validate rules.
2. Fix Cloud Function telemetry field names.
3. Align Cloud Function event filtering with canonical event codes.
4. Normalize notification throttle keys.
5. Remove or deprecate unused device-level FCM token writes only after all readers are confirmed.
6. Add backend tests for authorized, unauthorized, and invalid writes.

### Later Android work

Android should then consume the finalized contract rather than inventing independent mappings.

## 15. Verification Required

Rules:
- unauthenticated read/write denied where expected;
- non-owner cannot control another device;
- owner can control owned device;
- device can write only its own telemetry/reported/events;
- invalid settings rejected by RTDB rules;
- invalid countdown duration rejected by RTDB rules;
- ownership remains immutable to clients.

Events:
- firmware dry-run event reaches RTDB;
- firmware max-runtime event reaches RTDB;
- Cloud Function recognizes canonical code;
- FCM notification is emitted for enabled users;
- unrelated events do not trigger safety notifications.

Telemetry:
- firmware values appear under canonical field names;
- Cloud Functions read the same names;
- notification text contains actual telemetry values.

Countdown:
- start request starts local timer;
- stale desired state cannot restart it after completion;
- completion cleanup is persisted;
- safety interruption cannot leave a restartable stale command;
- concurrent-start behavior matches the finalized contract.

Shadow:
- last_fault_code is published;
- Android DTO consumes it;
- machine-readable code and human-readable message remain separate.

## 16. Audit Boundary

Not verified:
- live production Firebase rules deployment;
- live RTDB data;
- deployed Cloud Function versions;
- FCM delivery;
- Android foreground/background/terminated notification behavior;
- physical ESP32/relay/pump behavior;
- physical sensor/RS-485 behavior;
- OTA/provisioning end-to-end behavior.

## 17. Status

WP-06A reconnaissance: COMPLETE

WP-06B Firebase/backend audit: COMPLETE

WP-06C contract amendment: NEXT

WP-06D backend implementation: BLOCKED UNTIL WP-06C DECISIONS ARE RECORDED

No backend behavior was changed during this audit.
