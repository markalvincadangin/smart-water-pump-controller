#include "state.h"

volatile uint32_t flowPulseCount = 0;
volatile uint32_t flowPulseDiscardCount = 0;
volatile uint32_t flowLastPulseUs = 0;

int   snWaterLevelPct = 0;
float snLastDistanceCm = -1.0f;
float snFlowRateLpm   = 0.0f;
int   snErrCode       = 0;

uint32_t snLastLevelUpdateMs = 0;
uint32_t snLastFlowUpdateMs  = 0;
int      snLastGoodLevelPct  = 0;
bool     snLevelError        = false;
bool     snFlowError         = false;
uint8_t  snSeq               = 0;
uint16_t snLevelDiscardCount = 0;
uint8_t  flowErrAssertCount  = 0;
uint8_t  flowErrClearCount   = 0;

