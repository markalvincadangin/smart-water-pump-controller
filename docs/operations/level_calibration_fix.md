# Water Level Sensor Calibration Fix

## Problem Diagnosed
- ESP32 master is converting tank sensor distance to level percent using wrong calibration (122/8 cm)
- Sensor node is sending correct raw distances (120/30 cm field calibration)
- Result: Level calculations are off by ~10% (e.g., showing 45% when actual is 55%)

## Root Cause
RTDB `/pump_system/config/device` is missing or has incorrect `tank_empty_cm` and `tank_full_cm` fields.

## Technical Details
- **Sensor node config** (sensor_node): `TANK_US_DIST_EMPTY_CM=120`, `TANK_US_DIST_FULL_CM=30`
- **ESP32 hardcoded defaults** (master_node): `TANK_EMPTY_CM=122`, `TANK_FULL_CM=8` (obsolete)
- **ESP32 conversion formula** (rs485_comm.cpp:170-174):
  ```cpp
  pct = 100.0 * (cfgTankEmptyCm - dist) / (cfgTankEmptyCm - cfgTankFullCm)
  ```
- **ESP32 reads RTDB every 30 seconds** to update `cfgTankEmptyCm` and `cfgTankFullCm` from device config

## How to Fix

### Option 1: Firebase Console (Manual)
1. Go to Firebase Console → project `smart-water-pump-system`
2. Select **Realtime Database**
3. Navigate to `/pump_system/config/device`
4. Edit to add or update:
   ```json
   {
     "tank_empty_cm": 120,
     "tank_full_cm": 30
   }
   ```
5. Wait 30+ seconds for ESP32 to read and apply

### Option 2: Admin Script (Automated)
1. Prerequisites:
   - Service account key JSON (download from Firebase Console → Project Settings → Service Accounts)
   - Node.js installed
   
2. Run from repo root:
   ```bash
   cd scripts
   npm install
   FIREBASE_DATABASE_URL="https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app" \
   GOOGLE_APPLICATION_CREDENTIALS="/path/to/serviceAccountKey.json" \
   node fix-device-config-calibration.js
   ```
   
   Or with explicit flags:
   ```bash
   node fix-device-config-calibration.js \
     --keyfile /path/to/serviceAccountKey.json \
     --database-url "https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app"
   ```

## Verification

After applying fix, verify within 30-60 seconds:

1. **Check RTDB status**:
   - Path: `/pump_system/status/water_level_percent`
   - Should show ~55% when measured distance is ~70.5 cm

2. **Check ESP32 serial logs**:
   - Watch for: `"Device config updated."` message
   - Indicates ESP32 has read new calibration

3. **Field test**:
   - Compare RTDB level estimate against flow-based estimate after pump cycle
   - Should now match within ±2%

## Technical Background

### Why This Mattered
The ESP32 **always** derives level percent from raw distance using its configured tank geometry:
- Empty reference (sensor mounted): 120 cm above tank bottom
- Full reference (surface at): 30 cm distance
- Range: 120 - 30 = 90 cm (tank volume capacity span)

When RTDB was missing calibration:
- ESP32 fell back to hardcoded 122/8 (from old tank setup)
- Range became: 122 - 8 = 114 cm (wrong)
- Sensor sent DIST=70.5 cm
- ESP32 calculated: 100 × (122-70.5) / (122-8) = 45% ❌
- Correct calculation: 100 × (120-70.5) / (120-30) = 55% ✓

### Data Flow
```
Sensor (NodeMCU)
  └─ reads ultrasonic → 70.5 cm distance
  └─ sends: DIST:70.5
  
Master (ESP32)
  └─ receives DIST from RS-485
  └─ reads cfgTankEmptyCm, cfgTankFullCm from RTDB every 30s
  └─ converts: pct = 100 * (cfgTankEmptyCm - 70.5) / (cfgTankEmptyCm - cfgTankFullCm)
  └─ publishes water_level_percent to RTDB
  
Dashboard
  └─ displays level% and compares against flow estimate
```

## Documentation References
- ESP32 calibration read: `firmware/master_node/src/connectivity/connectivity_cloud.cpp:148-171`
- Distance-to-percent conversion: `firmware/master_node/src/rs485/rs485_comm.cpp:170-174`
- Sensor node config: `firmware/sensor_node/src/config/config.h:39-47`
