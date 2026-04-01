// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

#include "sensor_node_shared.h"

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

// Phase 1: log level (LOG_INFO = production default)
// Raised to LOG_DEBUG when DEBUG_USB_MODE=1 for bench sessions.
uint8_t  snLogLevel          = LOG_INFO;

// Phase 2: bug-fix state variables
uint16_t snLevelDiscardCount = 0;  // H-02: plausibility-rejected readings this measurement window
uint8_t  flowErrAssertCount  = 0;  // H-04: consecutive 1s windows above noise-assert threshold
uint8_t  flowErrClearCount   = 0;  // H-04: consecutive 1s windows below noise-clear threshold

