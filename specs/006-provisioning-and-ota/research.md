# Research: Provisioning, OTA, and Recovery

## Stable device authentication

**Decision:** Authenticate firmware as `device:{deviceId}` using a Firebase custom token issued by a Cloud Function after a per-device HMAC bootstrap exchange.

**Rationale:** Anonymous Firebase sign-in creates a different UID for each enrollment, so a restrictive RTDB rule cannot identify a rebooted device. Custom tokens establish a controlled UID and limited claims; RTDB rules can use those claims to authorize only the matching device path.

**Alternatives considered:** Anonymous authentication (rejected: unstable identity); unauthenticated RTDB writes (rejected: unsafe); embedding a Firebase service-account credential in firmware (rejected: credential disclosure).

## Durable user ownership

**Decision:** A claimed SmartFlow device belongs to a durable user account, initially supported through Google sign-in and verified email/password. Anonymous authentication is limited to an optional temporary local trial and cannot create permanent ownership or remote-control access.

**Rationale:** Consumer IoT products bind cameras and smart-home devices to an account and require explicit transfer/release. For a pump controller, a durable owner provides cross-phone access, accountability, and safe recovery after app reinstall. Firebase documents anonymous accounts as temporary and supports upgrading them by linking a durable provider, rather than treating them as permanent identity.

**Alternatives considered:** Permanent anonymous ownership (rejected: user deletion and expiry can orphan claims); device-local owner only (rejected: does not support remote access or recovery); account optional for remote pump control (rejected: insufficient accountability for safety-relevant control).

## Email verification gate

**Decision:** Email/password users must verify their email before they can claim hardware or issue remote pump controls. Google sign-in is eligible at sign-in because the provider supplies a verified account identity.

**Rationale:** Claiming a pump controller using an unverified address can bind ownership to a typoed or inaccessible inbox. The gate makes account recovery and audit attribution more reliable without adding friction for Google users.

**Alternatives considered:** Defer verification until after claim (rejected: ownership ambiguity); Google-only launch (rejected: excludes users without a Google account).

## Atomic setup-time claim

**Decision:** The backend, not an Android RTDB client, makes an atomic ownership claim after validating a short-lived setup pairing proof. During an active BLE session the firmware delivers the raw proof only to the nearby app; after device bootstrap it publishes only a verifier and expiry. The callable claim consumes the proof and writes ownership plus audit data together.

**Rationale:** A device ID is observable and must not be a claim credential. Combining authenticated user identity with local possession makes setup comparable to camera/smart-home onboarding, while an atomic backend operation prevents the current two-step partial-claim failure.

**Alternatives considered:** Client writes to `claimedByUid` (rejected: race-prone and bypassable); QR-only claim (deferred: valuable manufacturing option but unavailable on current hardware); BLE-only claim with no server verification (rejected: no durable transaction/audit).

## Ownership lifecycle

**Decision:** The initial release changes ownership only through an explicit owner-authorized release/transfer. Lost-owner recovery is deferred to a separately specified support policy. Local Wi-Fi reset preserves cloud ownership, and data-path deletion must not implicitly unclaim a device.

**Rationale:** Ring and similar consumer IoT products protect existing ownership and require release/transfer. Deferring recovery avoids creating an unverified takeover route while the product has no installed physical reset/proof hardware. This prevents physical access, app removal, or an abandoned anonymous account from changing who controls the pump.

**Alternatives considered:** Unclaim on deletion of the user-device mapping (rejected: accidental/orphaned deletion changes ownership); factory reset transfers to next setup user (rejected: unsafe physical takeover); indefinite lock with no recovery (rejected: poor supportability).

## Account deletion

**Decision:** Self-service account deletion is blocked while a user owns any device. The user must explicitly transfer or release each device first.

**Rationale:** Automatically releasing devices lets an accidental account deletion change who can control a pump; keeping devices indefinitely locked to a deleted identity creates an avoidable support problem. Explicit lifecycle actions preserve intent and an audit trail.

**Alternatives considered:** Release all devices on account deletion (rejected: unsafe transfer opportunity); permit deletion while retaining locks (rejected: strands hardware); allow deletion with an unverified acknowledgement (rejected: weak accountability).

## Access model

**Decision:** The initial release supports exactly one authoritative owner per device. Household sharing, delegated technician roles, and multiple concurrent owners are deferred.

**Rationale:** A single-owner model minimizes authorization paths while the critical claim, reset, and transfer behavior is validated. It provides a clear audit principal for a safety-relevant pump controller.

**Alternatives considered:** Household member roles now (deferred: requires role and revocation design); unlimited account sharing (rejected: weak auditability and credential-sharing risk).

## Transfer confirmation

**Decision:** A transfer requires confirmation by the current owner and a fresh BLE-local possession proof from the intended recipient, who must be signed in with an eligible durable account. Release remains distinct: after owner confirmation, it returns the device to unclaimed state and the next nearby eligible user follows the normal claim flow.

**Rationale:** A recipient email or account identifier is not proof of physical control. Requiring proximity prevents remote takeover and aligns transfer with the security properties of initial claim.

**Alternatives considered:** Remote owner-to-email transfer (rejected: no physical possession proof); open release followed by any nearby claim (retained only as the explicit release choice, not transfer).

## Ownership pairing availability

**Decision:** A transfer or release uses an owner-authorized, expiring ownership-pairing maintenance request. Firmware turns the pump OFF before temporarily enabling BLE, then stops BLE on completion, cancellation, or expiry while retaining Wi-Fi and the current owner.

**Rationale:** Provisioned devices ordinarily do not advertise BLE. Reusing a bounded maintenance path gives a recipient local proof without turning a reset, a device ID, or an open BLE listener into an ownership-takeover route.

**Alternatives considered:** Permanent BLE after provisioning (rejected: unnecessary attack surface); automatic unclaim before recipient pairing (rejected: expiry would strand the device); remote-only transfer (rejected: lacks possession proof).

## Bootstrap proof

**Decision:** Each device has a unique bootstrap secret, injected during manufacturing/development setup and excluded from Git. Its backend copy is stored as a per-device Google Cloud Secret Manager secret; an operator-only active/revoked registry controls whether the bootstrap endpoint will issue a token. The ESP32 sends `deviceId`, nonce, timestamp, and an HMAC over the request; the backend verifies freshness and consumes the nonce.

**Rationale:** A custom token issuer must authenticate the device without giving it a service-account private key. A device-specific secret limits blast radius compared with a fleet-wide secret, and an active/revoked registry gives operators a non-destructive response to credential compromise.

**Alternatives considered:** A common firmware password (rejected: one compromise affects all devices); app-only pairing proof (deferred: would complicate retry after reboot); service-account JSON (rejected: never safe on device).

## OTA credentials

**Decision:** Development OTA requires a password supplied locally through ignored firmware configuration and a PlatformIO environment variable.

**Rationale:** The prior local network test worked, but an unauthenticated OTA listener accepts firmware from any reachable LAN peer.

**Alternatives considered:** No password (rejected); production OTA endpoint (out of scope); physical action to enable each upload (deferred).

## Recovery

**Decision:** Physical reset is a future GPIO32, normally-open-to-GND, 10-second action. It clears local Wi-Fi/provisioning enrollment only and preserves cloud ownership, immutable device identity, and safety latches.

**Rationale:** GPIO32 is non-strapping on ESP32; a long press prevents accidental action. Preserving ownership avoids transfer through physical access alone.

**Alternatives considered:** EN reset (rejected: reboot only); GPIO0/boot reset (rejected: boot-strapping risk); erase all cloud data (rejected: destructive and insecure).

## Diagnostics

**Decision:** Start the existing TCP log sink only in development builds on a trusted LAN; deployed devices use cloud diagnostics/status.

**Rationale:** The TCP sink has no authentication. It is suitable for developer observability but not permanent deployment exposure.

**Alternatives considered:** Permanent TCP console (rejected); USB-only logging (rejected: defeats cable-free debugging); cloud logs only (retained as production path, insufficient alone for rapid LAN debugging).

## Post-BLE cloud-registration handoff

**Decision:** After terminal BLE provisioning, Android treats the existing authenticated `claimDevice` callable as both the cloud-readiness check and the atomic claim. It displays a `WaitingForCloud` state and retries only retryable results every two seconds for at most 45 attempts (90 seconds). A timeout offers an explicit in-memory retry before an explicit new BLE setup; the raw proof is neither persisted nor logged.

**Rationale:** The ESP32 deliberately closes BLE after it has delivered the final response and starts Wi-Fi/cloud bootstrap. A direct Android RTDB readiness read would be subject to RTDB authorization and offline-cache behavior, adds a check-then-act race, and would require exposing more pairing state. The existing callable is already authenticated, idempotent for the unavailable state, and performs the final ownership transaction atomically. Firebase documents callable functions as the supported Android-to-backend interface and recommends RTDB listeners for server-confirmed updates rather than using local cache as authoritative state. The bounded wait remains well inside the five-minute pairing-proof lifetime.

**Alternatives considered:** Direct read of `/devices/{deviceId}/pairing/current` (rejected: client is intentionally denied access and it leaks pairing state); separate readiness callable (rejected for this iteration: duplicates the claim precondition and cannot replace the final atomic claim); automatically restarting BLE scanning (rejected: the device is expected to have stopped advertising); unbounded polling (rejected: poor UX and obscures a real bootstrap failure).

## Fully fresh Firebase test reset

**Decision:** The test reset remains backup-first and acknowledgement-gated, deletes RTDB and Firebase Auth users, then optionally reseeds exactly one explicitly named active `deviceRegistry/{deviceId}` record from a supplied non-secret Secret Manager resource name. The script does not preserve RTDB data implicitly.

**Rationale:** The device bootstrap endpoint deliberately requires the active registry record before issuing a custom token. A root RTDB deletion removes that record, so a configured ESP32 can join Wi-Fi but cannot authenticate or publish its pairing verifier unless the registry is reseeded. Firebase documents that custom-token sign-in creates the custom UID's Auth record if it no longer exists, so deleting all Auth users is compatible with a subsequent device bootstrap. The reseed contains a resource name, not the secret value.

**Alternatives considered:** Preserve `/deviceRegistry` during the reset (rejected: violates the request for an explicit all-RTDB-data reset and makes tests less deterministic); copy the secret into the reset command or RTDB (rejected: leaks a bootstrap credential); restore the complete backup (rejected: restores ownership, user, pairing, and telemetry test contamination).

## References

- Firebase custom tokens and their one-hour token exchange window: <https://firebase.google.com/docs/auth/admin/create-custom-tokens>
- RTDB rules can use custom claims on `auth.token`: <https://firebase.google.com/docs/database/security/rules-conditions>
- ESP32 strapping-pin guidance: <https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html>
- Firebase anonymous accounts and account upgrade: <https://firebase.google.com/docs/auth/android/anonymous-auth>
- Firebase Android account linking: <https://firebase.google.com/docs/auth/android/account-linking>
- Firebase callable functions: <https://firebase.google.com/docs/functions/callable>
- Firebase Realtime Database Android reads, listeners, and offline behavior: <https://firebase.google.com/docs/database/android/read-and-write>
- Firebase Admin user management and batch deletion: <https://firebase.google.com/docs/auth/admin/manage-users>
- Firebase custom-token sign-in and automatic custom-UID creation: <https://firebase.google.com/docs/auth/admin/create-custom-tokens>
- Firebase App Check Android Play Integrity rollout and enforcement: <https://firebase.google.com/docs/app-check/android/play-integrity-provider>
- TP-Link Tapo account-backed camera onboarding: <https://www.tp-link.com/us/support/faq/2710/>
- Google Home device setup: <https://support.google.com/googlehome/answer/17074648>
- Ring ownership transfer: <https://ring.com/gb/en/support/articles/k8jn9/transfer-device-ownership?redirect=true>
