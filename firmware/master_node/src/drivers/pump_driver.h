#pragma once

class PumpDriver {
public:
    static void init();
    static void turnOn();
    static void turnOff();
    static bool isOn();
};
