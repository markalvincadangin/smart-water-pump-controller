#include "pump_hal.h"
#include "../config/hardware.h"

void PumpHal::init() {
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, HIGH); // Active-LOW: HIGH means OFF initially
}

void PumpHal::enable(bool on) {
    digitalWrite(PIN_RELAY, on ? LOW : HIGH); // Active-LOW
}

bool PumpHal::isEnabled() {
    return digitalRead(PIN_RELAY) == LOW; // Active-LOW
}
