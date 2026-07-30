#pragma once

// =========================================================================
// FEATURE FLAGS
// Centralized control for SmartFlow Master Node capabilities.
// =========================================================================

// Enables RS-485 sensor node polling and communication.
// When false, Master Node operates strictly on local timer/cloud commands.
#define FEATURE_SENSOR_SERVICE false

// Enables automatic pump operation based on tank levels.
// When false, only Manual and Countdown modes are permitted.
#define FEATURE_AUTO_MODE false
