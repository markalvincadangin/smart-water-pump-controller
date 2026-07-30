---
status: draft
version: 0.1
last-reviewed: 2026-07-30
source: auto-generated
---

# Feature Specification: App Push Notifications

**Feature Branch**: `feature/push-notifications`

**Created**: 2026-07-30

**Status**: Draft

**Input**: User description: "how about the app notifications, is it implemented? can we create it in this spep?"

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | No | Notifications are a reporting layer, not controlling. |
| II. Dry-Run Lockout | No | |
| III. Overflow Protection | No | |
| IV. TOR Independence | No | |
| V. Sensor Freshness / E-Stop | No | |
| VI. Backward Compatibility | Yes | Notification logic in Cloud Functions must not break RTDB schema. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Critical Error Alerts (Priority: P1)

As a user, I want to be immediately notified if a critical error occurs (like Dry-Run or Overflow) so I can take action and protect my equipment.

**Why this priority**: Hardware protection is the primary goal of this project.

**Independent Test**: Can be fully tested by triggering a mock error state in RTDB and verifying a push notification arrives on a sleeping device.

**Acceptance Scenarios**:

1. **Given** the app is closed, **When** the ESP32 enters a DRY_RUN error state, **Then** a high-priority push notification is delivered to the user's phone.

---

### User Story 2 - Countdown Completion (Priority: P2)

As a user, I want to know when my countdown timer has finished watering, so I don't have to keep checking the app.

**Why this priority**: Improves user experience and provides peace of mind.

**Independent Test**: Can be fully tested by letting a countdown expire and verifying the notification.

**Acceptance Scenarios**:

1. **Given** the app is in the background, **When** the countdown reaches 0 and the pump stops, **Then** a notification "Countdown Complete" is delivered.

---

### User Story 3 - Notification Permissions & Setup (Priority: P1)

As an Android 13+ user, I must be prompted to allow notifications when I first open the app.

**Why this priority**: Android requires explicit permission; without this, the feature fails silently on modern devices.

**Acceptance Scenarios**:

1. **Given** a fresh install on Android 13, **When** launching the app, **Then** the OS permission dialog for notifications appears.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: App MUST request POST_NOTIFICATIONS permission on startup.
- **FR-002**: App MUST register the device's FCM token to the RTDB (e.g., under `/devices/{id}/fcmTokens`).
- **FR-003**: Cloud Functions MUST monitor RTDB state changes and dispatch FCM messages to registered tokens.
- **FR-004**: System MUST ONLY send notifications for critical/automated events (Dry Run, Overflow, Countdown Finish). Manual ON/OFF pump toggles initiated by the user MUST NOT trigger notifications to avoid spam.
- **FR-005**: System MUST rely on the OS-level Android Notification settings for opting in/out. No custom in-app notification toggles are required for the MVP.

### Key Entities

- **FCM Token**: A unique device identifier required by Firebase to route messages to specific Android instances.

---

## Firebase Contract *(if applicable)*

### RTDB Schema Changes
```json
{
  "devices": {
    "[device_id]": {
      "fcmTokens": {
        "[token_string]": true
      }
    }
  }
}
```

### Cloud Functions Changes
- Function name(s) affected: `onPumpStateChanged`, `onPumpError` (new)
- Trigger: RTDB `onUpdate` for `/devices/{id}/state` and `/devices/{id}/error`
- Breaking changes: NONE

---

## Dashboard UX *(if applicable)*

### User-Facing Changes
- Notification Permission Request on Android 13+.
- Foreground Notification Handling (if the app is open when a notification arrives, it might display a Snackbar instead of a system tray notification).

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of critical error events trigger an FCM dispatch within 3 seconds.
- **SC-002**: Device successfully receives background push notifications when the app is swiped away.

### Validation Commands

```bash
# Cloud Functions
cd functions && npm run build && firebase deploy --only functions

# Android
.\gradlew.bat assembleDebug
```

---

## Assumptions

- We will use Firebase Cloud Messaging (FCM) since the project already uses Firebase RTDB.
- The user has a physical Android device or emulator with Google Play Services to test FCM.
- No iOS support is required.
