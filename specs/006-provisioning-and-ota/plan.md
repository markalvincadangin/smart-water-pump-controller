# Implementation Plan: Provisioning & Wireless Flashing

**Branch**: `feature/provisioning-and-ota` | **Date**: 2026-07-28 | **Spec**: [spec.md](file:///c:/Users/markc/_Projects/micro-controller/smartflow/specs/006-provisioning-and-ota/spec.md)

**Input**: Feature specification from `specs/006-provisioning-and-ota/spec.md`

---

## Summary

This feature activates BLE-based provisioning in the Android App (so new, unclaimed devices guide the user through setup) and enables `ArduinoOTA` in the `master_node` firmware strictly for the local development environment (`platformio.ini`) to allow wireless flashing during active development.

## User Review Required

> [!IMPORTANT]  
> The OTA implementation will use an `ENABLE_OTA` preprocessor macro. This ensures the heavy `ArduinoOTA` library is compiled into the firmware *only* when you select the `esp32dev_ota` environment in PlatformIO, keeping the production firmware light and secure. 
> Please review the proposed changes below and let me know if you approve this approach!

## Proposed Changes

### Android App
- Fetch actual claimed devices from `DeviceRepository` on load (instead of using the mock `"SmartFlow-123"` list).
- If the fetched list of devices is empty, auto-navigate to the `provisioning` route to guide the user into setting up their pump.

#### [MODIFY] [MainActivity.kt](file:///c:/Users/markc/_Projects/micro-controller/smartflow/app/src/main/java/com/smartflow/MainActivity.kt)
#### [MODIFY] [DeviceListScreen.kt](file:///c:/Users/markc/_Projects/micro-controller/smartflow/app/src/main/java/com/smartflow/presentation/DeviceListScreen.kt)

---

### Firmware (master_node)

- Add a new environment block `[env:esp32dev_ota]` inheriting from the default.
- Set `upload_protocol = espota`.
- Add `-D ENABLE_OTA` to `build_flags`.

#### [MODIFY] [platformio.ini](file:///c:/Users/markc/_Projects/micro-controller/smartflow/firmware/master_node/platformio.ini)

- Include `<ArduinoOTA.h>` wrapped in `#ifdef ENABLE_OTA`.
- In `setup()` or `loop()`, after Wi-Fi is connected, run `ArduinoOTA.begin()` once.
- In `loop()`, run `ArduinoOTA.handle()` to listen for incoming wireless flashes, wrapped in `#ifdef ENABLE_OTA`.

#### [MODIFY] [main.cpp](file:///c:/Users/markc/_Projects/micro-controller/smartflow/firmware/master_node/src/main.cpp)

---

## Constitution Gate

| Principle | Status | Evidence |
|-----------|--------|----------|
| I. Fail Toward Pump OFF | Pass | OTA restarts implicitly reset the hardware relay state to OFF (safe default). |
| II. Dry-Run Lockout | N/A | |
| III. Overflow Protection | N/A | |
| IV. TOR Independence | Pass | OTA doesn't change physical sensors/relay paths. |
| V. Sensor Freshness / E-Stop | N/A | |
| VI. Backward Compatibility | N/A | No RTDB schema change is introduced for this dev-only feature. |

---

## Verification Plan

### Automated Tests
- App builds clean without regression.
- Firmware compiles clean for both `esp32dev` (prod) and `esp32dev_ota` (dev).

### Manual Verification
- You can manually test by compiling with `env:esp32dev_ota` and ensuring the firmware uploads wirelessly in VSCode/PlatformIO.
- You can validate the App by logging into an account with no devices; it should immediately prompt the provisioning screen.
