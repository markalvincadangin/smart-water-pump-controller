/**
 * @file wifi_manager.cpp
 * @brief Manages Wi-Fi connectivity and lifecycle.
 */
#include "wifi_manager.h"
#include <WiFi.h>
#include "../config/config.h"
#include "../state/state.h"
#include "../utils/app_logger.h"

namespace {
  enum class WifiState {
    IDLE,
    CONNECTING,
    CONNECTED
  };

  WifiState wifiState = WifiState::IDLE;
  unsigned long lastWifiRetryMs = 0;
  unsigned long connectStartMs = 0;
  unsigned long currentBackoffMs = WIFI_BACKOFF_INITIAL_MS;
  bool credentialsLoaded = false;
  String cachedSsid = "";
  String cachedPassword = "";

  void loadCredentials() {
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
      cachedSsid = prefs.getString("wifi_ssid", "");
      cachedPassword = prefs.getString("wifi_pass", "");
      prefs.end();
    }
    credentialsLoaded = (cachedSsid.length() > 0);
  }
}

void WifiManager::init() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // We handle auto-reconnect manually for brownout safety
  WiFi.persistent(false);
  loadCredentials();
}

void WifiManager::connect() {
  loadCredentials(); // Re-read in case of reprovisioning
  
  if (!credentialsLoaded) {
    LOG(APP_LOG_LEVEL_WARN, "WIFI", "No SSID configured in NVS. Skipping connection.");
    return;
  }

  LOG(APP_LOG_LEVEL_INFO, "WIFI", "Connecting to: %s", cachedSsid.c_str());
  
  WiFi.begin(cachedSsid.c_str(), cachedPassword.c_str());
  
  wifiState = WifiState::CONNECTING;
  connectStartMs = millis();
}

void WifiManager::loop() {
  if (deviceLifecycle != DeviceLifecycle::ONLINE && deviceLifecycle != DeviceLifecycle::PROVISIONING) {
    return; 
  }

  if (!credentialsLoaded) {
    // Standard IoT Pattern: Unprovisioned devices shouldn't spam radio
    return;
  }

  unsigned long now = millis();

  switch (wifiState) {
    case WifiState::IDLE:
      if (now - lastWifiRetryMs >= currentBackoffMs) {
        lastWifiRetryMs = now;
        app_logger.logCloudEvent(APP_LOG_LEVEL_WARN, LogCategory::SYSTEM, EventCode::EVT_WIFI_DISCONNECTED, "Disconnected. Attempting reconnect...");
        connect();
      }
      break;

    case WifiState::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        wifiState = WifiState::CONNECTED;
        wifiWasConnected = true;
        wifiRssi = WiFi.RSSI();
        currentBackoffMs = WIFI_BACKOFF_INITIAL_MS; // Reset backoff on success
        LOG(APP_LOG_LEVEL_INFO, "WIFI", "Connected! IP: %s | RSSI: %d dBm", WiFi.localIP().toString().c_str(), wifiRssi);
      } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL || (now - connectStartMs > 15000)) {
        // Connection failed or timed out (15s)
        wifiState = WifiState::IDLE;
        lastWifiRetryMs = now; // Start backoff timer from now
        
        // Exponential backoff calculation
        currentBackoffMs = (currentBackoffMs * 2);
        if (currentBackoffMs > WIFI_BACKOFF_MAX_MS) {
          currentBackoffMs = WIFI_BACKOFF_MAX_MS;
        }
        // Add jitter (0 to WIFI_JITTER_MS)
        unsigned long jitter = esp_random() % WIFI_JITTER_MS;
        currentBackoffMs += jitter;
        
        LOG(APP_LOG_LEVEL_ERROR, "WIFI", "Connection failed. Retrying in %lu ms.", currentBackoffMs);
        WiFi.disconnect(); // Standard disconnect without turning off radio entirely
      }
      break;

    case WifiState::CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        wifiState = WifiState::IDLE;
        lastWifiRetryMs = now; // Immediately start backoff
        LOG(APP_LOG_LEVEL_WARN, "WIFI", "Wi-Fi dropped! Starting backoff timer.");
      } else {
        wifiRssi = WiFi.RSSI();
      }
      break;
  }
}

bool WifiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int WifiManager::getRssi() {
  return wifiRssi;
}
