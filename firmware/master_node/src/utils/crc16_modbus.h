#pragma once

#include <stdint.h>
#include <stddef.h>

class Crc16 {
public:
    static uint16_t calculateModbus(const uint8_t* data, size_t len);
};
