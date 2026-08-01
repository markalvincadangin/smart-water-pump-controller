/**
 * @file cloud_manager.cpp
 * @brief Manages cloud connectivity and data synchronization (Firebase RTDB).
 */
#include "cloud_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <mbedtls/md.h>

#include "../config/config.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include "device_shadow.h"
#include "../core/lifecycle/bootloader.h"
#include "../network/ble_provisioning.h"
#include "../safety/safety_pump.h"
#include "../persistence/persistence.h"

static FirebaseData fbdo_cloud;
static FirebaseAuth auth_cloud;
static FirebaseConfig config_cloud;
static String deviceId;
static unsigned long lastSyncMs = 0;
static bool metadataPublished = false;
static bool firebaseStarted = false;
static unsigned long lastBootstrapAttemptMs = 0;
static String pendingPairingProof;
static String pendingPairingPurpose;
static uint32_t pendingPairingLifetimeMs = 0;
static unsigned long pendingPairingQueuedAtMs = 0;
static bool pairingVerifierPublished = false;
static String activeOwnershipPairingRequestId;

static constexpr unsigned long BOOTSTRAP_RETRY_MS = 30000UL;
static constexpr time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01 UTC

static bool isConfigured(const char* value) {
  return value != nullptr && value[0] != '\0' && String(value).indexOf("replace_") < 0 && String(value).indexOf("your-project") < 0;
}

static String makeNonce() {
  char nonce[33];
  for (size_t i = 0; i < 16; ++i) {
  snprintf(nonce + i * 2, 3, "%02x", static_cast<unsigned>(esp_random() & 0xFF));
  }
  return String(nonce);
}

static bool hmacSha256Hex(const String& message, String& proof) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr) return false;
  unsigned char digest[32];
  if (mbedtls_md_hmac(info,
      reinterpret_cast<const unsigned char*>(DEVICE_BOOTSTRAP_SECRET), strlen(DEVICE_BOOTSTRAP_SECRET),
      reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), digest) != 0) {
  return false;
  }
  char hex[65];
  for (size_t i = 0; i < sizeof(digest); ++i) snprintf(hex + i * 2, 3, "%02x", digest[i]);
  proof = String(hex);
  return true;
}

static bool sha256Hex(const String& value, String& hash) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr) return false;
  unsigned char digest[32];
  if (mbedtls_md(info, reinterpret_cast<const unsigned char*>(value.c_str()), value.length(), digest) != 0) return false;
  char hex[65];
  for (size_t i = 0; i < sizeof(digest); ++i) snprintf(hex + i * 2, 3, "%02x", digest[i]);
  hash = String(hex);
  return true;
}

static void flushPairingVerifier() {
  if (pendingPairingProof.length() == 0) return;
  // A BLE-local proof must never become valid after its original pairing
  // interval simply because Wi-Fi or cloud bootstrap was delayed.
  if (static_cast<unsigned long>(millis() - pendingPairingQueuedAtMs) >= pendingPairingLifetimeMs) {
  pendingPairingProof = "";
  pendingPairingPurpose = "";
  pendingPairingLifetimeMs = 0;
  pendingPairingQueuedAtMs = 0;
  pairingVerifierPublished = false;
  LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Expired unpublished pairing verifier discarded");
  return;
  }
  if (!Firebase.ready()) return;
  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH) return;
  String proofHash;
  if (!sha256Hex(pendingPairingProof, proofHash)) {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Pairing verifier hash failed");
  return;
  }
  FirebaseJson verifier;
  verifier.set("proofHash", proofHash);
  verifier.set("purpose", pendingPairingPurpose);
  verifier.set("expiresAtMs", static_cast<uint64_t>(epoch) * 1000ULL + pendingPairingLifetimeMs);
  verifier.set("issuedAtMs", static_cast<uint64_t>(epoch) * 1000ULL);
  String path = "/devices/" + deviceId + "/pairing/current";
  if (!Firebase.RTDB.setJSON(&fbdo_cloud, path.c_str(), &verifier)) {
  LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Pairing verifier publish deferred: %s", fbdo_cloud.errorReason().c_str());
  return;
  }
  pendingPairingProof = "";
  pendingPairingPurpose = "";
  pendingPairingLifetimeMs = 0;
  pendingPairingQueuedAtMs = 0;
  pairingVerifierPublished = true;
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Pairing verifier published");
}

void CloudManager::init() {
  metadataPublished = false;
  lastSyncMs = 0;
  firebaseStarted = false;
  lastBootstrapAttemptMs = 0;
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();
  deviceId = "SF-" + mac.substring(6);
  
  config_cloud.api_key      = API_KEY;
  config_cloud.database_url = DATABASE_URL;

  config_cloud.token_status_callback = tokenStatusCallback;

  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Bootstrap authentication pending for device: %s", deviceId.c_str());
}

void CloudManager::sync() {
  if (!firebaseStarted) {
  const unsigned long now = millis();
  if (lastBootstrapAttemptMs == 0 || now - lastBootstrapAttemptMs >= BOOTSTRAP_RETRY_MS) {
    lastBootstrapAttemptMs = now;
    bootstrapAndStartFirebase();
  }
  return;
  }
  if (!Firebase.ready()) return;

  flushPairingVerifier();

  // Publish identity before normal telemetry. The previous implementation
  // compared an arbitrary millis() timestamp to the interval constant, so
  // this almost never ran and RTDB remained empty.
  if (!metadataPublished) {
  metadataPublished = pushMetadata();
  }
  
  unsigned long now = millis();
  if (now - lastSyncMs >= FIREBASE_INTERVAL_MS) {
  lastSyncMs = now;

  readSettings();
  readShadow();
  processWifiReprovisionRequest();
  processOwnershipPairingRequest();
  pushTelemetry();
  pushStatus();
  pushShadow();
  pushDiagnostics();
  }
}

void CloudManager::processWifiReprovisionRequest() {
  const String maintenancePath = "/devices/" + deviceId + "/maintenance";
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, maintenancePath.c_str())) return;
  FirebaseJson maintenance = fbdo_cloud.to<FirebaseJson>();
  FirebaseJsonData value;
  maintenance.get(value, "activeWifiReprovisionRequestId");
  const String requestId = value.success ? value.stringValue : "";
  if (requestId.length() == 0) return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;
  const String processedRequestId = prefs.getString("reprov_req_id", "");
  prefs.end();
  if (processedRequestId == requestId) return;

  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH) {
  LOG(APP_LOG_LEVEL_WARN, "REPROVISION", "Wi-Fi recovery request deferred until NTP time is available.");
  return;
  }

  const String requestPath = maintenancePath + "/requests/" + requestId;
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, requestPath.c_str())) return;
  FirebaseJson request = fbdo_cloud.to<FirebaseJson>();
  request.get(value, "action");
  const String action = value.success ? value.stringValue : "";
  request.get(value, "status");
  const String status = value.success ? value.stringValue : "";
  request.get(value, "nonce");
  const String nonce = value.success ? value.stringValue : "";
  request.get(value, "expiresAtMs");
  const uint64_t expiresAtMs = value.success ? static_cast<uint64_t>(value.doubleValue) : 0;
  const uint64_t nowMs = static_cast<uint64_t>(epoch) * 1000ULL;
  if (action != "WIFI_REPROVISION" || status != "pending" || nonce.length() < 24 ||
  expiresAtMs == 0 || expiresAtMs <= nowMs) {
  LOG(APP_LOG_LEVEL_WARN, "REPROVISION", "Ignoring invalid, expired, or completed Wi-Fi recovery request.");
  return;
  }

  // Bootloader applies setPump(false) before it changes any enrollment data.
  // It then clears only Wi-Fi/device-auth enrollment and restarts into BLE.
  Bootloader::applyWifiReprovisionRequest(requestId.c_str());
}

void CloudManager::processOwnershipPairingRequest() {
  String ownershipPath = "/devices/" + deviceId + "/ownership";
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, ownershipPath.c_str())) return;
  FirebaseJson ownership = fbdo_cloud.to<FirebaseJson>();
  FirebaseJsonData value;
  ownership.get(value, "ownershipPairingRequestId");
  const String requestId = value.success ? value.stringValue : "";
  ownership.get(value, "ownershipPairingExpiresAtMs");
  const uint64_t expiresAtMs = value.success ? static_cast<uint64_t>(value.doubleValue) : 0;
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
  (state != "transfer_pending" && state != "release_pending")) return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;
  const String processedRequestId = prefs.getString("own_pair_req", "");
  prefs.end();
  if (processedRequestId == requestId) return;

  const time_t epoch = time(nullptr);
  if (epoch < MIN_VALID_EPOCH) {
  LOG(APP_LOG_LEVEL_WARN, "OWNERSHIP", "Pairing request deferred until NTP time is available.");
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

  String requestPath = "/devices/" + deviceId + "/maintenance/requests/" + requestId;
  if (!Firebase.RTDB.getJSON(&fbdo_cloud, requestPath.c_str())) return;
  FirebaseJson request = fbdo_cloud.to<FirebaseJson>();
  request.get(value, "action");
  const String action = value.success ? value.stringValue : "";
  request.get(value, "purpose");
  const String purpose = value.success ? value.stringValue : "";
  request.get(value, "status");
  const String status = value.success ? value.stringValue : "";
  if (action != "OWNERSHIP_PAIRING" || status != "pending" ||
  (purpose != "transfer" && purpose != "release")) return;

  const uint64_t remaining64 = expiresAtMs - nowMs;
  const uint32_t remainingMs = remaining64 > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(remaining64);
  // Pump OFF is the first state-changing action for a remotely requested
  // ownership pairing session. Failure to start BLE leaves it OFF.
  setPump(false);
  if (!BleProvisioning::startOwnershipPairing(purpose, remainingMs)) {
  LOG(APP_LOG_LEVEL_ERROR, "OWNERSHIP", "Unable to start temporary ownership pairing BLE.");
  return;
  }
  if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
  prefs.putString("own_pair_req", requestId);
  prefs.end();
  }
  activeOwnershipPairingRequestId = requestId;
  LOG(APP_LOG_LEVEL_WARN, "OWNERSHIP", "Temporary %s pairing active for request %s.", purpose.c_str(), requestId.c_str());
}

void CloudManager::queuePairingVerifier(const String& rawProof, const String& purpose, uint32_t lifetimeMs) {
  if (rawProof.length() == 0 || rawProof.length() > 256 ||
  (purpose != "claim" && purpose != "transfer" && purpose != "release")) {
  LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Rejected invalid pairing verifier request");
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
  // therefore represents the usable authenticated session; RTDB rules remain
  // the authority that binds the token's deviceId claim to this path.
  return firebaseStarted && Firebase.ready();
}

bool CloudManager::isPairingVerifierPublished() {
  return pairingVerifierPublished;
}

bool CloudManager::bootstrapAndStartFirebase() {
  if (WiFi.status() != WL_CONNECTED) return false;
  const String rootCa = DEVICE_BOOTSTRAP_ROOT_CA;
  if (!isConfigured(DEVICE_BOOTSTRAP_SECRET) || !isConfigured(DEVICE_BOOTSTRAP_URL) ||
  !rootCa.startsWith("\n-----BEGIN CERTIFICATE-----") || rootCa.indexOf("replace_") >= 0) {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap configuration is missing; cloud access remains disabled");
  return false;
  }

  const time_t nowEpoch = time(nullptr);
  if (nowEpoch < MIN_VALID_EPOCH) {
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Bootstrap deferred until NTP time is available");
  return false;
  }

  const String nonce = makeNonce();
  const uint64_t timestampMs = static_cast<uint64_t>(nowEpoch) * 1000ULL;
  char timestampText[24];
  snprintf(timestampText, sizeof(timestampText), "%llu", static_cast<unsigned long long>(timestampMs));
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
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap rejected (%d): %s", httpCode, response.c_str());
  return false;
  }

  StaticJsonDocument<1600> result;
  if (deserializeJson(result, response) != DeserializationError::Ok) {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap response was not valid JSON");
  return false;
  }
  const char* customToken = result["customToken"] | "";
  const char* returnedUid = result["deviceUid"] | "";
  const String expectedUid = "device:" + deviceId;
  if (String(customToken).isEmpty() || String(returnedUid) != expectedUid) {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Bootstrap response did not contain this device identity");
  return false;
  }

  Firebase.setCustomToken(&config_cloud, customToken);
  Firebase.begin(&config_cloud, &auth_cloud);
  Firebase.reconnectWiFi(true);
  Firebase.RTDB.setReadTimeout(&fbdo_cloud, 10000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo_cloud, "medium");
  firebaseStarted = true;
  metadataPublished = false;
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Custom-token sign-in started for %s", expectedUid.c_str());
  return true;
}

bool CloudManager::pushMetadata() {
  String path = "/devices/" + deviceId + "/metadata";
  const String expectedUid = "device:" + deviceId;
  if (!isAuthenticated()) {
  LOG(APP_LOG_LEVEL_WARN, "CLOUD", "Metadata deferred: custom-token session is not ready");
  return false;
  }

  FirebaseJson json;
  
  json.set("firmwareVersion", "2.0.0");
  json.set("hardwareVersion", "ESP32-WROOM-32");
  json.set("protocolVersion", "1.0");
  json.set("serialNumber", deviceId);
  json.set("deviceAuthUid", expectedUid.c_str());

  if (!Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json)) {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Metadata publish failed: %s", fbdo_cloud.errorReason().c_str());
  return false;
  }

  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Metadata published for %s (firmware UID: %s)",
  deviceId.c_str(), expectedUid.c_str());
  return true;
}

void CloudManager::readSettings() {
  String path = "/devices/" + deviceId + "/settings";
  if (Firebase.RTDB.getJSON(&fbdo_cloud, path.c_str())) {
  FirebaseJson json = fbdo_cloud.to<FirebaseJson>();
  FirebaseJsonData jd;

  json.get(jd, "pump_start_level_pct");
  if (jd.success) cfgPumpStartLevel = jd.intValue;

  json.get(jd, "pump_stop_level_pct");
  if (jd.success) cfgPumpStopLevel = jd.intValue;

  json.get(jd, "dry_run_threshold_lpm");
  if (jd.success) cfgDryRunThresholdLpm = jd.floatValue;

  json.get(jd, "max_pump_runtime_min");
  if (jd.success) cfgMaxPumpRuntimeMin = jd.intValue;
  }
}

void CloudManager::pushDiagnostics() {
  String path = "/devices/" + deviceId + "/diagnostics";
  FirebaseJson json;
  
  json.set("freeHeap", ESP.getFreeHeap());
  json.set("wifiRSSI", WiFi.RSSI());
  json.set("restartReason", Bootloader::getBootReasonString());

  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json);
}

void CloudManager::pushCloudEvent(const String& level, const String& component, const String& code, const String& details) {
  if (!Firebase.ready()) return;

  if (level != "INFO" && level != "WARN" && level != "ERROR") return;

  uint64_t timestamp = millis();
  if (ntpSynced) {
  timestamp = (static_cast<uint64_t>(ntpEpochSecAtLastSync) * 1000ULL) + (millis() - ntpLastSyncMs);
  }
  const String eventsPath = "/devices/" + deviceId + "/events";
  
  FirebaseJson json;
  json.set("timestamp", (double)timestamp);
  json.set("severity", level);
  json.set("category", component);
  json.set("code", code);
  json.set("message", details);

  if (!Firebase.RTDB.pushJSON(&fbdo_cloud, eventsPath.c_str(), &json)) {
  return;
  }
}

void CloudManager::readShadow() {
  String path = "/devices/" + deviceId + "/shadow/desired";
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
  if (jd.success) mode = jd.stringValue;

  json.get(jd, "manual_desired");
  if (jd.success) manual_desired = jd.boolValue;

  json.get(jd, "countdown_start");
  if (jd.success) countdown_start = jd.boolValue;

  json.get(jd, "countdown_duration_min");
  if (jd.success) countdown_duration_min = jd.intValue;

  json.get(jd, "emergency_stop");
  if (jd.success) emergency_stop = jd.boolValue;

  json.get(jd, "reset_stop");
  if (jd.success) reset_stop = jd.boolValue;

  json.get(jd, "clear_error");
  if (jd.success) clear_error = jd.boolValue;

  json.get(jd, "bypass_level_sensor");
  if (jd.success) bypass_level_sensor = jd.boolValue;

  json.get(jd, "bypass_flow_sensor");
  if (jd.success) bypass_flow_sensor = jd.boolValue;

  json.get(jd, "reboot_device");
  if (jd.success) reboot_device = jd.boolValue;

  DeviceShadow::evaluateDesired(mode, manual_desired, countdown_start, countdown_duration_min, emergency_stop, reset_stop, clear_error, bypass_level_sensor, bypass_flow_sensor, reboot_device);
  }
}

void CloudManager::pushTelemetry() {
  String path = "/devices/" + deviceId + "/telemetry";
  FirebaseJson json;
  
  json.set("water_level_percent", waterLevelPct);
  json.set("flow_rate_lpm", flowRateLpm);
  json.set("ultrasonic_last_good_cm", (double)(ultrasonicLastGoodCmX10 / 10.0));

  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json);
}

void CloudManager::pushStatus() {
  String path = "/devices/" + deviceId + "/status";
  FirebaseJson json;
  
  json.set("lifecycle", deviceLifecycle == DeviceLifecycle::ONLINE ? "ONLINE" : "OFFLINE");
  json.set("uptimeSeconds", (int)(esp_timer_get_time() / 1000000ULL));
  json.set("firmwareVersion", "2.0.0");
  
  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json);
}

void CloudManager::clearCountdownDesiredState() {
  if (!Firebase.ready()) return;
  String path = "/devices/" + deviceId + "/shadow/desired";
  FirebaseJson update;
  update.set("countdown_start", false);
  update.set("mode", "MANUAL");
  update.set("manual_desired", false);
  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Cleared countdown state from desired shadow to prevent loops.");
}

bool CloudManager::clearRebootDesiredState() {
  if (!Firebase.ready()) return false;
  String path = "/devices/" + deviceId + "/shadow/desired";
  FirebaseJson update;
  update.set("reboot_device", false);
  bool success = Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  if (success) {
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Cleared reboot flag from desired shadow.");
  } else {
  LOG(APP_LOG_LEVEL_ERROR, "CLOUD", "Failed to clear reboot flag: %s", fbdo_cloud.errorReason().c_str());
  }
  return success;
}

void CloudManager::setErrorFallbackDesiredState() {
  if (!Firebase.ready()) return;
  String path = "/devices/" + deviceId + "/shadow/desired";
  FirebaseJson update;
  update.set("mode", "MANUAL");
  update.set("manual_desired", false);
  update.set("countdown_start", false);
  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
  LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Forced desired shadow to MANUAL OFF due to safety trip.");
}

void CloudManager::pushShadow() {
  String path = "/devices/" + deviceId + "/shadow";
  String reportedStr = DeviceShadow::getReportedJson();
  
  FirebaseJson reportedJson;
  reportedJson.setJsonData(reportedStr);
  
  FirebaseJson update;
  update.set("reported", reportedJson);
  
  Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
}
