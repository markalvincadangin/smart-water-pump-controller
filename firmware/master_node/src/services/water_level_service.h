#pragma once

#include <stdint.h>

class WaterLevelService {
public:
    static void update();
    static int getCurrentLevel();
    static bool isDataFresh();
    
private:
    static int lastKnownLevel;
    static uint32_t lastUpdateMs;
};
