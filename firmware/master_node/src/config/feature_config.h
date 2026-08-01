/**
 * @file feature_config.h
 * @brief Centralized control for SmartFlow Master Node capabilities.
 *
 * Use these flags to enable or disable major features (like RS-485
 * polling or auto-mode capabilities) at compile time.
 */
#pragma once

// =========================================================================
// FEATURE FLAGS
// =========================================================================

// Enables RS-485 sensor node polling and communication.
// When false, Master Node operates strictly on local timer/cloud commands.
#define FEATURE_SENSOR_SERVICE false

// Enables automatic pump operation based on tank levels.
// When false, only Manual and Countdown modes are permitted.
#define FEATURE_AUTO_MODE false
