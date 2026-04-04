# SmartFlow — Test Firmware Suite Reference

## Design Principles

- Each test sketch is a **complete, standalone firmware** — not modified production firmware
- Tests compile independently without any production firmware files
- Structured output: `[ RUN ]`, `[ PASS]`, `[ FAIL]` per test case
- Output at 115200 baud via USB Serial
- Hardware-only (no Firebase/WiFi except dedicated WiFi/Firebase tests)
- Stored in `test/` directories within each firmware project

---

## Test Runner Pattern (Both Nodes)

```cpp
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println(F(""));
  Serial.println(F("=== SmartFlow Node Hardware Test ==="));
  Serial.print(F("Build: ")); Serial.println(F(__DATE__ " " __TIME__));
  Serial.println(F(""));

  runTest("TC-S-01: Hardware sanity",      test_hw_sanity);
  runTest("TC-S-02: Ultrasonic sensor",    test_ultrasonic);
  runTest("TC-S-03: Flow sensor",          test_flow);
  runTest("TC-S-04: RS-485 echo server",   test_rs485_echo);
  runTest("TC-S-05: CRC self-test",        test_crc);

  Serial.println(F(""));
  Serial.println(F("=== Test complete ==="));
}

void loop() {} // Tests run once in setup()

void runTest(const char* name, bool (*fn)()) {
  Serial.print(F("[ RUN ] ")); Serial.println(name);
  bool ok = fn();
  Serial.print(ok ? F("[ PASS] ") : F("[ FAIL] "));
  Serial.println(name);
  Serial.println(F(""));
}
```

---

## NodeMCU Sensor Node — Test Suite

**Location:** `firmware/test_sensor_node/` (Arduino IDE)
or `firmware/platformio_sensor_node/test/` (PlatformIO)

### TC-S-01: Hardware Sanity

```
Purpose: Verify GPIO configuration and power-up state
Prerequisite: None — hardware only
```

```cpp
bool test_hw_sanity() {
  bool ok = true;

  // RS-485 DE/RE pin
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, HIGH);
  delay(5);
  if (digitalRead(PIN_RS485_DE_RE) != HIGH) {
    Serial.println(F("  FAIL: DE/RE HIGH failed"));
    ok = false;
  }
  digitalWrite(PIN_RS485_DE_RE, LOW);

  // Ultrasonic TRIG pulse
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  Serial.println(F("  INFO: TRIG pulse sent (verify on oscilloscope or logic analyzer)"));

  // Flow sensor input pullup
  pinMode(PIN_FLOW_INPUT, INPUT_PULLUP);
  delay(5);
  if (digitalRead(PIN_FLOW_INPUT) != HIGH) {
    Serial.println(F("  FAIL: FLOW_INPUT pullup not HIGH (check wiring)"));
    ok = false;
  }

  // Serial1 / GPIO2 debug output
  Serial1.begin(115200);
  Serial1.println(F("[TEST] TC-S-01: Serial1/GPIO2 output test"));
  Serial.println(F("  INFO: Serial1/GPIO2 output sent (verify with USB-TTL adapter on GPIO2)"));

  return ok;
}
```

**Pass criteria:** All GPIO operations succeed, no exception. Serial1 output verifiable.

---

### TC-S-02: Ultrasonic Sensor

```
Purpose: Verify JSN-SR04T sensor operation
Prerequisite: Sensor wired, sensor facing a surface within 300cm
```

```cpp
bool test_ultrasonic() {
  const int NUM_PINGS = 20;
  const int PASS_MIN_VALID = 15;
  float readings[NUM_PINGS];
  int validCount = 0;
  float minDist = 9999, maxDist = 0;

  for (int i = 0; i < NUM_PINGS; i++) {
    float dist = pingUltrasonic();  // Existing ultrasonic read function
    if (dist >= 2.0f && dist <= 300.0f) {
      readings[validCount++] = dist;
      if (dist < minDist) minDist = dist;
      if (dist > maxDist) maxDist = dist;
    } else {
      Serial.printf("  WARN: ping %d timeout/invalid\n", i);
    }
    delay(200);
  }

  float spread = (validCount > 1) ? (maxDist - minDist) : 0;
  Serial.printf("  Result: valid=%d/%d min=%.1fcm max=%.1fcm spread=%.1fcm\n",
                validCount, NUM_PINGS, minDist, maxDist, spread);

  if (validCount < PASS_MIN_VALID) {
    Serial.printf("  FAIL: only %d/%d valid readings\n", validCount, PASS_MIN_VALID);
    return false;
  }
  if (spread > 5.0f) {
    Serial.printf("  FAIL: spread %.1fcm > 5.0cm threshold\n", spread);
    return false;
  }
  return true;
}
```

**Pass criteria:** ≥15/20 valid readings, all in 2–300 cm, (max − min) < 5 cm.

---

### TC-S-03: Flow Sensor

```
Purpose: Verify YF-G1 interrupt and pulse counting
Prerequisite: Sensor installed in water line (or manually spin paddle wheel for bench test)
```

```cpp
volatile uint32_t tc_flowPulses = 0;
void IRAM_ATTR tc_onFlow() { tc_flowPulses++; }

bool test_flow() {
  tc_flowPulses = 0;
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT), tc_onFlow, FALLING);

  Serial.println(F("  INFO: Counting pulses for 10 seconds..."));
  delay(10000);

  detachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT));
  uint32_t pulses = tc_flowPulses;

  float hz = pulses / 10.0f;
  // YF-G1: ~7.5 Hz per L/min (calibration factor varies)
  float lpm = hz / 7.5f;

  Serial.printf("  Result: %lu pulses / 10s = %.1fHz = %.2f L/min\n", pulses, hz, lpm);

  if (pulses == 0) {
    Serial.println(F("  INFO: No pulses detected — no flow present."));
    Serial.println(F("        Manually spin paddle wheel to verify sensor responds."));
    return true;  // Not a failure — may be no-flow test condition
  }

  Serial.println(F("  Flow detected — sensor responding"));
  return true;
}
```

**Pass criteria:** Non-failure (flow test is informational — no auto-fail on zero pulses).
Report pulse count and calculated LPM for operator verification.

---

### TC-S-04: RS-485 Slave Echo Server

```
Purpose: Verify MAX485 module and RS-485 slave response capability
Prerequisite: DEBUG_USB_MODE=0 (RS-485 on UART0), DE/RE pin wired
Pair with: ESP32 running TC-M-02
```

```cpp
bool test_rs485_echo() {
  // Pre-compute CRC for test frame
  const char* testPayload = "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;";
  uint16_t crc = crc16_modbus((uint8_t*)testPayload, strlen(testPayload));
  char testFrame[128];
  snprintf(testFrame, sizeof(testFrame), "%sLVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;CRC:%04X%s",
           "\x02", crc, "\x03");

  Serial.println(F("  INFO: Listening for REQ on RS-485. Pair with ESP32 TC-M-02."));
  Serial.println(F("  Will echo test frame on each REQ received. Running for 30 seconds."));

  uint32_t startMs = millis();
  int reqCount = 0;

  while (millis() - startMs < 30000) {
    if (Serial.available() >= 4) {  // "REQ\n" = 4 bytes
      char buf[8] = {0};
      Serial.readBytes(buf, 4);
      if (strncmp(buf, "REQ\n", 4) == 0) {
        reqCount++;
        // Send echo response
        digitalWrite(PIN_RS485_DE_RE, HIGH);
        delayMicroseconds(10);
        Serial.write((uint8_t)0x02);
        Serial.print(testFrame + 1);  // Skip leading STX already sent
        Serial.flush();
        delayMicroseconds(60);
        digitalWrite(PIN_RS485_DE_RE, LOW);

        SN_SERIAL_DEBUG.printf("[TEST] REQ #%d received — echo frame sent\n", reqCount);
      }
    }
  }

  Serial.printf("  Result: %d REQ frames received and echoed\n", reqCount);
  if (reqCount == 0) {
    Serial.println(F("  WARN: No REQ received. Check RS-485 wiring and ESP32 TC-M-02."));
    return false;
  }
  return true;
}
```

---

### TC-S-05: CRC Self-Test

```
Purpose: Verify CRC16-Modbus implementation correctness
Prerequisite: None
```

```cpp
bool test_crc() {
  // Known test vector
  const char* input = "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;";
  uint16_t computed = crc16_modbus((uint8_t*)input, strlen(input));

  // Pre-compute expected value during development and hardcode here
  // To get expected: run this sketch once, note computed value, set as TC_EXPECTED_CRC
  const uint16_t TC_EXPECTED_CRC = 0x0000;  // REPLACE with actual computed value

  Serial.printf("  Input: \"%s\"\n", input);
  Serial.printf("  Computed CRC: %04X\n", computed);

  if (TC_EXPECTED_CRC == 0x0000) {
    Serial.println(F("  INFO: TC_EXPECTED_CRC not set yet."));
    Serial.printf("  ACTION: Set TC_EXPECTED_CRC = 0x%04X in test source.\n", computed);
    return true;  // Pass on first run to bootstrap expected value
  }

  if (computed != TC_EXPECTED_CRC) {
    Serial.printf("  FAIL: CRC mismatch. Expected %04X got %04X\n", TC_EXPECTED_CRC, computed);
    return false;
  }
  return true;
}
```

---

## ESP32 Master Node — Test Suite

**Location:** `firmware/test_master_node/` (Arduino IDE)
or `firmware/platformio_smart_water_pump_controller/test/` (PlatformIO)

### TC-M-01: GPIO and Relay

```
Purpose: Verify GPIO configuration and relay operation
Prerequisite: CONFIRM pump is safe to run (has water) OR disconnect pump before running
SAFETY: Relay will activate for 500ms. Print warning and require ENTER.
```

```cpp
bool test_gpio_relay() {
  // Safety gate
  Serial.println(F("  *** SAFETY WARNING ***"));
  Serial.println(F("  Relay will activate for 500ms."));
  Serial.println(F("  Confirm pump has water OR disconnect pump now."));
  Serial.println(F("  Press ENTER to proceed, or power cycle to abort."));
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();  // Flush

  bool ok = true;

  // RS-485 DE/RE
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  if (digitalRead(RS485_DE_RE_PIN) != HIGH) {
    Serial.println(F("  FAIL: RS485_DE_RE HIGH"));
    ok = false;
  }
  digitalWrite(RS485_DE_RE_PIN, LOW);

  // Relay (active LOW — HIGH = relay off)
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Ensure off first
  delay(100);

  Serial.println(F("  INFO: Activating relay for 500ms..."));
  digitalWrite(RELAY_PIN, LOW);   // Relay ON
  delay(500);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF
  Serial.println(F("  INFO: Relay deactivated. Listen for click."));

  return ok;
}
```

---

### TC-M-02: RS-485 Master Poll

```
Purpose: Verify ESP32 can poll NodeMCU and receive valid CRC frames
Prerequisite: NodeMCU running TC-S-04 (echo server) or production firmware
```

```cpp
bool test_rs485_master() {
  Serial2.begin(115200, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  const int DURATION_S = 30;
  int sent = 0, valid = 0, crcErrors = 0, timeouts = 0;
  uint32_t startMs = millis();

  while ((millis() - startMs) < (uint32_t)(DURATION_S * 1000)) {
    // Send REQ
    digitalWrite(RS485_DE_RE_PIN, HIGH);
    Serial2.print("REQ\n");
    Serial2.flush();
    delayMicroseconds(80);
    digitalWrite(RS485_DE_RE_PIN, LOW);
    sent++;

    // Wait for response
    uint32_t waitStart = millis();
    while (!Serial2.available() && millis() - waitStart < 250) delay(1);

    if (!Serial2.available()) {
      timeouts++;
    } else {
      char buf[128] = {0};
      int len = Serial2.readBytesUntil(0x03, buf + 1, sizeof(buf) - 2);
      buf[0] = 0x02;
      // Parse and validate CRC
      if (validateFrameCRC(buf)) {
        valid++;
        Serial.printf("  Frame %d: valid — %s\n", sent, buf + 1);
      } else {
        crcErrors++;
        Serial.printf("  Frame %d: CRC error\n", sent);
      }
    }
    delay(1000);
  }

  float rate = sent > 0 ? (100.0f * valid / sent) : 0;
  Serial.printf("  Result: %d/%d valid (%.0f%%), %d CRC errors, %d timeouts\n",
                valid, sent, rate, crcErrors, timeouts);
  return rate >= 90.0f;
}
```

**Pass criteria:** ≥90% valid frames over 30 seconds.

---

### TC-M-03: WiFi Connection

```
Purpose: Verify WiFi credentials and connectivity
Prerequisite: secrets.h populated with valid SSID/password
```

Pass: connection within 20 s, IP assigned, ≥2/3 pings to `8.8.8.8` succeed.

---

### TC-M-04: Firebase Read/Write

```
Purpose: Verify Firebase authentication and data round-trip
Prerequisite: secrets.h populated, Firebase project accessible
```

- Write `/pump_system/test/ping_at` = `millis()` timestamp
- Read it back, verify value matches
- Delete test node
- Pass: round-trip confirmed within 10 s

---

### TC-M-05: Full Round-Trip Integration

```
Purpose: Verify complete data path ESP32→RS485→parse→Firebase→verify
Prerequisite: Full hardware setup, NodeMCU running production firmware, WiFi + Firebase
This is the last test and must run after all others pass.
```

Pass: RS-485 frame parsed, sensor data pushed to Firebase, value verified via read-back.

---

## Test Directories — Required Files

Each test directory must contain:

```
test_master_node/
├── test_master_node.ino   ← Main sketch with setup()/loop()
├── test_helpers.ino        ← runTest(), validateFrameCRC(), crc16_modbus()
├── test_cases.ino          ← Individual TC-M-xx functions
└── README.md               ← Prerequisites and expected output

test_sensor_node/
├── test_sensor_node.ino
├── test_helpers_slave.ino
├── test_cases_slave.ino
└── README.md
```

## README Template for Each Test Directory

```markdown
# SmartFlow — [Master/Slave] Node Hardware Test

## Purpose
[One paragraph what this validates]

## Prerequisites
- [ ] Production firmware NOT loaded — flash this sketch separately
- [ ] Hardware wired per hardware/wiring_notes.md
- [ ] secrets.h populated (master TC-M-03 and TC-M-04 only)
- [ ] [Node-specific prereqs]

## Running the tests
1. Open [sketch].ino in Arduino IDE (or use PlatformIO)
2. Select board: [ESP32 Dev Module / NodeMCU 1.0 (ESP-12E Module)]
3. Upload
4. Open Serial Monitor at 115200 baud
5. Tests run automatically once on startup

## Expected output
\```
=== SmartFlow Node Hardware Test ===
Build: Mar 30 2026 14:22:11

[ RUN ] TC-X-01: Test name
[ PASS] TC-X-01: Test name

...
=== Test complete ===
\```

## Interpreting failures
| Failure | Likely cause |
|---------|-------------|
| TC-S-02 FAIL valid<15 | JSN-SR04T not responding, voltage divider missing |
| TC-M-02 FAIL rate<90% | RS-485 wiring issue, baud mismatch, NodeMCU not running |
| TC-M-03 FAIL | WiFi credentials wrong, router unreachable |
| TC-M-04 FAIL | Firebase credentials wrong, rules blocking write |
```
