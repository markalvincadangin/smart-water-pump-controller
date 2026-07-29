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

1. For a fully fresh test, run the acknowledgement-gated test reset with the named device registry reseed option. Confirm that RTDB contains only `deviceRegistry/{deviceId}` with `state: active` and its non-secret Secret Manager reference; no owner, pairing, telemetry, user index, or prior Firebase Auth user may remain.
2. Create or sign in to a durable Google or verified email/password account after the reset.
3. OTA-upload a build with a new one-time reprovision request ID. Confirm it clears only local Wi-Fi/device enrollment, retains the immutable device ID and safety state, and advertises BLE.
4. Android discovers the BLE service, receives streamed scan records, submits credentials, and displays progress.
5. After BLE reports `provisioned`, confirm the app displays SmartFlow Cloud registration progress while it performs at most 45 authenticated callable-claim attempts at two-second intervals. It must not return to BLE scanning during this phase.
6. Confirm the cloud claim creates exactly one owner index, owner marker, and audit event. If the bounded wait expires before the proof expires, confirm **Retry cloud registration** retries the callable claim without scanning; **Start provisioning again** is a separate action.
7. Confirm the device restarts and its RTDB updates use the stable `device:{deviceId}` authorization identity.
8. During active BLE setup, confirm the app receives the locally exchanged pairing proof but no raw proof is readable from cloud data.
9. Sign in with the same durable account on a second phone and confirm the device appears without repeating provisioning.
10. Verify an anonymous user, a guessed device ID, expired proof, and a second user cannot change ownership. Verify the already-claimed result does not reveal the current owner.
11. Record elapsed setup time from scan start to successful claim and verify it is no more than two minutes.

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

1. In a development build, connect one TCP client to `<device-ip>:2323` after Wi-Fi joins. Confirm the `Buffered Logs` and `Live Logs` delimiters, at least one live application record, then disconnect. Reconnect after the previous client is closed and confirm the previous record is replayed before new live records. The console accepts one active client; a concurrent client is deliberately rejected.
2. In a production build, confirm port 2323 is not listening and that safe diagnostics are visible through the cloud path.
3. Generate more than 50 WARN/ERROR events and confirm the trusted `retainDeviceEvents` backend trigger leaves `/devices/{deviceId}/events` with only the 50 newest push-ID-ordered records; INFO/DEBUG records must not be uploaded.
