# SmartFlow Deployment Safety Checklist

**⚠️ CRITICAL: READ BEFORE DEPLOYMENT**

SmartFlow controls a 1.5 HP water pump connected to 220V AC power. Improper deployment can result in:
- Equipment damage (pump, tank, electrical enclosure)
- Flooding or water waste (hundreds of liters)
- Electrical hazards (electrocution, short circuits)
- Property damage

**This checklist is mandatory before any production deployment.**

---

## Phase 1: Hardware Verification (Before Powering Controller)

### Electrical System
- [ ] **Circuit Breaker (MCB)** — Verified at 20A rating
  - Tested: Toggle OFF/ON confirms relay clicks
  - Location: Accessible and clearly labeled
- [ ] **Thermal Overload Relay (TOR)** — LR2-D13 installed and calibrated
  - Dial set to **8–9 A** (matches motor nameplate)
  - Test: Press manual trip button (should cut power)
  - Tested this month? **Date:** ________
- [ ] **220V Live/Neutral Continuity** — Multimeter test performed
  - No shorts between Live and Neutral (ohms reading ∞)
  - Grounding wire continuity verified (< 1Ω from enclosure to pump casing)
- [ ] **All 220V wires tight** — Physical tug test on terminal blocks
  - No loose connections in contactor terminals
- [ ] **CAT6 cable ends** — Verified both tank and enclosure
  - No exposed copper
  - Connectors seated fully
- [ ] **PG cable glands** — All tightened and weatherproof
  - No water ingress visible
  - Gasket in place

### Sensor Verification
- [ ] **Ultrasonic Sensor (JSN-SR04T-2.0)**
  - Physically mounted above water surface (no obstruction)
  - Cable continuity tested
  - Test reading: **______ cm** (should be ~120 cm empty, ~30 cm full)
- [ ] **Flow Sensor (YF-G1)**
  - Mounted inline in return line
  - Pulse output tested with multimeter (should see 0-5V switching)
  - Calibration factor stored: **7.5** Hz/LPM
- [ ] **RS-485 Cable**
  - Continuity check on A, B, GND lines
  - No shorts between any pair
  - Cable rated for outdoor use (CAT6 or better)

### Controller Hardware
- [ ] **ESP32 Power Supply** — Voltmeter reading: **_____ V** (should be ~5V)
- [ ] **Firebase credentials** — `secrets.h` verified (file exists, not committed)
- [ ] **Serial Monitor Connection** — USB to ESP32 works, baud rate 115200

---

## Phase 2: Firmware & Configuration (Before First Run)

### Firmware Upload
- [ ] **Firmware version:** ________ (matching docs release)
- [ ] **PlatformIO build successful** (no errors/warnings)
  - Command: `pio run -d firmware/platformio_smart_water_pump_controller`
  - Output: `✓ SUCCESS`
- [ ] **Sensor Node firmware** uploaded and tested separately
  - Command: `pio run -d firmware/platformio_sensor_node -e nodemcuv2`
  - Serial output shows: `Boot: Sensor node debug enabled`

### Configuration Validation
- [ ] **Tank Calibration values** confirmed on-site
  - Measured empty distance: **______ cm** → TANK_US_DIST_EMPTY_CM
  - Measured full distance: **______ cm** → TANK_US_DIST_FULL_CM
  - Offset trim applied: **______ cm** → TANK_US_DISTANCE_OFFSET_CM
- [ ] **Hysteresis thresholds** set appropriately
  - Pump start level: **______ %** (typical 20–30%)
  - Pump stop level: **______ %** (typical 85–95%)
- [ ] **Safety timeouts configured**
  - Dry-run threshold: **______ LPM** (typical 1.0)
  - Dry-run timeout: **______ sec** (typical 30)
  - Max runtime: **______ min** (typical 120)

### Firebase Setup
- [ ] **Firebase Realtime Database created**
  - Location: us-central1 or preferred region
  - Rules deployed: `firebase deploy --only database`
- [ ] **Email/Password account created** in Firebase Auth
  - Test login from dashboard works
  - No errors in browser console
- [ ] **Dashboard running locally**
  - `cd dashboard && npm run dev`
  - Port 3000 loads without errors
  - All telemetry displays "—" or zero (waiting for first frame)

---

## Phase 3: Dry-Run Testing (Non-Critical Environment)

### RS-485 Link Test
- [ ] **Ping test from master to sensor node**
  - Serial monitor shows: `[SN][INFO] Boot: Sensor node...`
  - `REQ` frames visible in logs
  - Response frames received every ~1 second
- [ ] **Level reading updates**
  - Ultrasonic distance reported
  - Level% calculation is correct: `(EMPTY - dist) / (EMPTY - FULL) * 100`
  - No jitter or wild swings (median filtering working)

### Pump Logic Simulation (Relay OFF, Firebase Only)
- [ ] **Mode switching works**
  - AUTO → MANUAL → COUNTDOWN → AUTO (via dashboard)
  - Each change logged and confirmed
- [ ] **Manual mode intent**
  - Set `manual_desired = true` in Firebase
  - Dashboard shows pump as if running (no actual relay click)
- [ ] **Countdown mode**
  - Start 5-minute countdown
  - Timer counts down visibly
  - Manual +2min extends timer
  - Stop countdown works
- [ ] **Emergency stop**
  - Trigger via dashboard
  - Status shows latched
  - Reset via "reset stop" button works

### Sensor Error Handling
- [ ] **Unplug ultrasonic sensor**
  - After 5 consecutive timeouts, `is_level_sensor_error = true`
  - Dashboard alerts operator
  - Pump stays OFF in AUTO mode
- [ ] **Simulate flow sensor issue**
  - Block flow pulses
  - After configured timeout, `is_flow_sensor_error = true`
  - Dry-run lockout activates (if already running)

---

## Phase 4: Live Testing (With Relay Active, Tank Empty)

### Initial Pump Activation
- [ ] **Tank is EMPTY** (verified visually)
  - Level reading in Dashboarrd shows 0–5%
  - Hysteresis: pump will NOT auto-start until level drops below start threshold
- [ ] **Relay click test** (no water flow yet)
  - Switch mode to MANUAL
  - Set `manual_desired = true` in Firebase
  - **Listen for relay click** (should hear distinct click)
  - Motor hums or spins briefly
  - Relay de-energizes quickly (should hear second click within 1 sec)
  - **Purpose:** Verify GPIO4 → relay connection works
- [ ] **Seconds pump run** (10 seconds max)
  - Repeat above, watch serial monitor for logs
  - Confirm: dry-run detection is working (flow should be ~0)
  - After 2–3 seconds at zero flow, dry-run timer starts
  - If timer reaches threshold, `isDryRunError = true`
  - Pump stops (relay de-energized)

### Full Tank Fill Cycle (Dry-Run Recovery)
- [ ] **Clear dry-run error** via Firebase
  - Set `clear_error = true`
  - Status updates to `is_error = false`
- [ ] **Energize pump again** (with hose/bucket now attached to outlet)
  - Set `manual_desired = true`
  - Flow sensor should now report **> dry_run_threshold_lpm**
  - Dry-run timer resets
  - Pump continues running
- [ ] **Fill until tank reaches stop level**
  - Watch level % climb in dashboard
  - When level ≥ stop_level %, pump stops automatically
  - Relay de-energizes (click heard)
  - Confirm: mode is still MANUAL (pump remains OFF until manual_desired = false)

### AUTO Mode Hysteresis Test
- [ ] **Set mode to AUTO**
- [ ] **Tank at stop level (≥ 90%)**
  - Pump should be OFF
  - run_mode should show "AUTO_STANDBY"
- [ ] **Manually drain water** (open outlet or use bypass)
  - Watch level % drop in dashboard
  - When level ≤ start_level (e.g., 30%), pump auto-starts
  - Confirm: run_mode shows "AUTO" and relay clicks
  - Flow sensor shows **> 0 LPM**
- [ ] **Let tank fill back up**
  - Pump stops when level ≥ stop_level (e.g., 90%)
  - Mode reverts to "AUTO_STANDBY"
  - Repeat cycle 3 times to confirm stability

---

## Phase 5: Safety Edge Cases (Controlled Scenarios)

### Dry-Run Lockout (Intentional)
- [ ] **Close outlet slightly to reduce flow**
  - Set pump to MANUAL with reduced outlet flow
  - Flow sensor reads **< dry_run_threshold_lpm** (e.g., 0.5)
  - Timer starts
  - After `dry_run_timeout_sec` (e.g., 30 sec), pump stops
  - Status shows `is_error = true` and `last_fault_code = "DRY_RUN"`
- [ ] **Clear error and recover**
  - Fully open outlet
  - Click "Clear Error"
  - Restart pump to verify flow normalizes

### Overflow Protection (Max Runtime)
- [ ] **Set max_pump_runtime_min to 2 minutes** (temporary test value)
- [ ] **Start pump in MANUAL mode**
  - Pump runs continuously
  - Watch countdown in dashboard
  - After 1 minute 54 seconds (90% threshold), `manual_runtime_warning = true`
  - After 2 minutes exactly, `isOverflowError = true`
  - Pump stops forcibly (relay de-energized)
  - Status shows `last_fault_code = "OVERFLOW"`
- [ ] **Restore max_pump_runtime_min to production value** (e.g., 120)
  - Clear error
  - Verify pump operates normally again

### Communication Loss Recovery
- [ ] **Disconnect RS-485 cable** (simulate sensor node offline)
  - Master shows `remote_sensor_stable = false`
  - After ~5 seconds, `remoteSensorOnline = false`
  - In AUTO mode: pump should stop (failsafe)
  - Status shows appropriate error
- [ ] **Reconnect RS-485 cable**
  - Frames resume flowing
  - System recovers (connection re-established)
  - AUTO mode can restart pump if below start_level

---

## Phase 6: Production Deployment (Real Tank)

### Installation
- [ ] **Tank mounted** at final location
  - Ultrasonic sensor positioned above water surface (clear path)
  - Flow sensor inline in return line
  - All cabling secured and labeled
- [ ] **Electrical enclosure** mounted
  - IP65 or better rating
  - Accessible for emergency stop
  - Grounding verified
- [ ] **WiFi signal strength** checked
  - Dashboard loads without timeout
  - RSSI in Firebase shows **≥ -70 dBm** (good signal)
  - If weaker, deploy range extender

### First Production Run
- [ ] **Operator trained** on:
  - How to switch modes (AUTO / MANUAL / COUNTDOWN)
  - Emergency stop procedure
  - How to read error codes from dashboard
  - What to do if pump won't start
- [ ] **Initialization sequence**
  - Power up ESP32 (relay will be OFF initially)
  - Wait 10 seconds for WiFi + Firebase connection
  - Dashboard shows live telemetry
  - Verify mode is AUTO and pump is OFF
- [ ] **Monitor for 24 hours**
  - Log any errors or anomalies
  - Verify level readings are stable
  - Check that auto-start/stop works correctly
  - Uptime and cycle counts are incrementing

### Documentation
- [ ] **Calibration parameters logged**
  - File: `PRODUCTION_CONFIG_BACKUP.txt`
  - Contents: tank dimensions, sensor offsets, thresholds used
  - Date: ________
  - Location: ________
- [ ] **Initial telemetry snapshot captured**
  - Screenshot of first 12-hour history
  - Saved as proof of correct operation
  - Stored with commissioning documentation

---

## Ongoing Maintenance

### Monthly (Every 30 Days)
- [ ] Thermal overload relay manual trip test (verify it trips)
- [ ] Visually inspect enclosure for water intrusion
- [ ] Check dashboard for any ERROR entries in telemetry history
- [ ] Verify WiFi RSSI remains strong (> -75 dBm)

### Quarterly (Every 90 Days)
- [ ] Review pump cycle count and total runtime
  - Expected: ~100 cycles/month, ~300 min runtime/month
  - Anomalies suggest sensor drift or pump wear
- [ ] Test emergency stop button
- [ ] Recalibrate tank levels if drifts observed
  - Measure actual empty/full distances
  - Compare to configured values
  - Update TANK_US_DIST_* if delta > 2 cm

### Annually (Every 12 Months)
- [ ] Firmware security update check
- [ ] Sensor replacement consideration (esp. ultrasonic membranes can degrade)
- [ ] Contactor inspection (look for burn marks)
- [ ] TOR calibration verification

---

## Emergency Procedures

### Pump Won't Stop (Stuck ON)
1. **Immediate:** Flip main breaker (MCB) OFF to de-energize circuit
2. **Check:** thermal overload relay (TOR) — verify it's functioning
3. **Diagnosis:** Check relay GPIO on ESP32; attempt remote off via dashboard
4. **Recovery:** Once cooled, cycle power and test

### Tank Overflowing (Despite Software)
1. **Immediate:** Manually close outlet valve or switch to bypass
2. **Check:** Had `isOverflowError` triggered? If not, sensor may have failed
3. **Recovery:** 
   - Drain tank to safe level
   - Clear error on dashboard
   - Verify ultrasonic reading before restarting

### No WiFi Connection (Dashboard Shows Offline)
1. **Check:** WiFi SSID visible on phone
2. **Pump behavior:** Will continue in last-known-good mode (safe failback)
3. **Recovery:**
   - Restart controller (power cycle)
   - Check WiFi credentials in `secrets.h`
   - Verify Firebase credentials if that's the issue

### Sensor Fault (Ultrasonic or Flow)
1. **Dashboard Alert:** Will show colored error badge
2. **Pump Behavior:** 
   - AUTO mode: Pump stops (failsafe)
   - MANUAL mode: Pump continues but with warning
3. **Recovery:**
   - Check physical sensor connection
   - Clean sensor lens (ultrasonic)
   - Verify flex cable isn't kinked
   - Replace sensor if continuity test fails

---

## Sign-Off

By deploying SmartFlow, you acknowledge:

✅ You have read and understood all items in this checklist
✅ You have tested all safety features working correctly
✅ You understand the risks of operating at 220V/1.5HP
✅ You take full responsibility for hardware damage or flooding
✅ You will maintain the system per the maintenance schedule

**Commissioning Date:** _______________

**Location:** _______________

**Operator Name:** _______________

**Signature:** _______________

---

**Questions or Issues?** Refer to [docs/operations/troubleshooting.md](docs/operations/troubleshooting.md) or check telemetry history in Firebase for error logs.
