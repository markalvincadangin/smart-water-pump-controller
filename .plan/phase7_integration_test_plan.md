# SmartFlow Integration Testing Plan — Phase 7

**Status:** Production Readiness Validation  
**Date Created:** March 31, 2026  
**Test Scope:** 21 integration tests across firmware (ESP32+NodeMCU), RS-485, Firebase, and Next.js dashboard  
**Target:** 100% pass rate before production deployment  

---

## 1. Overview & Objectives

### Phase 7 Goals
1. ✅ Validate all Phase 1–6 implementations in integrated system
2. ✅ Verify end-to-end data flow: sensors → master → cloud → dashboard
3. ✅ Confirm safety behaviors (dry-run, overflow, E-stop) work correctly
4. ✅ Prove Firebase schema matches dashboard type system
5. ✅ Document production-ready status with audit trail

### Test Categories
| Category | Tests | Purpose |
|----------|-------|---------|
| **RS-485 Comms** | IT-01 to IT-05 | Master/slave HW coordination |
| **Sensor Data** | IT-06 to IT-10 | Real sensor readouts + filtering |
| **Firebase Sync** | IT-11 to IT-15 | Cloud persistence + consistency |
| **Dashboard UI** | IT-16 to IT-20 | Type binding + display validation |
| **System E2E** | IT-21 | Full workflow under real conditions |

**Total Expected Runtime:** 45–60 minutes (mix of automated + manual observation)

---

## 2. Prerequisites

### Hardware Setup
- **ESP32 Master Controller** (arduino_smart_water_pump_controller/)
  - GPIO4: Relay control pin
  - GPIO17/25/5: RS-485 TX/RX/DE pins
  - USB: Serial monitor connection (115200 baud)
- **NodeMCU V2 Sensor Slave** (arduino_sensor_node/)
  - GPIO4/GPIO5: Ultrasonic trigger/echo (JSN-SR04T)
  - GPIO12: Flow sensor ISR input (YF-G1, optional)
  - GPIO1/GPIO3: RS-485 TX/RX (DEBUG_USB_MODE or hardware serial)
  - USB/Serial: Debug console connection
- **MAX485 Transceiver** × 2 (one per node, half-duplex RS-485 network)
- **Tank Setup:** Actual or simulated (water present for level readings)
- **Network:** WiFi SSID + password, Firebase project active

### Software Prerequisites
- Arduino IDE 1.8.x with ESP32 board support
- Both firmware images compiled & ready to flash
- `secrets.h` file with:
  ```cpp
  #define WIFI_SSID "your-network"
  #define WIFI_PASSWORD "your-password"
  #define FIREBASE_PROJECT_ID "your-project"
  #define FIREBASE_API_KEY "..."
  #define FIREBASE_RTDB_URL "https://your-project.firebaseio.com"
  ```
- Dashboard running locally or deployed to Vercel
- Firebase Realtime Database active with pump_system node created
- Permissions: Authenticated user must have read/write access to `/pump_system/`

### Test User Setup
- **Admin User:** Has read/write to control, config, and admin metadata
- **Observer User:** Read-only access to status
- **Permissions verified in Firebase Rules:** `/pump_system/control/` writable by admins only

---

## 3. Integration Tests (IT-01 to IT-21)

### Category A: RS-485 Communication (IT-01 to IT-05)

#### IT-01: Master Initiates Request to Slave

**Purpose:** Verify ESP32 → NodeMCU RS-485 request/response cycle works.

**Pre-conditions:**
- Both boards powered and programmed with production firmware
- RS-485 wiring: master A→slave A, master B→slave B, common GND
- Serial monitor open on both

**Steps:**
1. Power up ESP32 master → observe `[INFO] RS-485: Starting up` in serial
2. Power up NodeMCU slave → observe `[INFO] RS-485 Slave ready: listening on UART1`
3. Master should begin polling every 1–3 seconds
4. Observe at least 5 valid request/response cycles on slave console

**Pass Criteria:**
- ✅ Slave receives `REQ` frame (STX + "REQ" + ETX)
- ✅ Slave responds with frame containing LVL/FLOW/ERR fields
- ✅ Master logs "Frame received: LVL=X FLOW=Y ERR=Z"
- ✅ No CRC errors reported
- ✅ Frame latency < 250ms per spec

**Failure Troubleshooting:**
| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| No response (timeout) | Slave not listening / baud mismatch | Restart slave; verify 115200 baud both sides |
| CRC error on every frame | Baud rate drift / noise | Shorten RS-485 cable; add 120Ω termination resistor |
| Frame appears truncated | Buffer overflow / ISR overrun | Reduce poll frequency; check master loop timing |

---

#### IT-02: Slave Discards Invalid Frames with Grace

**Purpose:** Verify RS-485 slave ignores malformed requests without crashing.

**Pre-conditions:**
- IT-01 passing (master/slave can communicate)
- Master serial console open

**Steps:**
1. On master console, simulate malformed frame injection (if firmware supports debug injection)
2. Or: Introduce CRC error by temporarily corrupting TX data in simulation
3. Slave should remain responsive (no hang)
4. Master should retry request within 3 attempts

**Pass Criteria:**
- ✅ Slave logs `[WARN] Invalid frame (CRC mismatch/timeout)` 
- ✅ Slave does not enter error state or reset
- ✅ Next valid request processed normally
- ✅ No memory leaks (heap remains stable)

**Failure Troubleshooting:**
| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| Slave hangs after bad frame | Input buffer overflow / missing timeout | Restart; add stall timeout reset (M-03) |
| Master gives up after 1 error | Retry logic broken | Check RS485 comm code for retry loop |

---

#### IT-03: Slave LDSC Field Increments on Sensor Discard

**Purpose:** Verify Level Discard Counter (LDSC) field in response frame.

**Pre-conditions:**
- IT-01 passing
- Ultrasonic sensor connected to NodeMCU (or simulated with test pin)

**Steps:**
1. Run production firmware on NodeMCU
2. Monitor response frames for LDSC value
3. Tap/vibrate ultrasonic sensor 10+ times to trigger reading errors
4. Observe LDSC counter incrementing in later frames
5. Verify LDSC is optional (master still processes frames without LDSC)

**Pass Criteria:**
- ✅ LDSC field appears in response frame (e.g., `LDSC:42`)
- ✅ LDSC increments when readings fail validation
- ✅ LDSC does not decrement (monotonic)
- ✅ Master parser handles LDSC field correctly (or skips if absent)

**Note:** Backward compatibility check — old firmware without LDSC should still work with new master.

---

#### IT-04: Master Applies Hysteretic Flow Error State

**Purpose:** Verify ES32 flow-error hysteresis (3s assert, 5s clear) works.

**Pre-conditions:**
- IT-01 passing
- Flow sensor connected or simulated

**Steps:**
1. With pump OFF, flow reading should be ~0 LPM
2. Start pump (e.g., via dashboard MANUAL ON)
3. Turn off water valve to simulate dry-run → flow drops below threshold
4. Observe error flag behavior:
   - **At 0s:** flow_error = false
   - **At 1s:** flow drops below threshold, error still false (within assert window)
   - **At 3.5s:** error flag transitions to true (assert complete)
   - Restore water flow
   - **At 3.5s after restore:** error flag still true (within clear window)
   - **At 9s after restore:** error flag transitions to false (clear complete)

**Pass Criteria:**
- ✅ ASSERT_WINDOW (3s) observed before transitioning to error
- ✅ CLEAR_WINDOW (5s) observed before clearing error
- ✅ No race conditions (flag doesn't oscillate)
- ✅ Dashboard reflects final error state

---

#### IT-05: Master Handles Slave Timeout Gracefully

**Purpose:** Verify master doesn't crash if slave stops responding.

**Pre-conditions:**
- IT-01 passing, test in progress

**Steps:**
1. Power off NodeMCU slave mid-communication
2. Master should detect missing responses within 250ms × 3 retries = ~750ms
3. Master should transition to error state or safe standby
4. Master should not hang waiting for response indefinitely
5. Power NodeMCU back up
6. Communication should resume within 5 seconds

**Pass Criteria:**
- ✅ Master logs timeout error (not silent)
- ✅ Master does not hang
- ✅ Master retries 3 times before giving up per spec
- ✅ Resume works after slave returns
- ✅ No memory corruption or state leak

---

### Category B: Sensor Data Collection (IT-06 to IT-10)

#### IT-06: Ultrasonic Level Sensor Readings Are Stable

**Purpose:** Verify JSN-SR04T produces repeatable, stable readings.

**Pre-conditions:**
- NodeMCU running with ultrasonic sensor connected
- Tank with known water level (or test fixture)
- Serial monitor on NodeMCU console

**Steps:**
1. Let sensor stabilize for 30 seconds (5–10 pings)
2. Record 20 consecutive readings over 2–3 minutes
3. Calculate mean, min, max, std dev
4. Verify range is within 5cm (per spec)
5. Check for outliers (>10cm deviation from mean)

**Pass Criteria:**
- ✅ 20 valid readings obtained
- ✅ Std dev < 2cm (excellent stability)
- ✅ No timeout/error readings in sequence
- ✅ Readings match known tank level (±5cm)

**Failure Troubleshooting:**
| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| High jitter (>5cm range) | Reflective surface interference | Reposition sensor; use foam dampening |
| Timeout readings | Sensor misaligned or power glitch | Check wiring; reseat connector |
| Readings too low/high | Sensor misalignment | Adjust sensor angle (should be ~45° to surface) |

---

#### IT-07: Flow Sensor ISR Counts Match Calibration

**Purpose:** Verify YF-G1 flow sensor pulse counting is accurate.

**Pre-conditions:**
- NodeMCU with flow sensor connected to ISR pin (GPIO12)
- Test bench with controllable water flow (or simulated pulses)
- Calibration factor: 7.5 pulses/liter in firmware

**Steps:**
1. Run known volume through sensor (e.g., 1 liter in 10 seconds = 0.1 LPS)
2. Let NodeMCU count pulses over 10 seconds
3. Expected pulse count: 0.1 LPS × 7.5 pulses/L × 10s = 7.5 pulses
4. Allow ±10% tolerance for timing jitter
5. Repeat at 3 different flow rates (low, medium, high)

**Pass Criteria:**
- ✅ Pulse count within ±10% of expected
- ✅ Repeatability across 3 runs (std dev < 5%)
- ✅ No double-counting or missed pulses
- ✅ ISR doesn't interfere with RS-485 comms

**Note:** If flow sensor unavailable, IT-07 can be simulated by injecting test pulses.

---

#### IT-08: Sensor Data Persists Through C-02 Init State

**Purpose:** Verify waterLevelPct initializes to -1 and data updates properly.

**Pre-conditions:**
- ESP32 running production firmware
- Cold boot (power cycle)
- Firebase console open to monitor `/pump_system/status/water_level_percent`

**Steps:**
1. Power cycle ESP32
2. Monitor Firebase for first status push
3. Verify initial value is either:
   - `-1` (uninitialized, per C-02 fix), or
   - First valid sensor reading (if slave responds immediately)
4. Over next 30 seconds, observe level percentage stabilizing to valid range (0–100)
5. No NaN, undefined, or garbage values allowed

**Pass Criteria:**
- ✅ Level starts at -1 or first valid reading
- ✅ Transitions to stable reading within 30s
- ✅ No negative values persist in data
- ✅ Dashboard displays "-?" or "Initializing" gracefully during transition

---

#### IT-09: Dry-Run Lockout Engages After 30s Below Threshold

**Purpose:** Verify dry-run timeout mechanism (configured as 30s).

**Pre-conditions:**
- Both nodes running, communicating
- Flow sensor connected or simulated
- Pump running in MANUAL mode via dashboard
- Water valve can be closed to simulate dry-run

**Steps:**
1. Start pump with full water flow
2. Observe flow rate stabilizes (1–5 LPM depending on pressure)
3. Close water valve → flow drops to <0.5 LPM
4. Monitor time until error flag sets (should be ~30s)
5. Dashboard should display dry-run alert
6. Pump should automatically stop (relay OFF)
7. Reopen valve, observe dry-run clear and resumption

**Pass Criteria:**
- ✅ Error flag sets after ≤30s in dry-run
- ✅ Dashboard alerts user immediately
- ✅ Pump stops automatically (fail-safe ON)
- ✅ Recovery works: open valve → error clears → pump can restart

**Safety Note:** This is a critical safety test. Ensure no risk of system damage.

---

#### IT-10: Overflow Protection Engages After Max Runtime

**Purpose:** Verify max_pump_runtime_min configuration prevents overflow.

**Pre-conditions:**
- ESP32 running with max_pump_runtime_min configured (default: 120 min)
- Tank near full level
- Pump able to run for test duration (or use simulated time)

**Steps:**
1. Start pump in AUTO mode with tank empty
2. Let pump run continuously
3. Monitor cumulative runtime (uptime_minutes in status)
4. When runtime reaches max_pump_runtime_min (e.g., 120 min):
   - Pump should automatically stop
   - Overflow error flag should set
   - Dashboard should display overflow alert
5. Manual reset required (clear_error command from dashboard)

**Pass Criteria:**
- ✅ Pump stops at ≤120 min runtime
- ✅ Overflow error persists until cleared
- ✅ Fail-safe: pump stays OFF until explicitly cleared
- ✅ Dashboard notification visible to user

**Note:** For testing: configure max_pump_runtime_min to a small value (e.g., 1 min) to accelerate test.

---

### Category C: Firebase Synchronization (IT-11 to IT-15)

#### IT-11: Status Push to Firebase Every 3 Seconds When Running

**Purpose:** Verify periodic status push cadence.

**Pre-conditions:**
- ESP32 connected to WiFi and Firebase
- Dashboard or Firebase console open
- Pump running in MANUAL mode

**Steps:**
1. Start pump
2. Monitor `/pump_system/status/` timestamp (or use dashboard Activity panel)
3. Record push timestamps
4. Calculate intervals between consecutive pushes
5. Should see push every 2.8–3.2 seconds (within ±10%)
6. Continue for 1 minute (≥20 pushes)

**Pass Criteria:**
- ✅ ≥20 pushes in 1 minute
- ✅ Average interval = 3.0s ± 0.3s
- ✅ All 6 new Phase 5 fields present in push
- ✅ No duplicate timestamps (monotonic)

**New Fields in Push (Phase 5):**
```json
{
  "pump_cooldown_remaining_sec": 45,
  "manual_runtime_warning": false,
  "is_idle_mode": false,
  "debug_log_level": 2,
  "remote_level_discard_count": 3
}
```

---

#### IT-12: Control Command Read Latency < 5 Seconds

**Purpose:** Verify dashboard commands reach firmware within 5s.

**Pre-conditions:**
- Dashboard logged in as admin
- Both ESP32 and dashboard connected
- Pump in MANUAL OFF state

**Steps:**
1. Open browser DevTools (Network tab) and Firebase console side-by-side
2. Click "MANUAL ON" button on dashboard
3. Observe:
   - Dashboard writes to `/pump_system/control/manual_desired = true`
   - Monitor Firebase for the write
   - Monitor ESP32 serial console for "Reading control: manual_desired=true"
4. Time from button click to relay activation should be < 5 seconds
5. Repeat with other commands (mode change, clear_error, etc.)
6. Record latencies for 5 commands

**Pass Criteria:**
- ✅ 100% of commands arrive within 5s
- ✅ Average latency < 2s (normal WiFi)
- ✅ No commands lost (1:1 ratio of writes:reads)
- ✅ Stale command detection working (old commands ignored)

---

#### IT-13: New Phase 5 Fields Persisted Correctly

**Purpose:** Verify all 5 new type fields are stored and retrievable.

**Pre-conditions:**
- Firebase console open to `/pump_system/status/`
- ESP32 running and pushing status

**Steps:**
1. Let ESP32 push full status (pump running in COUNTDOWN mode)
2. In Firebase, verify presence of:
   - `pump_cooldown_remaining_sec` (number, 0 if not cooldown)
   - `manual_runtime_warning` (boolean, false if not exceeded)
   - `is_idle_mode` (boolean, true only when tank ≥90%)
   - `debug_log_level` (number, 0–4 matching firmware LOG level)
   - `remote_level_discard_count` (number, from LDSC field in RS-485)
3. Verify types match TypeScript schema
4. Verify no NaN, null, or string mismatches

**Pass Criteria:**
- ✅ All 5 fields present in Firebase status
- ✅ Data types match `PumpStatus` interface
- ✅ Values are realistic (log level 0–4, cooldown ≥0, etc.)
- ✅ Fields persist across multiple pushes (no disappearing data)

---

#### IT-14: Backward Compatibility: Old Firmware Fields Still Work

**Purpose:** Ensure Phase 5 additions don't break old firmware.

**Pre-conditions:**
- A version of firmware without the 5 new fields (or manually simulate old status)

**Steps:**
1. Temporarily edit ESP32 status push to exclude new fields
2. Push legacy status without: pump_cooldown_remaining_sec, manual_runtime_warning, etc.
3. Dashboard should still render without crashing
4. Older fields (is_running, flow_rate_lpm, etc.) should display correctly
5. New component (CooldownTimer, IdleModeBadge, etc.) should render null gracefully
6. Revert to full Phase 5 firmware

**Pass Criteria:**
- ✅ Dashboard doesn't crash on missing Phase 5 fields
- ✅ Optional fields handled with ?? operator
- ✅ ErrorBoundary not triggered
- ✅ Smooth transition when firmware updates to Phase 5

---

#### IT-15: CRC Corruption Detected and Logged

**Purpose:** Verify Firebase data integrity via CRC or checksum.

**Pre-conditions:**
- IT-11 passing (status pushing)

**Steps:**
1. Introduce temporary CRC corruption in firmware status push (debug mode)
2. Firebase receives corrupted data
3. Dashboard should either:
   - Log CRC validation error, or
   - Reject update with warning
4. System should remain stable (not crash)
5. Revert corruption

**Pass Criteria:**
- ✅ Corruption detected before use
- ✅ Error logged with context
- ✅ System remains operational
- ✅ Next valid push succeeds

---

### Category D: Dashboard UI Binding (IT-16 to IT-20)

#### IT-16: CooldownTimer Displays When in Cooldown Mode

**Purpose:** Verify new Phase 6 CooldownTimer component works.

**Pre-conditions:**
- Dashboard running locally or deployed
- ESP32 in AUTO_COOLDOWN or MANUAL_COOLDOWN mode
- `pump_cooldown_remaining_sec` > 0

**Steps:**
1. Trigger cooldown (run pump, then trigger safety stop)
2. Observe dashboard main grid (left stats rail)
3. CooldownTimer should appear showing "Mm Ss cooldown" format
4. Countdown should tick down smoothly every second
5. When remaining_sec reaches 0, timer should disappear
6. Pump should transition out of cooldown mode

**Pass Criteria:**
- ✅ Timer appears only in cooldown modes
- ✅ Display format correct (e.g., "2m 30s")
- ✅ Countdown is accurate (within ±1s over 30s)
- ✅ Smooth disappearance when done
- ✅ No flickering or re-renders

---

#### IT-17: IdleModeBadge Displays When Tank ≥ 90%

**Purpose:** Verify Phase 6 IdleModeBadge component.

**Pre-conditions:**
- Dashboard open, pump in AUTO mode

**Steps:**
1. Fill tank to 100% (or simulate high level in Firebase)
2. Start pump → tank level decreases → reaches 90%
3. Dashboard should show IdleModeBadge: "Idle Mode (90%)"
4. Pump should switch to slow poll (verify via serial console: idle_firebase_interval_ms)
5. Empty tank below 90%
6. Badge should disappear, pump returns to normal poll

**Pass Criteria:**
- ✅ Badge appears at ≥90%
- ✅ Badge displays current level%
- ✅ Badge disappears at <90%
- ✅ No visual glitches

---

#### IT-18: LogLevelControl Dropdown Changes Remote Log Level

**Purpose:** Verify admin can adjust firmware logging level.

**Pre-conditions:**
- User logged in as admin
- Dashboard open
- ESP32 connected

**Steps:**
1. Locate LogLevelControl in DashboardSystemInfo (or device config panel)
2. Admin should see dropdown with 5 options: ERROR, WARN, INFO, DEBUG, VERBOSE
3. Select DEBUG
4. Monitor Firebase: `/pump_system/config/debug_log_level` should change to 3
5. Monitor ESP32 serial console: should log `[INFO] Remote log level updated: 3 (DEBUG)`
6. Increase verbosity of firmware output (should see more [DEBUG] messages)
7. Select ERROR level → firmware output reduces

**Pass Criteria:**
- ✅ Dropdown only visible to admins
- ✅ Selection writes to Firebase immediately
- ✅ ESP32 reads new level within 3 seconds
- ✅ Output verbosity follows new level
- ✅ Non-admins see read-only display

---

#### IT-19: RemoteDiscard (LDSC) Shows Sensor Health

**Purpose:** Verify RemoteDiscard component displays sensor diagnostics.

**Pre-conditions:**
- Dashboard open, pump running
- remote_level_discard_count in status (from RS-485)

**Steps:**
1. In Status System Info section, locate RemoteDiscard badge
2. Shows "LDSC: X" where X is count from NodeMCU
3. Color coding:
   - Green: LDSC < 20 (healthy)
   - Yellow: LDSC 20–50 (degraded)
   - Red: LDSC ≥ 50 (unhealthy)
4. Tap/vibrate ultrasonic sensor to increase discard count
5. Observe color transition from green → yellow → red
6. Verify tooltip shows detailed explanation on hover

**Pass Criteria:**
- ✅ LDSC value displayed accurately
- ✅ Color transitions as LDSC increases
- ✅ Tooltip provides helpful diagnostic context
- ✅ Updates every status push (3s cadence)

---

#### IT-20: ErrorBoundary Prevents Cascade Failures

**Purpose:** Verify error handling for critical components.

**Pre-conditions:**
- Dashboard running with full integration

**Steps:**
1. Inject JavaScript error into one component (e.g., modify DashboardMainGrid to throw)
2. Test error boundary:
   - Component crashes
   - Fallback UI displays with error message
   - "Reload page" button works
   - Other sections (history, activity) remain functional
3. Fix injected error, reload dashboard
4. Component resumes normal operation

**Pass Criteria:**
- ✅ Error caught and displayed in fallback
- ✅ Other dashboard sections still interactive
- ✅ Reload button restores full functionality
- ✅ No console errors leaking to user

---

### Category E: End-to-End System (IT-21)

#### IT-21: Full System Workflow Under Real Conditions

**Purpose:** Validate complete system integration: sensors → master → Firebase → dashboard.

**Pre-conditions:**
- All Phase 5 tests passing (IT-01 to IT-20)
- Full hardware setup: tank, pump, sensors, WiFi, Firebase
- Admin user logged into dashboard
- Both firmware images production-ready

**Steps:**

**A. Initial State (5 min)**
1. Power on entire system
2. Observe dashboard "connecting" state
3. Within 30s: ESP32 online, status data flowing
4. Within 60s: sensor readings stable (level % settled)

**B. Automated Mode Workflow (10 min)**
1. Set pump_start_level = 30%, pump_stop_level = 100%
2. Empty tank to 20% (via drain valve or manual removal)
3. Switch to AUTO mode on dashboard
4. Pump should start (relay ON)
5. Monitor level % increasing every 3 seconds
6. When level reaches 100%, pump stops (relay OFF)
7. Verify:
   - Flow rate showing during run (1–5 LPM)
   - No dry-run or overflow errors
   - Dashboard status updates fluent (no lag >5s)
   - Firebase shows consistent data
8. Close drain valve, wait for level stabilization
9. Repeat cycle: empty to 20% → AUTO start → fill to 100% → stop

**C. Manual Mode Workflow (5 min)**
1. Switch to MANUAL mode
2. Click "Manual ON" → pump starts immediately
3. Monitor for 30 seconds:
   - Relay energized
   - Flow rate rising to 2–3 LPM
   - No sensor errors
4. Click "Manual OFF" → pump stops immediately
5. Repeat 3 times, verify consistent response

**D. Emergency Stop Workflow (3 min)**
1. Start pump in MANUAL mode
2. Click "Emergency Stop" on dashboard
3. Pump must stop within 1 second
4. Emergency stop latch should appear (red banner)
5. "Clear Error & Restart" button visible
6. Click to clear latch
7. Pump resumes operation normally

**E. Safety Boundary Tests (5 min)**
1. **Dry-run test:** Start pump, close water valve
   - At ~30s: error flag should set
   - Dashboard shows dry-run alert
   - Pump stops automatically
   - Reopen valve → pump can restart after clear

2. **Overflow test:** (If max_pump_runtime_min configured to 1–2 min for testing)
   - Start pump in AUTO
   - Let run until max runtime reached
   - Pump stops
   - Overflow error visible
   - Clear error from dashboard

**F. Data Consistency Check (5 min)**
1. Open two browser tabs: dashboard + Firebase console
2. Run 30 seconds of pump operation
3. Compare data:
   - water_level_percent matches TankVisual display
   - flow_rate_lpm matches FlowStrip display
   - is_running matches pump ON/OFF indicator
   - run_mode matches ModeControls selection
   - All new Phase 5 fields present and reasonable
4. No data divergence observed

**G. Rollback Resilience (5 min)**
1. Power cycle ESP32 while pump running
2. Pump should stop immediately (fail-safe)
3. After restart, dashboard should reconnect within 30s
4. Status returns to normal (no stale data from before crash)
5. Pump should resume operation if still in AUTO

**Pass Criteria (All Must Pass):**
- ✅ System starts and stabilizes within 60s
- ✅ Automated fill/drain cycle works 3 consecutive times
- ✅ Manual mode responsive (<1s latency)
- ✅ Emergency stop works instantly
- ✅ Dry-run detection and failsafe works
- ✅ No data divergence between dashboard ↔ Firebase
- ✅ All Phase 5 fields present and accurate
- ✅ Rollback and restart handling graceful
- ✅ Zero crashes or hangs during 45 min runtime
- ✅ Serial logs contain no warnings or errors

---

## 4. Test Execution Checklist

### Pre-Test (Day Before)
- [ ] Compile both firmware images (sensor + master) with production config
- [ ] Verify Arduino IDE serial monitor can connect to each board
- [ ] Test WiFi SSID reachable from test location
- [ ] Verify Firebase rules allow admin user full read/write
- [ ] Deploy dashboard to local machine or Vercel
- [ ] Fill tank to test level
- [ ] Review all test procedures above

### During Test Execution
- [ ] Document start time
- [ ] Execute IT-01 through IT-20 in order (allow 5–10 min per test)
- [ ] Record any warnings/errors in log
- [ ] Failed tests: document root cause and mitigation
- [ ] Execute IT-21 end-to-end (45 min planned, may be 30–60 min actual)
- [ ] Document stop time and total duration

### Post-Test Validation
- [ ] Count passed tests: ___/21
- [ ] If <21 passed: determine blocking issues
- [ ] Collect artifacts:
  - Serial logs from both boards (copy/paste to file)
  - Firebase console screenshots (status, control)
  - Dashboard screenshots (main grid, alerts, settings)
  - Browser console errors (if any)
- [ ] Generate test summary (see section 5)

---

## 5. Test Result Summary Template

```
[PHASE 7 INTEGRATION TEST REPORT]
Date: ___________
Tester: ___________
Duration: _________ min
System:
  - ESP32 firmware build: ________ (date/commit)
  - NodeMCU firmware build: ______ (date/commit)
  - Dashboard version: __________ (local/Vercel)

Results:
RS-485 Communication:
  [ ] IT-01: Master-to-Slave Request   PASS / FAIL
  [ ] IT-02: Slave Handles Invalid      PASS / FAIL
  [ ] IT-03: LDSC Field Increments     PASS / FAIL
  [ ] IT-04: Flow Hysteresis 3s/5s     PASS / FAIL
  [ ] IT-05: Slave Timeout Handling    PASS / FAIL

Sensor Data:
  [ ] IT-06: Ultrasonic Stability      PASS / FAIL
  [ ] IT-07: Flow Calibration          PASS / FAIL
  [ ] IT-08: Init State (-1)           PASS / FAIL
  [ ] IT-09: Dry-Run (30s timeout)     PASS / FAIL
  [ ] IT-10: Overflow (max_runtime)    PASS / FAIL

Firebase Sync:
  [ ] IT-11: Status Push 3s Cadence    PASS / FAIL
  [ ] IT-12: Command Latency <5s       PASS / FAIL
  [ ] IT-13: Phase 5 Fields Persist    PASS / FAIL
  [ ] IT-14: Backward Compatibility    PASS / FAIL
  [ ] IT-15: CRC Integrity Check       PASS / FAIL

Dashboard UI:
  [ ] IT-16: CooldownTimer             PASS / FAIL
  [ ] IT-17: IdleModeBadge             PASS / FAIL
  [ ] IT-18: LogLevelControl Dropdown  PASS / FAIL
  [ ] IT-19: RemoteDiscard LDSC Badge  PASS / FAIL
  [ ] IT-20: ErrorBoundary Catch       PASS / FAIL

End-to-End:
  [ ] IT-21: Full System Workflow      PASS / FAIL

Summary:
  Total Passed: ___/21
  Total Failed: ___/21

Blocking Issues (if any):
  1. ___________________________
  2. ___________________________

Sign-off:
  Ready for production: YES / NO
  Comments: ________________________
```

---

## 6. Production Rollout Checklist

**Only proceed if all 21 tests PASS.**

- [ ] Generate test summary (section 5)
- [ ] Obtain approval from project lead
- [ ] Archive firmware + dashboard versions with test date
- [ ] Document any workarounds or known limitations
- [ ] Create release notes:
  - Phases 1–7 summary
  - New fields (Phase 5 type system)
  - Branding changes (Phase 6)
  - Safety guarantees
- [ ] Notify end-user community of availability
- [ ] Monitor crash logs for first week post-deployment
- [ ] Plan sprint for bug fixes (estimated 3–5 issues from production)

---

## 7. References

- [Refactor Plan](../../smartflow_refactor_plan_v2.md)
- [Phase 5 Tests (Firmware)](../../firmware/test_sensor_node/README.md) & [Master](../../firmware/test_master_node/README.md)
- [RS-485 Protocol Spec](../../docs/specs/rs485_protocol.md)
- [Dashboard Type System](../../dashboard/lib/types.ts)
- [Safety Rules](../../firmware/arduino_smart_water_pump_controller/00_safety_config.h)
- [Firebase Schema](../../database.rules.json)

---

**End of Phase 7 Integration Test Plan**

*Approval: Pending test execution and pass criteria validation.*
