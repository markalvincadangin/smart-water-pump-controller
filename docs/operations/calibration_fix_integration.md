# SmartFlow Water Level Calibration Fix — Integration Guide

**Status:** Ready to deploy
**Issue:** Water level percent calculations off by ~10% due to RTDB device config missing calibration fields
**Solution:** Update `/pump_system/config/device` with correct `tank_empty_cm` and `tank_full_cm`

---

## Executive Summary

The ESP32 master firmware reads tank calibration from Firebase RTDB every 30 seconds. When these fields are missing, it defaults to obsolete hardcoded values (122/8 cm) that don't match the actual field installation (120/30 cm). This causes level percent to miscalculate.

**Fix:** Add two fields to RTDB device config:
```json
{
  "tank_empty_cm": 120,
  "tank_full_cm": 30
}
```

**Impact:** Level percent will immediately become accurate. Dashboard tank graphic and system automation will track actual tank state correctly.

---

## Implementation Options

### Quick Fix (Firebase Console, < 1 minute)

1. Go to https://console.firebase.google.com
2. Select project **smart-water-pump-system**
3. Go to **Realtime Database** tab
4. Find `/pump_system/config/device` in the tree
5. Click the **+** button and add two entries:
   - Key: `tank_empty_cm`, Value: `120`
   - Key: `tank_full_cm`, Value: `30`
6. Wait 30 seconds — ESP32 will log "Device config updated"
7. Verify: Check `/pump_system/status/water_level_percent` — should now be accurate

### Automated Fix (Node.js script, < 2 minutes setup)

**Prerequisites:**
- Node.js 16+ installed
- Firebase service account key JSON file (download from Firebase Console → Project Settings → Service Accounts → Generate new private key)

**Steps:**

```bash
# 1. Navigate to scripts folder
cd c:\Users\markc\_Projects\micro-controller\smart-water-pump-controller\scripts

# 2. Install dependencies (one-time)
npm install

# 3. Run diagnostic and fix (choose one):

# Option A: Environment variables
$env:FIREBASE_DATABASE_URL = "https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app"
$env:GOOGLE_APPLICATION_CREDENTIALS = "C:\path\to\serviceAccountKey.json"
node diagnose-and-fix-calibration.js

# Option B: Command-line flags
node diagnose-and-fix-calibration.js `
  --keyfile C:\path\to\serviceAccountKey.json `
  --db-url "https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app"
```

**Scripts available:**
- `diagnose-and-fix-calibration.js` — **Recommended**: comprehensive diagnostic + auto-fix with colored output
- `fix-device-config-calibration.js` — Simple update-only script

**Expected output:**
```
Reading current device config...
Current config: {...}

Applying calibration fix:
  tank_empty_cm: 122 → 120 (sensor reference height)
  tank_full_cm: 8 → 30 (sensor reading at full tank)

✓ Device config updated successfully!
  ESP32 will read new calibration within 30 seconds.
  Level percent should now calculate correctly:
    - When DIST=70.5cm, level% ≈ 55% (was miscalculated as ~45%)
```

---

## Verification Checklist

**Step 1: RTDB confirms update**
- [ ] RTDB `/pump_system/config/device` shows `tank_empty_cm: 120` and `tank_full_cm: 30`

**Step 2: ESP32 reads and applies (within 30–60 seconds)**
- [ ] ESP32 serial monitor shows: `[INFO] FIREBASE: Device config updated.`
- [ ] RTDB `/pump_system/config/device` in NVS cache matches RTDB values

**Step 3: Level calculations correct**
- [ ] Current measured distance: ~70.5 cm
- [ ] RTDB water_level_percent: ~55% (not 45%)
- [ ] Formula check: 100 × (120−70.5) / (120−30) = 100 × 49.5 / 90 ≈ 55% ✓

**Step 4: Dashboard reflects accuracy**
- [ ] Tank graphic on dashboard shows level matching expected state
- [ ] Level trends in status history are plausible (gradual changes, not jumps)

---

## Technical Background

### Why This Fix Works

**ESP32 Conversion Formula** (rs485_comm.cpp:170-174):
```cpp
float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);  // 120 - 30 = 90 cm
float pct = 100.0f * ((float)cfgTankEmptyCm - dist) / rangeCm;
// pct = 100 * (120 - 70.5) / 90 = 100 * 49.5 / 90 ≈ 55%
```

**Before fix** (ESP32 using hardcoded defaults):
```
rangeCm = 122 - 8 = 114 cm  (WRONG)
pct = 100 * (122 - 70.5) / 114 = 100 * 51.5 / 114 ≈ 45%  (INCORRECT)
```

**After fix** (ESP32 reads and uses RTDB calibration):
```
rangeCm = 120 - 30 = 90 cm  (CORRECT)
pct = 100 * (120 - 70.5) / 90 ≈ 55%  (CORRECT)
```

### Data Flow

```
NodeMCU (Sensor Node)
  ├─ Reads JSN-SR04T ultrasonic
  ├─ Calibration: empty=120cm, full=30cm (local)
  └─ Sends: DIST:70.5;FLOW:2.3;...

RS-485 Network (40m CAT6)
  └─ Noise-tolerant half-duplex link

ESP32 (Master)
  ├─ Receives DIST:70.5 from RS-485
  ├─ Reads cfgTankEmptyCm, cfgTankFullCm from RTDB every 30s
  │   └─ If RTDB missing: uses hardcoded 122/8 (WRONG)
  │   └─ If RTDB has values: uses 120/30 (CORRECT)
  ├─ Converts: pct = 100 * (cfgTankEmptyCm - dist) / (cfgTankEmptyCm - cfgTankFullCm)
  ├─ Publishes water_level_percent to RTDB (~3s cadence)
  └─ Uses for pump automation

Firebase RTDB
  ├─ /pump_system/config/device (ESP32 reads every 30s)
  │   ├─ tank_empty_cm: 120
  │   ├─ tank_full_cm: 30  ← FIX ENSURES THESE EXIST
  │   └─ ...other config
  └─ /pump_system/status/water_level_percent (ESP32 writes ~every 3s)
      └─ Should now be ≈ 55% (was 45%)

Dashboard (Next.js PWA)
  ├─ Reads water_level_percent from RTDB
  ├─ Displays tank graphic scaled 0–100%
  └─ Compares against flow-based estimate for sanity check
```

---

## Rollback Plan

If for any reason the fix causes issues, revert in RTDB:
1. Delete `tank_empty_cm` and `tank_full_cm` from `/pump_system/config/device`
2. ESP32 will revert to hardcoded defaults within 30 seconds
3. Contact field technician to verify actual tank geometry and recalibrate

---

## References

- **Firmware config read:** [`firmware/master_node/src/connectivity/connectivity_cloud.cpp:148-171`](../../../firmware/master_node/src/connectivity/connectivity_cloud.cpp)
- **Distance-to-percent formula:** [`firmware/master_node/src/rs485/rs485_comm.cpp:170-174`](../../../firmware/master_node/src/rs485/rs485_comm.cpp)
- **Sensor config:** [`firmware/sensor_node/src/config/config.h:39-47`](../../../firmware/sensor_node/src/config/config.h)
- **RTDB Device Config Schema:** [`docs/operations/FIRMWARE_CONFIG_FROM_DATABASE.md`](FIRMWARE_CONFIG_FROM_DATABASE.md)
- **Firmware operational rules:** [`docs/specs/firmware_operational_rules.md`](../specs/firmware_operational_rules.md)
