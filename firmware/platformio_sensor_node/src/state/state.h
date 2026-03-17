#pragma once

#include "../config/config.h"

extern volatile uint32_t flowPulseCount;
extern volatile uint32_t flowPulseDiscardCount;
extern volatile uint32_t flowLastPulseUs;

extern int   snWaterLevelPct;
extern float snLastDistanceCm;
extern float snFlowRateLpm;
extern int   snErrCode;

extern uint32_t snLastLevelUpdateMs;
extern uint32_t snLastFlowUpdateMs;
extern int      snLastGoodLevelPct;
extern bool     snLevelError;
extern bool     snFlowError;
extern uint8_t  snSeq;

