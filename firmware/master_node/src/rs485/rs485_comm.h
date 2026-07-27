#pragma once

#include "../config/config.h"

struct Rs485SensorData {
  int   waterLevelPct;   // 0..100
  float flowRateLpm;     // >=0
  int   errCode;         // protocol ERR:<code>
  bool  online;          // derived from last receive age
};

class Rs485Comm {
public:
    static void init();
    static bool requestData(uint32_t timeBudgetMs = 0);
    static Rs485SensorData getParsedData();
};
