# Provisioning, OTA, and Ownership Checkpoint — 2026-07-29

## Purpose

This is the resume point for Spec Kit feature `006-provisioning-and-ota` on branch `feature/provisioning-and-ota`.

## Completed and build-verified

- Durable Firebase account gate and removal of normal anonymous ownership flow.
- Stable device bootstrap identity using `device:{deviceId}` custom-token authentication.
- BLE pairing proofs are short-lived, purpose-bound, hashed before RTDB publication, and never logged as raw values.
- Atomic backend ownership claim plus audited transfer/release lifecycle.
- Owner-authorized Wi-Fi recovery: the backend creates an expiring, audited request; firmware validates it, calls `setPump(false)` before clearing only local Wi-Fi/device-auth enrollment, then restarts into BLE onboarding without changing cloud ownership or safety latches.
- Development OTA configuration and modular development TCP log-sink implementation.
- Android owner management, nearby transfer/release claim, and explicit Wi-Fi recovery confirmation/handoff.
- Live fresh-start reset: the acknowledgement-gated reset deleted the prior Firebase Auth users and RTDB state, then retained only the non-secret `deviceRegistry/SF-67D42C` seed.
- Live OTA validation: an incorrect local OTA password was rejected; the authenticated `esp32dev_ota_reprovision` image was accepted and transferred over LAN. The Windows callback rule is restricted to TCP 20000 from `192.168.1.0/24` on Private and Public profiles.
- Live provisioning and durable-claim validation: a new Google account completed BLE provisioning and successfully claimed `SF-67D42C`. RTDB shows one authoritative owner mapping for that durable user, a consumed hashed pairing proof, and the device reporting `lifecycle: ONLINE` under its stable `device:SF-67D42C` principal.
- Follow-up normal OTA validation: the corrected one-time re-provision image (`r16`) cleared Wi-Fi enrollment and reopened BLE. A subsequent normal `esp32dev_ota` image was verified not to contain the re-provision request, was accepted through authenticated LAN OTA, and the device returned to `192.168.1.21` after reboot. Development TCP logging was live-validated: the first connection received a live `TCP console client connected` record and application diagnostics; after clean disconnect, the second connection received the buffered/live delimiters and replayed the first connection event. A production image compiled without development TCP markers.

## Verification evidence

- `functions`: `npm test` passed (15 tests); `npm run build` passed.
- Android: `./gradlew.bat :app:assembleDebug` passed.
- Firmware: `pio run -e esp32dev_usb_ota`, `pio run -e esp32dev_ota`, and `esp32dev_ota_reprovision` passed. The latter was uploaded successfully after an incorrect-password rejection check.
- Current task tracker retains production-runtime port-closure and 50-event-retention validation as open work in `specs/006-provisioning-and-ota/tasks.md`.

## Deliberately not claimed as complete

Physical pump-safety observation, second-phone transfer/release testing, owner Wi-Fi recovery, production-build port-2323 runtime negative testing, and a live 50-event cloud-retention test remain unvalidated. Do not represent those as validated.

## Resume order

1. With the ESP32 connected, execute the owner Wi-Fi recovery matrix in the feature quickstart and record pump-off, ownership, and latch-preservation evidence.
2. Validate second-phone durable access and the transfer/release ownership-pairing matrix.
3. Verify a production device image does not expose port 2323 at runtime. The cloud retention cap is live: `retainDeviceEvents` repaired the observed oversized history and retained exactly 50 WARN/ERROR records on 2026-07-29.
4. Add and validate the future GPIO32 physical long-press reset path when hardware is available.
5. Finish the remaining Spec Kit tasks and run `speckit-analyze` plus `speckit-converge` before feature finalization.

## Safety boundary

Never use blanket NVS erase for recovery. The recovery boundary preserves device identity, cloud ownership, dry-run/overflow/E-stop state, and safety configuration. Any physical test must confirm the pump transitions OFF before enrollment data changes.
