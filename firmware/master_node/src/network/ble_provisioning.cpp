#include "ble_provisioning.h"
#include <NimBLEDevice.h>
#include "../config/config.h"
#include "../state/state.h"
#include "../utils/app_logger.h"

// Generate a random-looking UUID for SmartFlow provisioning
#define PROV_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SSID_CHAR_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define PASS_CHAR_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define TOKEN_CHAR_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define COMMIT_CHAR_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26ab"

static String provSsid = "";
static String provPass = "";
static String provToken = "";
static bool provisionCommitReceived = false;

class ProvCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        String uuid = pCharacteristic->getUUID().toString().c_str();
        String value = pCharacteristic->getValue().c_str();
        
        if (uuid == SSID_CHAR_UUID) {
            provSsid = value;
            LOG(APP_LOG_LEVEL_INFO, "BLE", "Received SSID: %s", provSsid.c_str());
        } else if (uuid == PASS_CHAR_UUID) {
            provPass = value;
            LOG(APP_LOG_LEVEL_INFO, "BLE", "Received Password (length: %d)", provPass.length());
        } else if (uuid == COMMIT_CHAR_UUID) {
            if (value == "1" || value == "true" || value == "commit") {
                LOG(APP_LOG_LEVEL_INFO, "BLE", "Commit command received.");
                provisionCommitReceived = true;
            }
        }
    }
};

void BleProvisioning::init() {
    String mac = NimBLEDevice::getAddress().toString().c_str();
    mac.replace(":", "");
    mac.toUpperCase();
    String deviceName = "SmartFlow-" + mac.substring(6); // Last 6 chars

    NimBLEDevice::init(deviceName.c_str());
    NimBLEServer* pServer = NimBLEDevice::createServer();
    NimBLEService* pService = pServer->createService(PROV_SERVICE_UUID);
    
    ProvCallbacks* callbacks = new ProvCallbacks();

    NimBLECharacteristic* pSsidChar = pService->createCharacteristic(SSID_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
    pSsidChar->setCallbacks(callbacks);

    NimBLECharacteristic* pPassChar = pService->createCharacteristic(PASS_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
    pPassChar->setCallbacks(callbacks);

    // Load or generate Claim Token
    if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        provToken = prefs.getString("claim_token", "");
        if (provToken.length() == 0) {
            // Generate 16-char random hex token
            char tokenBuf[17];
            sprintf(tokenBuf, "%08X%08X", esp_random(), esp_random());
            provToken = String(tokenBuf);
            prefs.putString("claim_token", provToken);
        }
        prefs.end();
    }

    NimBLECharacteristic* pTokenChar = pService->createCharacteristic(TOKEN_CHAR_UUID, NIMBLE_PROPERTY::READ);
    pTokenChar->setValue(provToken.c_str());

    NimBLECharacteristic* pCommitChar = pService->createCharacteristic(COMMIT_CHAR_UUID, NIMBLE_PROPERTY::WRITE);
    pCommitChar->setCallbacks(callbacks);

    pService->start();
    
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(PROV_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();
    
    LOG(APP_LOG_LEVEL_INFO, "BLE", "Provisioning started. Advertising as %s", deviceName.c_str());
}

void BleProvisioning::loop() {
    if (provisionCommitReceived) {
        provisionCommitReceived = false;
        
        if (provSsid.length() > 0) {
            if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
                prefs.putString("wifi_ssid", provSsid);
                prefs.putString("wifi_pass", provPass);
                prefs.end();
                
                LOG(APP_LOG_LEVEL_INFO, "BLE", "Credentials saved to NVS. Transitioning to ONLINE.");
                deviceLifecycle = DeviceLifecycle::ONLINE;
                stop();
            }
        } else {
            LOG(APP_LOG_LEVEL_WARN, "BLE", "Commit received but SSID is empty.");
        }
    }
}

void BleProvisioning::stop() {
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
