# QUICK START: Fix Water Level Calibration

## TL;DR
Water level shows ~45% but should be ~55% because RTDB device config is missing calibration. Takes 2 minutes to fix.

## The Fix (Choose One)

### 1. Firebase Console (Fastest)
1. Go to Firebase Console → Project `smart-water-pump-system` → Realtime Database
2. Find `/pump_system/config/device`
3. Add two fields:
   - `tank_empty_cm` = `120`
   - `tank_full_cm` = `30`
4. Wait 30 seconds ✓

### 2. Command Line (Automated)
```bash
cd scripts
npm install  # one-time
node diagnose-and-fix-calibration.js --keyfile serviceAccountKey.json --db-url "https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app"
```

## Verify
- RTDB `/pump_system/status/water_level_percent` should now show ~55% (was ~45%)
- ESP32 serial log shows: `[INFO] FIREBASE: Device config updated.`

## Full Docs
- Technical diagnosis: `docs/operations/LEVEL_CALIBRATION_FIX.md`
- Deployment guide: `docs/operations/CALIBRATION_FIX_INTEGRATION.md`
- Executive summary: `docs/operations/WATER_LEVEL_CALIBRATION_FIX_COMPLETE.md`
