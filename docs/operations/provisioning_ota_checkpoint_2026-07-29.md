# Provisioning, OTA, and Ownership Checkpoint — 2026-07-29

## Purpose

This is the resume point for Spec Kit feature `006-provisioning-and-ota` on branch `feature/provisioning-and-ota`.

## Completed and build-verified

- Durable Firebase account gate and removal of normal anonymous ownership flow.
- Stable device bootstrap identity using `device:{deviceId}` custom-token authentication.
- BLE pairing proofs are short-lived, purpose-bound, hashed before RTDB publication, and never logged as raw values.
- Atomic backend ownership claim plus audited transfer/release lifecycle.
- Owner-authorized Wi-Fi recovery: the backend creates an expiring, audited request; firmware validates it, calls `setPump(false)` before clearing only local Wi-Fi/device-auth enrollment, then restarts into BLE onboarding without changing cloud ownership or safety latches.
- Development OTA configuration and development TCP logger implementation.
- Android owner management, nearby transfer/release claim, and explicit Wi-Fi recovery confirmation/handoff.

## Verification evidence

- `functions`: `npm test` passed (15 tests); `npm run build` passed.
- Android: `./gradlew.bat :app:assembleDebug` passed.
- Firmware: `pio run -e esp32dev_usb_ota` and `pio run -e esp32dev_ota` passed. The latter emits its normal missing-`upload_port` warning after compiling because no OTA target was supplied; no firmware upload occurred.
- Current task tracker: 21 complete, 17 open in `specs/006-provisioning-and-ota/tasks.md`.

## Deliberately not claimed as complete

No Firebase deployment, USB flash, OTA upload, physical pump-safety observation, BLE recovery exercise, or TCP-log connection test was performed for this checkpoint. Do not represent any of those as validated.

## Resume order

1. Deploy the reviewed Functions and RTDB rules to the designated test project, then run the account/claim/ownership matrix.
2. With the ESP32 connected, execute the owner Wi-Fi recovery matrix in the feature quickstart and record pump-off, ownership, and latch-preservation evidence.
3. Perform accepted and rejected OTA uploads using the device IP and local OTA password; verify post-reboot identity and safety state.
4. Validate buffered/live development TCP logging, then verify production does not expose port 2323.
5. Finish the remaining Spec Kit tasks and run `speckit-analyze` plus `speckit-converge` before feature finalization.

## Safety boundary

Never use blanket NVS erase for recovery. The recovery boundary preserves device identity, cloud ownership, dry-run/overflow/E-stop state, and safety configuration. Any physical test must confirm the pump transitions OFF before enrollment data changes.
