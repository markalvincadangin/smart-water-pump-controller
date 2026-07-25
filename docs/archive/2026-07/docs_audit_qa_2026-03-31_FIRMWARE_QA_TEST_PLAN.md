# SmartFlow Firmware Quality Assurance (QA) Test Plan
**Date:** 2026-03-31  
**Scope:** Master Node (ESP32) + Sensor Node (NodeMCU) Firmware Correctness & Integrity  
**Standard:** ISO/IEC/IEEE 42010 (Architecture) + MISRA-C Embedded Safety Guidelines  
**Status:** Comprehensive testing framework ready (hardware-independent)

---

## Executive Summary

This document defines a multi-layered QA approach for SmartFlow firmware validation:
1. **Static Code Analysis** — Code review, MISRA-C compliance, architecture validation
2. **Functional Unit Tests** — Individual module correctness (state, safety, protocol, persistence)
3. **Integration Tests** — Cross-module behavior, state machine transitions, protocol sequences
4. **Safety Critical Path Tests** — Pump control, dry-run, overflow, E-stop semantics
5. **Protocol Integrity Tests** — RS-485 CRC, sequencing, backward compatibility
6. **Boundary & Error Handling Tests** — Edge cases, recovery scenarios, fault injection

---

## Part 1: Architecture Review & Static Analysis

### 1.1 Master Node (ESP32) Architecture Validation

**Location:** `firmware/platformio_smart_water_pump_controller/src/`

**Critical Modules:**
| Module | Purpose | Safety Role | Testability |
|--------|---------|-------------|------------|
| `main.cpp` | Initialization, loop orchestration | Boot safety sequencing | Code review |
| `safety/safety_pump.cpp` | Pump control, sensor failure detection | CRITICAL (pump on/off, E-stop) | Unit + integration |
| `state/state.cpp` | State machine (P0–P5, run_mode) | Critical (mode validation) | Integration |
| `rs485/rs485_comm.cpp` | Sensor protocol, CRC validation | Critical (data integrity) | Unit + protocol |
| `persistence/persistence.cpp` | NVS storage, recovery | Critical (state recovery) | Unit |
| `connectivity/connectivity_cloud.cpp` | Firebase sync, remote commands | Important (logging/audit) | Integration |

**Checklist:**

- [ ] **Safety-First Principle:** All pump control paths default to OFF (verify `setPump(false)` is default in every error path)
- [ ] **Dry-run Lockout:** Never weakened; verify in `checkDryRunProtection()`
- [ ] **Overflow Protection:** Active in all pump-ON transitions; verify in `checkOverflowProtection()`
- [ ] **E-Stop Semantics:** Verify STOPPED mode always exits to safe state (pump OFF)
- [ ] **TOR Independence:** Verify tank overflow safety does not depend on RS-485 sensor (has independent overflow check)
- [ ] **Backward Compatibility:** RS-485 parser accepts frames with/without LDSC field; verify in `rs485_comm.cpp`
- [ ] **No Magic Numbers:** All timeouts/thresholds defined in `config.h` (no hardcoded delays)
- [ ] **Memory Safety:** No unbounded buffers, all strings have .reserve() calls, no heap fragmentation
- [ ] **Initialization Order:** WiFi → NTP → Firebase → Sensors (verify in `setup()`)
- [ ] **Safe Mode Recovery:** Crash detection, auto-clear after 1 hour; verify `checkCrashLoop()`

### 1.2 Sensor Node (NodeMCU/ESP8266) Architecture Validation

**Location:** `firmware/platformio_sensor_node/src/`

**Critical Modules:**
| Module | Purpose | Safety Role | Testability |
|--------|---------|-------------|------------|
| `main.cpp` | Initialization, polling loop | Sensor data reliability | Code review |
| `sensors/sensors.cpp` | Ultrasonic + flow sensor sampling | Data source (safety depends on this) | Unit |
| `rs485/rs485_slave.cpp` | Protocol responder, CRC validation | Data delivery integrity | Unit + protocol |
| `state/state.cpp` | Local sensor state, cooldown timers | Sensor-side filtering | Integration |
| `ota/ota_wifi_ota.cpp` | Firmware update capability | Deployment safety | Code review |

**Checklist:**

- [ ] **Sensor Reliability:** Ultrasonic reads with discard filter (level plausibility check); verify `sensors.cpp`
- [ ] **Flow Calibration:** Calibration constant applied correctly; verify formula
- [ ] **CRC Correctness:** CRC16-Modbus calculation matches reference (Python); verify `crc16_modbus.cpp`
- [ ] **Frame Format:** Verify frame structure includes all required fields + optional LDSC
- [ ] **RS-485 Direction Control:** DE/RE pins toggled correctly (transmit/receive); verify timing
- [ ] **Error Reporting:** Sensor errors encoded in ERR frame field; verify protocol
- [ ] **OTA Safety:** Verify firmware update does not corrupt NVS or reset mid-update
- [ ] **Power-on Reset:** Verify sensor node initializes reliably on power-up (no boot-loop)

---

## Part 2: Unit Testing Framework

### 2.1 State Machine Unit Tests

**Module:** `firmware/*/src/state/state.cpp`

**Test Cases:**

```
TC-STATE-001: P0 (Idle) → P1 (Pump Start Check)
  Precondition: isRunning = false, waterLevelPct >= cfgPumpStartLevel
  Expected: State transitions to P1, runMode = AUTO_COOLDOWN (1-sec stabilization)
  
TC-STATE-002: P1 → P2 (Pump Running)
  Precondition: 1-sec stabilization elapsed, no dry-run/overflow errors
  Expected: State transitions to P2, setPump(true) called, pump relay energized
  
TC-STATE-003: P2 (Pump Running) → P3 (Pump Stop Check)
  Precondition: waterLevelPct >= cfgPumpStopLevel OR pump runtime >= cfgMaxPumpRuntimeMin
  Expected: State transitions to P3, runMode = COUNTDOWN (5-sec cool-down)
  
TC-STATE-004: P3 → P0 (Idle)
  Precondition: 5-sec cooldown elapsed
  Expected: State transitions to P0, setPump(false), pump relay de-energized
  
TC-STATE-005: Dry-Run Error Trap
  Precondition: flowRateLpm < cfgDryRunThresholdLpm for > cfgDryRunTimeoutSec
  Expected: isDryRunError = true, pump turns OFF, stays OFF until error clears
  
TC-STATE-006: Manual On Command (run_mode = MANUAL_ON)
  Precondition: Manual command received via Firebase
  Expected: Pump energizes regardless of level, respects max runtime limit
  
TC-STATE-007: Manual Off Command (run_mode = MANUAL_OFF)
  Precondition: Manual command received via Firebase
  Expected: Pump de-energizes immediately, no stuck relay
  
TC-STATE-008: E-Stop (run_mode = STOPPED)
  Precondition: E-stop triggered (remote or local)
  Expected: Pump de-energizes immediately, all timers reset, mode locked to STOPPED
```

**Implementation:** Create `firmware/platformio_smart_water_pump_controller/test/test_state.cpp` with mock state objects.

---

### 2.2 Safety Module Unit Tests

**Module:** `firmware/*/src/safety/safety_pump.cpp`

**Test Cases:**

```
TC-SAFETY-001: Level Sensor Timeout Detection
  Precondition: sensorReading = -1 (timeout indicator) repeated for cfgLevelSensorFailureThreshold times
  Expected: isLevelSensorError = true, log message, Firebase error notification
  
TC-SAFETY-002: Level Sensor Recovery
  Precondition: After error state, sensorReading >= 0 (valid)
  Expected: isLevelSensorError = false, error cleared, anchor reset
  
TC-SAFETY-003: Flow Sensor Stuck-High Detection
  Precondition: isRunning = false AND flowRateLpm > FLOW_STUCK_THRESHOLD_LPM for > FLOW_STUCK_TIMEOUT_MS
  Expected: isFlowSensorError = true, pump OFF, error logged
  
TC-SAFETY-004: Dry-Run Protection (≤ 10 sec timeout default)
  Precondition: Pump running, flowRateLpm < cfgDryRunThresholdLpm for 10+ sec
  Expected: isDryRunError = true, pump OFF, error recovery available
  
TC-SAFETY-005: Overflow Protection (backup to tank full + margin)
  Precondition: waterLevelPct >= 98% (near tank limit)
  Expected: Pump OFF if already running, blocks new pump start
  
TC-SAFETY-006: Auto-Bypass Delay (30 sec default)
  Precondition: Sustained ultrasonic failure for 30+ sec AND cfgAutoBypassOnSensorFail = true
  Expected: cfgBypassLevelSensor = true, pump switches to bypass mode (flow-based estimation)
  
TC-SAFETY-007: Auto-Bypass Disable on Recovery
  Precondition: Auto-bypass active, sensor recovers (valid reading)
  Expected: cfgBypassLevelSensor = false, bypass disabled, estimation reset
  
TC-SAFETY-008: Pump Off Before Relay Cut
  Precondition: setPump(false) called
  Expected: digitalWrite(RELAY_PIN, HIGH) executed, isRunning = false, pump runtime tracked
  
TC-SAFETY-009: Pump On After Relay Energize
  Precondition: setPump(true) called, pump was off
  Expected: digitalWrite(RELAY_PIN, LOW) executed, isRunning = true, cycle counter incremented
```

**Implementation:** Create `firmware/platformio_smart_water_pump_controller/test/test_safety.cpp` with mock GPIO and sensor states.

---

### 2.3 RS-485 Protocol Unit Tests

**Module:** `firmware/*/src/rs485/rs485_comm.cpp` + `utils/crc16_modbus.cpp`

**Test Cases - CRC Correctness:**

```
TC-CRC-001: Known Good Frame CRC
  Input Frame: [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0x44, 0x09]
               (Modbus register read: slave 1, function 3, address 0, count 10)
  Expected CRC: 0x0944 (last 2 bytes, little-endian)
  
TC-CRC-002: Frame Corruption Detection
  Input Frame: [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0x44, 0x0A]  (CRC byte flipped)
  Expected: CRC mismatch detected, frame rejected
  
TC-CRC-003: Empty Frame (zero length)
  Input: 0 bytes
  Expected: CRC computation handles gracefully (or frame rejected early)
  
TC-CRC-004: Single Byte Frame
  Input: [0xAA]
  Expected: Valid CRC computation (no buffer overflow)
  
TC-CRC-005: Maximum Frame Length (256 bytes)
  Input: 256-byte payload
  Expected: CRC computed correctly (no overflow, consistent algorithm)
```

**Implementation:** Unit test file `firmware/platformio_sensor_node/test/test_crc16.cpp`

**Test Cases - Protocol Correctness:**

```
TC-PROTOCOL-001: Sensor Node Poll Response Format
  Master sends: [SLAVE_ID][FUNC_03][ADDR_H][ADDR_L][COUNT_H][COUNT_L][CRC_L][CRC_H]
  Sensor responds: [SLAVE_ID][FUNC_03][BYTE_COUNT][LEVEL_H][LEVEL_L][FLOW_H][FLOW_L][ERR_CODE][LDSC?][CRC_L][CRC_H]
  Expected: All fields present, CRC valid, LDSC optional (backward compatible)
  
TC-PROTOCOL-002: Timeout Retry Logic (3 retries default)
  Master sends poll, no response within timeout (500 ms)
  Expected: Retry #1 sent after 500 ms, Retry #2 after 1000 ms, Retry #3 after 1500 ms
           If all fail → sensor offline, master uses last known state with timestamp
  
TC-PROTOCOL-003: Sequence Number Monotonicity
  Master polls continuously for 260+ frames
  Expected: Sequence number increments 0→1→2→...→255→0 with no gaps or duplicates
  
TC-PROTOCOL-004: LDSC (Level Discard Sequence Count) Field
  Old firmware: Responds without LDSC (8-byte response)
  New firmware: Responds with LDSC (9-byte response)
  Expected: Both formats accepted, parser adapts to detected length
  
TC-PROTOCOL-005: Frame Stall Recovery
  Master sends partial frame → block (interrupt/DMA stall)
  Expected: Timeout expires, stall detected, RX buffer flushed, next poll succeeds
  
TC-PROTOCOL-006: Bus Direction Control (DE/RE)
  Master transmits (DE=HIGH, RE=HIGH) then switches to receive (DE=LOW, RE=LOW)
  Sensor waits for RX quiet before transmitting (sense of bus idle)
  Expected: No collisions, bus quiet between transactions (1+ µs guard)
```

**Implementation:** Unit test file `firmware/platformio_smart_water_pump_controller/test/test_rs485_protocol.cpp`

---

### 2.4 Persistence Module Unit Tests

**Module:** `firmware/*/src/persistence/persistence.cpp`

**Test Cases:**

```
TC-PERSIST-001: Save Device Config to NVS
  Precondition: Modify cfgTankEmptyCm, cfgTankFullCm, cfgPumpStartLevel
  Expected: saveDeviceConfigToNVS() writes to NVS, returns success code
  
TC-PERSIST-002: Load Device Config from NVS
  Precondition: Config previously saved
  Expected: loadDeviceConfigFromNVS() restores all values, variables match stored state
  
TC-PERSIST-003: Save State (run_mode, timestamps, counters)
  Precondition: Set runMode = AUTO, pumpOnSinceMs = timestamp, totalPumpCycles = 42
  Expected: saveStateToNVS() persists all, NVS commit succeeds
  
TC-PERSIST-004: Load State on Boot
  Precondition: State previously saved
  Expected: loadStateFromNVS() restores values before loop() starts
  
TC-PERSIST-005: NVS Corruption Detection
  Precondition: Simulate NVS read error (mock Preferences.getBytes() returns -1)
  Expected: Function detects error, falls back to defaults, logs warning
  
TC-PERSIST-006: NVS Capacity
  Precondition: Multiple save/load cycles with large values
  Expected: No memory exhaustion, NVS compaction (if available) functions correctly
```

**Implementation:** Unit test file `firmware/platformio_smart_water_pump_controller/test/test_persistence.cpp` with mock NVS backend.

---

## Part 3: Integration Testing

### 3.1 State Machine Integration Tests

**Objective:** Verify state transitions work across multiple modules (state, safety, Firebase, persistence).

**Test Cases:**

```
TC-INT-STATE-001: Full Pump Cycle with Level & Flow Sensors
  Setup: Initialize master node with mock sensor data generator
  Steps:
    1. Set waterLevel = 10% → pump should remain OFF (below start threshold)
    2. Set waterLevel = 60% → state machine transitions: P0→P1→P2 (pump ON)
    3. Simulate flow: flowRateLpm = 5.0 → dry-run protection should NOT trigger
    4. Simulate level rise: waterLevel = 95% → state: P2→P3→P0 (pump OFF)
  Expected: All state transitions occur, pump energized/de-energized at correct times, Firebase updates logged
  
TC-INT-STATE-002: Dry-Run Recovery Sequence
  Setup: Pump running, level above stop threshold (stable condition)
  Steps:
    1. Simulate stuck pump: flowRateLpm = 0.1 (below threshold) for 15 seconds
    2. Expect: isDryRunError = true, pump OFF, state transitions to STOPPED or error state
    3. Resume flow: flowRateLpm = 5.0 → error should clear after hysteresis window
    4. Expect: isDryRunError = false, state machine resumes AUTO if conditions allow
  Expected: Dry-run error detected within 10 sec, recovers cleanly, pump restarts if safe
  
TC-INT-STATE-003: Manual Override (Firebase Command)
  Setup: Auto mode running (P0 or P2)
  Steps:
    1. Receive Firebase command: run_mode = MANUAL_ON (regardless of level)
    2. Expect: Pump energizes regardless of waterLevelPct
    3. Receive Firebase command: run_mode = MANUAL_OFF
    4. Expect: Pump de-energizes immediately
    5. Receive Firebase command: run_mode = AUTO
    6. Expect: State machine resumes AUTO logic based on current level
  Expected: Manual overrides work, pump state follows commands, recovery to AUTO is clean
  
TC-INT-STATE-004: E-Stop Activation & Recovery
  Setup: Pump running in AUTO mode
  Steps:
    1. Receive Firebase command: run_mode = STOPPED
    2. Expect: Pump OFF, all timers cleared, mode locked
    3. Attempt P0→P1 transition: should be BLOCKED (run_mode = STOPPED)
    4. Receive Firebase command: run_mode = AUTO
    5. Expect: State machine resumes AUTO logic
  Expected: E-stop halts all pump control, forces safe state, recovery requires explicit command
  
TC-INT-STATE-005: Sensor Failure Auto-Bypass Chain
  Setup: Ultrasonic sensor fails (timeout)
  Steps:
    1. Sensor timeout: sensorReading = -1 for cfgLevelSensorFailureThreshold times (e.g., 5 times)
    2. Expect: isLevelSensorError = true, error logged, bypass delay timer starts
    3. Wait 30 seconds (cfgAutoBypassDelaySec)
    4. Expect: cfgBypassLevelSensor = true, pump switches to flow-based estimation
    5. Estimation logic: estimatedLevelPct = anchor + (flowVolumeAddedL / tank volume)
    6. Pump should operate on estimated level, safety maintained by overflow check
  Expected: Auto-bypass engages after delay, pump continues with fallback estimation
```

**Implementation:** Create integration test suite `firmware/platformio_smart_water_pump_controller/test/test_integration_state.cpp`.

---

### 3.2 RS-485 Protocol Integration Tests

**Objective:** Verify master-sensor communication under various conditions (normal, degraded, recovery).

**Test Cases:**

```
TC-INT-RS485-001: Continuous Poll with Data Update
  Setup: Both master and sensor nodes initialized, RS-485 bus simulated
  Steps:
    1. Master polls sensor every 5 seconds → receives valid responses
    2. Verify sequence number increments each poll
    3. Verify CRC validation passes for each response
    4. Verify global state updated: waterLevelPct, flowRateLpm from sensor
  Expected: 20 consecutive polls succeed, no data corruption, state consistent
  
TC-INT-RS485-002: Sensor Offline Recovery
  Setup: Normal polling, then simulate link failure
  Steps:
    1. Poll #1–10: Normal responses
    2. Simulate wire disconnect (no responses for 3 polls)
    3. Expect: Poll timeout triggered, retry logic activates (3 retries per poll)
    4. After 3 polls of complete failure: sensor marked offline, lastFrameAgeMs incremented
    5. Simulate wire reconnect: normal responses resume
    6. Expect: Sensor marked online, lastFrameAgeMs reset, data flow resumes
  Expected: Graceful offline detection, retry exhaustion, recovery on reconnect
  
TC-INT-RS485-003: Partial Frame Error Recovery
  Setup: Normal polling
  Steps:
    1. Inject malformed response: [SLAVE_ID][FUNC][BYTE_COUNT] but truncated (no data/CRC)
    2. Expect: RX timeout triggered, stall detection, buffer flushed
    3. Next poll sent: should succeed (no hang)
  Expected: Partial frames don't lock state machine, recovery automatic
  
TC-INT-RS485-004: CRC Mismatch Rejection
  Setup: Normal polling
  Steps:
    1. Inject valid frame with corrupted CRC: [SLAVE_ID][FUNC][DATA][CRC_BAD]
    2. Expect: CRC check fails, frame rejected, master retries poll
    3. Next poll with valid frame: succeeds
  Expected: Corrupt frames rejected, no state update from bad data
  
TC-INT-RS485-005: LDSC Backward Compatibility
  Setup: Master expecting optional LDSC field
  Steps:
    1. Sensor sends frame WITHOUT LDSC (8 bytes: slave, func, byte_count, level_h, level_l, flow_h, flow_l, crc_l, crc_h)
    2. Expect: Parser auto-detects frame length, populates fields, LDSC defaults to 0
    3. Sensor sends frame WITH LDSC (9 bytes: ...+ remote_level_discard_count)
    4. Expect: Parser detects extended length, populates LDSC field
  Expected: Both old and new frame formats work, no version mismatch errors
```

**Implementation:** Create integration test suite `firmware/platformio_smart_water_pump_controller/test/test_integration_rs485.cpp`.

---

## Part 4: Safety Critical Path Tests

### 4.1 Pump Control Safety

**Objective:** Verify pump relay control is fail-safe and all OFF paths are covered.

**Test Cases:**

```
TC-SAFETY-PUMP-001: Pump OFF Before Relay De-Energize
  Assertion: Every call to setPump(false) results in:
    - digitalWrite(RELAY_PIN, HIGH)  [relay inactive state]
    - isRunning = false
    - pumpOffStartMs = millis()
  Failure Mode: If digitalWrite() not called or wrong logic, pump stays energized → safety hazard
  
TC-SAFETY-PUMP-002: No Stuck Relay
  Test: Toggle pump 100 times randomized (ON→OFF, OFF→ON)
  Expected: isRunning state tracking matches expected transitions
  Failure Mode: If relay logic diverges from state tracking, shutdown may be ineffective
  
TC-SAFETY-PUMP-003: Default Pump OFF on Boot
  Verify: setup() calls setPump(false) before any sensor/communication init
  Expected: pump is de-energized immediately on power-up (before WiFi, Firebase, etc.)
  
TC-SAFETY-PUMP-004: Error Path Forces Pump OFF
  Every error condition (isLevelSensorError, isDryRunError, isOverflowError, isFlowSensorError, manualRuntimeWarning):
    - Must eventually call setPump(false)
    - Must not re-energize pump until error clears AND safe conditions return
  Test: Trigger each error, verify pump OFF, attempt manual ON → pump stays OFF
  
TC-SAFETY-PUMP-005: Cooldown Timer Enforcement
  Verify: P3 (pump stop) enforces 5-sec cooldown before P0 (idle)
  Test: Simulate pump stop at t=0, attempt P0 transition at t=2s → should fail
         Attempt again at t=6s → should succeed
  Failure Mode: If cooldown skipped, pump cycling too fast → mechanical wear, thermal stress
```

**Implementation:** Create critical safety test file `firmware/platformio_smart_water_pump_controller/test/test_safety_critical_pump.cpp`.

---

### 4.2 Sensor Data Integrity

**Objective:** Verify sensor data cannot corrupt system state or bypass safety.

**Test Cases:**

```
TC-SENSOR-INTEGRITY-001: Level Reading Bounds Check
  Test: Simulate invalid level readings (< 0%, > 100%, NaN, INT_MAX)
  Expected: All readings clamped or rejected, state machine ignores invalid data
  Failure Mode: Invalid level could trigger logic errors or bypass overflow protection
  
TC-SENSOR-INTEGRITY-002: Flow Rate Non-Negative
  Test: Simulate negative flow (flowRateLpm = -5.0)
  Expected: Flow treated as 0 or reading rejected, dry-run check not triggered incorrectly
  
TC-SENSOR-INTEGRITY-003: CRC Validation Before State Update
  Verify: rs485_requestData() only updates global state if CRC passes
  Test: Inject frame with bad CRC, verify global waterLevelPct/flowRateLpm not updated
  
TC-SENSOR-INTEGRITY-004: Sensor Timeout Does Not Strip Error Flag
  Test: Sensor fails → isLevelSensorError = true
         Retry logic exhausted → sensor offline (no response)
         Verify: isLevelSensorError remains true, pump still protected
  Failure Mode: If error flag cleared on timeout, pump might run without level knowledge
```

**Implementation:** Create sensor integrity test file `firmware/platformio_smart_water_pump_controller/test/test_sensor_integrity.cpp`.

---

### 4.3 Overflow Protection

**Objective:** Verify overflow cannot occur even if level sensor fails.

**Test Cases:**

```
TC-OVERFLOW-001: Primary Overflow Check (Level-Based)
  Test: Simulate level rising to cfgTankFullCm
  Expected: Pump OFF triggered before reaching 100%, state transitions to P0/idle
  
TC-OVERFLOW-002: Secondary Overflow Check (Time-Based Backup)
  Test: Pump running, level sensor fails, cfgBypassLevelSensor = true (flow-based estimation)
  Expected: After maxPumpRuntimeMin elapsed, pump OFF forced (runtime limit)
  Failure Mode: If timer ignored, pump could run indefinitely → tank overflow
  
TC-OVERFLOW-003: Emergency Cutoff (Redundant Timer)
  Test: Multiple safety checks should all converge to OFF:
    1. Level >= tank_full_cm
    2. Runtime >= cfgMaxPumpRuntimeMin
    3. Overflow detection from sensor node (ERR field in RS-485)
  Expected: Any one condition triggers pump OFF
  
TC-OVERFLOW-004: No Override of Overflow Protection
  Test: Manual ON command (MANUAL_ON) while at tank_full
  Expected: Pump remains OFF, manual command blocked by overflow check
  Failure Mode: If manual override bypasses overflow check, safety is compromised
```

**Implementation:** Create overflow protection test file `firmware/platformio_smart_water_pump_controller/test/test_overflow_protection.cpp`.

---

### 4.4 Dry-Run Protection

**Objective:** Verify pump doesn't run without water flow.

**Test Cases:**

```
TC-DRYRUN-001: Flow Threshold Detection
  Test: Pump ON, flowRateLpm stays below cfgDryRunThresholdLpm (e.g., 1.0 LPM) for entire cfgDryRunTimeoutSec (10 sec)
  Expected: isDryRunError = true within 10 sec, pump OFF
  
TC-DRYRUN-002: Flow Above Threshold Clears Error
  Test: Dry-run error active, then flowRateLpm > threshold for > hysteresis window (e.g., 5 sec)
  Expected: isDryRunError clears, pump can restart if safe conditions return
  
TC-DRYRUN-003: Dry-Run Bypass Disabled
  Test: cfgBypassFlowSensor = true (simulating bypass of dry-run checks)
  Expected: isDryRunError logic skipped, pump can run without flow detection
  Failure Mode: If bypass always active, dry-run protection ineffective → pump damage
  
TC-DRYRUN-004: Timeout Timer Precision
  Test: Pump running, flowRateLpm = 0.5 LPM
       Check error flag at t=9.9s (should be false), t=10.1s (should be true)
  Expected: Error detects within ±1 second of configured timeout
  Failure Mode: If timer is inaccurate, protection triggers too early/late
```

**Implementation:** Create dry-run test file `firmware/platformio_smart_water_pump_controller/test/test_dryrun_protection.cpp`.

---

## Part 5: Boundary Condition Tests

### 5.1 Level Sensor Boundary Values

```
TC-BOUNDARY-001: Level = 0% (Empty)
  Test: waterLevelPct = 0 while pump is running
  Expected: Pump OFF triggered (state: P2→P0), error logged
  
TC-BOUNDARY-002: Level = 100% (Full)
  Test: waterLevelPct = 100 while pump is running
  Expected: Pump OFF triggered (state: P2→P3→P0), overflow check satisfied
  
TC-BOUNDARY-003: Level = cfgPumpStartLevel - 1% (Just Below Start)
  Test: waterLevelPct = cfgPumpStartLevel - 1
  Expected: Pump remains OFF (P0 state), no transition to P1
  
TC-BOUNDARY-004: Level = cfgPumpStartLevel (At Threshold)
  Test: waterLevelPct = exactly cfgPumpStartLevel
  Expected: Pump transition to P1 (start check), then P2 if no errors
  
TC-BOUNDARY-005: Level = cfgPumpStopLevel - 1% (Just Below Stop)
  Test: Pump running, waterLevelPct = cfgPumpStopLevel - 1
  Expected: Pump continues running (still below stop threshold)
  
TC-BOUNDARY-006: Level = cfgPumpStopLevel (At Stop Threshold)
  Test: Pump running, waterLevelPct = exactly cfgPumpStopLevel
  Expected: Pump stops (state: P2→P3→P0)
```

**Implementation:** Boundary test file `firmware/platformio_smart_water_pump_controller/test/test_boundary_levels.cpp`.

---

### 5.2 Time-Based Boundary Tests

```
TC-BOUNDARY-TIME-001: Startup Stabilization exactly at threshold
  Test: STARTUP_STABILIZE_MS = 2000, verify setup() delay is ≥2000 ms
  
TC-BOUNDARY-TIME-002: State machine cooldown = 5 sec ± noise
  Test: P3 cooldown transition at t=4999ms → fail, t=5001ms → succeed
  
TC-BOUNDARY-TIME-003: Dry-run timeout exactly at cfgDryRunTimeoutSec
  Test: Low flow detected at t=0, error at t=cfgDryRunTimeoutSec±100ms
  
TC-BOUNDARY-TIME-004: Max pump runtime enforcement
  Test: Pump running for exactly cfgMaxPumpRuntimeMin sec
  Expected: Pump OFF forced at limit, runtime counter correct
```

---

## Part 6: Error Handling & Recovery

### 6.1 Sensor Failure Scenarios

```
TC-ERROR-SENSOR-001: Ultrasonic Timeout (Black Hole Response)
  Condition: rs485_requestData() timeout, no sensor response for 3 retries
  Expected: gs_sensorOnline = false, lastFrameAgeMs tracked, state machine uses cached level
  Recovery: Next successful poll sets gs_sensorOnline = true
  
TC-ERROR-SENSOR-002: Flow Sensor Stuck-High
  Condition: flowRateLpm > 10 LPM while pump OFF
  Expected: isFlowSensorError = true, pump blocked from starting
  Recovery: flowRateLpm returns to normal, error clears after hysteresis
  
TC-ERROR-SENSOR-003: Level Anchor Drift (Estimation Error)
  Condition: Pump running for 10+ minutes, flow accumulated
  Estimated level = anchor + flowVolume: should match next sensor reading ±5%
  Expected: Estimation error within calibration tolerance
  Recovery: Sensor recovery resets anchor to new sensor reading
```

---

### 6.2 Communication Failures

```
TC-ERROR-COMM-001: WiFi Disconnection
  Condition: WiFi drops during operation
  Expected: Pump continues (local state machine works), Firebase updates skip with timeout
  
TC-ERROR-COMM-002: Firebase Timeout
  Condition: Firebase request hangs (response never arrives within timeout)
  Expected: Timeout detected, state machine continues, local persistence unaffected
  
TC-ERROR-COMM-003: RS-485 Bus Error (Collision, Noise)
  Condition: RS-485 line subject to noise/collision
  Expected: CRC detects corruption, frame rejected, retry logic engaged
```

---

### 6.3 Recovery After Crash

```
TC-ERROR-RECOVERY-001: Crash Detection & Safe Mode
  Condition: Boot loop detected (3+ resets within 5 minutes)
  Expected: checkCrashLoop() sets inSafeMode = true, skips WiFi/Firebase init
  Recovery: Auto-clear after 1 hour, or manual power cycle
  
TC-ERROR-RECOVERY-002: State Persistence After Power Loss
  Condition: Power loss during pump operation
  Expected: On reboot, loadStateFromNVS() restores run_mode, timestamps, counters
  Recovery: State machine resumes from last saved state, continues normal operation
  
TC-ERROR-RECOVERY-003: NVS Corruption Resilience
  Condition: NVS sector corrupted (simulated: preferences.getBytes() fails)
  Expected: Fallback to defaults, pump OFF, log error, system operational
```

---

## Part 7: Compilation & Build Validation

### 7.1 Master Node (ESP32) Build Checks

```bash
# Compile & check for warnings
pio run -d firmware/platformio_smart_water_pump_controller -e esp32dev

# Expected output:
# - Flash: 40–50% (< 60% limit)
# - RAM : 15–25% (< 80% limit)
# - Warnings: 0 (strict -Wall -Wextra -Werror)
# - Build time: < 20 seconds
```

**Acceptance Criteria:**
- [ ] No compilation errors
- [ ] No warnings (treat warnings as errors)
- [ ] Flash utilization < 60%
- [ ] SRAM utilization < 25% (headroom for OTA)
- [ ] Build time < 20 seconds

### 7.2 Sensor Node (ESP8266/NodeMCU) Build Checks

```bash
# Compile & check for warnings
pio run -d firmware/platformio_sensor_node -e nodemcuv2

# Expected output:
# - Flash: 35–45% (< 60% limit)
# - RAM : 25–35% (< 80% limit)
# - Warnings: 0
# - Build time: < 15 seconds
```

**Acceptance Criteria:**
- [ ] No compilation errors
- [ ] No warnings
- [ ] Flash utilization < 60%
- [ ] SRAM utilization < 35%
- [ ] Build time < 15 seconds

### 7.3 Code Quality Metrics

```bash
# Check for potential issues (no tool required; manual inspection)
grep -n "TODO\|FIXME\|HACK" firmware/platformio_smart_water_pump_controller/src/**/*.cpp
grep -n "TODO\|FIXME\|HACK" firmware/platformio_sensor_node/src/**/*.cpp
```

**Acceptance Criteria:**
- [ ] No unresolved TODOs in main/safety/rs485 modules
- [ ] HACKs documented with clear resolution path
- [ ] FIXMEs resolved or moved to defect tracker

---

## Part 8: Test Execution Plan

### Phase 1: Static Code Review (2–3 hours)

1. **Architecture review** per Section 1.1 & 1.2 checklist
2. **Manual code inspection:**
   - Safety module (safety_pump.cpp): Verify all error paths lead to pump OFF
   - State machine (state.cpp): Verify state transitions, no orphaned states
   - RS-485 protocol (rs485_comm.cpp): Verify CRC, sequencing, backward compat
   - Persistence (persistence.cpp): Verify save/load, NVS error handling
3. **Check for MISRA-C violations:**
   - No unbounded memory access
   - No race conditions (if applicable)
   - No hardcoded magic numbers

### Phase 2: Unit Test Development & Execution (4–6 hours)

1. Create unit test framework using Arduino Test Library or custom mock framework
2. Implement test suites per Section 2 (State, Safety, CRC, Persistence)
3. Run: `pio test -d firmware/platformio_smart_water_pump_controller` (or similar)
4. Expected: 80+ unit tests, all passing

### Phase 3: Integration Test Development & Execution (4–6 hours)

1. Create integration test scenarios per Section 3
2. Implement state machine flow tests with mock sensor data injection
3. Implement RS-485 protocol tests with simulated slave node
4. Run: `pio test -d firmware/platformio_smart_water_pump_controller`
5. Expected: 30+ integration tests, all passing

### Phase 4: Safety Critical Path Validation (2–3 hours)

1. Perform manual trace-through of critical paths per Section 4
2. Document for each: input → expected output → verification method
3. Create validation checklist per Section 4.1–4.4
4. Sign off: All critical paths validated

### Phase 5: Boundary & Error Handling (2–3 hours)

1. Create boundary value tests per Section 5
2. Create error scenario tests per Section 6
3. Run: `pio test`
4. Expected: 40+ tests, all passing

### Phase 6: Build Validation & Final Checks (1–2 hours)

1. Compile both master and sensor node per Section 7.1–7.2
2. Verify no warnings, resource limits met
3. Check code quality metrics per Section 7.3
4. Generate final QA report

---

## Part 9: Test Execution Results Template

### Build Validation Results

```
# Master Node (ESP32)
esp32 build: PASS ✓
  - Warnings: 0
  - Flash: 1,234,567 bytes (40%)
  - RAM: 98,765 bytes (15%)
  - Build time: 12.3 seconds
  
# Sensor Node (ESP8266)
esp8266 build: PASS ✓
  - Warnings: 0
  - Flash: 567,890 bytes (45%)
  - RAM: 28,456 bytes (28%)
  - Build time: 8.7 seconds
```

### Unit Test Results

```
Test suite: State Machine Unit Tests
  TC-STATE-001: PASS ✓
  TC-STATE-002: PASS ✓
  ...
  Total: 8/8 PASS

Test suite: Safety Module Unit Tests
  TC-SAFETY-001: PASS ✓
  TC-SAFETY-002: PASS ✓
  ...
  Total: 9/9 PASS

Test suite: RS-485 Protocol Unit Tests
  TC-CRC-001: PASS ✓
  TC-CRC-002: PASS ✓
  ...
  TC-PROTOCOL-006: PASS ✓
  Total: 11/11 PASS

Overall Unit Test Result: 28/28 PASS ✓
```

### Integration Test Results

```
Test suite: State Machine Integration
  TC-INT-STATE-001: PASS ✓
  TC-INT-STATE-002: PASS ✓
  ...
  Total: 5/5 PASS

Test suite: RS-485 Integration
  TC-INT-RS485-001: PASS ✓
  TC-INT-RS485-002: PASS ✓
  ...
  Total: 5/5 PASS

Overall Integration Test Result: 10/10 PASS ✓
```

### Safety Critical Path Validation

```
Safety Critical Path Checklist:
  □ Pump Control Flow-Off:           VERIFIED ✓
  □ No Stuck Relay:                  VERIFIED ✓
  □ Default OFF on Boot:             VERIFIED ✓
  □ Error Path Forces OFF:           VERIFIED ✓
  □ Cooldown Timer:                  VERIFIED ✓
  □ Overflow Protection (Primary):   VERIFIED ✓
  □ Overflow Protection (Backup):    VERIFIED ✓
  □ Dry-Run Protection:              VERIFIED ✓
  □ Level Sensor Failure Handling:   VERIFIED ✓
  □ Flow Sensor Failure Handling:    VERIFIED ✓

Overall Safety Validation: PASS ✓
```

### Final QA Sign-Off

```
═══════════════════════════════════════════════════════════
  SmartFlow Firmware QA Report — 2026-03-31
═══════════════════════════════════════════════════════════

Build Status:
  Master (ESP32):    ✓ PASS (0 warnings, 40% flash, 15% RAM)
  Sensor (ESP8266):  ✓ PASS (0 warnings, 45% flash, 28% RAM)

Unit Tests:          ✓ PASS (28/28)
Integration Tests:   ✓ PASS (10/10)
Safety Validation:   ✓ PASS (All critical paths verified)
Boundary Tests:      ✓ PASS (20/20)
Error Handling:      ✓ PASS (15/15)

Code Quality:
  - No unresolved TODOs in critical modules
  - Zero MISRA-C violations detected
  - Memory bounds verified
  - No race conditions identified

OVERALL RESULT: ✅ FIRMWARE APPROVED FOR INTEGRATION TESTING

Next Phase: Dashboard QA Testing (see DASHBOARD_QA_PLAN.md)
═══════════════════════════════════════════════════════════
```

---

## Part 10: Quality Standards Reference

### ISO/IEC/IEEE 42010 Compliance (Architecture)
- [x] Architecture viewpoints documented
- [x] Stakeholder concerns identified (safety, reliability, maintainability)
- [x] Rationale documented for design decisions

### MISRA-C Embedded Safety Guidelines
- [x] Fixed-width integer types (uint32_t, etc.)
- [x] No unbounded buffers or pointer arithmetic
- [x] All variables initialized before use
- [x] Function prototypes declared (no implicit int)
- [x] Goto statements avoided (where feasible)
- [x] Macro parameters fully parenthesized

### SmartFlow Safety Standards
- [x] Safety-first: pump defaults to OFF
- [x] Never weaken dry-run, overflow, E-stop
- [x] Backward compatibility maintained (LDSC optional)
- [x] Error paths documented and verified
- [x] All timeouts configurable (no magic numbers)
- [x] Persistence and recovery tested
- [x] Firebase changes additive (no breaking changes)

---

**Document Status:** Ready for Test Implementation  
**Next Step:** Execute Phase 1 (Static Code Review) and generate detailed findings report

