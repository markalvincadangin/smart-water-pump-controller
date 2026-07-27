#include "ota_manager.h"
#include "../utils/app_logger.h"
#include "../config/config.h"
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "esp_ota_ops.h"

OtaManager& ota_manager = OtaManager::getInstance();

void OtaManager::begin() {
    app_logger.logEvent(APP_LOG_LEVEL_INFO, "OTA", "INIT", "OTA Manager initialized");
}

bool OtaManager::performUpdate(const String& url, const String& expectedSha256) {
    if (url.isEmpty()) {
        app_logger.logEvent(APP_LOG_LEVEL_ERROR, "OTA", "EMPTY_URL", "OTA URL is empty");
        return false;
    }

    app_logger.logEvent(APP_LOG_LEVEL_INFO, "OTA", "START", "Starting OTA update from: " + url);

    WiFiClientSecure client;
    // For production, set proper root CA. Using insecure for development.
    client.setInsecure(); 

    // Configure HTTPUpdate
    httpUpdate.rebootOnUpdate(false); // We want to handle reboot ourselves if needed

    t_httpUpdate_return ret = httpUpdate.update(client, url);

    switch (ret) {
        case HTTP_UPDATE_FAILED: {
            String err = httpUpdate.getLastErrorString();
            app_logger.logEvent(APP_LOG_LEVEL_ERROR, "OTA", "FAILED", "OTA Update failed: " + err);
            return false;
        }
        case HTTP_UPDATE_NO_UPDATES:
            app_logger.logEvent(APP_LOG_LEVEL_INFO, "OTA", "NO_UPDATES", "No updates available");
            return false;
        case HTTP_UPDATE_OK:
            app_logger.logEvent(APP_LOG_LEVEL_INFO, "OTA", "SUCCESS", "OTA Update successful. Rebooting.");
            delay(1000);
            ESP.restart();
            return true;
    }
    return false;
}

void OtaManager::markFirmwareValid() {
    // If rollback is enabled in bootloader, mark this boot as successful
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
        app_logger.logEvent(APP_LOG_LEVEL_INFO, "OTA", "VALIDATED", "Firmware marked as valid, rollback cancelled");
    } else if (err != ESP_ERR_NOT_SUPPORTED) {
        app_logger.logEvent(APP_LOG_LEVEL_WARN, "OTA", "MARK_FAILED", "Failed to mark firmware as valid");
    }
}
