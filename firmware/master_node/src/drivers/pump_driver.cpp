#include "pump_driver.h"
#include "../hal/pump_hal.h"
#include "../utils/app_logger.h"
#include "../config/config.h"

void PumpDriver::init() {
    PumpHal::init();
    LOG(APP_LOG_LEVEL_INFO, "PUMP_DRV", "Pump Driver Initialized");
}

void PumpDriver::turnOn() {
    PumpHal::enable(true);
}

void PumpDriver::turnOff() {
    PumpHal::enable(false);
}

bool PumpDriver::isOn() {
    return PumpHal::isEnabled();
}
