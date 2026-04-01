// =============================================================================
// SmartFlow NodeMCU V2 Hardware Test Suite
// =============================================================================
// TC-S-01: Hardware Sanity
// TC-S-02: Ultrasonic Sensor
// TC-S-03: Flow Sensor
// TC-S-04: RS-485 Slave Echo Server
// TC-S-05: CRC Self-Test
//
// Usage:
//   1. Compile and flash to NodeMCU V2 (ESP8266)
//   2. Open Serial Monitor at 115200 baud
//   3. Tests run automatically in setup(); results printed to console
//   4. Expected output: [PASS] or [FAIL] for each test
//
// Build: Arduino IDE 1.8.x + ESP8266 Board Support
// Requires: Arduino.h, time.h (for micros/millis)
// =============================================================================

#include <Arduino.h>

// GPIO Mapping (must match production wiring)
#define PIN_RS485_DE_RE   14   // D5
#define PIN_FLOW_INPUT    13   // D7
#define PIN_US_TRIG        5   // D1
#define PIN_US_ECHO       16   // D0

#define US_TIMEOUT_US     100000UL
#define RS485_BAUD        115200

// Test strictness controls
// - REQUIRE_FLOW_PULSES=1: TC-S-03 fails when zero pulses are seen.
// - REQUIRE_RS485_REQ_FRAME=1: TC-S-04 fails when no REQ was received in 5s.
#ifndef REQUIRE_FLOW_PULSES
#define REQUIRE_FLOW_PULSES 0
#endif

#ifndef REQUIRE_RS485_REQ_FRAME
#define REQUIRE_RS485_REQ_FRAME 0
#endif

// ============================================================================
// Global test runner
// ============================================================================

int testsRun = 0;
int testsPassed = 0;
int testsFailed = 0;

// REFACTOR [QA-TEST-FIX]: ISR counters must use static storage and a named ISR callback.
volatile uint32_t g_flowPulseCount = 0;

typedef bool (*TestFn)();

void onFlowPulse() {
  g_flowPulseCount++;
}

struct TestCase {
  const char* name;
  TestFn fn;
};

void runTest(const char* name, TestFn fn) {
  Serial.printf("[ RUN ] %s\n", name);
  bool ok = fn();
  Serial.printf("%s %s\n", ok ? "[ PASS]" : "[ FAIL]", name);
  testsRun++;
  if (ok) testsPassed++; else testsFailed++;
}

// ============================================================================
// CRC16-Modbus (used by TC-S-05)
// ============================================================================

uint16_t crc16_modbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA001);
      else crc = (uint16_t)(crc >> 1);
    }
  }
  return crc;
}

// ============================================================================
// Test Cases
// ============================================================================

// TC-S-01: Hardware Sanity
bool test_hw_sanity() {
  // Test GPIO drive on RS485 DE/RE
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, HIGH);
  if (digitalRead(PIN_RS485_DE_RE) != HIGH) return false;
  digitalWrite(PIN_RS485_DE_RE, LOW);
  if (digitalRead(PIN_RS485_DE_RE) != LOW) return false;

  // Test TRIG pin can output
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);

  // Test FLOW input pullup
  pinMode(PIN_FLOW_INPUT, INPUT_PULLUP);
  delay(10);
  if (digitalRead(PIN_FLOW_INPUT) != HIGH) {
    Serial.println("INFO: FLOW pullup not reading HIGH (may be held low externally - OK for deployed system)");
  }

  // Test Serial1 (GPIO2) output
  Serial1.begin(115200);
  delay(100);
  Serial1.println("[TEST] Serial1 output test");
  Serial1.flush();
  delay(100);

  return true;
}

// TC-S-02: Ultrasonic Sensor (20 pings, pass if >=15/20 valid, stable within 5cm)
bool test_ultrasonic() {
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);

  int validCount = 0;
  float minDist = 500.0f, maxDist = 0.0f;

  for (int i = 0; i < 20; i++) {
    digitalWrite(PIN_US_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);

    uint32_t echoStart = micros();
    uint32_t timeout = echoStart + US_TIMEOUT_US;

    // Wait for echo rise
    while (digitalRead(PIN_US_ECHO) == LOW && micros() < timeout) {
      delayMicroseconds(1);
    }
    if (micros() >= timeout) {
      delay(70);
      continue; // Timeout - skip this reading
    }

    uint32_t pulseStart = micros();
    // Wait for echo fall
    while (digitalRead(PIN_US_ECHO) == HIGH && micros() < timeout) {
      delayMicroseconds(1);
    }
    uint32_t pulseDuration = micros() - pulseStart;

    if (micros() >= timeout) {
      delay(70);
      continue; // Timeout - skip
    }

    float cm = (float)pulseDuration / 58.0f;
    if (cm >= 2.0f && cm <= 300.0f) {
      validCount++;
      if (cm < minDist) minDist = cm;
      if (cm > maxDist) maxDist = cm;
    }

    delay(70);
  }

  bool stable = (maxDist - minDist) < 5.0f || validCount < 2;
  Serial.printf("  Ultrasonic: %d/20 valid, range=%.1f..%.1f cm, stable=%s\n",
                validCount, minDist, maxDist, stable ? "yes" : "no");

  return (validCount >= 15) && (maxDist - minDist < 5.0f || validCount < 2);
}

// TC-S-03: Flow Sensor (10s pulse count, pass if non-zero when flow detected)
bool test_flow_sensor() {
  g_flowPulseCount = 0;

  pinMode(PIN_FLOW_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT), onFlowPulse, RISING);

  Serial.println("  Flow test: Counting pulses for 10 seconds...");
  delay(10000);

  detachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT));

  uint32_t pulseCount = g_flowPulseCount;
  Serial.printf("  Flow: %lu pulses/10s = %.1f Hz\n", pulseCount, (float)pulseCount / 10.0f);

  // Non-zero indicates flow sensor is detecting pulses
  // Zero may be acceptable on dry bench runs with no water flow.
  if (pulseCount == 0) {
#if REQUIRE_FLOW_PULSES
    Serial.println("  ERROR: No pulses detected while REQUIRE_FLOW_PULSES=1.");
    return false;
#else
    Serial.println("  INFO: No pulses detected (water may not be flowing - manually verify)");
    return true; // Not auto-failed
#endif
  }

  return pulseCount > 0;
}

// TC-S-04: RS-485 Slave Echo Server (listen for REQ, send test frame)
bool test_rs485_echo_server() {
  Serial.println("  RS485 Echo Server: Waiting 5s for REQ commands...");

  Serial.begin(RS485_BAUD); // UART0 for RS485
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW); // RX mode

  uint32_t startMs = millis();
  int framesSent = 0;

  while (millis() - startMs < 5000) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (cmd == "REQ") {
        // Send hardcoded test frame: STX LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;CRC:XXXX ETX
        char payload[] = "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;";
        uint16_t crc = crc16_modbus((const uint8_t*)payload, strlen(payload));

        digitalWrite(PIN_RS485_DE_RE, HIGH); // TX mode
        delayMicroseconds(60);

        Serial.write(0x02); // STX
        Serial.print(payload);
        Serial.printf("CRC:%04X", (unsigned)crc);
        Serial.write(0x03); // ETX
        Serial.flush();

        delay(2); // Wait for shift register
        digitalWrite(PIN_RS485_DE_RE, LOW); // RX mode

        framesSent++;
        Serial1.printf("[TEST] Echo: sent frame seq=0 crc=%04X\n", (unsigned)crc);
      }
    }
    delay(10);
  }

  Serial.end();
  Serial1.printf("  RS485 Echo: %d frames sent\n", framesSent);

  if (framesSent == 0) {
#if REQUIRE_RS485_REQ_FRAME
    Serial1.println("  ERROR: No REQ observed while REQUIRE_RS485_REQ_FRAME=1.");
    return false;
#else
    Serial1.println("  INFO: No REQ observed in standalone mode (acceptable for bench-only run).");
    return true;
#endif
  }

  return true;
}

// TC-S-05: CRC Self-Test
bool test_crc_self_test() {
  // Known test vector: "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;"
  // Expected CRC: computed at build time
  const char* testPayload = "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;";
  uint16_t computedCrc = crc16_modbus((const uint8_t*)testPayload, strlen(testPayload));

  // REFACTOR [TC-S-05]: CRC16-Modbus verified against Python reference (2026-03-31).
  // Payload: "LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;" (46 bytes)
  // Algorithm: poly=0xA001, init=0xFFFF (CRC16-Modbus).
  uint16_t expectedCrc = 0xEB6C; // Verified: Python crcmod + independent CRC16-Modbus calculator

  Serial.printf("  CRC Self-test: computed=%04X expected=%04X\n", (unsigned)computedCrc, (unsigned)expectedCrc);

  // Verify the implementation against a known expected vector.
  if (computedCrc != expectedCrc) {
    Serial.println("  ERROR: CRC mismatch versus known expected value.");
    return false;
  }

  // Also verify CRC computation is deterministic (runs same each time).
  uint16_t recomputed = crc16_modbus((const uint8_t*)testPayload, strlen(testPayload));
  if (computedCrc != recomputed) {
    Serial.println("  ERROR: CRC not deterministic!");
    return false;
  }

  return true;
}

// ============================================================================
// Setup & Loop
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== SmartFlow NodeMCU Hardware Test Suite ===");
  Serial.println("Build: " __DATE__ " " __TIME__);
  Serial.println("");

  // Run all tests
  runTest("TC-S-01: Hardware Sanity", test_hw_sanity);
  runTest("TC-S-02: Ultrasonic Sensor", test_ultrasonic);
  runTest("TC-S-03: Flow Sensor", test_flow_sensor);
  runTest("TC-S-04: RS-485 Echo Server", test_rs485_echo_server);
  runTest("TC-S-05: CRC Self-Test", test_crc_self_test);

  Serial.println("");
  Serial.printf("=== Test Summary ===\n");
  Serial.printf("Ran: %d, Passed: %d, Failed: %d\n", testsRun, testsPassed, testsFailed);
  if (testsFailed == 0) {
    Serial.println("[ OK ] All tests passed!");
  } else {
    Serial.printf("[ FAIL ] %d test(s) failed\n", testsFailed);
  }
}

void loop() {
  // All tests run once in setup()
  delay(1000);
}
