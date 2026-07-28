# Quickstart Validation Guide

Follow these steps to validate the Android App UI/UX and RTDB integration.

## Prerequisites
- Android Studio or `./gradlew` CLI tools.
- A physical Android device or emulator running API 26+.
- The `google-services.json` must be present in `app/`.

## 1. Setup Data Mocking
Since we are validating the UI without necessarily having the physical ESP32 powered on, we can mock the RTDB data.
1. Open the Firebase Console for project `smartflow-fed87`.
2. Navigate to Realtime Database -> Data.
3. Manually create a node `/devices/TEST_DEVICE/telemetry` with `waterLevel: 50`.
4. The Android app should be configured or hardcoded temporarily to connect to `TEST_DEVICE` (or whatever the active BLE provisioned device ID is).

## 2. Build and Run the App
```bash
cd app
./gradlew installDebug
```
Launch the SmartFlow app on the device.

## 3. Validation Scenarios

### Scenario 1: Real-time Telemetry Animation
- **Action**: In Firebase Console, change `telemetry/waterLevel` from 50 to 80.
- **Expected Outcome**: The Tank Level Card in the app instantly animates the wave fill from 50% to 80% with a smooth transition.

### Scenario 2: Remote Control Command
- **Action**: In the app, switch mode to MANUAL and tap the Pump Toggle button.
- **Expected Outcome**: In Firebase Console, observe `/devices/TEST_DEVICE/shadow/desired/pumpState` flip to `true`. The app UI might show a loading spinner or immediately update depending on the optimistic UI strategy.

### Scenario 3: Device Configuration
- **Action**: Open the config bottom sheet in the app and slide the "Low Level Threshold" to 35%.
- **Expected Outcome**: Firebase Console shows `/devices/TEST_DEVICE/config/lowLevelThreshold` changing to 35.
