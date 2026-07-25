# SmartFlow Water Level Calibration Fix — Verification Checklist

**Status**: Solution ready for deployment and verification  
**Date**: 2026-04-06  

---

## Pre-Deployment Verification (Local)

These checks can be done WITHOUT Firebase credentials:

- [ ] **Code files created**: Check all 7 scripts/docs exist
  - `scripts/diagnose-and-fix-calibration.js`
  - `scripts/fix-device-config-calibration.js`
  - `scripts/test-calibration-fix.js`
  - `docs/operations/LEVEL_CALIBRATION_FIX.md`
  - `docs/operations/CALIBRATION_FIX_INTEGRATION.md`
  - `docs/operations/WATER_LEVEL_CALIBRATION_FIX_COMPLETE.md`
  - `docs/operations/QUICKSTART_CALIBRATION_FIX.md`

- [ ] **Scripts are executable**: 
  ```bash
  node scripts/diagnose-and-fix-calibration.js --help 2>&1 | grep -q "Usage" && echo "✓"
  ```

- [ ] **NPM dependencies listed**: Check `scripts/package.json` has `firebase-admin`
  ```bash
  grep -q "firebase-admin" scripts/package.json && echo "✓"
  ```

---

## Deployment Phase 1: Apply Fix to RTDB

Choose one deployment method:

### Method A: Firebase Console (Manual)
- [ ] Access Firebase Console at https://console.firebase.google.com
- [ ] Select project `smart-water-pump-system`
- [ ] Navigate to **Realtime Database**
- [ ] Find `/pump_system/config/device` in tree
- [ ] Add child: `tank_empty_cm` = `120`
- [ ] Add child: `tank_full_cm` = `30`
- [ ] Save

### Method B: Diagnostic Script (Automated)
- [ ] Obtain Firebase service account key (Console → Settings → Service Accounts → Generate)
- [ ] From repo root:
  ```bash
  cd scripts
  npm install  # one-time
  FIREBASE_DATABASE_URL="https://smart-water-pump-system-default-rtdb.asia-southeast1.firebasedatabase.app" \
  GOOGLE_APPLICATION_CREDENTIALS="/path/to/serviceAccountKey.json" \
  node diagnose-and-fix-calibration.js
  ```
- [ ] Script output shows:
  - `✓ RTDB updated successfully!`
  - `tank_empty_cm: 120`
  - `tank_full_cm: 30`

### Method C: Test & Verify Script
- [ ] Run comprehensive test:
  ```bash
  cd scripts
  npm install
  FIREBASE_DATABASE_URL="..." GOOGLE_APPLICATION_CREDENTIALS="..." \
  node test-calibration-fix.js
  ```
- [ ] Output shows: `✓ ALL TESTS PASSED`
  - RTDB has correct values
  - Conversion formula verified (45% → 55%)
  - Multiple tank states validated

---

## Deployment Phase 2: ESP32 Reads New Config

**Timeline**: 30–60 seconds after fix applied

- [ ] **ESP32 reads RTDB**: (~30 second interval, automatic)
  - Check ESP32 serial monitor
  - Watch for: `[INFO] FIREBASE: Device config updated.`
  - This confirms ESP32 has received new values

- [ ] **RTDB cache updated**: ESP32 posts config to NVS persisted read
  - Path: `/pump_system/config/device` (master copy)
  - Should now match: `tank_empty_cm: 120`, `tank_full_cm: 30`

---

## Deployment Phase 3: Verify Level Calculations

**Timeline**: Immediate after Phase 2

### Visual Verification
- [ ] **Dashboard tank graphic**: 
  - Tank level shows ~55% (was ~45%)
  - Visual matches expected state

- [ ] **RTDB status path**:
  - `/pump_system/status/water_level_percent`
  - Should be ~55% (if measured distance is ~70.5 cm)

### Detailed Verification
- [ ] **Cross-check with flow estimate**:
  - After next pump cycle, compare:
    - Level change from ultrasonic: `level_new - level_old`
    - Flow-based estimate: `volume_pumped / tank_height`
  - Should now match within ±2% (was off by ~10%)

- [ ] **Serial monitor diagnostics**:
  - Watch for RS-485 frame logs:
    - `DIST:70.5` (raw distance)
    - `LVL:55` (calculated percent using new calibration)
  - Values should be consistent with field observations

### Log Pattern Expected
```
[RS485] RX: LVL:55;DIST:70.5;FLOW:2.3;ERR:0;SEQ:42;CRC:XXXX
[INFO] Level update: 55% (fresh)
```
(Not `LVL:45;DIST:70.5;...` — that would indicate old calibration)

---

## Deployment Phase 4: System Integration Tests

- [ ] **Pump automation responds correctly**:
  - Set `pump_start_level` = 30% in device config
  - Fill tank past 30%
  - Pump should **start** (now triggered correctly)
  - Was starting at wrong calibration point before

- [ ] **Overflow protection works**:
  - Set `pump_stop_level` = 85% in device config
  - Monitor max runtime limit
  - Should protect at correct level (was protecting at wrong level)

- [ ] **Dashboard statistics**:
  - "Level now" shows correct percent
  - History graph shows plausible trends (gradual changes, not jumps)
  - No erratic spikes or gaps

---

## Verification Success Criteria

**All tests pass if:**

1. ✓ RTDB `/pump_system/config/device`:
   - `tank_empty_cm` = `120`
   - `tank_full_cm` = `30`

2. ✓ ESP32 serial log shows: `Device config updated.`

3. ✓ Water level calculations accuracy:
   - When DIST ≈ 70.5 cm → Level ≈ 55% (±2%)
   - (Before fix: showed 45%, off by -10pp)

4. ✓ Dashboard & RTDB agree on level

5. ✓ Flow-based estimate ≈ ultrasonic-based level change

---

## Rollback (If Needed)

If anything goes wrong, instant rollback:

```bash
# Delete calibration fields from RTDB
# Path: /pump_system/config/device
# Remove: tank_empty_cm
# Remove: tank_full_cm

# ESP32 will revert to hardcoded 122/8 within 30 seconds
# Level will show 45% again (old behavior resumed)
```

Or via script:
```bash
firebase database:remove pump_system/config/device/tank_empty_cm
firebase database:remove pump_system/config/device/tank_full_cm
```

---

## Documentation References

- **Quick start**: `docs/operations/QUICKSTART_CALIBRATION_FIX.md`
- **Full integration guide**: `docs/operations/CALIBRATION_FIX_INTEGRATION.md`
- **Technical diagnosis**: `docs/operations/LEVEL_CALIBRATION_FIX.md`
- **Executive summary**: `docs/operations/WATER_LEVEL_CALIBRATION_FIX_COMPLETE.md`

---

## Sign-Off

| Phase | Status | Date | Notes |
|-------|--------|------|-------|
| Pre-deployment checks | ⬜ | | |
| Deploy to RTDB | ⬜ | | Method: Console / Script / Test |
| ESP32 reads config | ⬜ | | Check serial log for confirmation |
| Level calculations verified | ⬜ | | Visual + RTDB check |
| System integration test | ⬜ | | Pump automation, overflow protection |
| **COMPLETE** | ⬜ | | All tests passed? |
