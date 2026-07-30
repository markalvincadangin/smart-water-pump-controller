---
status: current
last-reviewed: 2026-07-29
source: hand-authored
---

# SmartFlow Android App Specification

## Account and device gate

The app is available only after Firebase Authentication establishes a durable account. Google accounts are eligible immediately; email/password accounts must verify their email before they can claim or control a device. Before device access or an ownership-sensitive callable, the app refreshes the Firebase session and rejects a locally cached user that has been deleted or revoked. Anonymous or guest sessions may not create, own, transfer, release, or control hardware. Cloud Functions independently resolve the caller from the current Firebase Auth record and enforce the same rule.

When a signed-in durable user has no claimed devices, the app routes to provisioning instead of rendering a controllable device dashboard. The app may read a user's device index for navigation, but all ownership changes occur through authenticated Cloud Functions; it must never write an authoritative owner marker directly.

## Provisioning handoff

After the ESP32 emits terminal BLE `provisioned`, the app shows cloud-registration progress for up to 90 seconds while retrying the callable claim with the in-memory pairing proof. It must not restart BLE scanning automatically during this period. If the window expires, the app offers a cloud-claim retry that preserves the in-memory proof and a separate explicit action to start a new BLE provisioning session.

## Owner management

Each device has exactly one owner. Owner management offers explicit Wi-Fi recovery, release, transfer, and cancellation actions; it does not offer sharing, technician roles, or lost-owner takeover. Wi-Fi recovery communicates that the pump is stopped and only local Wi-Fi/enrollment is reset. Release and transfer communicate the five-minute nearby BLE pairing requirement and preserve ownership until a valid replacement claim completes.

The Account screen checks server-authoritative deletion eligibility before any account-deletion action. Self-service account deletion is blocked while the signed-in account owns one or more devices; the app directs the owner to release or transfer every device first. A final account-delete action must require Firebase re-authentication and a separate explicit confirmation.

## Production diagnostics

The app presents the device health snapshot (`freeHeap`, `wifiRSSI`, `restartReason`) and at most 50 WARN/ERROR cloud events. It must not depend on or expose the development TCP log console.

## UI/UX and Design System

The app strictly adheres to the centralized brand design system via Material 3 composition. Dynamic color extraction from user wallpapers is explicitly disabled to ensure uncompromised brand identity across light and dark modes. All semantic styling relies exclusively on `MaterialTheme.colorScheme` tokens; hardcoded colors are forbidden. The core dashboard layout is intrinsically responsive, enforcing a scrolling single-column hierarchy in portrait and pivoting to a side-by-side grid in landscape to maximize telemetry visibility without obscuring controls.

## HMI and Idempotency

Human-machine interface (HMI) controls require tactile confirmation and strict state idempotency. Critical hardware actions invoke device haptic feedback (`LongPress`). To prevent race conditions from rapid input, state-mutating UI elements (like the Timer and Power toggles) must immediately transition to an active busy indicator and disable themselves until authoritative cloud telemetry confirms the hardware's actual state matches the user's desired state. The sole exception is the Emergency Stop (E-STOP) button, which remains perpetually enabled to allow redundant abort signaling.
