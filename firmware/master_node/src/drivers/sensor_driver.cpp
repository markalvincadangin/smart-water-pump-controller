#include "sensor_driver.h"
#include "../hal/ultrasonic_hal.h"
#include "../hal/flow_meter_hal.h"
#include "../utils/app_logger.h"
#include "../config/config.h"

void SensorDriver::init() {
    UltrasonicHal::init();
    FlowMeterHal::init();
    LOG(LOG_LEVEL_INFO, "SENS_DRV", "Sensor Driver Initialized");
}

int SensorDriver::getWaterLevelPercent() {
    return UltrasonicHal::readLevelPercent();
}

float SensorDriver::getFlowRateLpm() {
    return FlowMeterHal::readFlowRateLpm();
}

bool SensorDriver::isUltrasonicOnline() {
    return UltrasonicHal::isOnline();
}

bool SensorDriver::isFlowMeterOnline() {
    return FlowMeterHal::isOnline();
}
