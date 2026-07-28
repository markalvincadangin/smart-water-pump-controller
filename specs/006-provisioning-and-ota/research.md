# Phase 0: Research & Spike

## Android App UI (Provisioning Routing)
- **Current State**: `MainActivity.kt` uses Jetpack Compose `NavHost`. When a user logs in, the start destination is `device_list`. Currently, `DeviceListScreen` uses a hardcoded list of devices (`"SmartFlow-123"`, `"SmartFlow-456"`).
- **Requirement**: FR-001 states we must route users to the Provisioning flow if a device is not yet claimed. 
- **Decision**: We will update `MainActivity.kt` to dynamically fetch the user's claimed devices from Firebase (or use a ViewModel for `MainActivity`). If the list is empty upon fetching, it automatically navigates to `provisioning`. The actual BLE provisioning logic in `BleProvisioningClient` and `ProvisioningScreen` are already implemented and ready to use.

## Local Network OTA (PlatformIO)
- **Current State**: `platformio.ini` only defines `[env:esp32dev]` which defaults to serial USB upload. `main.cpp` does not include or initialize `ArduinoOTA`.
- **Requirement**: FR-003 requires local network OTA flashes (via ArduinoOTA) configured as a selectable environment.
- **Decision**: 
  - Add `[env:esp32dev_ota]` to `firmware/master_node/platformio.ini` with `upload_protocol = espota` and `build_flags = -D ENABLE_OTA`.
  - In `firmware/master_node/src/main.cpp`, conditionally `#include <ArduinoOTA.h>`, call `ArduinoOTA.begin()` when Wi-Fi is connected, and call `ArduinoOTA.handle()` in `loop()` only if `ENABLE_OTA` is defined. This ensures production firmware memory footprints remain small and secure.
