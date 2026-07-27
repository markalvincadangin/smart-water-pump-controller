# SmartFlow — Developer Environment Setup

**Permanent reference for new contributors and CI configuration.**
**Last updated**: 2026-07-27 (Feature `004-system-integration`)

---

## Overview

SmartFlow is a multi-component IoT system. Before you can develop or test, you need credentials for three separate systems:

| Component | Credentials Needed | File Location |
|-----------|-------------------|---------------|
| Android App | `google-services.json` | `app/google-services.json` |
| ESP32 Firmware | `API_KEY`, `DATABASE_URL`, Wi-Fi | `firmware/master_node/src/config/secrets.h` |
| Cloud Functions | `RESEND_API_KEY` | `functions/.env` |

> [!IMPORTANT]
> None of these credential files are committed to the repository. Each developer must obtain and configure them independently.

---

## 1. Firebase Project Setup

### 1.1 Prerequisites

- Firebase account with access to project `smart-water-pump-system`
- Firebase CLI: `npm install -g firebase-tools` then `firebase login`
- Node.js 22+ for Cloud Functions

### 1.2 Download `google-services.json` (Android App)

1. Firebase Console → Project `smart-water-pump-system` → ⚙ Project Settings → **Your apps**
2. Select the Android app (`com.smartflow`)
3. Click **Download google-services.json**
4. Place at `app/google-services.json`

> [!NOTE]
> A placeholder structure showing the required fields is at `app/google-services.json.example`.

### 1.3 Get Firebase API Key and RTDB URL (Firmware)

1. Firebase Console → Project Settings → **General** tab
2. Under "Your apps", note the **Web API Key** — this is `API_KEY`
3. Firebase Console → **Realtime Database** → Data tab — copy the URL at the top (format: `https://<project>-default-rtdb.<region>.firebasedatabase.app`)

### 1.4 Enable Anonymous Authentication

Firmware uses Firebase Anonymous Authentication (no service account needed):

1. Firebase Console → **Authentication** → Sign-in method
2. Enable **Anonymous** provider

### 1.5 Deploy Security Rules

```bash
# From repo root
firebase deploy --only database
```

Expected: `✔  Deploy complete!`

---

## 2. Firmware Credential Setup

```bash
cd firmware/master_node/src/config
cp secrets.h.example secrets.h
```

Edit `secrets.h` and fill in:

```cpp
#define WIFI_SSID       "YourNetworkName"
#define WIFI_PASSWORD   "YourNetworkPassword"
#define API_KEY         "AIzaSy..."          // From Step 1.3
#define DATABASE_URL    "https://...firebasedatabase.app"  // From Step 1.3
```

> [!NOTE]
> No email or password needed. The firmware performs Firebase Anonymous Authentication automatically on first boot and saves the returned UID to NVS.

### 2.1 Build and Flash

```bash
cd firmware/master_node
pio run -e esp32dev --target upload
pio device monitor   # Ctrl+C to exit
```

**Expected first-boot serial output:**
```
[BOOT] SmartFlow v2.0.0
[BLE] Provisioning started. Advertising as SmartFlow-XXXXXX
```

After provisioning via the Android app:
```
[CLOUD] Firebase Anonymous Auth OK. UID: <uid>
[CLOUD] Initialized for device: SF-XXXXXX
```

---

## 3. Cloud Functions Setup

```bash
cd functions
cp .env.example .env
```

Edit `functions/.env`:

```env
RESEND_API_KEY=re_...        # Get from resend.com dashboard
RESEND_FROM_EMAIL=SmartFlow <alerts@yourdomain.com>
```

> [!NOTE]
> `RESEND_FROM_EMAIL` is optional — defaults to `onboarding@resend.dev` (for testing only, not production delivery).

### 3.1 Build and Deploy

```bash
cd functions
npm install
npm run build
firebase deploy --only functions
```

---

## 4. Android App Setup

### 4.1 Open in Android Studio

1. `File → Open` → select the `smartflow/` repo root
2. Wait for Gradle sync (2-3 minutes on first open)

### 4.2 Required Permissions (granted at runtime)

On first launch, grant when prompted:
- **Bluetooth Scan** (`BLUETOOTH_SCAN`)
- **Bluetooth Connect** (`BLUETOOTH_CONNECT`)
- **Fine Location** (`ACCESS_FINE_LOCATION`)

These are required for BLE device discovery. The provisioning screen requests them automatically.

---

## 5. Firebase Emulator (Hardware-Free Testing)

For testing security rules and Cloud Functions without physical hardware:

```bash
# Start emulator with seed data from the exported RTDB snapshot
firebase emulators:start \
  --import=./<project>-default-rtdb-export.json \
  --only database,auth
```

Access the Emulator UI at `http://localhost:4000`.

To point the Android app at the emulator, add to `FirebaseCloudStore.kt`:
```kotlin
// DEVELOPMENT ONLY — remove before release
FirebaseDatabase.getInstance().useEmulator("10.0.2.2", 9000)
```

### Security Rule Smoke Test

| Scenario | Expected Result |
|----------|----------------|
| Owner writes `shadow/desired/pumpState` | ✅ ALLOWED |
| Non-owner writes `shadow/desired/pumpState` | ❌ PERMISSION_DENIED |
| Unauthenticated client reads any device node | ❌ PERMISSION_DENIED |

---

## 6. RTDB Data Schema (V2)

The canonical schema is documented in [`specs/004-system-integration/data-model.md`](../../specs/004-system-integration/data-model.md).

Key paths:

```
/devices/{deviceId}/
├── metadata/
│   ├── firmwareUid        ← set by firmware after Anonymous Auth
│   ├── claimedByUid       ← set by Android app after provisioning
│   ├── firmwareVersion
│   └── serialNumber
├── status/lifecycle       ← "ONLINE" | "OFFLINE"
├── telemetry/             ← waterLevel, flowRate (written by firmware)
├── shadow/
│   ├── desired/           ← written by Android app (pumpState, mode)
│   └── reported/          ← written by firmware (actual state)
├── diagnostics/           ← freeHeap, wifiRSSI, restartReason
└── events/{eventId}/      ← severity, code, message, timestamp

/users/{uid}/devices/{deviceId}    ← ownership index (true or deleted)
```

---

## 7. Common Issues

### Firmware won't connect to Firebase

- Verify `API_KEY` and `DATABASE_URL` in `secrets.h` are correct
- Ensure `Anonymous` auth provider is enabled in Firebase Console
- Check that `database.rules.json` has been deployed

### Android app crashes on BLE scan

- Confirm all three BLE/Location permissions were granted at runtime
- On Android 12+, Location permission is required for BLE scanning even if GPS isn't used

### Cloud Functions not triggering

- Ensure `npm run build` succeeded without errors before deploying
- Check Firebase Console → Functions → Logs for runtime errors
- Verify the `RESEND_API_KEY` secret is set: `firebase functions:secrets:set RESEND_API_KEY`

### "PERMISSION_DENIED" on legitimate writes

- Confirm `database.rules.json` was deployed: `firebase deploy --only database`
- Check that `firmwareUid` is correctly set at `/devices/{id}/metadata/firmwareUid`
- Ensure the user's claim is at `/users/{uid}/devices/{deviceId} = true`

---

## 8. Quick Reference Commands

```bash
# Deploy everything
firebase deploy --only database,functions

# Firmware: build only (no flash)
cd firmware/master_node && pio run -e esp32dev

# Firmware: flash + monitor
cd firmware/master_node && pio run -e esp32dev --target upload && pio device monitor

# Cloud Functions: build only
cd functions && npm run build

# Cloud Functions: run tests
cd functions && npm test

# Start emulator
firebase emulators:start --import=./<project>-default-rtdb-export.json --only database,auth
```

---

*For the full end-to-end validation walkthrough, see [`specs/004-system-integration/quickstart.md`](../../specs/004-system-integration/quickstart.md).*
