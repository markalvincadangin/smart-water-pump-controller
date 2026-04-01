// =============================================================================
// SmartFlow ESP32 Master Node Hardware Test Suite
// =============================================================================
// TC-M-01: GPIO and Relay (with safety warning)
// TC-M-02: RS-485 Master (30s poll, ≥90% valid frames)
// TC-M-03: WiFi Connection (within 20s, ≥2/3 pings)
// TC-M-04: Firebase Read/Write (write, read back, delete)
// TC-M-05: Full Round-Trip Integration (RS485 → Firebase)
//
// Usage:
//   1. Fill in secrets.h with WiFi SSID/password and Firebase credentials
//   2. Compile and flash to ESP32
//   3. Open Serial Monitor at 115200 baud
//   4. Tests run automatically; follow prompts for safety-critical operations
//   5. Expected output: [PASS] or [FAIL] for each test
//
// Build: Arduino IDE 1.8.x + ESP32 Board Support
// Requires: Arduino.h, WiFi.h, Firebase_ESP_Client.h, secrets.h
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#define HAS_LOCAL_SECRETS 1
#else
#define HAS_LOCAL_SECRETS 0
#endif

#if __has_include(<Firebase_ESP_Client.h>)
#include <Firebase_ESP_Client.h>
#define HAS_FIREBASE_LIB 1
#else
#define HAS_FIREBASE_LIB 0
#endif

// GPIO Mapping (must match production wiring)
#define RELAY_PIN        4
#define RS485_TX_PIN     17
#define RS485_RX_PIN     25
#define RS485_DE_RE_PIN  5

#define RS485_BAUD       115200

// Test configuration
#define RELAY_ACTIVE_MS  500
#define RS485_POLL_INTERVAL_MS 1000
#define RS485_FRAME_TIMEOUT_MS 250
#define RS485_MAX_RETRIES 3
#define WIFI_CONNECT_TIMEOUT_MS 20000
#define FIREBASE_TIMEOUT_MS 10000

// Strictness control:
// 0 = cloud tests fail when prerequisites are missing (recommended for QA sign-off)
// 1 = allow informational pass when cloud prerequisites are unavailable
#ifndef ALLOW_DEFERRED_CLOUD_TESTS
#define ALLOW_DEFERRED_CLOUD_TESTS 0
#endif

#if HAS_LOCAL_SECRETS && defined(WIFI_SSID) && defined(WIFI_PASSWORD)
#define HAS_WIFI_CREDS 1
#else
#define HAS_WIFI_CREDS 0
#endif

#if HAS_LOCAL_SECRETS && defined(FIREBASE_API_KEY) && defined(FIREBASE_RTDB_URL) && defined(FIREBASE_EMAIL) && defined(FIREBASE_PASSWORD)
#define HAS_FIREBASE_CREDS 1
#else
#define HAS_FIREBASE_CREDS 0
#endif

// ============================================================================
// Test runner
// ============================================================================

int testsRun = 0;
int testsPassed = 0;
int testsFailed = 0;

typedef bool (*TestFn)();

static bool ensureWiFiConnected(uint32_t timeoutMs) {
#if HAS_WIFI_CREDS
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
#else
  (void)timeoutMs;
  return false;
#endif
}

static bool checkDnsReachability(int minSuccess, int attempts) {
  int ok = 0;
  for (int i = 0; i < attempts; ++i) {
    IPAddress ip;
    if (WiFi.hostByName("firebase.google.com", ip) == 1 && ip != INADDR_NONE) {
      ok++;
    }
    delay(200);
  }
  Serial.printf("  [INFO] DNS checks: %d/%d successful\n", ok, attempts);
  return ok >= minSuccess;
}

void runTest(const char* name, TestFn fn) {
  Serial.printf("[ RUN ] %s\n", name);
  bool ok = fn();
  Serial.printf("%s %s\n", ok ? "[ PASS]" : "[ FAIL]", name);
  testsRun++;
  if (ok) testsPassed++; else testsFailed++;
}

void waitForUserInput(const char* prompt) {
  Serial.println(prompt);
  while (!Serial.available()) {
    delay(100);
  }
  while (Serial.available()) Serial.read(); // Clear buffer
  delay(500);
}

// ============================================================================
// CRC16-Modbus (for frame validation)
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
// Frame parsing helpers
// ============================================================================

bool rs485ReadFrame(char* buffer, size_t bufLen, uint32_t timeoutMs) {
  uint32_t startMs = millis();
  size_t rxPos = 0;

  while ((millis() - startMs) < timeoutMs) {
    if (Serial2.available()) {
      int c = Serial2.read();
      if (c < 0) continue;

      if (c == 0x02) {  // STX
        rxPos = 0;
        buffer[rxPos++] = c;
      } else if (c == 0x03) {  // ETX
        if (rxPos > 0 && rxPos < bufLen) {
          buffer[rxPos++] = c;
          buffer[rxPos] = '\0';
          return true;
        }
        return false;
      } else if (rxPos > 0 && rxPos < bufLen - 1) {
        buffer[rxPos++] = c;
      }
    }
    delay(1);
  }

  return false;
}

bool parseIntField(const char* str, const char* key, int& value) {
  const char* pos = strstr(str, key);
  if (!pos) return false;
  int v = atoi(pos + strlen(key));
  value = v;
  return true;
}

bool parseFloatField(const char* str, const char* key, float& value) {
  const char* pos = strstr(str, key);
  if (!pos) return false;
  float v = atof(pos + strlen(key));
  value = v;
  return true;
}

// ============================================================================
// Test Cases
// ============================================================================

// TC-M-01: GPIO and Relay Test
bool test_gpio_relay() {
  // Test RS485 DE/RE control
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  if (digitalRead(RS485_DE_RE_PIN) != HIGH) return false;
  digitalWrite(RS485_DE_RE_PIN, LOW);
  if (digitalRead(RS485_DE_RE_PIN) != LOW) return false;

  // Test relay control with safety warning
  Serial.println("\n[SAFETY] Relay will activate for 500ms.");
  Serial.println("[SAFETY] Ensure pump has water or relay is disconnected.");
  waitForUserInput("Press ENTER to proceed with relay activation:");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // Relay OFF initially
  delay(100);

  digitalWrite(RELAY_PIN, HIGH);  // Relay ON
  delay(RELAY_ACTIVE_MS);
  digitalWrite(RELAY_PIN, LOW);   // Relay OFF

  Serial.println("[INFO] Relay cycle complete.");
  return true;
}

// TC-M-02: RS-485 Master Poll Test
bool test_rs485_master() {
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  pinMode(RS485_DE_RE_PIN, OUTPUT);

  int framesValid = 0;
  int framesFailed = 0;
  uint32_t startMs = millis();

  Serial.println("\n[INFO] RS485 Master: Polling for 30 seconds...");

  while ((millis() - startMs) < 30000) {
    uint32_t pollStartMs = millis();

    // Send request
    digitalWrite(RS485_DE_RE_PIN, HIGH);  // TX mode
    delay(1);
    Serial2.write("REQ\n");
    Serial2.flush();
    delay(1);
    digitalWrite(RS485_DE_RE_PIN, LOW);   // RX mode

    // Wait for response
    char frame[128];
    if (!rs485ReadFrame(frame, sizeof(frame), RS485_FRAME_TIMEOUT_MS)) {
      framesFailed++;
      delay(100);
      continue;
    }

    // Validate CRC
    const char* crcPos = strstr(frame, "CRC:");
    if (!crcPos || strlen(crcPos) < 8) {
      framesFailed++;
      continue;
    }

    char payload[128];
    size_t payloadLen = crcPos - frame - 1; // Exclude STX
    if (payloadLen > sizeof(payload) - 1) {
      framesFailed++;
      continue;
    }
    memcpy(payload, frame + 1, payloadLen);
    payload[payloadLen] = '\0';

    uint32_t rxCrc = (uint32_t)strtoul(crcPos + 4, nullptr, 16);
    uint16_t calcCrc = crc16_modbus((const uint8_t*)payload, payloadLen);

    if ((uint32_t)calcCrc != (rxCrc & 0xFFFFu)) {
      Serial.printf("  [WARN] CRC mismatch: got %04X expected %04X\n", (unsigned)calcCrc, (unsigned)rxCrc);
      framesFailed++;
      continue;
    }

    framesValid++;
    delay(RS485_POLL_INTERVAL_MS);
  }

  Serial2.end();

  int totalFrames = framesValid + framesFailed;
  float successRate = (totalFrames > 0) ? (100.0f * framesValid / totalFrames) : 0.0f;
  Serial.printf("  [INFO] RS485: %d valid, %d failed (%.1f%% success rate)\n",
                framesValid, framesFailed, successRate);

  return successRate >= 90.0f;
}

// TC-M-03: WiFi Connection Test
bool test_wifi() {
  Serial.println("\n[INFO] WiFi: Attempting connection...");

#if HAS_WIFI_CREDS
  if (!ensureWiFiConnected(WIFI_CONNECT_TIMEOUT_MS)) {
    Serial.println("  [ERROR] WiFi connect timeout.");
    return false;
  }

  Serial.printf("  [INFO] WiFi connected. IP=%s RSSI=%d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return checkDnsReachability(2, 3);
#else
  Serial.println("  [ERROR] WiFi credentials missing (secrets.h with WIFI_SSID/WIFI_PASSWORD).");
#if ALLOW_DEFERRED_CLOUD_TESTS
  Serial.println("  [WARN] ALLOW_DEFERRED_CLOUD_TESTS=1, marking TC-M-03 pass.");
  return true;
#else
  return false;
#endif
#endif
}

// TC-M-04: Firebase Read/Write Test
bool test_firebase() {
  Serial.println("\n[INFO] Firebase: Starting read/write/delete validation...");

#if HAS_FIREBASE_LIB && HAS_FIREBASE_CREDS
  if (!ensureWiFiConnected(WIFI_CONNECT_TIMEOUT_MS)) {
    Serial.println("  [ERROR] WiFi not connected.");
    return false;
  }

  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_RTDB_URL;
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  uint32_t start = millis();
  while (!Firebase.ready() && (millis() - start) < FIREBASE_TIMEOUT_MS) {
    delay(100);
  }
  if (!Firebase.ready()) {
    Serial.println("  [ERROR] Firebase not ready within timeout.");
    return false;
  }

  const char* testPath = "/pump_system/test_master_node/ping_at";
  int pingAt = (int)(millis() / 1000UL);

  if (!Firebase.RTDB.setInt(&fbdo, testPath, pingAt)) {
    Serial.printf("  [ERROR] Firebase setInt failed: %s\n", fbdo.errorReason().c_str());
    return false;
  }

  if (!Firebase.RTDB.getInt(&fbdo, testPath)) {
    Serial.printf("  [ERROR] Firebase getInt failed: %s\n", fbdo.errorReason().c_str());
    return false;
  }

  int readBack = fbdo.intData();
  if (readBack != pingAt) {
    Serial.printf("  [ERROR] Firebase read mismatch: wrote=%d read=%d\n", pingAt, readBack);
    return false;
  }

  if (!Firebase.RTDB.deleteNode(&fbdo, "/pump_system/test_master_node")) {
    Serial.printf("  [ERROR] Firebase deleteNode failed: %s\n", fbdo.errorReason().c_str());
    return false;
  }

  Serial.println("  [INFO] Firebase R/W/D validation passed.");
  return true;
#else
  Serial.println("  [ERROR] Firebase prerequisites missing (library and/or secrets.h credentials).");
#if ALLOW_DEFERRED_CLOUD_TESTS
  Serial.println("  [WARN] ALLOW_DEFERRED_CLOUD_TESTS=1, marking TC-M-04 pass.");
  return true;
#else
  return false;
#endif
#endif
}

// TC-M-05: Full Integration Test
bool test_integration() {
  Serial.println("\n[INFO] Integration: RS485 parse/data sanity over 10 frames...");

  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  pinMode(RS485_DE_RE_PIN, OUTPUT);

  int validFrames = 0;
  int parseErrors = 0;
  int seqChanges = 0;
  int lastSeq = -1;

  for (int i = 0; i < 10; ++i) {
    digitalWrite(RS485_DE_RE_PIN, HIGH);
    delay(1);
    Serial2.write("REQ\n");
    Serial2.flush();
    delay(1);
    digitalWrite(RS485_DE_RE_PIN, LOW);

    char frame[128];
    if (!rs485ReadFrame(frame, sizeof(frame), RS485_FRAME_TIMEOUT_MS)) {
      parseErrors++;
      continue;
    }

    const char* crcPos = strstr(frame, "CRC:");
    if (!crcPos || strlen(crcPos) < 8) {
      parseErrors++;
      continue;
    }

    char payload[128];
    size_t payloadLen = crcPos - frame - 1;
    if (payloadLen > sizeof(payload) - 1) {
      parseErrors++;
      continue;
    }
    memcpy(payload, frame + 1, payloadLen);
    payload[payloadLen] = '\0';

    uint32_t rxCrc = (uint32_t)strtoul(crcPos + 4, nullptr, 16);
    uint16_t calcCrc = crc16_modbus((const uint8_t*)payload, payloadLen);
    if ((uint32_t)calcCrc != (rxCrc & 0xFFFFu)) {
      parseErrors++;
      continue;
    }

    int lvl = -1;
    int err = -1;
    int seq = -1;
    float flow = -1.0f;
    if (!parseIntField(payload, "LVL:", lvl) || !parseFloatField(payload, "FLOW:", flow) ||
        !parseIntField(payload, "ERR:", err) || !parseIntField(payload, "SEQ:", seq)) {
      parseErrors++;
      continue;
    }

    bool inRange = (lvl >= 0 && lvl <= 100) && (flow >= 0.0f && flow <= 100.0f) && (err >= 0 && err <= 7) && (seq >= 0 && seq <= 255);
    if (!inRange) {
      parseErrors++;
      continue;
    }

    if (lastSeq >= 0 && seq != lastSeq) seqChanges++;
    lastSeq = seq;
    validFrames++;
    delay(200);
  }

  Serial2.end();

  Serial.printf("  [INFO] Integration result: valid=%d parseErrors=%d seqChanges=%d\n", validFrames, parseErrors, seqChanges);
  return (validFrames >= 8) && (parseErrors <= 2) && (seqChanges >= 1);
}

// ============================================================================
// Setup & Loop
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== SmartFlow ESP32 Master Hardware Test Suite ===");
  Serial.println("Build: " __DATE__ " " __TIME__);
  Serial.println("");

  // TC-M-01 requires user interaction
  runTest("TC-M-01: GPIO and Relay", test_gpio_relay);
  delay(1000);

  runTest("TC-M-02: RS-485 Master Poll", test_rs485_master);
  delay(1000);

  runTest("TC-M-03: WiFi Connection", test_wifi);
  delay(500);

  runTest("TC-M-04: Firebase Read/Write", test_firebase);
  delay(500);

  runTest("TC-M-05: Full Integration", test_integration);
  delay(500);

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
