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

## References

- Firebase custom tokens and their one-hour token exchange window: <https://firebase.google.com/docs/auth/admin/create-custom-tokens>
- RTDB rules can use custom claims on `auth.token`: <https://firebase.google.com/docs/database/security/rules-conditions>
- ESP32 strapping-pin guidance: <https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html>
- Firebase anonymous accounts and account upgrade: <https://firebase.google.com/docs/auth/android/anonymous-auth>
- Firebase Android account linking: <https://firebase.google.com/docs/auth/android/account-linking>
- TP-Link Tapo account-backed camera onboarding: <https://www.tp-link.com/us/support/faq/2710/>
- Google Home device setup: <https://support.google.com/googlehome/answer/17074648>
- Ring ownership transfer: <https://ring.com/gb/en/support/articles/k8jn9/transfer-device-ownership?redirect=true>
