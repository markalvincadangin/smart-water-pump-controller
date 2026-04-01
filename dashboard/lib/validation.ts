import { DeviceConfig } from "./types";

/**
 * REFACTOR [D6.1]: Device Configuration Validation
 * Strict bounds checking for firmware-adjacent parameters.
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
  if (form.tank_empty_cm < 5 || form.tank_empty_cm > 200) {
    return { isValid: false, error: "Empty distance: enter 5\u2013200 cm." };
  }
  if (form.tank_full_cm < 1 || form.tank_full_cm >= form.tank_empty_cm) {
    return { isValid: false, error: "Full distance: enter 1 to (Empty - 1) cm." };
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
  if (form.max_pump_runtime_min < 1 || form.max_pump_runtime_min > 480) {
    return { isValid: false, error: "Max runtime: 1\u2013480 minutes." };
  }

  // Safety Guards
  if (form.dry_run_threshold_lpm < 0.1 || form.dry_run_threshold_lpm > 10) {
    return { isValid: false, error: "No-flow threshold: 0.1\u201310 L/min." };
  }
  if (form.dry_run_timeout_sec < 5 || form.dry_run_timeout_sec > 600) {
    return { isValid: false, error: "Shutdown delay: 5\u2013600 sec." };
  }
  if (form.auto_bypass_on_sensor_fail && (form.auto_bypass_delay_sec == null || form.auto_bypass_delay_sec < 5 || form.auto_bypass_delay_sec > 600)) {
    return { isValid: false, error: "Auto-bypass delay: 5\u2013600 sec when enabled." };
  }

  // Intervals & Calibration
  if (form.flow_calibration_factor < 0.1 || form.flow_calibration_factor > 20) {
    return { isValid: false, error: "Flow calibration factor: 0.1\u201320." };
  }
  if (form.idle_sensor_interval_ms < 1000 || form.idle_sensor_interval_ms > 120000) {
    return { isValid: false, error: "Idle level check: 1000\u2013120000 ms." };
  }
  if (form.idle_firebase_interval_ms < 10000 || form.idle_firebase_interval_ms > 600000) {
    return { isValid: false, error: "Sync interval: 10000\u2013600000 ms." };
  }

  return { isValid: true, error: null };
}
