# Hardware Flash & Test Execution Guide
**Last Updated:** 2026-03-31  
**Status:** Ready for manual hardware validation  
**Audience:** QA technician performing Layer 2 and Layer 3 tests

---

## Part 1: Hardware Prerequisites Checklist

Before starting any flash or test, verify:

- [ ] NodeMCU V2 board powered and accessible over USB
- [ ] ESP32 dev board powered and accessible over USB
- [ ] USB-TTL serial cable/adapter available (for NodeMCU debug logging)
- [ ] CAT6 twisted pair or shielded wire for RS-485 A/B + common ground
- [ ] RS-485 transceiver modules (MAX485 or equivalent) × 2
- [ ] USB hub with sufficient power (recommended for dual boards)
- [ ] Arduino IDE or `arduino-cli` installed and in PATH
- [ ] PlatformIO CLI installed (for firmware uploads)
- [ ] Terminal/console ready for log capture (PuTTY, miniterm, or Arduino IDE Serial Monitor)
- [ ] Document editor for recording results (Excel, markdown, or spreadsheet)

---

## Part 2: Firmware Locations & Build Status

### Sensor Node (NodeMCU V2)

**Test firmware location:**
```
firmware/test_sensor_node/test_sensor_node.ino
```

**Build status:** ✅ PASS (compile verified 2026-03-31)
- Size: 891660 bytes (68% flash utilization)
- Core: ESP8266 3.1.2
- Dependencies: Syslog v2.0.0, standard Arduino core

**Flash command (Arduino CLI):**
```bash
arduino-cli upload \
  --fqbn esp8266:esp8266:nodemcuv2 \
  --port COM3 \
  firmware/test_sensor_node/test_sensor_node.ino
```

**Flash command (PlatformIO):**
```bash
pio run -d firmware/platformio_sensor_node -e nodemcuv2 -t upload --upload-port COM3
```

**After successful upload:**
1. Open serial monitor (115200 baud)
2. Press **RESET** button on NodeMCU
3. Confirm test output begins (should show test case headers)

---

### Master Node (ESP32 Dev)

**Test firmware location:**
```
firmware/test_master_node/test_master_node.ino
```

**Build status:** ✅ PASS (compile verified 2026-03-31)
- Size: ~1.1MB (36% flash utilization)
- Core: ESP32 3.3.7
- Dependencies: Firebase Realtime Database SDK v11.0.2, standard Arduino core

**Flash command (Arduino CLI):**
```bash
arduino-cli upload \
  --fqbn esp32:esp32:esp32 \
  --port COM4 \
  firmware/test_master_node/test_master_node.ino
```

**Flash command (PlatformIO + USB):**
```bash
pio run -d firmware/platformio_smart_water_pump_controller -e esp32dev -t upload --upload-port COM4
```

**After successful upload:**
1. Open serial monitor (115200 baud)
2. Press **BOOT** button on ESP32
3. Confirm test output begins

---

## Part 3: Identifying COM Ports

### Method 1: Arduino CLI
```bash
arduino-cli board list
```
Sample output:
```
Port         Type              Board Name               FQBN                                     Core
COM3         Serial Port (USB) Arduino/Genuino Micro   arduino:avr:micro                        arduino:avr
COM4         Serial Port (USB) Generic ESP32            esp32:esp32:esp32dev                     esp32:esp32
```

### Method 2: PlatformIO
```bash
pio device list
```

### Method 3: Windows Device Manager
- Open **Device Manager**
- Expand **Ports (COM & LPT)**
- Note which COM ports are listed
- Connect each board one at a time to confirm which is which

### Method 4: Manual Serial Probe
```bash
# Test if COM port responds (PowerShell)
$port = "COM3"
$baud = 115200
# After board upload and reset, check for serial output in Arduino IDE or PuTTY
```

---

## Part 4: Layer 2 Manual Test Execution (Sensor Node)

### Test: SF-SN-001 Ultrasonic Sensor Accuracy

**Prerequisite:**
- NodeMCU flashed with `test_sensor_node.ino`
- Serial monitor open at 115200 baud
- No obstacles between sensor and tank

**Steps:**
1. Press **RESET** on NodeMCU
2. Watch serial output for `[TEST] SF-SN-001: Ultrasonic accuracy check started`
3. Place hand at **10 cm** from sensor, hold stable for 3 seconds
4. Record reading from serial output (e.g., `Measured: 10.2 cm, Error: +0.2%`)
5. Move to **50 cm**, hold stable for 3 seconds, record adjacent reading
6. Move to **100 cm** (or max range), hold stable for 3 seconds, record reading
7. Screenshot the serial monitor showing all three readings
8. **PASS if:** All readings within ±2% tolerance of true distance
9. Record result in `results.csv` (PASS/FAIL) with screenshot evidence path

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-SN-001_ultrasonic_accuracy.txt
docs/audit/qa/2026-03-31/day-1/screenshots/SF-SN-001_serial_output.png
```

---

### Test: SF-SN-002 Level Percentage Calculation

**Prerequisite:**
- NodeMCU still running from SF-SN-001
- Tank dimensions and calibration constants known

**Steps:**
1. Watch serial output for `[TEST] SF-SN-002: Level percentage calculation`
2. Trigger three known tank states: **EMPTY, 50%, FULL** (manually position water or use airflow if no liquid)
3. Record serial output showing calculated level percentage for each state
4. Verify formula: `Level % = (current_height / tank_height) * 100`
5. Screenshot evidence showing empty (0%), mid (50%), full (100%) readings
6. **PASS if:** Calculations match formula within ±3% across all three states

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/screenshots/SF-SN-002_level_calc_empty_mid_full.png
```

---

### Test: SF-SN-003 Flow Sensor Calibration

**Prerequisite:**
- Flow sensor connected to GPIO with interrupt active
- Water pump or manual flow tester available

**Steps:**
1. Watch for `[TEST] SF-SN-003: Flow sensor calibration`
2. Start water flow at known rate (e.g., 2 LPM from bucket timer)
3. Let flow run for 10 seconds, then stop
4. Record serial output showing: pulses, calculated LPM, expected LPM
5. Calculate error: `|measured - expected| / expected * 100`
6. Repeat at **4 LPM** and **6 LPM** if possible
7. **PASS if:** Error ≤ 5% at all tested flow rates
8. Screenshot showing all flow calibration evidence

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-SN-003_flow_calibration.csv
docs/audit/qa/2026-03-31/day-1/screenshots/SF-SN-003_flow_rates.png
```

---

### Test: SF-SN-004 Sensor Error Flag Behavior

**Prerequisite:**
- NodeMCU running test suite

**Steps:**
1. Watch for `[TEST] SF-SN-004: Error flag detection`
2. Disconnect ultrasonic sensor (simulate hardware failure)
3. Record timestamp when error flag triggers in serial output
4. Reconnect sensor and apply valid signal
5. Record timestamp when error flag auto-clears
6. **PASS if:** Error detected within 2 seconds of disconnection, auto-clears within 1 second of valid signal
7. Screenshot showing error state transitions

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/screenshots/SF-SN-004_error_flag_transitions.png
```

---

### Test: SF-SN-005 Flow Error Hysteresis

**Prerequisite:**
- Flow sensor connected, water/test flow available

**Steps:**
1. Watch for `[TEST] SF-SN-005: Hysteresis behavior`
2. Run stable flow for 10 seconds (baseline good state)
3. Abruptly stop flow (simulate clog/blockage)
4. Record how long before error flag sets (should honor dwell period, typically 15–30 sec)
5. Resume flow
6. Record how long before error flag clears (should honor clear dwell, typically 5–10 sec)
7. **PASS if:** Error state respects configured dwell windows (no false positive on transient flow drops)
8. Screenshot showing dwell timing evidence

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-SN-005_hysteresis_timeline.txt
```

---

### Test: SF-SN-006 Level Plausibility Filter

**Prerequisite:**
- Ultrasonic running in Layer 2 context

**Steps:**
1. Watch for `[TEST] SF-SN-006: Plausibility filter`
2. Apply rapid air pulses or obstructions to sensor (simulate false readings)
3. Observe serial output for `remote_level_discard_count` increment
4. Rapidly tap sensor dish or shake unit → count should increase
5. Let sensor settle → count should stabilize
6. **PASS if:** False readings are rejected and discard_count increments only on implausible jumps (> threshold)
7. Screenshot showing discard counter behavior

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/screenshots/SF-SN-006_discard_counter.png
```

---

## Part 5: Layer 3 Manual Test Execution (RS-485 Protocol)

### Prerequisite Setup for All Layer 3 Tests

**Hardware connections:**
- NodeMCU and ESP32 powered (both running test firmware)
- RS-485 A and B wired in parallel between MAX485 modules on each board
- Common ground wire connected between both boards
- CAT6 or shielded twisted pair used (not plain wire)
- Logic analyzer connected to RS-485 A/B if timing validation required
- Serial monitors open on BOTH boards (two terminal windows)

**Before starting Layer 3:**
1. Verify Layer 2 (SF-SN-001..006) complete and all PASS
2. Ensure baseline firmware hash recorded: **881195f**
3. Open tracker: `docs/audit/qa/2026-03-31/day-1/results.csv`
4. Open defect log: `docs/audit/qa/2026-03-31/day-1/defects.csv`

---

### Test: SF-RS-001 Frame CRC Integrity

**Objective:** Verify CRC16-Modbus calculation and rejection of corrupt frames

**Steps:**
1. Start both boards (NodeMCU + ESP32)
2. Allow several valid poll/response cycles (watch serial output for successful frame exchanges)
3. Screenshot serial output showing valid CRC (e.g., `CRC: 0xA1B2 OK`)
4. Manually inject a corrupt frame by external tool or forced firmware error (if available)
5. Observe ESP32 serial output rejecting frame with `CRC: 0xXXXX FAIL`
6. Verify sensor node does NOT process bad CRC frame
7. **PASS if:** Valid CRC frames processed, corrupt CRC frames rejected
8. Record evidence: serial excerpt showing CRC validation

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-001_crc_validation.txt
```

---

### Test: SF-RS-002 Timeout and Retry Behavior

**Objective:** Verify master detects offline sensor node and retries appropriately

**Steps:**
1. Both boards running, normal polling active
2. Screenshot normal poll interval (e.g., every 5 seconds)
3. Physically disconnect RS-485 A or B wire (simulate link failure)
4. Watch ESP32 serial output for timeout detection
5. Record timestamp when offline state triggered
6. Verify retry count increments (check serial output for `Retry: N`)
7. Reconnect wire, watch for online state recovery
8. Record timestamp when link re-establishes
9. **PASS if:** Timeout detected within 10 sec, retry count reasonable (3–5 attempts), online recovery within 5 sec of reconnect
10. Screenshot showing offline→retry→online transition

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-002_timeout_retry_recovery.txt
```

---

### Test: SF-RS-003 Partial Frame Stall Recovery

**Objective:** Verify incomplete frame doesn't lock state machine

**Steps:**
1. Normal polling running
2. Manually send partial frame (e.g., first 4 bytes only) from external source or forced firmware test
3. Observe NodeMCU serial output for stall detection
4. Verify state machine exits stall and waits for new frame
5. Resume normal polling, confirm next complete frame processes cleanly
6. **PASS if:** No parser crash, state machine recovers, next valid frame succeeds
7. Screenshot showing stall detection and recovery

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-003_partial_frame_recovery.txt
```

---

### Test: SF-RS-004 Sequence Monotonicity and Wrap

**Objective:** Verify sequence counter increments monotonically and wraps 255→0

**Steps:**
1. Normal polling active for at least 100 frames
2. Extract sequence numbers from serial logs or frame capture
3. Plot sequence: should be 0, 1, 2, ..., 254, 255, 0, 1, 2... with no gaps
4. Verify wrap occurs correctly at 255→0 boundary
5. Run extended poll to observe at least 2 complete cycles (0–255 twice)
6. **PASS if:** No gaps, monotonic increment, correct wrap behavior
7. Capture sequence trace showing full cycle

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-004_sequence_trace.csv
```

---

### Test: SF-RS-005 LDSC Backward Compatibility

**Objective:** Verify LDSC (Level Discard Sequence Count) optional field doesn't break legacy parsers

**Steps:**
1. Run Layer 2 tests with LDSC field enabled (firmware 881195f includes LDSC)
2. Capture frames with LDSC included in payload
3. Verify ESP32 correctly parses and updates discard counter
4. Disable LDSC in NodeMCU test firmware (simulated by sending frame without field)
5. Observe ESP32 gracefully handles omitted LDSC (no crash, no incorrect state)
6. Resume with LDSC enabled, verify parser accepts both variants
7. **PASS if:** Parser accepts frames with and without LDSC, no state corruption
8. Screenshot showing LDSC present and absent frame variants

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-005_ldsc_compatibility.txt
```

---

### Test: SF-RS-006 Bus Direction Control Timing

**Objective:** Verify RS-485 driver enable (DE) and receiver enable (RE) transitions are collision-free

**Steps:**
1. Connect logic analyzer to RS-485 A and B lines
2. Set logic analyzer to capture bus transitions and timing
3. Run 20–30 frame exchange cycles at normal polling rate
4. Analyze captured waveforms for:
   - No simultaneous transmit from both devices
   - Clean bus transitions (no glitches during DE/RE switch)
   - Proper time gap between transmitter release and next receiver enable
5. **PASS if:** No bus contention detected, all transitions clean, timing margins > 1µs
6. Export logic analyzer capture as evidence

**Evidence path:**
```
docs/audit/qa/2026-03-31/day-1/logs/SF-RS-006_logic_capture.csv
docs/audit/qa/2026-03-31/day-1/screenshots/SF-RS-006_bus_timing_waveform.png
```

---

## Part 6: Result Recording

### Recording PASS Results

For each completed test, update `results.csv`:
```csv
Test_ID,Status,Timestamp,Evidence_Path,Notes
SF-SN-001,PASS,2026-03-31T14:32:00Z,"logs/SF-SN-001_ultrasonic_accuracy.txt","10cm: 10.2cm, 50cm: 50.1cm, 100cm: 100.3cm — all ±2% tolerance"
SF-SN-002,PASS,2026-03-31T14:35:00Z,"screenshots/SF-SN-002_level_calc.png","Empty: 0%, Mid: 51%, Full: 100% — formula verified"
```

### Recording FAIL Results

For each failed test, **also create a defect entry** in `defects.csv`:
```csv
Defect_ID,Test_ID,Severity,Description,Root_Cause,Resolution_Status
D-002,SF-SN-001,CRITICAL,"Ultrasonic reads ±5% at 100cm distance","Sensor calibration constant incorrect","OPEN—needs sensor recalibration"
```

### Recording SKIP Results

Only SKIP if prerequisites cannot be met:
```csv
Test_ID,Status,Timestamp,Evidence_Path,Notes
SF-RS-006,SKIP,2026-03-31T16:00:00Z,"N/A","Logic analyzer not available this session; reschedule with instrumentation"
```

---

## Part 7: Exit Gate Validation

**Before declaring Layer 2 complete:**
- [ ] All SF-SN-001 to SF-SN-006 executed (status: PASS/FAIL/SKIP)
- [ ] Every PASS has supporting evidence artifact (screenshot, log, or measurement)
- [ ] No unresolved CRITICAL defects in SF-SN scope
- [ ] `results.csv` fully populated with timestamps and notes

**Before declaring Layer 3 complete:**
- [ ] All SF-RS-001 to SF-RS-006 executed (status: PASS/FAIL/SKIP)
- [ ] CRITICAL test results: SF-RS-001 ✓ PASS, SF-RS-002 ✓ PASS, SF-RS-006 ✓ PASS
- [ ] Every PASS has evidence artifact
- [ ] No unresolved CRITICAL protocol defects
- [ ] `defects.csv` includes all failures with severity/status
- [ ] Layer 2 preceding pre-requisites all resolved

---

## Part 8: Troubleshooting

### Board Not Detected in Arduino CLI
```bash
# Reinstall USB drivers
# Verify board is powered
# Try different USB cable/port
arduino-cli board list --verbose
```

### Serial Monitor Shows Garbage
- Baud rate mismatch: ensure 115200
- Wrong COM port: re-run `arduino-cli board list`
- Board not reset after upload: press RESET button manually

### Flash Upload Fails
```bash
# Check board is in bootloader mode
# Try esptool directly
esptool.py --chip esp32 --port COM4 --baud 460800 write_flash 0x1000 firmware.bin
```

### RS-485 Communication Failure
- Verify A/B wires physically connected
- Check common ground between boards
- Use multimeter to measure voltage differential on A/B (should be ~5V when idle)
- Swap A/B wires if no activity

### Test Firmware Compile Issues
All test sketches are pre-verified (compilation PASS as of 2026-03-31). If recompilation needed:
```bash
# For NodeMCU test
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 firmware/test_sensor_node/test_sensor_node.ino

# For ESP32 test
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/test_master_node/test_master_node.ino
```

---

## Part 9: Document & Advance

Once all tests complete:
1. **Commit results** to Git with message: `QA: Layer 2 & 3 hardware validation complete — <summary of results>`
2. **Upload evidence** folder to shared storage if applicable
3. **Review exit gates** against completion criteria above
4. **Advance to Phase 7** (integration validation) only if all CRITICAL tests pass

---

**Questions or issues?** Refer to Layer 2 and Layer 3 execution sheets in `docs/audit/qa/2026-03-31/day-1/` for detailed per-test criteria.

---

*Document prepared: 2026-03-31 by QA Agent*  
*Status: Ready for manual hardware test execution*
