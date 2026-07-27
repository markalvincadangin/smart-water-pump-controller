#pragma once

class UltrasonicHal {
public:
    // Initialize the ultrasonic hardware (or its communication bus)
    static void init();

    // Read the current water level percentage (0-100)
    static int readLevelPercent();

    // Check if the sensor is responding
    static bool isOnline();
};
