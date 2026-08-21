#pragma once

#include "../config/config.h"

/**
 * @brief Loads the device configuration from NVS storage.
 * 
 * Reads all hardware and policy parameters (e.g., tank thresholds, dry-run values).
 * Falls back to firmware defaults if the stored schema is outdated or values are corrupted.
 */
void loadDeviceConfigFromNVS();

/**
 * @brief Saves the current device configuration to NVS storage.
 * 
 * Persists updated thresholds and parameters to non-volatile flash.
 */
void saveDeviceConfigToNVS();

/**
 * @brief Evaluates boot cycles to detect continuous boot-looping.
 * 
 * Increments the boot counter. If the counter exceeds the threshold, places the 
 * device into Safe Mode to prevent hardware damage from rapid cycling.
 */
void checkCrashLoop();

/**
 * @brief Restores runtime state from NVS.
 * 
 * Loads pump mode, dry-run flags, bypass flags, and telemetry counters (cycles, run time).
 */
void loadStateFromNVS();

/**
 * @brief Persists runtime state to NVS.
 * 
 * Optimized to only write parameters that have actually changed since the last persist operation,
 * minimizing flash wear and boot latencies.
 */
void persistStateToNVS();

/**
 * @brief Clears only local Wi-Fi and ownership enrollment material.
 * 
 * Safety state and configuration parameters remain intact. Used when reprovisioning the device.
 * 
 * @return true if successfully cleared, false otherwise.
 */
bool clearNetworkEnrollment();
