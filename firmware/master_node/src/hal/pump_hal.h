#pragma once

#include <Arduino.h>

class PumpHal {
public:
    // Initialize the GPIO pin for the pump relay
    static void init();

    // Turn the pump on (true) or off (false)
    static void enable(bool on);

    // Get the current physical state of the pump pin
    static bool isEnabled();
};
