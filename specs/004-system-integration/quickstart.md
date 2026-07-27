# Quickstart: SmartFlow End-to-End Integration Validation

**Feature**: `004-system-integration`
**Date**: 2026-07-27

This guide documents how to set up the SmartFlow system from scratch and validate end-to-end integration. It is the canonical source for SC-001 and SC-005.

---

## Prerequisites

| Requirement | Details |
|------------|---------|
| Firebase Project | `smart-water-pump-system` provisioned and accessible |
| Firebase CLI | `npm install -g firebase-tools`, logged in |
| Android Studio | Hedgehog or newer, with Android SDK API 31+ |
| PlatformIO | Installed (`pip install platformio`) |
| ESP32 DevKit | Flashed with V2 firmware from `003-commercial-iot-architecture` branch |
| Physical Android | API 31+ with BLE support |

---

## Step 1: Firebase Setup (Backend)

### 1a. Get credentials
1. Go to Firebase Console → Project `smart-water-pump-system` → Project Settings → Service Accounts
2. Download `google-services.json` → copy to `app/google-services.json`
3. Note your **Firebase Web API Key** (Project Settings → General)
4. Note your **RTDB URL** (Realtime Database → Data tab, at the top)

### 1b. Deploy security rules
```bash
# From repo root
firebase deploy --only database
```
Expected output: `✔ Deploy complete!`

**Validate rules** (SC-002): Try writing to `/devices/SF-000001/shadow/desired/pumpState` as a different user — expect `PERMISSION_DENIED`.

### 1c. Configure Cloud Functions
```bash
cd functions
cp .env.example .env
# Edit .env: add RESEND_API_KEY and RESEND_FROM_EMAIL
npm install && npm run build
firebase deploy --only functions
```

---

## Step 2: Firmware Setup

### 2a. Configure secrets
```bash
cd firmware/master_node/src/config
cp secrets.h.example secrets.h
# Edit secrets.h:
#   WIFI_SSID / WIFI_PASSWORD → your dev Wi-Fi
#   API_KEY → Firebase Web API Key (from Step 1a)
#   DATABASE_URL → your RTDB URL
#   (No email/password needed — firmware uses Anonymous Auth automatically)
```

### 2b. Flash firmware
```bash
cd firmware/master_node
pio run -e esp32dev --target upload
pio device monitor   # Ctrl+C to exit
```

**Expected serial output**:
```
[BOOT] SmartFlow v2.0.0
[BLE] Provisioning started. Advertising as SmartFlow-XXXXXX
```

---

## Step 3: Android App Setup

### 3a. Place credentials
- `app/google-services.json` → already copied in Step 1a

### 3b. Open in Android Studio
1. `File → Open` → select repo root (`smartflow/`)
2. Wait for Gradle sync to complete (may take 2-3 min on first open)
3. Connect your physical Android device via USB
4. `Run → Run 'app'`

### 3c. Grant permissions on first launch
When prompted, grant: **Location**, **Bluetooth Scan**, **Bluetooth Connect**

---

## Step 4: End-to-End Provisioning Flow (SC-001 Validation)

1. **Open app** → tap **"Sign In"** → use your Firebase Auth account
2. **Tap "Add Device"** → app scans for `SmartFlow-XXXXXX` BLE beacon
   - ✅ Device appears within 10 seconds
3. **Enter Wi-Fi SSID and Password** → tap **"Provision"**
   - ✅ Progress screen shows "Connecting..." then "Success"
   - ✅ Serial monitor shows: `[WIFI] Connected! IP: 192.168.x.x`
   - ✅ Firebase RTDB shows: `/devices/SF-XXXXXX/status/lifecycle = "ONLINE"`
4. **Open Dashboard** → verify telemetry appears
   - ✅ `waterLevel` and `flowRate` show live values within 5 seconds (SC-004)

**Total time target**: < 15 minutes from blank environment (SC-001)

---

## Step 5: Device Shadow Round-Trip (SC-003 Validation)

1. **On Dashboard**: toggle pump ON
2. **In Firebase Console**: verify `/devices/SF-XXXXXX/shadow/desired/pumpState = true`
3. **On device**: listen for relay click (physical) or serial: `[PUMP] Relay ENERGIZED`
4. **In Firebase Console**: verify `/devices/SF-XXXXXX/shadow/reported/pumpState = true`
5. **App**: reported state reflects ON

**Time target**: < 5 seconds for full round-trip (SC-003)

---

## Step 6: Factory Reset (SC-006 Validation)

1. On Dashboard → tap **"Factory Reset"** → confirm dialog
2. **App side**: Firebase removes `/users/{uid}/devices/SF-XXXXXX`
3. **Firmware side**: NVS erased, device reboots
4. **Expected**: Device reappears as `SmartFlow-XXXXXX` BLE beacon within 10 seconds (SC-006)

---

## Step 7: Smoke Test Without Hardware (SC-002 / FR-010)

```bash
# Start Firebase Emulator with RTDB seed data
firebase emulators:start --import=./<project>-default-rtdb-export.json --only database,auth

# Point Android App to emulator:
# In FirebaseCloudStore.kt, temporarily set:
#   FirebaseDatabase.getInstance().useEmulator("10.0.2.2", 9000)

# Validate rule behavior using Firebase Emulator UI at http://localhost:4000
```

Verify:
- `/devices/SF-000001/shadow/desired/` → write as non-owner → expect DENIED ✅
- `/devices/SF-000001/shadow/desired/` → write as owner → expect ALLOWED ✅
- Unauthenticated read → expect DENIED ✅
