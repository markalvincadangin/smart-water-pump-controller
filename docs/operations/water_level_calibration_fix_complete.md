# WATER LEVEL CALIBRATION FIX — COMPLETE SOLUTION

**Date:** 2026-04-06  
**Issue:** Water level percent shows ~45% when actual is ~55% (off by 10%)  
**Root Cause:** RTDB device config missing `tank_empty_cm` and `tank_full_cm` fields  
**Status:** Solution ready for immediate deployment

---

## The Problem

The ESP32 master reads water level from the remote NodeMCU sensor node over RS-485, then converts raw distance (cm) to level percentage. The conversion uses calibration values from RTDB device config. When those fields are missing, ESP32 falls back to hardcoded defaults that don't match the actual field installation.

**Result:** Level calculations are systematically wrong by ~10%.

### Example
- Measured distance: 70.5 cm
- Expected level: 55% (tank mostly full)
- Actual reading: 45% (tank appears half-full)
- Error: -10 percentage points

---

## Why It Happened

### Field Installation (Correct)
- Sensor mounted 120 cm above tank bottom (empty reference)
- Reads 30 cm distance when tank is full
- Range: 120 − 30 = 90 cm

### ESP32 Hardcoded Defaults (Wrong)
```cpp
// firmware/master_node/src/config/config.h
#define TANK_EMPTY_CM   122  // ❌ obsolete, doesn't match actual setup
#define TANK_FULL_CM     8   // ❌ wrong
```

### RTDB Device Config (Missing)
```json
/pump_system/config/device was missing:
{
  "tank_empty_cm": null,    // ❌ missing → ESP32 uses hardcoded 122
  "tank_full_cm": null      // ❌ missing → ESP32 uses hardcoded 8
}
```

### Conversion Formula (Correct, but using wrong inputs)
```cpp
// firmware/master_node/src/rs485/rs485_comm.cpp:170-174
float rangeCm = cfgTankEmptyCm - cfgTankFullCm;  // 122 - 8 = 114 (WRONG)
float pct = 100.0f * (cfgTankEmptyCm - dist) / rangeCm;
// pct = 100 * (122 - 70.5) / 114 = 45% ❌
```

---

## The Fix

Update RTDB `/pump_system/config/device` to include:

```json
{
  "tank_empty_cm": 120,
  "tank_full_cm": 30
}
```

When ESP32 reads this (every 30 seconds), it will:
1. Update internal config: `cfgTankEmptyCm = 120`, `cfgTankFullCm = 30`
2. Log: `[INFO] FIREBASE: Device config updated.`
3. Use correct conversion: `pct = 100 * (120 - 70.5) / (120 - 30) = 55%` ✓

---

## How to Deploy

### Option 1: Manual (Firebase Console)
**Time:** ~1 minute  
**Skill:** None required

1. Go to https://console.firebase.google.com/project/smart-water-pump-system/database
2. Click on `/pump_system/config/device` in the tree
3. Add two children:
   - `tank_empty_cm` = `120`
   - `tank_full_cm` = `30`
4. Done. Wait 30 seconds for ESP32 to apply.

### Option 2: Automated (Admin Script)
**Time:** ~2 minutes (first-time setup)  
**Skill:** Node.js

**Scripts provided:**
- `scripts/fix-device-config-calibration.js` — updates RTDB
- `docs/operations/CALIBRATION_FIX_INTEGRATION.md` — complete guide

**Quick start:**
```powershell
cd scripts
npm install
node fix-device-config-calibration.js `
  --keyfile serviceAccountKey.json `
  --database-url "https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app"
```

---

## Verification Steps

### 1. Confirm RTDB has new values (immediate)
```
RTDB: /pump_system/config/device
  tank_empty_cm: 120   ✓
  tank_full_cm: 30     ✓
```

### 2. ESP32 reads and applies (30–60 seconds)
```
Serial monitor output:
  [INFO] FIREBASE: Device config updated.
  
RTDB: /pump_system/config/device (NVS cache, ESP32-posted)
  Shows same values as above
```

### 3. Level calculations are now correct
```
Sensor sends: DIST:70.5
ESP32 converts: pct = 100 * (120 - 70.5) / 90 = 55%
RTDB: /pump_system/status/water_level_percent = 55  ✓ (was 45)
```

### 4. Dashboard agrees
```
Tank graphic shows ~55% level
Matches flow-based estimate after pump cycle
Trends make physical sense (gradual drains/fills)
```

---

## What Changed

### Files Created
1. **`scripts/fix-device-config-calibration.js`** — Admin script for RTDB update
2. **`docs/operations/LEVEL_CALIBRATION_FIX.md`** — Technical diagnosis
3. **`docs/operations/CALIBRATION_FIX_INTEGRATION.md`** — Integration and deployment guide

### Files Not Changed
- All firmware (.cpp/.h) — no re-flashing needed
- Database rules — no schema changes
- Dashboard — no re-build needed

### Why No Firmware Changes
The ESP32 firmware **already supports reading calibration from RTDB**. It's been designed this way since inception (see `connectivity_cloud.cpp:148-171` and `rs485_comm.cpp:170-174`). We just need to populate the RTDB fields it reads.

---

## Impact Assessment

### Safety
- ✅ No firmware changes = No safety revalidation needed
- ✅ Calibration is pure data = Cannot introduce code bugs
- ✅ Once deployed, system works as originally designed

### Functionality
- ✅ Level displays will become accurate immediately
- ✅ Automation (pump start/stop) will trigger at correct thresholds
- ✅ Dashboard tank graphic will reflect true state
- ✅ Flow vs. level estimates will agree better

### Operations
- ✅ Minimal risk (data update only, not code)
- ✅ Instant rollback possible (delete fields from RTDB)
- ✅ Can be deployed any time (no downtime needed)

---

## Timeline

| When | What |
|------|------|
| **Now (2026-04-06 11:30 UTC+8)** | Solution ready; deployment instructions available |
| **Within 5 min** | Apply fix via Firebase Console or script |
| **+30 sec** | ESP32 reads new config (next 30-sec poll) |
| **+1 min** | ESP32 publishes corrected level% to RTDB |
| **+2 min** | Dashboard reflects new level from subscribed RTDB update |
| **Done ✓** | System now operating at correct calibration |

---

## Documentation & References

**Diagnostic reports:**
- Session memory: `/memories/session/smartflow_level_refresh_note.md`

**Technical deep-dives:**
- Level calibration fix: `docs/operations/LEVEL_CALIBRATION_FIX.md`
- Integration guide: `docs/operations/CALIBRATION_FIX_INTEGRATION.md`

**Firmware source (unchanged, but showing why fix works):**
- Config load: `firmware/master_node/src/connectivity/connectivity_cloud.cpp:148-171`
- Conversion: `firmware/master_node/src/rs485/rs485_comm.cpp:170-174`
- Sensor defaults: `firmware/sensor_node/src/config/config.h:39-47`

**RTDB schema:**
- `docs/operations/FIRMWARE_CONFIG_FROM_DATABASE.md`

**SmartFlow skill:**
- `.github/skills/smartflow/SKILL.md` (confirmed field calibration: 120/30 cm)

---

## Questions?

If deployment doesn't match expected results:

1. **Level % still wrong after 2 minutes?**
   - Check RTDB shows `tank_empty_cm: 120, tank_full_cm: 30`
   - Check ESP32 serial logs for "Device config updated"
   - Restart ESP32 (power cycle) to force immediate re-read

2. **What if I need to change calibration again?**
   - Edit RTDB `/pump_system/config/device` values
   - No firmware re-flashing needed
   - Changes take effect within 30 seconds

3. **Can this fix break anything?**
   - No. Calibration is data only, not code.
   - Worst case: revert by deleting fields from RTDB (instant rollback).

---

**Status:** Ready to deploy ✓  
**Risk:** Minimal ✓  
**Effort:** < 2 minutes ✓  
**Benefit:** System operates at designed accuracy ✓
