/**
 * @file water_level_service.h
 * @brief Service layer for managing and caching the water level.
 */
#pragma once

#include <stdint.h>

class WaterLevelService {
public:
  /**
   * @brief Update the cached water level from the sensor driver.
   */
  static void update();

  /**
   * @brief Get the currently cached water level.
   * @return The water level percentage (0-100).
   */
  static int getCurrentLevel();

  /**
   * @brief Check if the cached water level data is fresh.
   * @return true if data was recently updated, false otherwise.
   */
  static bool isDataFresh();
    
private:
  static int lastKnownLevel;
  static uint32_t lastUpdateMs;
};
