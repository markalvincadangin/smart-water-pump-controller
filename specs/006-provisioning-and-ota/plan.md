# Implementation Plan: Provisioning & Wireless Flashing

**Branch**: `006-provisioning-and-ota` | **Date**: 2026-07-28 | **Spec**: [spec.md](spec.md)

## Summary

Complete secure, cable-free development and recovery around the existing BLE provisioning flow. BLE protocol queuing, streamed Wi-Fi results, final `provisioned` status, dual OTA partitions, local OTA transport, and stable device bootstrap identity already exist. The remaining ownership work replaces anonymous user ownership and direct RTDB claims with durable user authentication, an atomic backend claim verified by setup-time local possession, and explicit ownership lifecycle controls. OTA, recovery, and diagnostics remain scoped as originally planned.

## Technical Context

| Area | Decision |
|---|---|
| Firmware | ESP32 Arduino/PlatformIO, existing NimBLE, WiFi, Firebase client, ArduinoOTA, NVS |
| Mobile | Android Kotlin/Compose, Firebase Authentication and RTDB |
| Backend | Node.js TypeScript Firebase Cloud Functions and Firebase Admin SDK |
| Database | Firebase Realtime Database, additive schema and rules only |
| Local OTA | `esp32dev_ota`, ArduinoOTA, fixed callback port 20000, password required |
| Recovery | GPIO32 reserved for an eventual button; no physical-reset implementation until wired |
| Device auth | Per-device bootstrap secret in Google Cloud Secret Manager + Cloud Function-issued custom Firebase token; stable UID `device:{deviceId}` |
| User auth | Durable Firebase Authentication account: Google sign-in and verified email/password; anonymous sessions are not eligible for ownership |
| Ownership | Cloud Function performs an atomic claim after validating a short-lived, locally exchanged setup proof; one authoritative owner is auditable |
| Observability | `AppLogger` dispatches records to independent compile-time `LogSink` transports. The initial development sink is TCP on a trusted LAN; production exposes a three-field RTDB health snapshot plus WARN/ERROR events retained by a trusted backend trigger to the newest 50 records. |

## Current State and Gaps

| Capability | Current state | Planned change |
|---|---|---|
| BLE provisioning | Implemented, including final `provisioned` notification | Retain contract; add cloud-bootstrap failure status if needed |
| Android success UI | Receives `provisioned`, then the ESP32 intentionally disconnects BLE while it bootstraps cloud identity | Render a bounded 90-second cloud-registration phase that retries the callable claim, retains the proof only in memory, and offers cloud retry separately from a new BLE setup |
| Test-environment reset | RTDB-backup-first script deletes RTDB root and Firebase Auth users | Require acknowledgement that Auth users are not recoverable by this workflow; reseed only an explicit, non-secret active `deviceRegistry/{deviceId}` record after deletion; never restore user, ownership, pairing, telemetry, Function, OAuth, Secret Manager, or local-secret state |
| Wi-Fi OTA | Verified with `esp32dev_ota` and port 20000 | Require a non-committed password for device and uploader |
| Device authorization | Stable `device:{deviceId}` custom-token bootstrap is in place | Retain and validate the device-only authorization boundary |
| User ownership | Android can sign in anonymously and writes `/users/{uid}/devices/{deviceId}` then `claimedByUid` directly | Require durable sign-in and replace two-step client claim with a backend-authorized atomic claim |
| Ownership lifecycle | Deleting a user-device mapping clears ownership through a database trigger | Replace implicit deletion ownership changes with explicit release/transfer and audit controls |
| Factory reset | BLE reset command exists; no physical hardware | Reserve GPIO32 and implement only when button hardware is installed |
| Logging | Development TCP log sink is available after Wi-Fi joins; its bounded ring buffer replays history before live records | Verify the replay/live lifecycle, cap production WARN/ERROR event retention at 50 records, and retain the development-only build boundary |

## Constitution Check

| Principle | Status | Plan evidence |
|---|---|---|
| I. Fail Toward Pump OFF | Pass | Every reset or remote reprovision request calls `setPump(false)` before network changes or reboot. |
| II. Dry-Run Lockout | Pass | Wi-Fi reprovision preserves dry-run configuration and latches. |
| III. Overflow Protection | Pass | Wi-Fi reprovision preserves overflow configuration and latches. |
| IV. TOR Independence | Pass | No feature changes the hardware contactor/TOR path. |
| V. Freshness / E-Stop | Pass | Maintenance work cannot clear safety latches or bypass control polling. |
| VI. Additive Contracts | Pass | RTDB maintenance and identity fields are additive; existing device paths remain valid. |

## Phase 0: Research Decisions

See [research.md](research.md). Decisions are: use Firebase custom tokens and RTDB custom claims for stable device authorization; use Google sign-in and verified email/password for durable user ownership; retain cloud ownership on physical reset; validate active BLE setup possession through a short-lived pairing proof; use a device-specific bootstrap secret rather than an embedded service-account credential; and keep TCP logs development-only.

## Phase 1: Design

See [data-model.md](data-model.md), [ownership contract](contracts/device_ownership.md), [maintenance recovery contract](contracts/maintenance_recovery.md), and [quickstart](quickstart.md).

## Implementation Sequence

1. Add OTA password configuration outside Git. Configure ArduinoOTA with it, pass it to PlatformIO via an environment variable, and verify rejected/accepted uploads.
2. Maintain `AppLogger` as the transport-independent dispatcher: application code writes only to it, while independent `LogSink` implementations own their lifecycle and output. Start registered sinks exactly once after serial initialization; each sink may perform non-blocking loop work and must not stall BLE, Wi-Fi, the control loop, or OTA. Enable sinks independently at compile time: development builds may enable Serial and the initial TCP log sink, while production preserves only RTDB diagnostics. The trusted `retainDeviceEvents` backend trigger retains the 50 newest WARN/ERROR RTDB records by push ID, rather than making correctness depend on device-side query/delete behavior. The TCP sink keeps a bounded recent-history buffer, replays it before live records for a newly accepted client, and permits only one active client. OTA and logging must coexist without logging blocking upload service; logging may be rate-limited or suspended during an OTA upload if future load measurements require it.
3. Baseline completed: the Cloud Function HTTPS bootstrap exchange validates device ID, nonce, timestamp, and HMAC using a per-device Google Cloud Secret Manager secret, then returns a Firebase custom token for `device:{deviceId}` with only `role=device` and `deviceId` claims. Revalidate it during integration.
4. Baseline completed: firmware uses the supported custom-token sign-in path rather than anonymous device sign-in. Revalidate persistent device authorization during integration.
5. Add durable Android account flows: Google sign-in and verified email/password. Require email verification for password users before claim or remote control. Remove automatic anonymous sign-in from normal device access; if a guest flow remains, require account linking before a cloud claim or remote control.
6. Add an active setup pairing proof: firmware creates a cryptographically random, short-lived proof during the BLE session, provides it only through the connected setup channel, and publishes only a non-reversible verifier under its already-authorized device path after cloud bootstrap.
7. Before rule cutover, implement an idempotent trusted migration that backfills only consistent legacy owner mappings; freeze missing or conflicting records and never infer, release, or transfer ownership. Provide a separate operator-only, per-device administrative resolution command requiring an evidence reference and immutable audit record; it must not be callable by an app or device.
8. Implement callable `claimDevice`. It requires a non-anonymous user, validates the active pairing proof and unclaimed state through a server-authoritative one-time verifier reservation, then uses an atomic multi-location update to create the owner mapping, authoritative owner marker, claim audit record, and bounded status. It must return distinct `ALREADY_CLAIMED`, `EXPIRED_PAIRING`, and authorization outcomes without leaking owner identity.
9. Treat `claimDevice` as the sole cloud-readiness check after BLE terminal provisioning. The Android client retries retryable results every two seconds for at most 45 attempts, visibly reports progress, and preserves the proof only for an explicit in-memory retry. Do not introduce a direct Android read of pairing/ownership data or a separate non-atomic pre-claim endpoint.
10. For a first-time-system test, write an ignored RTDB snapshot and require acknowledgement that Firebase Auth users cannot be restored by this workflow before deleting RTDB/Auth; record any needed test-account identifiers, then reseed the named active registry record using its Secret Manager reference. Rotate the one-time OTA reprovision request ID before upload so the ESP32 clears local Wi-Fi/device enrollment and returns to BLE without altering its identity or the safety boundary.
11. Restrict RTDB rules so mobile clients cannot directly write ownership fields, user-device ownership mappings, claim proof material, or maintenance requests. Device claims may write only their permitted telemetry/status/diagnostic and pairing-verifier paths; owners cannot impersonate a device.
12. Replace the implicit `onDeviceUnclaimed` deletion behavior with owner-authorized release/transfer lifecycle functions and block self-service account deletion while a user owns a device. Transfer/release request a five-minute, non-extendable ownership-pairing mode from the online device; firmware calls `setPump(false)` before temporary BLE, and expiry/cancellation stops BLE while preserving Wi-Fi and the existing owner. Transfer stores the intended recipient and completes only after that recipient's fresh BLE proof. Release keeps the original owner until a nearby eligible replacement claim succeeds. A local Wi-Fi reset, app uninstall, anonymous-account deletion, or arbitrary client path deletion must not silently transfer or abandon a device. The initial release has one owner only; member and technician roles are deferred.
13. Implement callable owner-only `requestWifiReprovision`. It checks the authoritative owner association, creates a nonce-bound, expiring, idempotent request plus audit record, and does not directly mutate Wi-Fi state.
14. Implement firmware request processing: validate the request and replay marker, call `setPump(false)` as its first state-changing action, acknowledge/audit, erase only Wi-Fi and device-auth enrollment, and reboot into BLE. It must retain safety latches, device identity, ownership, and reachable `reset_stop`/`clear_error` processing.
15. Update Android provisioning and owner-maintenance UX with account gate, local pairing feedback, explicit protected-owner errors, confirmation, in-progress/error state, app-scoped account-deletion eligibility check, and handoff to BLE onboarding after an authorized recovery.
16. When hardware is available, wire a normally-open button from GPIO32 to GND and add a debounced, wrap-safe 10-second press handler. Short presses do nothing; the long press performs the same local provisioning reset boundary as specified.

## Validation

- Build Android and Cloud Functions; compile both USB/OTA firmware environments.
- OTA without the configured password is rejected; an upload with it succeeds through `esp32dev_ota` without USB.
- A provisioned device restarts and uses the same `device:{deviceId}` principal for authorized RTDB writes.
- A signed-in durable user can claim an unclaimed, nearby provisioned device; a second phone signed in to that account sees the same device without provisioning again.
- An anonymous session, guessed device ID, expired pairing proof, and a second claimant cannot alter ownership; an already-claimed result does not reveal the owner identity.
- One successful claim writes one authoritative owner mapping and one audit record in the same logical operation; a failed claim leaves no partial mapping.
- A consistent legacy owner mapping migrates unchanged; a conflicting legacy mapping is frozen without ownership mutation unless a trusted operator resolves that specific device with evidence and an immutable audit event.
- A pending transfer records its recipient and expiry; remote or expired recipient attempts leave the existing owner unchanged.
- An ownership-pairing request turns the pump OFF before BLE begins; it lasts exactly five minutes, cannot be extended, and its expiry/cancellation stops BLE, retains Wi-Fi/ownership, and leaves the pump OFF.
- An explicit owner release or authorized transfer is auditable. App uninstall, local reset, or deletion of an obsolete anonymous account does not release or transfer ownership.
- A non-owner, expired, malformed, or replayed Wi-Fi reprovision request changes neither Wi-Fi state nor pump state.
- A valid owner request turns the pump OFF, records an audit event, retains safety latches and ownership, and returns to BLE provisioning.
- Development firmware accepts a LAN TCP log-sink connection after Wi-Fi joins, replays bounded recent records, then streams live application records. Production firmware has no development TCP sink and publishes diagnostics through its cloud path. Future TCP, WebSocket, MQTT, or Syslog sinks must not require changes to application logging calls.
- After GPIO32 hardware exists, an early release makes no change; a 10-second press resets local network enrollment only.

## Post-Design Constitution Check

Pass. The design introduces no alternate relay-write path, keeps safety state intact through recovery, uses wrap-safe timing for the future button, and keeps all database changes additive.
