import { DeviceConfig } from "./types";
import {
  DEVICE_CONFIG_ULTRASONIC_MAX_CM,
  DEVICE_CONFIG_ULTRASONIC_MIN_CM,
} from "./constants";

/**
 * Device configuration validation with strict bounds checking for firmware-adjacent parameters.
 */

export interface ValidationResult {
  isValid: boolean;
  error: string | null;
}

export function validateDeviceConfig(form: DeviceConfig): ValidationResult {
  // Tank Dimensions
  if (form.tank_full_cm >= form.tank_empty_cm) {
    return { isValid: false, error: "Full (cm) must be less than Empty (cm)." };
  }
  if (form.tank_empty_cm < DEVICE_CONFIG_ULTRASONIC_MIN_CM || form.tank_empty_cm > DEVICE_CONFIG_ULTRASONIC_MAX_CM) {
    return {
      isValid: false,
      error: `Empty distance: enter ${DEVICE_CONFIG_ULTRASONIC_MIN_CM}-${DEVICE_CONFIG_ULTRASONIC_MAX_CM} cm.`,
    };
  }
  if (form.tank_full_cm < DEVICE_CONFIG_ULTRASONIC_MIN_CM || form.tank_full_cm >= form.tank_empty_cm) {
    return {
      isValid: false,
      error: `Full distance: enter ${DEVICE_CONFIG_ULTRASONIC_MIN_CM} to (Empty - 1) cm.`,
    };
  }

  // Automation Thresholds
  if (form.pump_start_level >= form.pump_stop_level) {
    return { isValid: false, error: "Pump start level must be less than stop level." };
  }
  if (form.pump_start_level < 0 || form.pump_start_level > 100) {
    return { isValid: false, error: "Pump start: 0\u2013100%." };
  }
  if (form.pump_stop_level < 0 || form.pump_stop_level > 100) {
    return { isValid: false, error: "Pump stop: 0\u2013100%." };
  }
  // Per QA spec: 30–480 minutes
  if (form.max_pump_runtime_min < 30 || form.max_pump_runtime_min > 480) {
    return { isValid: false, error: "Max runtime: 30\u2013480 minutes." };
  }

  // Safety Guards
  if (form.dry_run_threshold_lpm < 0.1 || form.dry_run_threshold_lpm > 10) {
    return { isValid: false, error: "No-flow threshold: 0.1\u201310 L/min." };
  }
  // Per QA spec: 10–300 sec
  if (form.dry_run_timeout_sec < 10 || form.dry_run_timeout_sec > 300) {
    return { isValid: false, error: "Shutdown delay: 10\u2013300 sec." };
  }
  // Per QA spec: 10–300 sec
  if (form.auto_bypass_on_sensor_fail && (form.auto_bypass_delay_sec == null || form.auto_bypass_delay_sec < 10 || form.auto_bypass_delay_sec > 300)) {
    return { isValid: false, error: "Auto-bypass delay: 10\u2013300 sec when enabled." };
  }

  // Intervals & Calibration
  if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 20) {
    return { isValid: false, error: "Flow calibration factor: 0.1\u201320." };
  }
  // Per QA plan target: 5000–60000ms
  if (form.idle_sensor_interval_ms < 5000 || form.idle_sensor_interval_ms > 60000) {
    return { isValid: false, error: "Idle level check: 5000\u201360000 ms." };
  }
  // Per QA plan target: 10000–120000ms
  if (form.idle_firebase_interval_ms < 10000 || form.idle_firebase_interval_ms > 120000) {
    return { isValid: false, error: "Sync interval: 10000\u2013120000 ms." };
  }

  return { isValid: true, error: null };
}

export type DeviceConfigField =
  | "tank_empty_cm"
  | "tank_full_cm"
  | "pump_start_level"
  | "pump_stop_level"
  | "dry_run_threshold_lpm"
  | "dry_run_timeout_sec"
  | "max_pump_runtime_min"
  | "flow_calibration_factor"
  | "idle_sensor_interval_ms"
  | "idle_firebase_interval_ms"
  | "auto_bypass_delay_sec";

export type FieldErrors = Partial<Record<DeviceConfigField, string>>;

/**
 * Field-level validation for inline error display in configuration forms.
 */
export function validateDeviceConfigFields(form: DeviceConfig): FieldErrors {
  const errors: FieldErrors = {};

  if (form.tank_full_cm >= form.tank_empty_cm) {
    errors.tank_full_cm = "Must be less than Empty (cm).";
    errors.tank_empty_cm = "Must be greater than Full (cm).";
  }
  if (form.tank_empty_cm < DEVICE_CONFIG_ULTRASONIC_MIN_CM || form.tank_empty_cm > DEVICE_CONFIG_ULTRASONIC_MAX_CM) {
    errors.tank_empty_cm = `Enter ${DEVICE_CONFIG_ULTRASONIC_MIN_CM}-${DEVICE_CONFIG_ULTRASONIC_MAX_CM} cm.`;
  }
  if (form.tank_full_cm < DEVICE_CONFIG_ULTRASONIC_MIN_CM || form.tank_full_cm >= form.tank_empty_cm) {
    errors.tank_full_cm = `Enter ${DEVICE_CONFIG_ULTRASONIC_MIN_CM} to (Empty - 1) cm.`;
  }

  if (form.pump_start_level < 0 || form.pump_start_level > 100) {
    errors.pump_start_level = "Enter 0–100%.";
  }
  if (form.pump_stop_level < 0 || form.pump_stop_level > 100) {
    errors.pump_stop_level = "Enter 0–100%.";
  }
  if (form.pump_start_level >= form.pump_stop_level) {
    errors.pump_start_level = "Must be less than stop level.";
    errors.pump_stop_level = "Must be greater than start level.";
  }

  if (form.max_pump_runtime_min < 30 || form.max_pump_runtime_min > 480) {
    errors.max_pump_runtime_min = "Enter 30–480 min.";
  }

  if (form.dry_run_threshold_lpm < 0.1 || form.dry_run_threshold_lpm > 10) {
    errors.dry_run_threshold_lpm = "Enter 0.1–10.0 LPM.";
  }
  if (form.dry_run_timeout_sec < 10 || form.dry_run_timeout_sec > 300) {
    errors.dry_run_timeout_sec = "Enter 10–300 sec.";
  }

  if (form.auto_bypass_on_sensor_fail && (form.auto_bypass_delay_sec < 10 || form.auto_bypass_delay_sec > 300)) {
    errors.auto_bypass_delay_sec = "Enter 10–300 sec.";
  }

  if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 20) {
    errors.flow_calibration_factor = "Enter 0.1–20.0.";
  }
  if (form.idle_sensor_interval_ms < 5000 || form.idle_sensor_interval_ms > 60000) {
    errors.idle_sensor_interval_ms = "Enter 5000–60000 ms.";
  }
  if (form.idle_firebase_interval_ms < 10000 || form.idle_firebase_interval_ms > 120000) {
    errors.idle_firebase_interval_ms = "Enter 10000–120000 ms.";
  }

  return errors;
}
