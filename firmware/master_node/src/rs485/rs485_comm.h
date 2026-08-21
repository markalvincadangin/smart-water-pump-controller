/**
 * @file rs485_comm.h
 * @brief Manages RS485 communication with external sensors.
 */
#pragma once

#include "../config/config.h"

/**
 * @brief Structure containing parsed data from the RS485 sensor node.
 */
struct Rs485SensorData {
  int   waterLevelPct;   /**< 0..100 */
  float flowRateLpm;     /**< >=0 */
  int   errCode;         /**< protocol ERR:<code> */
  bool  online;          /**< derived from last receive age */
};

class Rs485Comm {
public:
  /**
   * @brief Initialize the RS485 subsystem.
   */
  static void init();

  /**
   * @brief Poll the remote sensor node for data.
   * @param timeBudgetMs Time budget in milliseconds (0 for unlimited).
   * @return true if a valid frame was received, false otherwise.
   */
  static bool requestData(uint32_t timeBudgetMs = 0);

  /**
   * @brief Get the most recently parsed sensor data.
   * @return The latest Rs485SensorData.
   */
  static Rs485SensorData getParsedData();
};
