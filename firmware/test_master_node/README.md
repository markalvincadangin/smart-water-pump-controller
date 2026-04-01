# SmartFlow ESP32 Master Node Test Guide

**Purpose:** Standalone hardware validation for ESP32 pump controller before integration testing.

**Tests Included:**
| Test ID | Name | Duration | Hardware Needed | Pass Criteria |
|---------|------|----------|-----------------|---------------|
| TC-M-00 | RS-485 Hello Handshake | ~3s | NodeMCU responder + RS-485 link | Receives `MSG:HELLO_FROM_NODE;` with valid CRC |
| TC-M-01 | GPIO & Relay | 1s + wait | Relay connected to GPIO4 | Relay cycles on/off, user confirms |
| TC-M-02 | RS-485 Master | 30s | NodeMCU running RS-485 slave | ≥90% valid frames in 30s window |
| TC-M-03 | WiFi Connection | ~20s | WiFi network + credentials | Connects within timeout, DNS checks pass (2/3) |
| TC-M-04 | Firebase I/O | ~10s | WiFi + Firebase creds + library | write/read/delete round-trip succeeds |
| TC-M-05 | Integration | ~3s | RS-485 link active | 10-frame parse/integrity sanity succeeds |

**Total Runtime:** ~35–40 seconds (mostly TC-M-02 polling)

---

## Prerequisites

### 1. Hardware Setup

**GPIO Mapping (must match production code):**
```
ESP32 GPIO4    → Relay control (active HIGH)
ESP32 GPIO17   → RS-485 TX
ESP32 GPIO25   → RS-485 RX
ESP32 GPIO5    → RS-485 DE/RE (transmit enable, active HIGH)
```

**Connections:**
- Relay: Connect GPIO4 to relay module (or leave disconnected for safety)
- RS-485: Connect TX/RX/DE/RE to MAX485 transceiver
- USB: Serial connection for monitoring

### 2. Software Setup

**Arduino IDE Steps:**
1. Install ESP32 Board Support:
   - **Boards Manager URL:** `https://dl.espressif.com/dl/package_esp32_index.json`
   - **Board:** ESP32 Dev Module (or your specific board)
   - **CPU Frequency:** 80 MHz (or higher)
   - **Flash:** 4MB

2. Install FirebaseESP32 library (optional, for Phase 6):
   ```
   Sketch → Include Library → Manage Libraries
   Search: "Firebase ESP Client" by Mobizt
   Version: ≥4.0.0
   ```

3. Create `secrets.h` (for WiFi + Firebase):
   ```cpp
   // secrets.h (do NOT commit to repo)
   #define WIFI_SSID "Your_WiFi_SSID"
   #define WIFI_PASSWORD "Your_WiFi_Password"
   #define FIREBASE_PROJECT_ID "your-project-id"
   #define FIREBASE_API_KEY "your-api-key"
   #define FIREBASE_RTDB_URL "https://your-project.firebaseio.com"
   ```

### 3. NodeMCU Sensor Node

For TC-M-02 (RS-485 polling), the NodeMCU must be running `test_sensor_node.ino`.

One-laptop flash sequence (recommended):
1. Flash NodeMCU first with `firmware/test_sensor_node/test_sensor_node.ino`.
2. Keep NodeMCU powered and connected on RS-485 bus.
3. Move USB cable to ESP32 and flash `firmware/test_master_node/test_master_node.ino`.
4. Open ESP32 serial monitor; the master test will run TC-M-00 hello first, then the full RS-485 poll.

**Connection:**
```
ESP32 MAX485 A      → NodeMCU MAX485 A
ESP32 MAX485 B      → NodeMCU MAX485 B
GND                 → GND (common)
```

---

## Detailed Test Procedures

### TC-M-00: RS-485 Hello Handshake

**Purpose:** Quick sanity test to confirm Node responds to master request before full polling.

**Procedure:**
1. NodeMCU runs sensor test sketch and is wired on RS-485.
2. ESP32 sends `PING` over RS-485.
3. Node replies with framed payload: `MSG:HELLO_FROM_NODE;` plus CRC.

**Pass Criteria:**
- Master receives framed reply.
- CRC is valid.
- Payload matches exactly `MSG:HELLO_FROM_NODE;`.

---

### TC-M-01: GPIO and Relay

**Purpose:** Verify GPIO control and relay switching without production dependencies.

**Hardware:** Relay module or indicator LED connected to GPIO4.

**Procedure:**
1. Open Serial Monitor (115200 baud)
2. When prompted: `[SAFETY] Relay will activate for 500ms. ...`
   - Review the safety warning
   - Ensure pump is OFF or relay is isolated
   - **Press ENTER** to proceed
3. Watch for:
   - Relay clicks/LED turns on (GPIO4 HIGH)
   - Relay clicks/LED turns off (GPIO4 LOW)
   - Serial output: `[INFO] Relay cycle complete.`

**Pass Criteria:**
- Relay activates for ~500ms
- Relay deactivates cleanly
- No GPIO errors reported

**Troubleshooting:**
| Issue | Root Cause | Fix |
|-------|-----------|-----|
| Relay won't activate | GPIO4 short/open | Verify wiring; re-flash |
| Relay stuck ON | GPIO wired backwards | Check polarity |
| No user prompt | Serial not responding | Restart device; check baud |

---

### TC-M-02: RS-485 Master Poll

**Purpose:** Verify ESP32 can act as RS-485 master and receive valid frames from NodeMCU.

**Hardware:**
- ESP32 connected to MAX485 (TX=GPIO17, RX=GPIO25, DE/RE=GPIO5)
- NodeMCU running `test_sensor_node.ino` (TC-S-04 echo mode)
- RS-485 A/B/GND connected between master and slave

**Procedure:**
1. Flash `test_sensor_node.ino` to NodeMCU
2. Ensure NodeMCU boots and shows `[INFO] RS-485 Echo Server ready`
3. Flash `test_master_node.ino` to ESP32
4. Open Serial Monitor (115200 baud) on ESP32
5. Watch as ESP32 polls NodeMCU every 1 second for 30 seconds
6. Output example:
   ```
   [INFO] RS485 Master: Polling for 30 seconds...
   [INFO] RS485: 28 valid, 2 failed (93.3% success rate)
   [ PASS] TC-M-02: RS-485 Master Poll
   ```

**Pass Criteria:**
- ≥90% of frames are valid (28+ out of 30)
- CRC validation passes
- No timeouts after frame ≥25 received

**Troubleshooting:**
| Issue | Root Cause | Fix |
|-------|-----------|-----|
| 0% frames received | RS-485 wiring open | Check A/B/GND connections |
| CRC mismatches | Baud rate mismatch | Verify 115200 on both sides |
| Timeouts (50% failure) | NodeMCU not responding | Restart NodeMCU; check echo test (TC-S-04) |
| Success rate 50–90% | Weak signal / noise | Shorten RS-485 cable; add termination resistors |

---

### TC-M-03: WiFi Connection

**Purpose:** Verify WiFi subsystem is functional and can detect networks.

**Hardware:** WiFi antenna connected to ESP32.

**Procedure:**
1. Open Serial Monitor (115200 baud)
2. Watch output:
   ```
   [INFO] WiFi: Attempting connection...
   [INFO] WiFi subsystem started.
   [DEBUG] WiFi MAC: XX:XX:XX:XX:XX:XX
   [ PASS] TC-M-03: WiFi Connection
   ```

**Pass Criteria:**
- WiFi connects within 20 seconds
- DNS reachability checks succeed at least 2/3 attempts
- No crashes

**Notes:**
- Requires `secrets.h` with `WIFI_SSID` and `WIFI_PASSWORD`
- If credentials are missing, test FAILS by default
- To allow bench-only defer mode, set `ALLOW_DEFERRED_CLOUD_TESTS=1` in sketch

**Troubleshooting:**
| Issue | Root Cause | Fix |
|-------|-----------|-----|
| WiFi subsystem hangs | Antenna disconnected | Reconnect antenna; re-flash |
| MAC address invalid | Corrupted NVRAM | Erase all flash; re-flash |

---

### TC-M-04: Firebase Read/Write

**Purpose:** Verify Firebase connectivity and RTDB JSON serialization.

**Hardware:** WiFi connection to network and internet access (Phase 6+).

**Procedure:**
1. Create `secrets.h` with Firebase credentials
2. Flash to ESP32
3. Open Serial Monitor (115200 baud)
4. Watch output:
   ```
   [INFO] Firebase: Connecting...
   [INFO] Firebase: Write /pump_system/test/ping_at = 1234567890
   [INFO] Firebase: Read back: 1234567890
   [INFO] Firebase: Delete /pump_system/test
   [ PASS] TC-M-04: Firebase Read/Write
   ```

**Pass Criteria:**
- Firebase connects within 10 seconds
- Write succeeds
- Read returns identical value
- Delete succeeds

**Notes:**
- Requires Firebase library and these `secrets.h` values:
   - `FIREBASE_API_KEY`
   - `FIREBASE_RTDB_URL`
   - `FIREBASE_EMAIL`
   - `FIREBASE_PASSWORD`
- Missing prerequisites cause FAIL by default.
- To allow defer mode, set `ALLOW_DEFERRED_CLOUD_TESTS=1`.

---

### TC-M-05: Full Integration

**Purpose:** End-to-end RS-485 data sanity: request → CRC verify → field parse/range checks.

**Procedure:**
1. NodeMCU running sensor test/echo
2. ESP32 running master node test
3. Observe 10 RS-485 request/response cycles
4. Expected output:
   ```
   [INFO] Integration result: valid=9 parseErrors=1 seqChanges=8
   [ PASS] TC-M-05: Full Integration
   ```

**Pass Criteria:**
- At least 8/10 frames valid
- Parse errors <= 2
- Sequence value changes at least once

---

## Expected Output Summary

### Successful Run (All Pass):
```
=== SmartFlow ESP32 Master Hardware Test Suite ===
Build: Dec 20 2025 14:30:00

[ RUN ] TC-M-01: GPIO and Relay
[SAFETY] Relay will activate for 500ms...
Press ENTER to proceed with relay activation:
[INFO] Relay cycle complete.
[ PASS] TC-M-01: GPIO and Relay

[ RUN ] TC-M-02: RS-485 Master Poll
[INFO] RS485 Master: Polling for 30 seconds...
[INFO] RS485: 29 valid, 1 failed (96.7% success rate)
[ PASS] TC-M-02: RS-485 Master Poll

[ RUN ] TC-M-03: WiFi Connection
[INFO] WiFi: Attempting connection...
[INFO] WiFi connected. IP: 192.168.1.20 RSSI: -56 dBm
[INFO] DNS checks: 3/3 successful
[ PASS] TC-M-03: WiFi Connection

[ RUN ] TC-M-04: Firebase Read/Write
[INFO] Firebase: Starting read/write/delete validation...
[INFO] Firebase R/W/D validation passed.
[ PASS] TC-M-04: Firebase Read/Write

[ RUN ] TC-M-05: Full Integration
[INFO] Integration: RS485 parse/data sanity over 10 frames...
[INFO] Integration result: valid=9 parseErrors=1 seqChanges=8
[ PASS] TC-M-05: Full Integration

=== Test Summary ===
Ran: 5, Passed: 5, Failed: 0
[ OK ] All tests passed!
```

### Failure Example (TC-M-02 only):
```
[ RUN ] TC-M-02: RS-485 Master Poll
[INFO] RS485 Master: Polling for 30 seconds...
[WARN] CRC mismatch: got A1F3 expected C2E4
[INFO] RS485: 2 valid, 28 failed (6.7% success rate)
[ FAIL] TC-M-02: RS-485 Master Poll

[ PASS] ... (other tests)

=== Test Summary ===
Ran: 5, Passed: 4, Failed: 1
[ FAIL ] 1 test(s) failed
```

---

## Phase 5 Exit Criteria

Phase 5 is complete when:

- ✅ `test_sensor_node.ino` compiles without errors
- ✅ `test_master_node.ino` compiles without errors
- ✅ Both test suites run independently (no production firmware deps)
- ✅ TC-S-01 through TC-S-05 pass on actual NodeMCU hardware
- ✅ TC-M-01 through TC-M-03 pass on actual ESP32 hardware
- ✅ All [PASS] or [FAIL] labels unambiguous in output
- ✅ No compilation errors reported by Arduino IDE

**When satisfied:** Proceed to Phase 6 (Dashboard Redesign) and Phase 7 (Integration Testing).

---

## Safety Notes

1. **TC-M-01 Relay Test:** Ensures pump is OFF or relay is isolated before test runs.
2. **TC-M-02 RS-485:** Should not interfere with production nodes (use test firmware).
3. **TC-M-04/M-05 Firebase:** Requires explicit WiFi credentials in `secrets.h` — **never commit credentials to repo**.

---

## Links to Related Files

- [Node MCU Test Suite](firebase/test_sensor_node/README.md)
- [RS-485 Protocol Spec](docs/specs/rs485_protocol.md)
- [Refactor Plan v2.0](smartflow_refactor_plan_v2.md)
- [Safety Rules](firmware/arduino_smart_water_pump_controller/00_safety_config.h)
