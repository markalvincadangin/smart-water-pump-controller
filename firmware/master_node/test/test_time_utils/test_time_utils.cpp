#include <Arduino.h>
#include <unity.h>

#include "../../src/utils/time_utils.h"

static void test_elapsed_millis_handles_counter_wrap(void) {
  const uint32_t start = UINT32_MAX - 9U;
  const uint32_t now = 15U;
  TEST_ASSERT_EQUAL_UINT32(25U, elapsedMillis32(now, start));
  TEST_ASSERT_TRUE(elapsedMillis32(now, start) >= 20U);
}

static void test_deadline_handles_counter_wrap(void) {
  const uint32_t deadline = 8U;
  TEST_ASSERT_FALSE(millisDeadlineReached(UINT32_MAX - 1U, deadline));
  TEST_ASSERT_TRUE(millisDeadlineReached(8U, deadline));
  TEST_ASSERT_TRUE(millisDeadlineReached(12U, deadline));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_elapsed_millis_handles_counter_wrap);
  RUN_TEST(test_deadline_handles_counter_wrap);
  UNITY_END();
}

void loop() {}
