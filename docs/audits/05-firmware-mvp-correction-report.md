# WP-05 — Firmware/MVP Correction Report

Status: implementation complete; software verification partial.

## Changes

- Countdown requests are now accepted only for 1–120 minutes.
- Invalid countdown durations are rejected with COMMAND_REJECTED instead of using a fallback duration.
- Level and flow sensor bypass defaults are now false.
- Cloud safety settings are range-checked before being applied.
- Persisted maximum runtime is constrained to 30–120 minutes.
- The user-facing maximum-runtime fault code is now MAX_RUNTIME.

## Scope preserved

AUTO was not completed or promoted to MVP. E-stop, dry-run, minimum OFF-time, RS-485 behavior, relay behavior, Android behavior, and Firebase schema were not changed by this work package.

The internal legacy names isOverflowError and STOP_OVERFLOW remain temporarily because they are shared with the current Android reported-state contract. Terminology normalization is deferred to the Firebase/Android audit.

## Verification

The implementation diff contains exactly five modified firmware files:

1. firmware/master_node/src/cloud/cloud_manager.cpp
2. firmware/master_node/src/cloud/device_shadow.cpp
3. firmware/master_node/src/core/app/pump_app.cpp
4. firmware/master_node/src/persistence/persistence.cpp
5. firmware/master_node/src/state/state.cpp

The repository currently has CI for Android and Cloud Functions, but no firmware build job. Therefore firmware compilation has not been claimed as CI-verified.

Physical commissioning is also still outstanding.

## Remaining verification

- Firmware CI build
- Physical E-stop verification
- Relay/contactor fail-safe verification
- Dry-run verification with the actual pump
- Maximum-runtime verification with the actual pump
- Sensor and RS-485 failure verification
- Tank-full verification
- Reboot/power-loss recovery
- Deployed Firebase/Cloud Functions verification
- Android runtime command confirmation

## Acceptance

- [x] Countdown boundary
- [x] Fail-safe bypass defaults
- [x] Cloud safety-setting validation
- [x] Persisted runtime ceiling
- [x] MAX_RUNTIME user-facing code
- [x] AUTO remains outside MVP
- [x] No physical verification claimed
- [ ] Firmware CI
- [ ] Hardware commissioning
