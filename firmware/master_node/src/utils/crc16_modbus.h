/**
 * @file crc16_modbus.h
 * @brief CRC-16 Modbus calculation utility.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Utility class for CRC-16 Modbus calculations.
 */
class Crc16 {
public:
  /**
   * @brief Calculate the CRC-16 Modbus value for a data buffer.
   * @param data Pointer to the data buffer.
   * @param len Length of the data buffer in bytes.
   * @return The 16-bit CRC value.
   */
  static uint16_t calculateModbus(const uint8_t* data, size_t len);
};
