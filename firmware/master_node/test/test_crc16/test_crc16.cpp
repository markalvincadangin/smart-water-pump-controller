#include <Arduino.h>
#include <unity.h>

#include "../../src/utils/crc16_modbus.h"

static void test_crc_known_modbus_vector(void) {
  const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
  uint16_t crc = Crc16::calculateModbus(frame, sizeof(frame));
  TEST_ASSERT_EQUAL_HEX16(0xCDC5, crc);
}

static void test_crc_zero_length_returns_seed(void) {
  uint16_t crc = Crc16::calculateModbus(nullptr, 0);
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc);
}

static void test_crc_single_byte_frame(void) {
  const uint8_t frame[] = {0xAA};
  uint16_t crc = Crc16::calculateModbus(frame, sizeof(frame));
  TEST_ASSERT_EQUAL_HEX16(0x3F3F, crc);
}

static void test_crc_detects_corruption(void) {
  const uint8_t original[] = {0x10, 0x06, 0x00, 0x2A, 0x00, 0x01};
  const uint8_t corrupted[] = {0x10, 0x06, 0x00, 0x2A, 0x00, 0x00};
  uint16_t crcOriginal = Crc16::calculateModbus(original, sizeof(original));
  uint16_t crcCorrupt = Crc16::calculateModbus(corrupted, sizeof(corrupted));
  TEST_ASSERT_NOT_EQUAL(crcOriginal, crcCorrupt);
}

static void test_crc_large_frame_stability(void) {
  uint8_t data[256];
  for (size_t i = 0; i < sizeof(data); ++i) data[i] = static_cast<uint8_t>(i);
  uint16_t crc1 = Crc16::calculateModbus(data, sizeof(data));
  uint16_t crc2 = Crc16::calculateModbus(data, sizeof(data));
  TEST_ASSERT_EQUAL_HEX16(crc1, crc2);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_crc_known_modbus_vector);
  RUN_TEST(test_crc_zero_length_returns_seed);
  RUN_TEST(test_crc_single_byte_frame);
  RUN_TEST(test_crc_detects_corruption);
  RUN_TEST(test_crc_large_frame_stability);
  UNITY_END();
}

void loop() {}
