#pragma once
#include <Arduino.h>

class Bootloader {
public:
    static void executeSetup();
    static String getBootReasonString();
};
