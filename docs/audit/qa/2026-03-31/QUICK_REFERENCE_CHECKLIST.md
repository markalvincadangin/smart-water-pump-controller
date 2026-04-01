# Quick Reference: Hardware Test Execution Checklist
**One-page reference for Layer 2 & 3 manual test execution**

---

## Phase 1: Identify Boards & Ports

```bash
# List all detected boards
arduino-cli board list

# Record COM ports:
# NodeMCU V2 → ___________ (e.g., COM3)
# ESP32 Dev  → ___________ (e.g., COM4)
```

---

## Phase 2: Flash Test Firmware

### NodeMCU (Sensor Node Test)
```bash
# Option A: Arduino CLI
arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 \
  --port COM3 firmware/test_sensor_node/test_sensor_node.ino

# Option B: PlatformIO
pio run -d firmware/platformio_sensor_node -e nodemcuv2 -t upload --upload-port COM3
```
- **Expected:** Upload completes, board resets
- **Verify:** Serial output shows test case headers at 115200 baud

### ESP32 (Master Node Test)
```bash
# Option A: Arduino CLI
arduino-cli upload --fqbn esp32:esp32:esp32 \
  --port COM4 firmware/test_master_node/test_master_node.ino

# Option B: PlatformIO
pio run -d firmware/platformio_smart_water_pump_controller -e esp32dev \
  -t upload --upload-port COM4
```
- **Expected:** Upload completes, board resets
- **Verify:** Serial output shows test case headers at 115200 baud

---

## Phase 3: Execute Layer 2 Tests (Sensor Node)

| Test ID | Name | Hardware | Duration | Pass Criteria |
|---------|------|----------|----------|---------------|
| SF-SN-001 | Ultrasonic Accuracy | Ultrasonic sensor | 2 min | ±2% tolerance at 10/50/100cm |
| SF-SN-002 | Level Calculation | None (simulated) | 1 min | 0%/50%/100% formula validated |
| SF-SN-003 | Flow Calibration | Flow sensor + pump | 5 min | ±5% error at multiple rates |
| SF-SN-004 | Error Flag | Sensor simulator | 2 min | Error detected <2s, cleared <1s |
| SF-SN-005 | Hysteresis | Flow sensor | 3 min | Dwell windows honored |
| SF-SN-006 | Discard Counter | Obsruct sensor | 2 min | Counter increments on false reads |

**Quick execution:**
1. Open serial monitor for NodeMCU at **115200 baud**
2. Press **RESET** on NodeMCU
3. Watch test cases run in order (auto-advancing every ~30–60 seconds)
4. Screenshot each test result
5. Record PASS/FAIL in `results.csv`

---

## Phase 4: Execute Layer 3 Tests (RS-485 Protocol)

| Test ID | Name | Dependencies | Duration | Pass Criteria |
|---------|------|--------------|----------|---------------|
| SF-RS-001 | CRC Integrity | Both nodes + RS-485 | 2 min | Valid frames pass, corrupt frames rejected |
| SF-RS-002 | Timeout/Retry | Both nodes + RS-485 | 5 min | Offline detected <10s, retry <5 attempts |
| SF-RS-003 | Partial Frame Recovery | Both nodes + RS-485 | 2 min | No crash, recovery on next frame |
| SF-RS-004 | Sequence Monotonicity | Both nodes + RS-485 | 3 min | Sequence 0–255 no gaps, wrap correct |
| SF-RS-005 | LDSC Compatibility | Both nodes + RS-485 | 2 min | Parser accepts LDSC present & absent |
| SF-RS-006 | Bus Direction Timing | Both nodes + RS-485 + Logic Analyzer | 5 min | No collisions, clean transitions |

**Quick execution:**
1. Verify Layer 2 all PASS (prerequisite)
2. Connect RS-485 wires: A/B between boards, common ground
3. Open serial monitors for **both boards** at 115200 baud
4. Allow normal polling to run 30+ seconds (baseline)
5. Screenshot normal poll sequence from both terminals
6. For each test: execute fault scenario per layer3_rs485_execution.md
7. Record PASS/FAIL with evidence path

---

## Recording Results

### Success Path
```
results.csv row format:
Test_ID, PASS, timestamp, evidence_path, "measurement/note"

Example:
SF-SN-001, PASS, 2026-03-31T14:32:00, logs/SF-SN-001_accuracy.txt, "10cm:10.2cm, 50cm:50.1cm, 100cm:100.3cm"
```

### Failure Path
```
1. Record in results.csv:
   SF-SN-001, FAIL, timestamp, logs/SF-SN-001_fail.txt, "Error beyond ±2%"

2. Create defect in defects.csv:
   D-NNN, SF-SN-001, CRITICAL/MAJOR/MINOR, "Description", "Root cause", "OPEN/RESOLVED"

3. Stop further tests in same layer until root cause addressed
```

---

## Troubleshooting Quick Fixes

| Issue | Fix |
|-------|-----|
| "Board not found" | `arduino-cli board list`, verify USB port in Device Manager |
| Serial shows garbage | Check baud at 115200, verify correct COM port |
| Upload fails | Press RESET/BOOT on board, try different USB cable |
| RS-485 no comms | Check A/B wires connected, verify common ground, test with multimeter |
| Test hangs mid-execution | Power cycle both boards, reload firmware |

---

## Exit Gate Checkpoint

**Before completing QA session:**

- [ ] All SF-SN-001 to SF-SN-006 rows in results.csv (PASS/FAIL/SKIP)
- [ ] Every PASS has screenshot/log evidence in `day-1/logs/` or `screenshots/`
- [ ] Layer 2 prerequisites met for Layer 3 (all PASS or waived SKIP)
- [ ] All SF-RS-001 to SF-RS-006 rows in results.csv
- [ ] CRITICAL tests SF-RS-001, SF-RS-002, SF-RS-006 are all PASS
- [ ] Any FAIL has matching defect entry in defects.csv with severity
- [ ] Git commit: `git add docs/audit/qa/2026-03-31 && git commit -m "QA: Layer 2&3 complete — X PASS, Y FAIL"`

---

## Detailed Guides

For step-by-step instructions per test, see:
- Layer 2 details: `docs/audit/qa/2026-03-31/day-1/layer2_sensor_node_execution.md`
- Layer 3 details: `docs/audit/qa/2026-03-31/day-1/layer3_rs485_execution.md`
- Full flash guide: `docs/audit/qa/2026-03-31/HARDWARE_FLASH_AND_TEST_GUIDE.md`

---

*Ready to flash and test — connect hardware and follow Phase 2.*
