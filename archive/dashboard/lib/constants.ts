// Dashboard constants

/** Seconds without a status update before the ESP32 is considered offline. */
export const ESP32_STALE_SEC = 15;

/** JSN-SR04T nominal measurable range (datasheet/field usage). */
export const JSN_SR04T_MIN_CM = 25;
export const JSN_SR04T_MAX_CM = 450;

/**
 * Dashboard device-config guardrail for ultrasonic calibration.
 * Upper bound is capped by firmware parser compatibility (<= 200 cm).
 */
export const DEVICE_CONFIG_ULTRASONIC_MIN_CM = JSN_SR04T_MIN_CM;
export const DEVICE_CONFIG_ULTRASONIC_MAX_CM = 200;
