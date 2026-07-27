#include "ultrasonic_hal.h"
#include "../rs485/rs485_comm.h"

void UltrasonicHal::init() {
    // RS485 initialization is currently handled globally by rs485_init() in main,
    // but in the future, if a direct ultrasonic sensor is attached, GPIO setup would go here.
}

int UltrasonicHal::readLevelPercent() {
    return Rs485Comm::getParsedData().waterLevelPct;
}

bool UltrasonicHal::isOnline() {
    return Rs485Comm::getParsedData().online;
}
