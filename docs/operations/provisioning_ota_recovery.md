# Provisioning, Device Bootstrap, OTA, and Recovery

## Per-device bootstrap setup

Each ESP32 requires a distinct random bootstrap secret. Never use a fleet-wide value and never store a bootstrap secret in RTDB, Android resources, Git, or a service-account file.

1. Derive the device ID from the Wi-Fi MAC as `SF-` followed by the final six uppercase hexadecimal characters. For example, MAC `20:E7:C8:67:D4:2C` becomes `SF-67D42C`.
2. Create the matching Secret Manager secret and add a random version. The deployed Cloud Functions service account needs `roles/secretmanager.secretAccessor` on that secret.
3. Call `setDeviceBootstrapState` as an account with the Firebase custom claim `admin: true`, using `state: "active"` and the fully qualified Secret Manager name when it is not the default name.
4. Put the same per-device secret, deployed `bootstrapDevice` URL, and public Google trust-root PEM in the ignored firmware `secrets.h`.
5. Deploy Functions and RTDB rules together, then provision the ESP32. A successful boot writes metadata with `deviceAuthUid: "device:{deviceId}"`.

Example operator commands (replace placeholders; do not put the secret in shell history for production use):

```powershell
gcloud secrets create smartflow-bootstrap-sf-67d42c --replication-policy=automatic
gcloud secrets versions add smartflow-bootstrap-sf-67d42c --data-file=bootstrap-secret.txt
firebase deploy --only functions:bootstrapDevice,functions:setDeviceBootstrapState,database
```

## OTA development

Set `SMARTFLOW_OTA_PASSWORD` in both ignored firmware configuration and the developer shell. The values must match.

```powershell
$env:SMARTFLOW_OTA_PASSWORD = 'your-local-ota-password'
cd firmware\master_node
pio run -e esp32dev_ota --target upload --upload-port <device-ip>
```

Verify one rejected upload with a missing/wrong password before treating OTA as available. USB remains the recovery path until the device is confirmed reachable through authenticated OTA.

## Recovery safety boundary

The current owner can request Wi-Fi recovery through the callable backend. The backend writes a five-minute `WIFI_REPROVISION` record, an audit entry, and one `maintenance/activeWifiReprovisionRequestId` pointer. Firmware reads that pointer, validates the matching pending request, nonce, expiry, and local replay marker, then calls `setPump(false)` before clearing only Wi-Fi and device-auth enrollment and restarting into BLE onboarding.

This does not remove the durable cloud owner, immutable device identity, dry-run/overflow/E-stop state, safety configuration, or historical maintenance/audit records. A physical-device validation is still required before relying on this recovery path in deployment.

## Ownership pairing and migration operations

An ownership change is never caused by a local reset, Wi-Fi reprovision, app removal, or a direct RTDB write. The recorded owner starts a release or transfer request; once the online device receives that request, it must turn the pump OFF before advertising BLE for a single, non-extendable five-minute pairing interval. Expiry or cancellation stops that temporary BLE session while retaining the current owner and all safety latches.

Legacy ownership migration is dry-run first. A record is migrated only when the deprecated `metadata/claimedByUid` marker and exactly one `/users/{uid}/devices/{deviceId}` entry agree. Missing or conflicting records are frozen as `migrationState: conflict`; do not repair them by editing RTDB.

An operator with the Firebase `admin: true` custom claim resolves one frozen device at a time with a chosen eligible owner UID and a non-secret support or incident reference. Use the trusted script only after building Functions, review its dry-run output first, and pass `--apply` only after that review:

```powershell
cd functions
npx ts-node scripts\resolve_ownership_migration_conflict.ts `
  --device-id SF-67D42C --owner-uid <uid> --operator-uid <admin-uid> `
  --evidence CASE-123
```

The script writes an immutable `migration_resolved` audit event. It is intentionally neither a Firebase callable nor a firmware command.
