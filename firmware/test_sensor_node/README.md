# SmartFlow NodeMCU V2 — Hardware Test Suite

**Location:** `firmware/test_sensor_node/`  
**Target:** ESP8266 NodeMCU V2  
**Test cases:** TC-S-01 through TC-S-05  
**Expected runtime:** ~30 seconds per full test suite run

## Prerequisites

### Hardware Requirements
- NodeMCU V2 (ESP8266) with uploaded test firmware
- JSN-SR04T ultrasonic sensor (JSN-SR04T-2.0) connected to TRIG/ECHO
- YF-G1 flow sensor connected to flow input
- MAX485 RS-485 transceiver wired to UART0 and DE/RE GPIO
- USB-TTL adapter for Serial1 debug output (optional, but recommended for production verification)
- Serial monitor or terminal at 115200 baud

### Arduino IDE Setup
1. Install ESP8266 Board Support Package (via Board Manager)
2. Select board: "NodeMCU 1.0 (ESP-12E Module)"
3. Baud rate: 115200
4. CPU Frequency: 80 MHz (or higher)
5. Flash Size: 4 MB (FS: 3 MB, OTA: ~512 KB)

## Test Cases

### TC-S-01: Hardware Sanity ✓
**Purpose:** Verify all GPIO pins can be driven and serial subsystems are operational.

**What it does:**
- Drive RS485 DE/RE pin HIGH and LOW, verify readback
- Pulse US TRIG output
- Check US ECHO can be read as INPUT
- Verify Flow input pull-up (should read HIGH when not driven)
- Test Serial1 (GPIO2) output at 115200 baud

**Expected output:** `[ PASS] TC-S-01`

**Pass criteria:** All GPIO transitions and serial output succeed without exception

---

### TC-S-02: Ultrasonic Sensor ✓
**Purpose:** Validate ultrasonic sensor operation over 20 pings.

**What it does:**
- Fire 20 trigger pulses at 70ms spacing
- Measure echo pulse width for each
- Convert to distance (pulse_width_us / 58)
- Report: valid count, min/max distance, stability

**Expected output:**
```
[ RUN ] TC-S-02: Ultrasonic Sensor
  Ultrasonic: 18/20 valid, range=44.2..45.1 cm, stable=yes
[ PASS] TC-S-02: Ultrasonic Sensor
```

**Pass criteria:**
- ≥15/20 readings within valid range (2–300 cm)
- All valid readings within 5 cm of each other (stability check)

**Troubleshooting:**
- If readings are erratic (span > 5 cm): sensor may have echo interference
- If <15/20 valid: sensor may have wiring issue or be out of range

---

### TC-S-03: Flow Sensor ✓
**Purpose:** Verify flow interrupt counter is incremented by sensor pulses.

**What it does:**
- Attach interrupt to flow input pin (RISING)
- Count pulses for 10 seconds
- Report total pulses and computed flow rate (Hz)

**Expected output:**
```
[ RUN ] TC-S-03: Flow Sensor
  Flow test: Counting pulses for 10 seconds...
  Flow: 18 pulses/10s = 1.8 Hz
[ PASS] TC-S-03: Flow Sensor
```

**Pass criteria:**
- Test is informational if no water flowing (PASS regardless)
- Non-zero count indicates sensor is working
- Count should be ~7.5 pulses/L/min (YF-G1 factory calibration)
- If `REQUIRE_FLOW_PULSES=1` is enabled in sketch, zero pulses is a FAIL

**Troubleshooting:**
- Zero pulses with water flowing: may indicate connection issue or sensor failure

---

### TC-S-04: RS-485 Slave Responder ✓
**Purpose:** Verify RS-485 transceiver and slave response capability.

**What it does:**
- Listen for 5 seconds on UART0 (RS-485)
- When "REQ\n" is received, send a hardcoded test frame:
  ```
  STX LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;CRC:3F9A ETX
  ```
- When "PING\n" is received, send a simple hello frame:
  ```
  STX MSG:HELLO_FROM_NODE;CRC:XXXX ETX
  ```
- Count responses sent and report

**Expected output:**
```
[ RUN ] TC-S-04: RS-485 Echo Server
  RS485 Responder: Waiting 5s for REQ/PING commands...
  RS485 Responder: REQ frames=1, PING replies=1
[ PASS] TC-S-04: RS-485 Echo Server
```

**Pass criteria:**
- At least 1 response sent within the 5s window (REQ frame and/or PING hello)
- With this test alone (no external master), 0 frames sent is acceptable when `REQUIRE_RS485_REQ_FRAME=0` (default)
- If `REQUIRE_RS485_REQ_FRAME=1` is enabled, 0 frames sent is a FAIL
- Combine with TC-M-02 (ESP32 test) to verify full half-duplex link

After the one-time test suite completes, the sketch keeps a persistent RS-485 responder active in `loop()` so the ESP32 master can be flashed later from the same laptop and still receive REQ/PING responses.

---

### TC-S-05: CRC Self-Test ✓
**Purpose:** Verify CRC16-Modbus implementation is deterministic and correct.

**What it does:**
- Compute CRC of known test payload: `"LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;"`
- Recompute to verify determinism
- Report both values

**Expected output:**
```
[ RUN ] TC-S-05: CRC Self-Test
  CRC Self-test: computed=EB6C expected=EB6C
[ PASS] TC-S-05: CRC Self-Test
```

**Pass criteria:**
- Computed CRC matches expected value
- Multiple computations produce identical result (deterministic)

---

## Running the Tests

### Procedure
1. **Flash firmware:** Upload `test_sensor_node.ino` to NodeMCU via Arduino IDE
2. **Open Serial Monitor:** 115200 baud, line ending set to "Newline"
3. **Press EN/RST button:** Tests start automatically
4. **Observe output:** All test results print to console
5. **Wait for summary:** Test count and pass/fail summary appears at end

### Expected Total Runtime
- TC-S-01: <1 second
- TC-S-02: ~15 seconds (20 pings @ 70ms each)
- TC-S-03: 10 seconds (10s pulse counting)
- TC-S-04: 5 seconds (listen window)
- TC-S-05: <1 second
- **Total: ~31 seconds**

### Interpreting Results

**Success (all PASS):**
```
[ PASS] TC-S-01: Hardware Sanity
[ PASS] TC-S-02: Ultrasonic Sensor
[ PASS] TC-S-03: Flow Sensor
[ PASS] TC-S-04: RS-485 Echo Server
[ PASS] TC-S-05: CRC Self-Test

=== Test Summary ===
Ran: 5, Passed: 5, Failed: 0
[ OK ] All tests passed!
```

**Failure example:**
```
[ PASS] TC-S-01: Hardware Sanity
[ FAIL] TC-S-02: Ultrasonic Sensor
[ PASS] TC-S-03: Flow Sensor
[ SKIP] TC-S-04: (not run if previous failed)
...

=== Test Summary ===
Ran: 5, Passed: 3, Failed: 2
[ FAIL ] 2 test(s) failed
```

---

## Troubleshooting

### Serial Monitor shows garbage or no output
- Verify baud rate is **115200**
- Check USB cable connection
- Try resetting board (EN button)

### TC-S-01 fails (GPIO test)
- Verify pin definitions match hardware/wiring_notes.md
- Check for GPIO conflicts with LED or other peripherals
- Retest after board reset

### TC-S-02 fails (ultrasonic)
- Verify sensor is powered (VCC/GND)
- Check TRIG connected to GPIO5, ECHO to GPIO16
- Ensure ECHO is level-shifted to 3.3V (sensor outputs 5V — circuit required)
- Test with object 30–100 cm away (optimal range)

### TC-S-03 shows zero pulses
- Verify flow sensor is powered
- Check GPIO13 (D7) connection
- If water is flowing, may indicate sensor failure

### TC-S-04 shows zero frames sent
- This is normal if running test alone (no external master sending REQ)
- To test fully: pair with TC-M-02 (ESP32 test) to send REQ commands

### TC-S-05 fails (CRC mismatch)
- Verify CRC16-Modbus algorithm matches reference
- Check polynomial and initial value
- Compare computed vs expected values in serial output

---

## Integration with Production Firmware

To transition from test firmware to production firmware:

1. **Stop test program** (no flashing required, just reset NodeMCU)
2. **Flash production firmware** `firmware/arduino_sensor_node/arduino_sensor_node.ino`
3. **Verify:** Monitor serial output for boot banner and first RS-485 frame transmission (if connected to ESP32)

---

## References

- **Pin mapping:** `hardware/wiring_notes.md`
- **Sensor datasheets:** `hardware/`
- **RS-485 protocol:** `docs/specs/rs485_protocol.md`
- **SmartFlow skill:** `.github/skills/smartflow/SKILL.md`

---

**Test suite version:** 1.0  
**Created:** 2026-03-31  
**Status:** READY FOR DEPLOYMENT
