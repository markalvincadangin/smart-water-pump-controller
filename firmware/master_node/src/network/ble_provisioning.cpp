#include "ble_provisioning.h"
#include <NimBLEDevice.h>
#include <WiFi.h>
#include "../config/config.h"
#include "../state/state.h"
#include "../cloud/cloud_manager.h"
#include "../persistence/persistence.h"
#include "../safety/safety_pump.h"
#include "../utils/app_logger.h"
#include "../utils/time_utils.h"
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Service and Characteristics
#define PROV_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CMD_CHAR_UUID            "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESP_CHAR_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a9"

enum class ProvState {
    IDLE,
    SCANNING_WIFI,
    CONNECTING_WIFI,
    PROVISIONED,
    ERR
};

static ProvState provState = ProvState::IDLE;
static NimBLECharacteristic* pRespChar = nullptr;
constexpr size_t COMMAND_QUEUE_DEPTH = 4;
constexpr size_t MAX_COMMAND_BYTES = 512;
struct QueuedCommand {
    char payload[MAX_COMMAND_BYTES];
};
static QueueHandle_t commandQueue = nullptr;
static String currentReqId = "";
static unsigned long connectStartedMs = 0;
static unsigned long provisionedAtMs = 0;
static bool provisionedStatusSent = false;
static bool provisioningActive = false;
static constexpr uint32_t PAIRING_PROOF_LIFETIME_MS = 5UL * 60UL * 1000UL;
static bool ownershipPairingActive = false;
static String ownershipPairingPurpose = "claim";
static uint32_t ownershipPairingDeadlineMs = 0;

static String getDeviceId() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toUpperCase();
    return "SF-" + mac.substring(6);
}

static void sendResponse(const String& payload) {
    if (pRespChar) {
        pRespChar->setValue(
            reinterpret_cast<const uint8_t*>(payload.c_str()),
            payload.length()
        );
        pRespChar->notify();
    }
}

static void sendError(const String& reqId, const String& code, const String& message) {
    StaticJsonDocument<200> doc;
    doc["v"] = 1;
    doc["id"] = reqId;
    doc["type"] = "error";
    doc["code"] = code;
    doc["message"] = message;
    String out;
    serializeJson(doc, out);
    sendResponse(out);
}

static void sendStatus(const String& reqId, const String& status) {
    StaticJsonDocument<200> doc;
    doc["v"] = 1;
    doc["id"] = reqId;
    doc["type"] = "status";
    doc["status"] = status;
    String out;
    serializeJson(doc, out);
    sendResponse(out);
}

class ProvCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        String value = pCharacteristic->getValue().c_str();
        if (value.length() >= MAX_COMMAND_BYTES) {
            LOG(APP_LOG_LEVEL_WARN, "BLE", "Ignoring oversized provisioning command.");
            return;
        }

        QueuedCommand command{};
        value.toCharArray(command.payload, sizeof(command.payload));
        if (commandQueue == nullptr || xQueueSend(commandQueue, &command, 0) != pdPASS) {
            LOG(APP_LOG_LEVEL_WARN, "BLE", "Provisioning command queue is full.");
            return;
        }
        LOG(APP_LOG_LEVEL_INFO, "BLE", "Queued provisioning command.");
    }
};

void BleProvisioning::init() {
    if (provisioningActive) return;
    if (commandQueue != nullptr) {
        vQueueDelete(commandQueue);
        commandQueue = nullptr;
    }
    commandQueue = xQueueCreate(COMMAND_QUEUE_DEPTH, sizeof(QueuedCommand));
    if (commandQueue == nullptr) {
        LOG(APP_LOG_LEVEL_ERROR, "BLE", "Unable to allocate provisioning command queue.");
        return;
    }

    // NimBLE's address accessor is valid only after NimBLEDevice::init().
    // WiFi MAC is already available because WifiManager initializes STA mode first.
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toUpperCase();
    String deviceName = "SmartFlow-" + mac.substring(6);

    NimBLEDevice::init(deviceName.c_str());
    NimBLEServer* pServer = NimBLEDevice::createServer();
    NimBLEService* pService = pServer->createService(PROV_SERVICE_UUID);
    
    ProvCallbacks* callbacks = new ProvCallbacks();

    NimBLECharacteristic* pCmdChar = pService->createCharacteristic(
        CMD_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pCmdChar->setCallbacks(callbacks);

    pRespChar = pService->createCharacteristic(
        RESP_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(PROV_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    LOG(APP_LOG_LEVEL_INFO, "BLE", "Provisioning started. Advertising as %s", deviceName.c_str());
    provState = ProvState::IDLE;
    provisionedStatusSent = false;
    provisioningActive = true;
}

bool BleProvisioning::startOwnershipPairing(const String& purpose, uint32_t lifetimeMs) {
    if ((purpose != "transfer" && purpose != "release") || lifetimeMs == 0 || provisioningActive) {
        return false;
    }
    init();
    if (!provisioningActive) return false;
    ownershipPairingActive = true;
    ownershipPairingPurpose = purpose;
    ownershipPairingDeadlineMs = addMillisSaturated(millis(), lifetimeMs);
    LOG(APP_LOG_LEVEL_WARN, "BLE", "Temporary %s ownership pairing started.", purpose.c_str());
    return true;
}

static String getAuthModeName(wifi_auth_mode_t mode) {
    switch(mode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        case WIFI_AUTH_WAPI_PSK: return "WAPI_PSK";
        default: return "UNKNOWN";
    }
}

void BleProvisioning::loop() {
    if (ownershipPairingActive && millisDeadlineReached(millis(), ownershipPairingDeadlineMs)) {
        LOG(APP_LOG_LEVEL_WARN, "BLE", "Temporary ownership pairing expired.");
        stop();
        return;
    }
    if (provState == ProvState::CONNECTING_WIFI) {
        if (WiFi.status() == WL_CONNECTED) {
            LOG(APP_LOG_LEVEL_INFO, "BLE", "Wi-Fi connected. Initializing cloud services.");
            CloudManager::init();
            sendStatus(currentReqId, "connected");
            provState = ProvState::PROVISIONED;
            provisionedAtMs = millis();
            provisionedStatusSent = false;
        } else if (millis() - connectStartedMs >= 20000UL) {
            WiFi.disconnect(false, false);
            sendError(currentReqId, "TIMEOUT", "Wi-Fi connection timed out");
            provState = ProvState::IDLE;
        }
        return;
    }

    if (provState == ProvState::PROVISIONED) {
        // Allow the client to receive each final state notification separately.
        if (!provisionedStatusSent && millis() - provisionedAtMs >= 250UL) {
            sendStatus(currentReqId, "provisioned");
            provisionedStatusSent = true;
            provisionedAtMs = millis();
        } else if (provisionedStatusSent && millis() - provisionedAtMs >= 1000UL) {
            stop();
        }
        return;
    }

    QueuedCommand queuedCommand{};
    if (provState == ProvState::IDLE && commandQueue != nullptr &&
        xQueueReceive(commandQueue, &queuedCommand, 0) == pdTRUE) {
        String cmdStr = queuedCommand.payload;

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, cmdStr);
        
        if (err) {
            sendError("", "INVALID_JSON", "Malformed request");
            return;
        }

        String reqId = doc["id"] | "";
        currentReqId = reqId;
        int version = doc["v"] | 0;
        String cmd = doc["cmd"] | "";

        if (version != 1) {
            sendError(reqId, "INVALID_COMMAND", "Unsupported protocol version");
            return;
        }

        if (reqId.length() == 0) {
            sendError("", "INVALID_COMMAND", "Missing request ID");
            return;
        }

        if (ownershipPairingActive && cmd != "get_token") {
            sendError(reqId, "INVALID_STATE", "Only pairing proof retrieval is available during ownership pairing");
            return;
        }

        if (cmd == "scan_wifi") {
            provState = ProvState::SCANNING_WIFI;
            LOG(APP_LOG_LEVEL_INFO, "BLE", "Starting Wi-Fi scan...");

            // Emit scan_start
            StaticJsonDocument<200> startDoc;
            startDoc["v"] = 1;
            startDoc["id"] = reqId;
            startDoc["type"] = "scan_start";
            String startStr;
            serializeJson(startDoc, startStr);
            sendResponse(startStr);

            int n = WiFi.scanNetworks(false, true); // (async=false, show_hidden=true)
            if (n > 0) {
                for (int i = 0; i < n; ++i) {
                    StaticJsonDocument<300> netDoc;
                    netDoc["v"] = 1;
                    netDoc["id"] = reqId;
                    netDoc["type"] = "network";
                    netDoc["ssid"] = WiFi.SSID(i);
                    netDoc["bssid"] = WiFi.BSSIDstr(i);
                    netDoc["rssi"] = WiFi.RSSI(i);
                    netDoc["auth"] = getAuthModeName(WiFi.encryptionType(i));

                    String netStr;
                    serializeJson(netDoc, netStr);
                    sendResponse(netStr);
                    delay(50); // small delay to prevent congestion
                }
            }
            WiFi.scanDelete();

            // Emit scan_complete
            StaticJsonDocument<200> completeDoc;
            completeDoc["v"] = 1;
            completeDoc["id"] = reqId;
            completeDoc["type"] = "scan_complete";
            String completeStr;
            serializeJson(completeDoc, completeStr);
            sendResponse(completeStr);

            provState = ProvState::IDLE;
        }
        else if (cmd == "get_token") {
            char tokenBuf[65];
            snprintf(tokenBuf, sizeof(tokenBuf), "%08X%08X%08X%08X%08X%08X%08X%08X",
                esp_random(), esp_random(), esp_random(), esp_random(),
                esp_random(), esp_random(), esp_random(), esp_random());
            const String token(tokenBuf);
            const String purpose = ownershipPairingActive ? ownershipPairingPurpose : "claim";
            const uint32_t lifetimeMs = ownershipPairingActive
                ? elapsedMillis32(ownershipPairingDeadlineMs, millis()) : PAIRING_PROOF_LIFETIME_MS;
            if (ownershipPairingActive && lifetimeMs == 0) {
                sendError(reqId, "TIMEOUT", "Ownership pairing window expired");
                stop();
                return;
            }
            CloudManager::queuePairingVerifier(token, purpose, lifetimeMs);

            StaticJsonDocument<200> resp;
            resp["v"] = 1;
            resp["id"] = reqId;
            resp["type"] = "token";
            resp["value"] = token;
            resp["device_id"] = getDeviceId();
            String out;
            serializeJson(resp, out);
            sendResponse(out);
        }
        else if (cmd == "connect_wifi") {
            String ssid = doc["ssid"] | "";
            String password = doc["password"] | "";

            if (ssid.length() == 0) {
                sendError(reqId, "INVALID_COMMAND", "Missing SSID");
                return;
            }

            provState = ProvState::CONNECTING_WIFI;
            sendStatus(reqId, "connecting");

            if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
                prefs.putString("wifi_ssid", ssid);
                prefs.putString("wifi_pass", password);
                prefs.end();
            }

            LOG(APP_LOG_LEVEL_INFO, "BLE", "Credentials saved. Connecting to Wi-Fi.");
            deviceLifecycle = DeviceLifecycle::ONLINE;
            WiFi.disconnect(false, false);
            WiFi.begin(ssid.c_str(), password.c_str());
            connectStartedMs = millis();
            provState = ProvState::CONNECTING_WIFI;
        }
        else if (cmd == "reset") {
            // This is a scoped local reprovisioning reset, never a full NVS erase.
            // Pump OFF is intentionally the first state-changing action.
            setPump(false);
            LOG(APP_LOG_LEVEL_INFO, "BLE", "Local provisioning reset received. Clearing Wi-Fi enrollment only.");
            clearNetworkEnrollment();
            esp_restart();
        }
        else {
            sendError(reqId, "INVALID_COMMAND", "Unknown command");
        }
    } else if (provState != ProvState::IDLE && commandQueue != nullptr &&
               xQueueReceive(commandQueue, &queuedCommand, 0) == pdTRUE) {
        // Discard or return busy
        String cmdStr = queuedCommand.payload;

        StaticJsonDocument<200> doc;
        deserializeJson(doc, cmdStr);
        String reqId = doc["id"] | "";

        sendError(reqId, "BUSY", "Device is busy processing another command");
    }
}

void BleProvisioning::stop() {
    if (!provisioningActive) {
        return;
    }

    provisioningActive = false;
    ownershipPairingActive = false;
    ownershipPairingPurpose = "claim";
    ownershipPairingDeadlineMs = 0;
    pRespChar = nullptr;
    if (commandQueue != nullptr) {
        vQueueDelete(commandQueue);
        commandQueue = nullptr;
    }
    NimBLEDevice::deinit(true);
    LOG(APP_LOG_LEVEL_INFO, "BLE", "Provisioning stopped and BLE de-initialized.");
}

bool BleProvisioning::isProvisioned() {
    bool hasWifi = false;
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
        hasWifi = prefs.getString("wifi_ssid", "").length() > 0;
        prefs.end();
    }
    return hasWifi;
}

bool BleProvisioning::isActive() {
    return provisioningActive;
}

bool BleProvisioning::isOwnershipPairingActive() {
    return provisioningActive && ownershipPairingActive;
}

void BleProvisioning::stopOwnershipPairing() {
    if (ownershipPairingActive) {
        LOG(APP_LOG_LEVEL_WARN, "BLE", "Temporary ownership pairing stopped by backend state.");
        stop();
    }
}

void BleProvisioning::updateStatus(const char* status) {
    if (currentReqId.length() > 0) {
        sendStatus(currentReqId, status);
    }
}
