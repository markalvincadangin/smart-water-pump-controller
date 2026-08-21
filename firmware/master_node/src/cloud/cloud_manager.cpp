/**
 * @file cloud_manager.cpp
 * @brief Manages cloud connectivity and data synchronization (Firebase RTDB).
 */
#include "cloud_manager.h"
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>
#include <mbedtls/md.h>


#include "../config/config.h"
#include "../core/lifecycle/bootloader.h"
#include "../network/ble_provisioning.h"
#include "../persistence/persistence.h"
#include "../safety/safety_pump.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include "device_shadow.h"


namespace {
FirebaseData fbdo_cloud;
FirebaseData fbdo_stream;
FirebaseAuth auth_cloud;
FirebaseConfig config_cloud;

// Caches & States
bool streamStarted = false;
float lastWaterLevel = -1.0;
float lastFlowRate = -1.0;
float lastUltrasonic = -1.0;
String lastLifecycleStr = "";
String lastShadowReported = "";
String deviceId;
String deviceBasePath; // Cached base path: /devices/<deviceId>

unsigned long lastSyncMs = 0;
bool metadataPublished = false;
bool firebaseStarted = false;
unsigned long lastBootstrapAttemptMs = 0;

String pendingPairingProof;
String pendingPairingPurpose;
uint32_t pendingPairingLifetimeMs = 0;
unsigned long pendingPairingQueuedAtMs = 0;
bool pairingVerifierPublished = false;
String activeOwnershipPairingRequestId;

/**
 * Deferred cloud-event queue.
 *
 * pushCloudEvent() can be called from deep inside setPump(), which sits well
 * below executeLogic() -> checkSafetyCutoff() -> setPump() -> logCloudEvent().
 * Issuing a synchronous Firebase.RTDB.pushJSON() at that depth can starve the
 * ESP32 interrupt watchdog (INT_WDT, 300 ms budget).  Instead, we enqueue the
 * event here and flush exactly one entry per sync() iteration at the top of
 * the main loop where the call stack is shallow.
 */
struct CloudEvent {
  String level;
  String component;
  String code;
  String details;
  bool   used = false;
};

constexpr uint8_t EVENT_QUEUE_SIZE = 8;
CloudEvent eventQueue[EVENT_QUEUE_SIZE];
uint8_t    eventQueueHead  = 0; ///< Next write slot
uint8_t    eventQueueTail  = 0; ///< Next flush slot
uint8_t    eventQueueCount = 0; ///< Entries pending

constexpr unsigned long BOOTSTRAP_RETRY_MS = 30000UL;
constexpr time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01 UTC

constexpr unsigned long SYNC_INTERVAL_ACTIVE_MS = 3000UL;
constexpr unsigned long SYNC_INTERVAL_IDLE_MS = 15000UL;
constexpr unsigned long DIAGNOSTICS_INTERVAL_MS = 300000UL; // 5 minutes

/**
 * @brief Check if a string is a valid configuration value.
 */
bool isConfigured(const char *value) {
  return value != nullptr && value[0] != '\0' &&
         String(value).indexOf("replace_") < 0 &&
         String(value).indexOf("your-project") < 0;
}

/**
 * @brief Create a random 16-byte hex string (32 characters).
 */
String makeNonce() {
  char nonce[33];
  for (size_t i = 0; i < 16; ++i) {
    snprintf(nonce + i * 2, 3, "%02x",
             static_cast<unsigned>(esp_random() & 0xFF));
  }
  return String(nonce);
}

/**
 * @brief Compute an HMAC-SHA256 signature using the device bootstrap secret.
 */
bool hmacSha256Hex(const String &message, String &proof) {
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr)
    return false;

  unsigned char digest[32];
  if (mbedtls_md_hmac(
          info,
          reinterpret_cast<const unsigned char *>(DEVICE_BOOTSTRAP_SECRET),
          strlen(DEVICE_BOOTSTRAP_SECRET),
          reinterpret_cast<const unsigned char *>(message.c_str()),
          message.length(), digest) != 0) {
    return false;
  }

  char hex[65];
  for (size_t i = 0; i < sizeof(digest); ++i) {
    snprintf(hex + i * 2, 3, "%02x", digest[i]);
  }
  proof = String(hex);
  return true;
}

/**
 * @brief Compute a standard SHA256 hash.
 */
bool sha256Hex(const String &value, String &hash) {
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr)
    return false;

  unsigned char digest[32];
  if (mbedtls_md(info, reinterpret_cast<const unsigned char *>(value.c_str()),
                 value.length(), digest) != 0) {
    return false;
  }

  char hex[65];
  for (size_t i = 0; i < sizeof(digest); ++i) {
    snprintf(hex + i * 2, 3, "%02x", digest[i]);
  }
  hash = String(hex);
  return true;
}

/**
 * @brief Generate an absolute path using the cached device root.
 */
String getPath(const char *suffix) {
  String path;
  path.reserve(deviceBasePath.length() + strlen(suffix) + 1);
  path = deviceBasePath;
  path += suffix;
  return path;
}

/**
 * @brief Attempt to publish a queued pairing verifier to RTDB.
 */
void flushPairingVerifier() {
  if (pendingPairingProof.length() == 0)
    return;

  // A BLE-local proof must never become valid after its original pairing
  // interval simply because Wi-Fi or cloud bootstrap was delayed.
  if (static_cast<unsigned long>(millis() - pendingPairingQueuedAtMs) >=
      pendingPairingLifetimeMs) {
    pendingPairingProof = "";
    pendingPairingPurpose = "";
    pendingPairingLifetimeMs = 0;
    pendingPairingQueuedAtMs = 0;
    pairingVerifierPublished = false;
    LOG(APP_LOG_LEVEL_WARN, "CLOUD",
        "Expired unpublished pairing verifier discarded");
    return;
  }

  if (!Firebase.ready())
    return;

  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH)
    return;

  String proofHash;
  if (!sha256Hex(pendingPairingProof, proofHash)) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Pairing verifier hash failed");
    return;
  }

  FirebaseJson verifier;
  verifier.set("proofHash", proofHash);
  verifier.set("purpose", pendingPairingPurpose);
  verifier.set("expiresAtMs", static_cast<uint64_t>(epoch) * 1000ULL +
                                  pendingPairingLifetimeMs);
  verifier.set("issuedAtMs", static_cast<uint64_t>(epoch) * 1000ULL);

  String path = getPath("/pairing/current");
  if (!Firebase.RTDB.setJSON(&fbdo_cloud, path.c_str(), &verifier)) {
    LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Pairing verifier publish deferred: %s",
        fbdo_cloud.errorReason().c_str());
    return;
  }

  pendingPairingProof = "";
  pendingPairingPurpose = "";
  pendingPairingLifetimeMs = 0;
  pendingPairingQueuedAtMs = 0;
  pairingVerifierPublished = true;
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Pairing verifier published");
}
} // end anonymous namespace

void CloudManager::init() {
  metadataPublished = false;
  lastSyncMs = 0;
  streamStarted = false;
  firebaseStarted = false;
  lastBootstrapAttemptMs = 0;

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  deviceId = "SF-" + mac.substring(6);
  deviceBasePath = "/devices/" + deviceId;

  config_cloud.api_key = API_KEY;
  config_cloud.database_url = DATABASE_URL;
  config_cloud.token_status_callback = tokenStatusCallback;

  LOG(APP_LOG_LEVEL_INFO, "CLOUD",
      "Bootstrap authentication pending for device: %s", deviceId.c_str());
}

void CloudManager::sync() {
  if (!firebaseStarted) {
    const unsigned long now = millis();
    if (lastBootstrapAttemptMs == 0 ||
        now - lastBootstrapAttemptMs >= BOOTSTRAP_RETRY_MS) {
      lastBootstrapAttemptMs = now;
      (void)bootstrapAndStartFirebase();
    }
    return;
  }

  if (!Firebase.ready())
    return;

  flushPairingVerifier();

  // Flush one queued cloud event per loop iteration.
  // Events are enqueued by pushCloudEvent() and flushed here to keep
  // Firebase calls away from deep pump-control call stacks.
  if (eventQueueCount > 0) {
    CloudEvent& ev = eventQueue[eventQueueTail];
    if (ev.used) {
      uint64_t timestamp = millis();
      if (ntpSynced) {
        timestamp = (static_cast<uint64_t>(ntpEpochSecAtLastSync) * 1000ULL) +
                    (millis() - ntpLastSyncMs);
      }
      String      eventsPath = getPath("/events");
      FirebaseJson evJson;
      evJson.set("timestamp", (double)timestamp);
      evJson.set("severity",  ev.level);
      evJson.set("category",  ev.component);
      evJson.set("code",      ev.code);
      evJson.set("message",   ev.details);
      Firebase.RTDB.pushJSON(&fbdo_cloud, eventsPath.c_str(), &evJson);
      ev.used = false;
    }
    eventQueueTail = (eventQueueTail + 1) % EVENT_QUEUE_SIZE;
    eventQueueCount--;
  }

  // Publish identity before normal telemetry.
  if (!metadataPublished) {
    metadataPublished = pushMetadata();
  }

  unsigned long now = millis();

  if (!streamStarted) {
    String desiredPath = getPath("/shadow/desired");
    if (Firebase.RTDB.beginStream(&fbdo_stream, desiredPath.c_str())) {
      streamStarted = true;
      LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Shadow stream started");
    }
  } else {
    if (Firebase.RTDB.readStream(&fbdo_stream)) {
      if (fbdo_stream.streamAvailable()) {
        if (fbdo_stream.dataType() == "json" && fbdo_stream.dataPath() == "/") {
          FirebaseJson* jsonPtr = fbdo_stream.to<FirebaseJson *>();
          if (!jsonPtr) return;
          
          FirebaseJson& json = *jsonPtr;
          String payloadStr;
          json.toString(payloadStr, false);

          LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Stream event: path=%s, type=%s, payload=%s",
              fbdo_stream.dataPath().c_str(), fbdo_stream.dataType().c_str(), payloadStr.c_str());

          FirebaseJsonData jd;

          String mode = "";
          bool manual_desired = false;
          bool countdown_start = false;
          int countdown_duration_min = 0;
          bool emergency_stop = false;
          bool reset_stop = false;
          bool clear_error = false;
          bool bypass_level_sensor = false;
          bool bypass_flow_sensor = false;
          bool reboot_device = false;

          json.get(jd, "mode");
          if (!jd.success) {
            LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Partial update received via stream, triggering full readShadow()");
            readShadow();
            return;
          }
          mode = jd.stringValue;
          
          json.get(jd, "manual_desired");
          if (jd.success)
            manual_desired = jd.boolValue;
          json.get(jd, "countdown_start");
          if (jd.success)
            countdown_start = jd.boolValue;
          json.get(jd, "countdown_duration_min");
          if (jd.success)
            countdown_duration_min = jd.intValue;
          json.get(jd, "emergency_stop");
          if (jd.success)
            emergency_stop = jd.boolValue;
          json.get(jd, "reset_stop");
          if (jd.success)
            reset_stop = jd.boolValue;
          json.get(jd, "clear_error");
          if (jd.success)
            clear_error = jd.boolValue;
          json.get(jd, "bypass_level_sensor");
          if (jd.success)
            bypass_level_sensor = jd.boolValue;
          json.get(jd, "bypass_flow_sensor");
          if (jd.success)
            bypass_flow_sensor = jd.boolValue;
          json.get(jd, "reboot_device");
          if (jd.success)
            reboot_device = jd.boolValue;

          DeviceShadow::evaluateDesired(
              mode, manual_desired, countdown_start, countdown_duration_min,
              emergency_stop, reset_stop, clear_error, bypass_level_sensor,
              bypass_flow_sensor, reboot_device);
        } else if (fbdo_stream.dataPath() != "/") {
          LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Stream event: path=%s, type=%s, stringData=%s",
              fbdo_stream.dataPath().c_str(), fbdo_stream.dataType().c_str(), fbdo_stream.stringData().c_str());
        } else {
          // Partial update fallback
          LOG(APP_LOG_LEVEL_WARN, "CLOUD",
              "Partial update received, performing full readShadow()");
          readShadow();
        }
      }
    }
    if (fbdo_stream.streamTimeout()) {
      LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Stream timeout, reconnecting...");
      streamStarted = false;
    }
  }

  if (forceCloudManualOverride && Firebase.ready()) {
    setErrorFallbackDesiredState();
    forceCloudManualOverride = false;
  }

  // Adaptive interval: 3s if pump is running or counting down, else 15s.
  bool isActive = isRunning || (pumpMode == "COUNTDOWN");
  unsigned long intervalMs =
      isActive ? SYNC_INTERVAL_ACTIVE_MS : SYNC_INTERVAL_IDLE_MS;

  if (now - lastSyncMs >= intervalMs) {
    lastSyncMs = now;
    readSettings();
    processWifiReprovisionRequest();
    processOwnershipPairingRequest();
    pushTelemetry();
    pushStatus();
    pushShadow();
    pushDiagnostics();
  }
}

void CloudManager::processWifiReprovisionRequest() {
  String maintenancePath = getPath("/maintenance");
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, maintenancePath.c_str()))
    return;

  FirebaseJson maintenance = fbdo_cloud.to<FirebaseJson>();
  FirebaseJsonData value;
  maintenance.get(value, "activeWifiReprovisionRequestId");
  const String requestId = value.success ? value.stringValue : "";
  if (requestId.length() == 0)
    return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, true))
    return;
  const String processedRequestId = prefs.getString("reprov_req_id", "");
  prefs.end();

  if (processedRequestId == requestId)
    return;

  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH) {
    LOG(APP_LOG_LEVEL_WARN, "REPROVISION",
        "Wi-Fi recovery request deferred until NTP time is available.");
    return;
  }

  String requestPath = maintenancePath + "/requests/" + requestId;
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, requestPath.c_str()))
    return;

  FirebaseJson request = fbdo_cloud.to<FirebaseJson>();
  request.get(value, "action");
  const String action = value.success ? value.stringValue : "";
  request.get(value, "status");
  const String status = value.success ? value.stringValue : "";
  request.get(value, "nonce");
  const String nonce = value.success ? value.stringValue : "";
  request.get(value, "expiresAtMs");
  const uint64_t expiresAtMs =
      value.success ? static_cast<uint64_t>(value.doubleValue) : 0;
  const uint64_t nowMs = static_cast<uint64_t>(epoch) * 1000ULL;

  if (action != "WIFI_REPROVISION" || status != "pending" ||
      nonce.length() < 24 || expiresAtMs == 0 || expiresAtMs <= nowMs) {
    LOG(APP_LOG_LEVEL_WARN, "REPROVISION",
        "Ignoring invalid, expired, or completed Wi-Fi recovery request.");
    return;
  }

  // Bootloader applies setPump(false) before it changes any enrollment data.
  // It then clears only Wi-Fi/device-auth enrollment and restarts into BLE.
  Bootloader::applyWifiReprovisionRequest(requestId.c_str());
}

void CloudManager::processOwnershipPairingRequest() {
  String ownershipPath = getPath("/ownership");
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, ownershipPath.c_str()))
    return;

  FirebaseJson ownership = fbdo_cloud.to<FirebaseJson>();
  FirebaseJsonData value;
  ownership.get(value, "ownershipPairingRequestId");
  const String requestId = value.success ? value.stringValue : "";

  ownership.get(value, "ownershipPairingExpiresAtMs");
  const uint64_t expiresAtMs =
      value.success ? static_cast<uint64_t>(value.doubleValue) : 0;

  ownership.get(value, "state");
  const String state = value.success ? value.stringValue : "";

  if (activeOwnershipPairingRequestId.length() > 0 &&
      (requestId != activeOwnershipPairingRequestId ||
       (state != "transfer_pending" && state != "release_pending"))) {
    BleProvisioning::stopOwnershipPairing();
    activeOwnershipPairingRequestId = "";
    return;
  }

  if (requestId.length() == 0 || expiresAtMs == 0 ||
      (state != "transfer_pending" && state != "release_pending"))
    return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, true))
    return;
  const String processedRequestId = prefs.getString("own_pair_req", "");
  prefs.end();

  if (processedRequestId == requestId)
    return;

  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH) {
    LOG(APP_LOG_LEVEL_WARN, "OWNERSHIP",
        "Pairing request deferred until NTP time is available.");
    return;
  }

  const uint64_t nowMs = static_cast<uint64_t>(epoch) * 1000ULL;
  if (expiresAtMs <= nowMs) {
    if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
      prefs.putString("own_pair_req", requestId);
      prefs.end();
    }
    LOG(APP_LOG_LEVEL_WARN, "OWNERSHIP", "Expired pairing request ignored.");
    return;
  }

  String requestPath = getPath("/maintenance/requests/") + requestId;
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, requestPath.c_str()))
    return;

  FirebaseJson request = fbdo_cloud.to<FirebaseJson>();
  request.get(value, "action");
  const String action = value.success ? value.stringValue : "";

  request.get(value, "purpose");
  const String purpose = value.success ? value.stringValue : "";

  request.get(value, "status");
  const String status = value.success ? value.stringValue : "";

  if (action != "OWNERSHIP_PAIRING" || status != "pending" ||
      (purpose != "transfer" && purpose != "release")) {
    return;
  }

  const uint64_t remaining64 = expiresAtMs - nowMs;
  const uint32_t remainingMs = remaining64 > UINT32_MAX
                                   ? UINT32_MAX
                                   : static_cast<uint32_t>(remaining64);

  // Pump OFF is the first state-changing action for a remotely requested
  // ownership pairing session. Failure to start BLE leaves it OFF.
  setPump(false);

  if (!BleProvisioning::startOwnershipPairing(purpose, remainingMs)) {
    LOG(APP_LOG_LEVEL_ERROR, "OWNERSHIP",
        "Unable to start temporary ownership pairing BLE.");
    return;
  }

  if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
    prefs.putString("own_pair_req", requestId);
    prefs.end();
  }
  activeOwnershipPairingRequestId = requestId;
  LOG(APP_LOG_LEVEL_WARN, "OWNERSHIP",
      "Temporary %s pairing active for request %s.", purpose.c_str(),
      requestId.c_str());
}

void CloudManager::queuePairingVerifier(const String &rawProof,
                                        const String &purpose,
                                        uint32_t lifetimeMs) {
  if (rawProof.length() == 0 || rawProof.length() > 256 ||
      (purpose != "claim" && purpose != "transfer" && purpose != "release")) {
    LOG(APP_LOG_LEVEL_WARN, "CLOUD",
        "Rejected invalid pairing verifier request");
    return;
  }
  pendingPairingProof = rawProof;
  pendingPairingPurpose = purpose;
  pendingPairingLifetimeMs = lifetimeMs;
  pendingPairingQueuedAtMs = millis();
  pairingVerifierPublished = false;
}

bool CloudManager::isAuthenticated() {
  // The Firebase Arduino client's custom-token endpoint returns an ID token
  // but does not populate auth_cloud.token.uid for this flow. Firebase.ready()
  // therefore represents the usable authenticated session.
  return firebaseStarted && Firebase.ready();
}

bool CloudManager::isPairingVerifierPublished() {
  return pairingVerifierPublished;
}

bool CloudManager::bootstrapAndStartFirebase() {
  if (WiFi.status() != WL_CONNECTED)
    return false;

  const String rootCa = DEVICE_BOOTSTRAP_ROOT_CA;
  if (!isConfigured(DEVICE_BOOTSTRAP_SECRET) ||
      !isConfigured(DEVICE_BOOTSTRAP_URL) ||
      !rootCa.startsWith("\n-----BEGIN CERTIFICATE-----") ||
      rootCa.indexOf("replace_") >= 0) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD",
        "Bootstrap configuration is missing; cloud access remains disabled");
    return false;
  }

  const time_t nowEpoch = time(nullptr);
  if (nowEpoch < MIN_VALID_EPOCH) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    LOG(APP_LOG_LEVEL_WARN, "CLOUD",
        "Bootstrap deferred until NTP time is available");
    return false;
  }

  const String nonce = makeNonce();
  const uint64_t timestampMs = static_cast<uint64_t>(nowEpoch) * 1000ULL;
  char timestampText[24];
  snprintf(timestampText, sizeof(timestampText), "%llu",
           static_cast<unsigned long long>(timestampMs));

  const String canonical = deviceId + "." + String(timestampText) + "." + nonce;
  String proof;
  if (!hmacSha256Hex(canonical, proof)) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Unable to create bootstrap proof");
    return false;
  }

  StaticJsonDocument<384> request;
  request["deviceId"] = deviceId;
  request["timestampMs"] = timestampMs;
  request["nonce"] = nonce;
  request["proof"] = proof;

  String payload;
  serializeJson(request, payload);

  WiFiClientSecure client;
  client.setCACert(rootCa.c_str());
  HTTPClient http;
  http.setTimeout(10000);

  if (!http.begin(client, DEVICE_BOOTSTRAP_URL)) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap HTTPS client setup failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  const int httpCode = http.POST(payload);
  const String response = http.getString();
  http.end();

  if (httpCode != HTTP_CODE_OK) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap rejected (%d): %s", httpCode,
        response.c_str());
    return false;
  }

  StaticJsonDocument<1600> result;
  if (deserializeJson(result, response) != DeserializationError::Ok) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap response was not valid JSON");
    return false;
  }

  const char *customToken = result["customToken"] | "";
  const char *returnedUid = result["deviceUid"] | "";
  const String expectedUid = "device:" + deviceId;

  if (String(customToken).isEmpty() || String(returnedUid) != expectedUid) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD",
        "Bootstrap response did not contain this device identity");
    return false;
  }

  Firebase.setCustomToken(&config_cloud, customToken);
  Firebase.begin(&config_cloud, &auth_cloud);
  Firebase.reconnectWiFi(true);
  Firebase.RTDB.setReadTimeout(&fbdo_cloud, 10000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo_cloud, "medium");

  firebaseStarted = true;
  metadataPublished = false;
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Custom-token sign-in started for %s",
      expectedUid.c_str());
  return true;
}

bool CloudManager::pushMetadata() {
  String path = getPath("/metadata");
  const String expectedUid = "device:" + deviceId;

  if (!isAuthenticated()) {
    LOG(APP_LOG_LEVEL_WARN, "CLOUD",
        "Metadata deferred: custom-token session is not ready");
    return false;
  }

  FirebaseJson json;
  json.set("firmwareVersion", "2.0.0");
  json.set("hardwareVersion", "ESP32-WROOM-32");
  json.set("protocolVersion", "1.0");
  json.set("serialNumber", deviceId);
  json.set("deviceAuthUid", expectedUid.c_str());

  if (!Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json)) {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Metadata publish failed: %s",
        fbdo_cloud.errorReason().c_str());
    return false;
  }

  LOG(APP_LOG_LEVEL_INFO, "CLOUD",
      "Metadata published for %s (firmware UID: %s)", deviceId.c_str(),
      expectedUid.c_str());
  return true;
}

void CloudManager::readSettings() {
  String path = getPath("/settings");
  if (Firebase.RTDB.getJSON(&fbdo_cloud, path.c_str())) {
    FirebaseJson json = fbdo_cloud.to<FirebaseJson>();
    FirebaseJsonData jd;

    json.get(jd, "pump_start_level_pct");
    if (jd.success)
      cfgPumpStartLevel = jd.intValue;

    json.get(jd, "pump_stop_level_pct");
    if (jd.success)
      cfgPumpStopLevel = jd.intValue;

    json.get(jd, "dry_run_threshold_lpm");
    if (jd.success)
      cfgDryRunThresholdLpm = jd.floatValue;

    json.get(jd, "max_pump_runtime_min");
    if (jd.success)
      cfgMaxPumpRuntimeMin = jd.intValue;
  }
}

void CloudManager::pushDiagnostics() {
  static unsigned long lastDiagPush = 0;
  if (millis() - lastDiagPush < DIAGNOSTICS_INTERVAL_MS) {
    return;
  }

  String path = getPath("/diagnostics");
  FirebaseJson json;

  json.set("freeHeap", ESP.getFreeHeap());
  json.set("wifiRSSI", WiFi.RSSI());
  json.set("restartReason", Bootloader::getBootReasonString());

  if (Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json)) {
    lastDiagPush = millis();
  }
}

void CloudManager::pushCloudEvent(const String& level, const String& component,
                                  const String& code,  const String& details) {
  // Enqueue only — do NOT call Firebase here.
  // This function may be called from within setPump(), deep inside the pump
  // control stack.  A synchronous pushJSON() at that depth exceeds the ESP32
  // interrupt-watchdog budget (INT_WDT, 300 ms).  sync() drains the queue.
  if (level != "INFO" && level != "WARN" && level != "ERROR")
    return;
  if (eventQueueCount >= EVENT_QUEUE_SIZE)
    return; // Queue full; oldest unread event is preserved.

  CloudEvent& slot = eventQueue[eventQueueHead];
  slot.level     = level;
  slot.component = component;
  slot.code      = code;
  slot.details   = details;
  slot.used      = true;
  eventQueueHead  = (eventQueueHead + 1) % EVENT_QUEUE_SIZE;
  eventQueueCount++;
}

void CloudManager::readShadow() {
  String path = getPath("/shadow/desired");
  if (Firebase.RTDB.getJSON(&fbdo_cloud, path.c_str())) {
    FirebaseJson json = fbdo_cloud.to<FirebaseJson>();
    FirebaseJsonData jd;

    String mode = "";
    bool manual_desired = false;
    bool countdown_start = false;
    int countdown_duration_min = 0;
    bool emergency_stop = false;
    bool reset_stop = false;
    bool clear_error = false;
    bool bypass_level_sensor = false;
    bool bypass_flow_sensor = false;
    bool reboot_device = false;

    json.get(jd, "mode");
    if (jd.success)
      mode = jd.stringValue;
    json.get(jd, "manual_desired");
    if (jd.success)
      manual_desired = jd.boolValue;
    json.get(jd, "countdown_start");
    if (jd.success)
      countdown_start = jd.boolValue;
    json.get(jd, "countdown_duration_min");
    if (jd.success)
      countdown_duration_min = jd.intValue;
    json.get(jd, "emergency_stop");
    if (jd.success)
      emergency_stop = jd.boolValue;
    json.get(jd, "reset_stop");
    if (jd.success)
      reset_stop = jd.boolValue;
    json.get(jd, "clear_error");
    if (jd.success)
      clear_error = jd.boolValue;
    json.get(jd, "bypass_level_sensor");
    if (jd.success)
      bypass_level_sensor = jd.boolValue;
    json.get(jd, "bypass_flow_sensor");
    if (jd.success)
      bypass_flow_sensor = jd.boolValue;
    json.get(jd, "reboot_device");
    if (jd.success)
      reboot_device = jd.boolValue;

    DeviceShadow::evaluateDesired(mode, manual_desired, countdown_start,
                                  countdown_duration_min, emergency_stop,
                                  reset_stop, clear_error, bypass_level_sensor,
                                  bypass_flow_sensor, reboot_device);
  }
}

void CloudManager::pushTelemetry() {
  float currentUltrasonic = (float)(ultrasonicLastGoodCmX10 / 10.0);

  if (abs(waterLevelPct - lastWaterLevel) < 1.0 &&
      abs(flowRateLpm - lastFlowRate) < 0.5 &&
      abs(currentUltrasonic - lastUltrasonic) < 1.0) {
    return; // No significant change
  }

  String path = getPath("/telemetry");
  FirebaseJson json;

  json.set("water_level_percent", waterLevelPct);
  json.set("flow_rate_lpm", flowRateLpm);
  json.set("ultrasonic_last_good_cm", (double)currentUltrasonic);

  if (Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json)) {
    lastWaterLevel = waterLevelPct;
    lastFlowRate = flowRateLpm;
    lastUltrasonic = currentUltrasonic;
  }
}

void CloudManager::pushStatus() {
  String currentLifecycle =
      deviceLifecycle == DeviceLifecycle::ONLINE ? "ONLINE" : "OFFLINE";

  // Always push status at least every 15s to ensure heartbeat, or if lifecycle
  // changes
  static unsigned long lastPushTime = 0;
  if (currentLifecycle == lastLifecycleStr &&
      (millis() - lastPushTime < 15000)) {
    return;
  }

  String path = getPath("/status");
  FirebaseJson json;

  json.set("lifecycle", currentLifecycle.c_str());
  json.set("uptimeSeconds", (int)(esp_timer_get_time() / 1000000ULL));
  json.set("firmwareVersion", "2.0.0");

  if (Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json)) {
    lastLifecycleStr = currentLifecycle;
    lastPushTime = millis();
  }
}

void CloudManager::clearCountdownDesiredState() {
  if (!Firebase.ready())
    return;

  // Only clear the countdown_start flag to prevent re-triggering on reboot.
  // Do NOT change the mode here — the ESP32's own stream listener would
  // receive the mode change and interpret it as a user intent (STOP_MANUAL),
  // immediately aborting the active countdown. The mode reverts to MANUAL
  // naturally when handleStateTransitions() detects the timer has expired.
  String path = getPath("/shadow/desired");
  FirebaseJson update;
  update.set("countdown_start", false);

  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  LOG(APP_LOG_LEVEL_INFO, "CLOUD",
      "Cleared countdown_start flag from desired shadow.");
}

bool CloudManager::clearRebootDesiredState() {
  if (!Firebase.ready())
    return false;

  String path = getPath("/shadow/desired");
  FirebaseJson update;
  update.set("reboot_device", false);

  bool success = Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  if (success) {
    LOG(APP_LOG_LEVEL_INFO, "CLOUD",
        "Cleared reboot flag from desired shadow.");
  } else {
    LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Failed to clear reboot flag: %s",
        fbdo_cloud.errorReason().c_str());
  }
  return success;
}

void CloudManager::setErrorFallbackDesiredState() {
  if (!Firebase.ready())
    return;

  String path = getPath("/shadow/desired");
  FirebaseJson update;
  update.set("mode", "MANUAL");
  update.set("manual_desired", false);
  update.set("countdown_start", false);

  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  LOG(APP_LOG_LEVEL_INFO, "CLOUD",
      "Forced desired shadow to MANUAL OFF due to safety trip.");
}

void CloudManager::clearErrorDesiredState() {
  if (!Firebase.ready()) return;
  String path = getPath("/shadow/desired");
  FirebaseJson update;
  update.set("clear_error", false);
  update.set("reset_stop", false);
  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Reset clear_error and reset_stop flags in desired shadow.");
}

void CloudManager::pushShadow() {
  String reportedStr = DeviceShadow::getReportedJson();
  if (reportedStr == lastShadowReported) {
    return; // No change
  }

  String path = getPath("/shadow");
  FirebaseJson reportedJson;
  reportedJson.setJsonData(reportedStr);

  FirebaseJson update;
  update.set("reported", reportedJson);

  if (Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update)) {
    lastShadowReported = reportedStr;
  }
}
