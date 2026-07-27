# Data Model: SmartFlow System Integration

**Feature**: `004-system-integration`
**Date**: 2026-07-27

---

## Firebase RTDB Schema (V2 — Canonical)

This is the authoritative V2 data model that all three components (Firmware, Android, Functions) must converge on.

```
/
├── users/
│   └── {uid}/                          # Firebase Auth UID
│       ├── profile/
│       │   └── email: String
│       └── devices/
│           └── {deviceId}: true        # Ownership claim — Boolean true
│
├── devices/
│   └── {deviceId}/                     # e.g. "SF-000001"
│       ├── metadata/
│       │   ├── firmwareVersion: String  # "2.0.0"
│       │   ├── hardwareVersion: String  # "ESP32-WROOM-32"
│       │   ├── protocolVersion: String  # "1.0"
│       │   ├── serialNumber: String     # "SF-000001"
│       │   └── claimedByUid: String     # UID of claiming user
│       │
│       ├── status/
│       │   ├── lifecycle: String        # "UNCLAIMED" | "PROVISIONING" | "ONLINE"
│       │   └── uptimeSeconds: Number
│       │
│       ├── telemetry/
│       │   ├── waterLevel: Number       # Percentage 0-100
│       │   └── flowRate: Number         # LPM
│       │
│       ├── shadow/
│       │   ├── desired/
│       │   │   ├── pumpState: Boolean   # Written by Android App (owner only)
│       │   │   └── mode: String         # "AUTO" | "MANUAL" | "COUNTDOWN"
│       │   └── reported/
│       │       ├── pumpState: Boolean   # Written by Firmware
│       │       └── mode: String         # Written by Firmware
│       │
│       ├── settings/
│       │   ├── configVersion: Number
│       │   ├── tankHeight: Number       # cm
│       │   ├── lowThreshold: Number     # Percentage
│       │   └── highThreshold: Number    # Percentage
│       │
│       ├── diagnostics/
│       │   ├── freeHeap: Number         # bytes
│       │   ├── wifiRSSI: Number         # dBm (negative)
│       │   └── restartReason: String    # "POWERON_RESET" | "SW_RESET" | ...
│       │
│       └── events/
│           └── {eventId}/              # Push key or epoch timestamp
│               ├── timestamp: Number
│               ├── severity: String    # "INFO" | "WARN" | "ERROR"
│               ├── category: String    # "SAFETY" | "NETWORK" | "OTA"
│               ├── code: String        # "DRY_RUN" | "OVERFLOW" | ...
│               └── message: String
│
└── firmware/
    └── stable/
        └── {version}: String           # e.g. "2.0.0": "https://storage.../fw.bin"
```

---

## Android App — Kotlin Data Classes

```kotlin
// Device.kt (already created in 003)
data class Device(
    val deviceId: String = "",
    val firmwareVersion: String = "",
    val lifecycle: String = "UNCLAIMED",
    val claimedByUid: String = ""
)

data class DeviceShadow(
    val desired: ShadowState = ShadowState(),
    val reported: ShadowState = ShadowState()
)

data class ShadowState(
    val pumpState: Boolean = false,
    val mode: String = "AUTO"
)

data class DeviceTelemetry(
    val waterLevel: Double = 0.0,
    val flowRate: Double = 0.0
)

data class DeviceEvent(
    val timestamp: Long = 0L,
    val severity: String = "",
    val category: String = "",
    val code: String = "",
    val message: String = ""
)

// NEW for 004
data class DeviceDiagnostics(
    val freeHeap: Long = 0L,
    val wifiRSSI: Int = 0,
    val restartReason: String = ""
)
```

---

## RTDB Security Rules — V2 Owner-Based Access Control

```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "auth != null && auth.uid === $uid",
        ".write": "auth != null && auth.uid === $uid"
      }
    },
    "devices": {
      "$deviceId": {
        ".read": "auth != null && root.child('users').child(auth.uid).child('devices').child($deviceId).val() === true",
        "metadata": {
          ".write": "auth != null"
        },
        "status": {
          ".write": "auth != null"
        },
        "telemetry": {
          ".write": "auth != null"
        },
        "shadow": {
          "desired": {
            ".write": "auth != null && root.child('users').child(auth.uid).child('devices').child($deviceId).val() === true"
          },
          "reported": {
            ".write": "auth != null"
          }
        },
        "settings": {
          ".write": "auth != null && root.child('users').child(auth.uid).child('devices').child($deviceId).val() === true"
        },
        "diagnostics": {
          ".write": "auth != null"
        },
        "events": {
          "$eventId": {
            ".write": "auth != null"
          }
        }
      }
    },
    "firmware": {
      ".read": "auth != null",
      ".write": false
    }
  }
}
```

> **Note**: In V2, the firmware authenticates with **Firebase Anonymous Authentication** (see FR-003 clarification). Each ESP32 signs in anonymously on first boot, stores its anonymous UID in NVS, and writes it to `/devices/{deviceId}/metadata/firmwareUid`. RTDB rules for firmware-writable paths validate `auth.uid === data.child('metadata/firmwareUid').val()`. The `shadow/desired` write is owner-restricted to the claiming user. Full service-account separation is a designated future hardening step.

---

## BLE GATT — Factory Reset Extension

Extending the existing provisioning service with one additional characteristic:

| Characteristic | UUID | Properties | Description |
|---------------|------|-----------|-------------|
| Factory Reset | `beb5483e-36e1-4688-b7f5-ea07361b26ac` | Write | Write `"RESET"` to trigger NVS erase + reboot |
