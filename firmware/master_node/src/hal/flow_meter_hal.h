#pragma once

class FlowMeterHal {
public:
    // Initialize the flow meter hardware (or its communication bus)
    static void init();

    // Read the current flow rate in Liters Per Minute (LPM)
    static float readFlowRateLpm();

    // Check if the sensor is responding
    static bool isOnline();
};
