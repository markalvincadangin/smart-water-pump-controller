#pragma once

class SensorDriver {
public:
    static void init();
    
    static int getWaterLevelPercent();
    static float getFlowRateLpm();
    
    static bool isUltrasonicOnline();
    static bool isFlowMeterOnline();
};
