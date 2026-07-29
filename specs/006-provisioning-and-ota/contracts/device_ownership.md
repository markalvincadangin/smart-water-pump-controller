# Device Ownership and Account Contract

## Identity boundaries

- **Device identity:** `device:{deviceId}` is a firmware-only authorization principal created by bootstrap. It is not a user account and cannot own hardware.
- **User identity:** a non-anonymous, durable SmartFlow account authenticated through a supported provider owns and manages hardware.
- **Setup proof:** a cryptographically random pairing value exchanged over the active BLE setup session. An ownership-pairing proof expires exactly five minutes after backend issuance, cannot be extended, and its raw value is never written to cloud data or logs.

The initial release supports exactly one owner; shared household access and technician/delegate roles are out of scope.

## Account gate

Before a cloud claim or remote pump control, the Android app requires an eligible non-anonymous authenticated user. Initial durable providers are Google and verified email/password. An email/password user must verify their email first; Google sign-in is eligible immediately. If a guest session exists, it must be linked to a durable account before claim; guest identity itself is never recorded as `ownerUid`.

## Claim contract

`claimDevice({ deviceId, pairingProof })`

Preconditions:

- Caller is authenticated with a non-anonymous durable account.
- Device bootstrap has completed and the device is unclaimed.
- The presented proof matches the active pairing verifier and is not expired or consumed.

Success atomically creates:

- authoritative owner state at `/devices/{deviceId}/ownership`;
- compatible `metadata/claimedByUid` value during migration;
- `/users/{callerUid}/devices/{deviceId}` owner index;
- a `claimed` audit event.

Stable failures: `UNAUTHENTICATED`, `DURABLE_ACCOUNT_REQUIRED`, `INVALID_DEVICE`, `EXPIRED_PAIRING`, `ALREADY_CLAIMED`, `CLAIM_IN_PROGRESS`, `CLAIM_UNAVAILABLE`.

The response never identifies the existing owner.

## Legacy-owner migration

Before ownership rules rely on `/ownership/ownerUid`, a trusted backend migration backfills only a consistent legacy pair: `metadata/claimedByUid` and `/users/{uid}/devices/{deviceId}` must name the same UID. It records `migrationState: migrated`. Missing or contradictory records are marked `migrationState: conflict` with a bounded diagnostic code and remain unchanged.

A conflict is resolved only through a trusted, operator-only per-device administrative command. It requires the operator identity, a non-secret evidence reference (for example, an incident or support case ID), and an explicit chosen durable owner UID. The command atomically creates the authoritative owner marker and matching owner index, records `migration_resolved` with the operator and evidence reference, and marks the migration resolved. It is never exposed as a Firebase callable, RTDB client write, or firmware command. It cannot release, transfer, or choose an owner implicitly.

## Release and transfer contract

Only the recorded owner can request release or start a transfer. Starting either action sends an owner-authorized ownership-pairing request to the online device that expires exactly five minutes after backend issuance. It cannot be extended; the owner must start a new request after expiry. Firmware first calls `setPump(false)`, then exposes BLE only for that pairing interval; it does not clear Wi-Fi, ownership, or safety latches. A transfer includes an intended eligible recipient UID and completes only after that recipient provides a fresh BLE-local proof. A release keeps the original owner until a nearby eligible user completes the release pairing and becomes the new owner. Expiry or cancellation stops BLE, retains the original owner, and leaves the pump OFF. Both record an audit event. Lost-owner recovery is out of scope for this release. Local reset, Wi-Fi reprovision, app uninstall, or a direct client write can never release or transfer ownership.

## Account deletion

Within the SmartFlow app, self-service account deletion is denied while the account owns one or more devices. The app must call the ownership eligibility check and direct the owner to transfer or release each device first; it must not invoke Firebase account deletion until the check succeeds. A denied account-deletion attempt must not mutate ownership data.

## Authorization rules

- Android clients may read devices they own and submit callable ownership requests.
- Android clients do not directly write `claimedByUid`, `/ownership`, `/users/{uid}/devices`, pairing proofs, or maintenance request records.
- A device principal may write only its own permitted operational and pairing-verifier paths.
- Backend ownership functions execute the atomic authoritative writes and audit records.
