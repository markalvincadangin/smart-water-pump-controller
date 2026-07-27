#pragma once

#include <Arduino.h>

class OtaManager {
public:
    static OtaManager& getInstance() {
        static OtaManager instance;
        return instance;
    }

    void begin();
    
    // Perform an OTA update from the given HTTPS URL
    // Expected to validate the SHA256 hash before committing the swap.
    bool performUpdate(const String& url, const String& expectedSha256);

    // After a successful boot following an OTA, this marks the partition as valid
    // to prevent rollback.
    void markFirmwareValid();

private:
    OtaManager() = default;
    ~OtaManager() = default;

    OtaManager(const OtaManager&) = delete;
    OtaManager& operator=(const OtaManager&) = delete;
};

extern OtaManager& ota_manager;
