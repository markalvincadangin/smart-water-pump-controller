/**
 * @file water_level_service.cpp
 * @brief Service layer for managing and caching the water level.
 */
#include "water_level_service.h"
#include "../drivers/sensor_driver.h"
#include "../config/config.h"
#include <Arduino.h>

int WaterLevelService::lastKnownLevel = 0;
uint32_t WaterLevelService::lastUpdateMs = 0;

void WaterLevelService::update() {
  if (SensorDriver::isUltrasonicOnline()) {
    lastKnownLevel = SensorDriver::getWaterLevelPercent();
    lastUpdateMs = millis();
  }
}

int WaterLevelService::getCurrentLevel() {
  return lastKnownLevel;
}

bool WaterLevelService::isDataFresh() {
  return (millis() - lastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS;
}
