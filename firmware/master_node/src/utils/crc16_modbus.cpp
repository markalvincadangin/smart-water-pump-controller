/**
 * @file crc16_modbus.cpp
 * @brief CRC-16 Modbus calculation utility implementation.
 */
#include "crc16_modbus.h"

uint16_t Crc16::calculateModbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA001);
      else crc = (uint16_t)(crc >> 1);
    }
  }
  return crc;
}
