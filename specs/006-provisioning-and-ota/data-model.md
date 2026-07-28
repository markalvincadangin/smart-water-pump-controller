# Data Model: Provisioning, Identity, and Recovery

All RTDB changes are additive. `deviceId` is the canonical immutable SmartFlow ID, for example `SF-67D42C`.

## Device identity

`/devices/{deviceId}/metadata`

| Field | Type | Writer | Rule |
|---|---|---|---|
| `deviceAuthUid` | string | Cloud Function | Must equal `device:{deviceId}` |
| `claimedByUid` | string | Owner claim flow | Set only by an authorized owner flow |
| `provisioningState` | string | Firmware / Function | `unprovisioned`, `connecting`, `registered`, `provisioned`, `failed` |
| `updatedAtMs` | number | Authorized writer | Server timestamp |

## Bootstrap exchange

The exchange request is HTTPS, not an RTDB node.

| Field | Type | Validation |
|---|---|---|
| `deviceId` | string | Canonical device ID; allowlisted by backend |
| `timestampMs` | number | Within backend freshness window |
| `nonce` | string | Cryptographically random and single-use |
| `proof` | string | HMAC of canonical request using the device-specific bootstrap secret |

The response contains a Firebase custom token only. It does not return a service-account key or store the bootstrap secret in RTDB.

The backend stores each bootstrap secret in a per-device Google Cloud Secret Manager secret. An operator-only device registry stores only non-secret lifecycle metadata (`active` or `revoked`, creation time, revocation reason) and is not writable by devices or Android clients.

## Durable ownership and pairing

`/devices/{deviceId}/ownership`

| Field | Type | Writer | Validation |
|---|---|---|---|
| `ownerUid` | string | Claim/release/transfer backend function | Durable, non-anonymous account UID; mirrors `metadata/claimedByUid` during migration |
| `state` | string | Backend function | `unclaimed`, `claimed`, `transfer_pending`, `release_pending` |
| `claimedAtMs` | number | Backend function | Backend-generated |
| `updatedAtMs` | number | Backend function | Backend-generated |
| `migrationState` | string | Backend migration | `not_required`, `migrated`, `conflict` |
| `migrationConflictCode` | string | Backend migration | Bounded code; present only when `migrationState` is `conflict` |
| `migrationResolvedAtMs` | number | Trusted operator workflow | Present only after an audited conflict resolution |
| `migrationResolutionAuditId` | string | Trusted operator workflow | Immutable audit-event ID; present only after resolution |
| `transferId` | string | Transfer backend function | Present only while `state` is `transfer_pending` |
| `pendingRecipientUid` | string | Transfer backend function | Eligible durable recipient; present only while transfer is pending |
| `transferExpiresAtMs` | number | Transfer backend function | Backend-generated, short-lived expiry |
| `ownershipPairingRequestId` | string | Backend function | Active maintenance request while pairing is available |
| `ownershipPairingExpiresAtMs` | number | Backend function | Exact, non-extendable expiry for either transfer or release pairing |

`/users/{uid}/devices/{deviceId}` remains the owner index during this feature. It is written only by trusted backend ownership functions, never directly by an Android client.

The initial release permits one `ownerUid` and one matching owner-index entry per device. No member, technician, delegate, or multi-owner collection is created in this feature.

### Legacy-owner migration

Before ownership rules use `/ownership/ownerUid` as authoritative, the backend backfills it only where `metadata/claimedByUid` and the matching `/users/{uid}/devices/{deviceId}` index agree. It writes `migrationState: "migrated"` and preserves that owner. Missing or contradictory legacy data is written as `migrationState: "conflict"` with a bounded conflict code; ownership is left unchanged. Only a trusted, per-device administrative workflow may resolve a conflict: it requires an operator identity, evidence reference, explicit chosen owner UID, and atomically writes the owner marker/index plus a `migration_resolved` audit event. It is not a callable function and cannot be invoked by an Android client or device principal. No migration or resolution path releases or transfers a device.

`/devices/{deviceId}/pairing/current`

| Field | Type | Writer | Validation |
|---|---|---|---|
| `verifier` | string | Firmware using device identity | One-way verifier of the raw BLE-delivered proof; never the raw proof |
| `expiresAtMs` | number | Firmware | Short-lived setup window |
| `sessionId` | string | Firmware | Random active setup session identifier |
| `purpose` | string | Firmware | `claim`, `transfer`, or `release` |
| `state` | string | Firmware / backend | `active`, `consumed`, `expired`, `cancelled` |

The Android app supplies the raw proof only to the callable claim operation. The backend validates and consumes it atomically; non-owners and clients cannot write pairing or ownership paths.

### Transfer state

`transfer_pending` is created only by the current owner and contains a random `transferId`, `pendingRecipientUid`, short-lived `transferExpiresAtMs`, and an ownership-pairing request. Firmware processes that request by calling `setPump(false)`, temporarily enabling BLE, and publishing a `transfer` pairing verifier. The intended recipient must be authenticated as `pendingRecipientUid` and present that fresh proof before the backend atomically replaces the owner/index and writes `transfer_completed`. Expiry or owner cancellation stops BLE, preserves the original owner, and writes a bounded audit outcome.

`release_pending` similarly keeps the original owner until a nearby eligible user consumes a fresh `release` pairing proof. It never becomes unclaimed merely because the session expires or is cancelled.

`/devices/{deviceId}/ownership/audit/{eventId}` records `action` (`claimed`, `released`, `transfer_started`, `transfer_completed`, `claim_rejected`), opaque actor/recipient UIDs when applicable, a bounded result code, and backend timestamp. It must never store Wi-Fi credentials, raw pairing proofs, user passwords, or another owner's personal profile.

## Maintenance request

`/devices/{deviceId}/maintenance/requests/{requestId}`

| Field | Type | Validation |
|---|---|---|
| `action` | string | `WIFI_REPROVISION` or `OWNERSHIP_PAIRING` |
| `requestedByUid` | string | Set by Cloud Function from authenticated caller |
| `issuedAtMs` / `expiresAtMs` | number | Backend-generated; `OWNERSHIP_PAIRING` expires exactly 300,000 ms after issuance |
| `nonce` | string | Unique per request |
| `status` | string | `pending`, `acknowledged`, `completed`, `rejected`, `expired` |
| `resultCode` | string | Bounded machine-readable result |

For `OWNERSHIP_PAIRING`, the request also contains a bounded `purpose` (`transfer` or `release`) and a pairing expiry exactly five minutes after issuance. The pairing verifier and transfer expiry cannot outlive that request and requests cannot be extended. Firmware treats it as a non-destructive temporary BLE action: it turns the pump OFF before advertising, then retains Wi-Fi, ownership, and safety state when pairing is consumed, cancelled, or expires.

State transition: `pending` → `acknowledged` → `completed`; invalid, expired, or replayed requests transition to `rejected`/`expired` without clearing local state.

## Audit event

`/devices/{deviceId}/maintenance/audit/{eventId}` records `requestId`, `action`, `actorUid`, `outcome`, `createdAtMs`, and an optional safe diagnostic code. The Cloud Function creates intent records; firmware records the device outcome.

## Local state boundary

| State | Full physical reset | Owner Wi-Fi reprovision |
|---|---|---|
| Wi-Fi SSID/password | Clear | Clear |
| Device-auth enrollment | Clear | Clear |
| Immutable device ID | Retain | Retain |
| Cloud ownership | Retain | Retain |
| Safety latches/configuration | Retain | Retain |
| Processed-request replay records | Clear only when local auth enrollment is intentionally reset | Retain until expiry window passes |

Local enrollment resets never modify the durable ownership or pairing-audit records.
