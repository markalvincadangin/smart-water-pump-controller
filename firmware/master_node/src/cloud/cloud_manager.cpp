#include "cloud_manager.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include "../config/config.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include "device_shadow.h"
#include "../core/lifecycle/bootloader.h"

static FirebaseData fbdo_cloud;
static FirebaseAuth auth_cloud;
static FirebaseConfig config_cloud;
static String deviceId;
static unsigned long lastSyncMs = 0;

void CloudManager::init() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toUpperCase();
    deviceId = "SF-" + mac.substring(6);
    
    config_cloud.api_key      = API_KEY;
    config_cloud.database_url = DATABASE_URL;
    config_cloud.signer.anonymous = true;

    config_cloud.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config_cloud, &auth_cloud);
    Firebase.reconnectWiFi(true);

    Firebase.RTDB.setReadTimeout(&fbdo_cloud, 10000);
    Firebase.RTDB.setwriteSizeLimit(&fbdo_cloud, "medium");

    LOG(APP_LOG_LEVEL_INFO, "CLOUD", "Initialized for device: %s", deviceId.c_str());
}

void CloudManager::sync() {
    if (!Firebase.ready()) return;
    
    unsigned long now = millis();
    if (now - lastSyncMs >= FIREBASE_INTERVAL_MS) {
        lastSyncMs = now;
        
        if (lastSyncMs == FIREBASE_INTERVAL_MS) { // First sync
            pushMetadata();
        }
        
        readSettings();
        readShadow();
        pushTelemetry();
        pushStatus();
        pushShadow();
        pushDiagnostics();
    }
}

void CloudManager::pushMetadata() {
    String path = "/devices/" + deviceId + "/metadata";
    FirebaseJson json;
    
    json.set("firmwareVersion", "2.0.0");
    json.set("hardwareVersion", "ESP32-WROOM-32");
    json.set("protocolVersion", "1.0");
    json.set("serialNumber", deviceId);
    if (auth_cloud.token.uid.length() > 0) {
        json.set("firmwareUid", auth_cloud.token.uid.c_str());
        
        // Also save to NVS per T008
        if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
            prefs.putString("firmwareUid", auth_cloud.token.uid.c_str());
            prefs.end();
        }
    }

    Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &json);
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

void CloudManager::pushEventLog(const String& level, const String& component, const String& message) {
    if (!Firebase.ready()) return;
    
    unsigned long timestamp = millis(); // Simple timestamp. Should be NTP ideally.
    String eventId = "evt_" + String(timestamp);
    String path = "/devices/" + deviceId + "/events/" + eventId;
    
    FirebaseJson json;
    json.set("timestamp", (int)timestamp);
    json.set("severity", level);
    json.set("category", component);
    json.set("code", "LOG");
    json.set("message", message);

    Firebase.RTDB.setJSON(&fbdo_cloud, path.c_str(), &json);
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

        DeviceShadow::evaluateDesired(mode, manual_desired, countdown_start, countdown_duration_min, emergency_stop, reset_stop, clear_error, bypass_level_sensor, bypass_flow_sensor);
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

void CloudManager::pushShadow() {
    String path = "/devices/" + deviceId + "/shadow";
    String reportedStr = DeviceShadow::getReportedJson();
    
    FirebaseJson reportedJson;
    reportedJson.setJsonData(reportedStr);
    
    FirebaseJson update;
    update.set("reported", reportedJson);
    
    Firebase.RTDB.updateNode(&fbdo_cloud, path.c_str(), &update);
}
