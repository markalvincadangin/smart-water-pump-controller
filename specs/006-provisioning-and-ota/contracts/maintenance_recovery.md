# Maintenance and Recovery Contract

## OTA

- Development OTA is available only in the `esp32dev_ota` environment.
- The device requires `SMARTFLOW_OTA_PASSWORD`; the uploader supplies the same value through its local environment.
- The password is never committed, logged, or exposed to the Android app.

## Device bootstrap HTTPS contract

`POST /bootstrapDevice`

Request fields: `deviceId`, `timestampMs`, `nonce`, `proof`.

Success response: `{ "customToken": "…", "deviceUid": "device:{deviceId}" }`.

Failure responses use stable codes: `INVALID_DEVICE`, `REVOKED_DEVICE`, `INVALID_PROOF`, `EXPIRED_REQUEST`, `REPLAYED_NONCE`, `RATE_LIMITED`.

The function verifies the HMAC and nonce before creating a custom token. The token contains only `role: "device"` and `deviceId` authorization claims.

## Owner Wi-Fi reprovision contract

The Android app calls an authenticated callable function with `{ deviceId }`. The function verifies the authoritative owner marker and atomically creates a short-lived `WIFI_REPROVISION` request, audit record, and `maintenance/activeWifiReprovisionRequestId` pointer. Firmware reads only that pointer; it does not scan arbitrary maintenance records.

Firmware accepts a request only when its device ID, action, nonce, pending status, expiry, and replay marker are valid. Its first state-changing action is `setPump(false)`; it records the local replay marker, clears Wi-Fi and device-auth enrollment only, and reboots to BLE provisioning. The server-side request/audit record remains durable for operator visibility. Maintenance handling must not make `reset_stop` or `clear_error` unreachable.

Clients never write maintenance requests directly to RTDB. Cloud commands never call blanket `nvs_flash_erase()` and never clear dry-run, overflow, or E-stop state.

## Ownership pairing maintenance contract

Only the current owner may request temporary ownership pairing for an online device. The backend creates an `OWNERSHIP_PAIRING` request with a `transfer` or `release` purpose, an expiry exactly five minutes after issuance, and an audit event. The request and its pairing verifier cannot be extended or renewed; a new owner-authorized request is required after expiry. Firmware validates replay and expiry, calls `setPump(false)` before enabling BLE, publishes the appropriate short-lived pairing verifier, and stops BLE on completion, cancellation, or expiry. It retains Wi-Fi credentials, device identity, cloud ownership, dry-run/overflow/E-stop state, and all safety configuration. Expiry or cancellation leaves the pump OFF and does not release or transfer ownership.

## Reset behavior

The future GPIO32 button is normally open to GND and uses a pull-up. A continuous 10-second press performs the local reset boundary above; short presses are ignored. EN and boot/strapping pins are not used as maintenance controls.

## Diagnostics

Development builds may expose read-only TCP logs on port 2323 to a trusted LAN. Production builds do not start this listener and instead publish bounded diagnostics/status through cloud paths.
