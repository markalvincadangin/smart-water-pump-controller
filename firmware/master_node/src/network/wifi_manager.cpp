#include "wifi_manager.h"
#include <WiFi.h>
#include "../config/config.h"
#include "../state/state.h"
#include "../utils/app_logger.h"

static unsigned long lastWifiRetryMs = 0;

void WifiManager::init() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
}

void WifiManager::connect() {
    String ssid = "";
    String password = "";
    
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
        ssid = prefs.getString("wifi_ssid", "");
        password = prefs.getString("wifi_pass", "");
        prefs.end();
    }

    if (ssid.length() == 0) {
        LOG(APP_LOG_LEVEL_WARN, "WIFI", "No SSID configured in NVS.");
        return;
    }

    LOG(APP_LOG_LEVEL_INFO, "WIFI", "Connecting to: %s", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiWasConnected = true;
        wifiRssi = WiFi.RSSI();
        LOG(APP_LOG_LEVEL_INFO, "WIFI", "Connected! IP: %s | RSSI: %d dBm", WiFi.localIP().toString().c_str(), wifiRssi);
    } else {
        LOG(APP_LOG_LEVEL_ERROR, "WIFI", "Connection failed. Will retry later.");
    }
}

void WifiManager::loop() {
    if (deviceLifecycle != DeviceLifecycle::ONLINE && deviceLifecycle != DeviceLifecycle::PROVISIONING) {
        return; 
    }

    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        // Wait at least WIFI_BACKOFF_INITIAL_MS before reconnecting
        if (now - lastWifiRetryMs >= WIFI_BACKOFF_INITIAL_MS) {
            lastWifiRetryMs = now;
            LOG(APP_LOG_LEVEL_WARN, "WIFI", "Disconnected. Attempting reconnect...");
            WiFi.disconnect();
            connect();
        }
    } else {
        wifiRssi = WiFi.RSSI();
    }
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int WifiManager::getRssi() {
    return wifiRssi;
}
