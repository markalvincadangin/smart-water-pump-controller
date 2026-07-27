#pragma once

#include <stdint.h>
#include <stddef.h>

// Standard Modbus CRC-16 (poly 0xA001, init 0xFFFF).
uint16_t crc16_modbus(const uint8_t* data, size_t len);

