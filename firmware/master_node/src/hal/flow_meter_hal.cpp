/**
 * @file flow_meter_hal.cpp
 * @brief Hardware Abstraction Layer for the Flow Meter.
 */
#include "flow_meter_hal.h"
#include "../rs485/rs485_comm.h"

void FlowMeterHal::init() {
  // RS485 initialization is currently handled globally
}

float FlowMeterHal::readFlowRateLpm() {
  return Rs485Comm::getParsedData().flowRateLpm;
}

bool FlowMeterHal::isOnline() {
  return Rs485Comm::getParsedData().online;
}
