# SmartFlow Cloud Data Model (v2)

This document defines the vendor-agnostic cloud schema for the SmartFlow ecosystem.

## Realtime Database / JSON Store Schema

### 1. `users/` Collection
Manages user identity and their claimed devices.

```json
{
  "users": {
    "<user_id>": {
      "devices": {
        "<device_id>": true
      }
    }
  }
}
```

### 2. `devices/` Collection
Manages the complete state of a given SmartFlow device.

```json
{
  "devices": {
    "<device_id>": {
      "metadata": {
        "firmwareVersion": "2.0.0",
        "hardwareVersion": "ESP32-WROOM-32",
        "protocolVersion": "1.0",
        "serialNumber": "SF-000001"
      },
      "status": {
        "lifecycle": "ONLINE",
        "uptimeSeconds": 3600
      },
      "telemetry": {
        "waterLevel": 85.0,
        "flowRate": 12.5
      },
      "settings": {
        "configVersion": 2,
        "tankHeight": 200,
        "lowThreshold": 20
      },
      "shadow": {
        // See contracts/device_shadow.md
      },
      "diagnostics": {
        "freeHeap": 150000,
        "wifiRSSI": -65,
        "restartReason": "POWERON_RESET"
      },
      "events": {
        // See contracts/events.md
      }
    }
  }
}
```

### 3. `firmware/` Collection
Manages OTA update manifests.

```json
{
  "firmware": {
    "stable": {
      "2.0.0": "https://storage.googleapis.com/.../fw.bin"
    }
  }
}
```
