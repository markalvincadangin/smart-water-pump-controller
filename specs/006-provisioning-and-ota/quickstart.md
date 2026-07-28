# Provisioning, OTA, and Recovery Validation

## Prerequisites

- ESP32 is powered and on the same LAN as the development computer.
- `SMARTFLOW_OTA_PASSWORD` is present in the local upload environment and firmware-only ignored configuration.
- Android app has a durable signed-in owner account (Google or verified email/password); Firebase emulator or test project is available for backend tests.

## Build checks

```powershell
cd app
.\gradlew.bat assembleDebug

cd ..\functions
npm run build

cd ..\firmware\master_node
pio run -e esp32dev_usb_ota
pio run -e esp32dev_ota
```

## OTA checks

1. Upload through `esp32dev_ota` with the configured password and confirm PlatformIO reports success.
2. Attempt an upload with no/incorrect password; confirm rejection and that the running application remains intact.
3. Reconnect to the device after restart and confirm normal Wi-Fi/cloud operation.

## Provisioning checks

1. Clear only local Wi-Fi/provisioning enrollment on a test device.
2. Android discovers the BLE service, receives streamed scan records, submits credentials, and displays progress.
3. The app reports success only after the final `provisioned` status, not after Wi-Fi association alone.
4. Confirm the device restarts and its RTDB updates use the stable `device:{deviceId}` authorization identity.
5. During active BLE setup, confirm the app receives the locally exchanged pairing proof but no raw proof is readable from cloud data.
6. Claim through the backend and verify exactly one owner index, owner marker, and audit event are created.
7. Sign in with the same durable account on a second phone and confirm the device appears without repeating provisioning.
8. Verify an anonymous user, a guessed device ID, expired proof, and a second user cannot change ownership. Verify the already-claimed result does not reveal the current owner.
9. Record elapsed setup time from scan start to successful claim and verify it is no more than two minutes.

## Legacy migration checks

1. Create a consistent legacy pair (`metadata/claimedByUid` plus matching user-device index) and verify migration preserves the owner and writes `migrationState: migrated`.
2. Create missing and contradictory legacy ownership fixtures and verify migration writes a bounded conflict state without releasing, transferring, or guessing ownership.
3. From the trusted operator environment, resolve one specified frozen fixture with an explicit chosen owner UID and non-secret evidence reference. Verify one `migration_resolved` audit event and matching owner/index writes; verify app users and device principals cannot invoke the resolution path.

## Ownership lifecycle checks

1. Verify an explicit owner release/transfer creates an audit event and permits only the intended next owner to claim.
2. Reset local Wi-Fi enrollment and confirm the device re-enters BLE onboarding while the durable cloud owner remains unchanged.
3. Remove an obsolete anonymous test account and confirm it does not unclaim or transfer any device.
4. Start a transfer and verify the online device turns the pump OFF, exposes temporary BLE, and only the intended recipient can complete it with a fresh proof within five minutes of backend issuance.
5. Start a release and verify the original owner remains until a nearby eligible replacement claim succeeds.
6. Verify transfer/release cancellation or expiry at five minutes stops BLE, retains Wi-Fi and the original owner, preserves safety latches, and leaves the pump OFF. Verify no renewal or extension succeeds; a new explicit owner request is required.
7. Attempt SmartFlow app account deletion while owning a device and confirm the app denies it before invoking account deletion.

## Recovery checks

1. As the current owner, request Wi-Fi reprovision through the maintenance UI.
2. Confirm pump OFF transition, durable server-side request/audit record, BLE onboarding, retained ownership, and retained safety latches.
3. Verify non-owner, expired, malformed, and replayed requests are rejected without changing device state.
4. With the future GPIO32 hardware installed, verify a short press does nothing and a continuous 10-second press resets only local enrollment.

## Diagnostics checks

1. In a development build, connect to `<device-ip>:2323` and confirm a read-only log stream after Wi-Fi joins.
2. In a production build, confirm port 2323 is not listening and that safe diagnostics are visible through the cloud path.
