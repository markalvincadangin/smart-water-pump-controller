# Research: SmartFlow System Integration & Environment Standardization

**Feature**: `004-system-integration`
**Date**: 2026-07-27

---

## 1. Secrets & Credential Management

### Decision: Component-specific secret file patterns (no single unified `.env`)

**Rationale**: The three components (Firmware C++, Android Kotlin, Firebase Functions Node.js) each use different build toolchains that cannot share a single env file format. Each uses the idiomatic pattern for its ecosystem.

| Component | Pattern | Gitignored | Template |
|-----------|---------|-----------|---------|
| ESP32 Firmware | `secrets.h` / `#define` macros | ✅ `firmware/**/secrets.h` | ✅ `secrets.h.example` exists |
| Android App | `google-services.json` (Firebase SDK) + `local.properties` (build) | ⚠️ `local.properties` yes; `google-services.json` NOT yet in `.gitignore` | ❌ Missing |
| Firebase Functions | `functions/.env` | ✅ `functions/.env` | ❌ Missing |

**Alternatives considered**:
- Single `.env` at repo root injected into all components via build scripts → Too complex for embedded; PlatformIO needs C macros, not shell env vars.
- NVS-only for firmware (no `secrets.h`) → Valid long-term but requires pre-flashed credentials via BLE provisioning; `secrets.h` needed for dev/CI fallback.

**Actions required**:
1. Add `app/google-services.json` to `.gitignore`
2. Create `app/google-services.json.example` with placeholder structure
3. Create `functions/.env.example` with variable names

---

## 2. Firebase RTDB Security Rules (V1 → V2 Migration)

### Decision: Full rewrite from V1 `pump_system/` schema to V2 multi-tenant `/devices/<id>/` + `/users/<uid>/` schema

**Rationale**: The current `database.rules.json` is entirely V1 (`pump_system/` root), relying on a shared `admins` registry. The V2 schema introduces per-user device ownership via `/users/<uid>/devices/<deviceId>: true`.

**V2 Rule Strategy**:
```
/users/$uid/              → readable/writable by $uid only
/users/$uid/devices/$id   → $uid can read, firmware service account can write on claim

/devices/$deviceId/       → readable only by device owner (check /users/$uid/devices/$deviceId)
  metadata/               → read: owner; write: firmware (service account)
  status/                 → read: owner; write: firmware
  telemetry/              → read: owner; write: firmware
  shadow/desired/         → read: firmware; write: owner only (security critical)
  shadow/reported/        → read: owner; write: firmware
  diagnostics/            → read: owner; write: firmware
  events/                 → read: owner; write: firmware

/firmware/                → read: any authenticated; write: admin only
```

**Key security rule for shadow control (FR-001)**:
```json
"shadow": {
  "desired": {
    ".write": "auth != null && root.child('users').child(auth.uid).child('devices').child($deviceId).val() === true"
  },
  "reported": {
    ".write": "auth.uid === 'firmware-service-account-uid'"
  }
}
```

**Firmware auth strategy**: The ESP32 authenticates to Firebase using Email/Password auth (existing `secrets.h` pattern). The firmware account UID must be granted write access to `reported`, `status`, `telemetry`, `diagnostics`, and `events`. This is enforced by checking `auth.uid === FIRMWARE_SERVICE_UID` in rules, or alternatively using a `device_id` custom claim.

**Alternatives considered**:
- Firebase Admin SDK (service account key on-device) → Not feasible; ESP32 cannot securely store an RSA private key.
- Firebase Custom Tokens → More secure but requires a Cloud Function to mint tokens; adds latency and complexity for MVP.
- Email/Password Auth (current) → Simpler, already implemented in firmware. Sufficient for MVP with API key restrictions.

---

## 3. Android BLE Runtime Permissions

### Decision: Request permissions inside `ProvisioningViewModel` before scan, with `rememberLauncherForActivityResult`

**Rationale**: Android 12+ (API 31+) splits BLE permissions. Apps must request `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` at runtime (not just manifest). `ACCESS_FINE_LOCATION` is also required on API < 31.

**Permission request flow**:
```
User taps "Scan for Devices"
  → ProvisioningScreen checks if permissions granted
  → If not: launches ActivityResultContracts.RequestMultiplePermissions
  → On grant: calls viewModel.startScan()
  → On deny: shows rationale UI
```

**Permissions required**:
```xml
<!-- AndroidManifest.xml — already declared -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
```

---

## 4. Cloud Functions V2 Schema Compatibility

### Decision: Update Cloud Functions to listen on `/devices/<id>/status` instead of V1 `/pump_system/status`

**Current state** (V1): `functions/src/index.ts` listens on `pump_system/status` and `pump_system/config/notifications_by_user`.

**Required change** (V2): Update trigger paths to `/devices/{deviceId}/status` and read notification preferences from `/users/{uid}/notification_prefs`.

**Rationale**: The Cloud Functions send email alerts for dry-run, low tank, and pump-start events. These triggers must be updated to the V2 paths or they will fire on non-existent paths and produce no notifications.

**Impact assessment**: Low risk — only the trigger path and data-read paths change. Email logic itself is unchanged.

---

## 5. Factory Reset Flow

### Decision: Two-phase reset — firmware clears NVS, then app calls Firebase to unclaim device

**Phase 1 — Firmware (NVS clear)**:
```cpp
void BleProvisioning::factoryReset() {
    nvs_flash_erase();       // clear all NVS namespaces
    esp_restart();           // reboot to UNCLAIMED state
}
```
Exposed via a new GATT characteristic `CHAR_RESET_UUID` (write "RESET" string triggers reset).

**Phase 2 — Android App (Firebase unclaim)**:
```kotlin
// DeviceRepository.kt
suspend fun unclaimDevice(uid: String, deviceId: String) {
    db.getReference("users/$uid/devices/$deviceId").removeValue()
}
```

**Trigger**: A "Factory Reset" button in `DashboardScreen` → confirmation dialog → calls `DashboardViewModel.factoryReset()` → writes to BLE reset char + calls Firebase unclaim.

---

## 6. Integration Smoke Test (No Hardware Required)

### Decision: RTDB export JSON seed + Android Emulator (API 34) for UI verification

**Strategy**:
1. Import the repo's RTDB export JSON into the Firebase Emulator Suite
2. Run the Android App pointing to the Firebase Emulator
3. Manually verify the Dashboard shows correct telemetry values
4. Simulate shadow round-trip by writing `shadow/desired/pumpState: true` directly in Emulator UI and checking app reflects it

**Firebase Emulator command**:
```bash
firebase emulators:start --import=./<rtdb-export>.json
```

**Android Emulator config**: Point `google-services.json` to `10.0.2.2:9000` (emulator localhost).
