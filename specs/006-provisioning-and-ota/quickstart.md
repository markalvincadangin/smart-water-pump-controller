# Provisioning & OTA Quickstart

## Local Network OTA via PlatformIO

To wirelessly flash firmware to your ESP32 on the local network without a USB cable:

1. Ensure the ESP32 is powered on and connected to your local Wi-Fi network.
2. In your IDE (VSCode with PlatformIO), select the `env:esp32dev_ota` environment.
3. If necessary, explicitly set the upload port to the device's IP address by adding `upload_port = 192.168.X.X` to `platformio.ini` (under the `esp32dev_ota` section), or rely on mDNS (`smartflow.local`) if your network supports it.
4. Click **Upload**. PlatformIO will compile with the `ENABLE_OTA` flag and push the firmware wirelessly.

## Android App Provisioning Validation

To test the automated routing to the provisioning flow:

1. Log into the Android app with a fresh account (an account with zero claimed devices in Firebase).
2. Observe that the app automatically navigates to the Provisioning screen.
3. Alternatively, click "Add New Device" from the Device List screen.
4. Follow the BLE provisioning steps on screen to pass Wi-Fi credentials to the ESP32.
