/**
 * Complete typed interfaces for all Firebase RTDB paths.
 * Source of truth: SmartFlow System Refactor Plan v2.0 §4.2
 */

export type RunMode =
  | 'AUTO_STANDBY'
  | 'AUTO'
  | 'AUTO_COOLDOWN'
  | 'MANUAL_ON'
  | 'MANUAL_OFF'
  | 'MANUAL_COOLDOWN'
  | 'COUNTDOWN'
  | 'STOPPED';

export type FaultCode =
  | 'DRY_RUN'
  | 'OVERFLOW'
  | 'E_STOP'
  | 'COMM_LOSS'
  | 'STALE_LEVEL'
  | 'LEVEL_SENSOR'
  | 'FLOW_SENSOR'
  | 'SAFE_MODE'
  | '';

export type LogLevel = 0 | 1 | 2 | 3 | 4;

export type ControlMode = 'AUTO' | 'MANUAL' | 'COUNTDOWN';

/**
 * /pump_system/status — written by ESP32 every 3 seconds
 */
export interface PumpStatus {
  // Core state
  water_level_percent?: number;          // omitted when -1 (not yet valid)
  is_running: boolean;
  flow_rate_lpm: number;
  run_mode: RunMode;
  pump_cooldown_remaining_sec: number;   // 0 when not in cooldown

  // Error flags
  is_error: boolean;                     // DRY_RUN lockout active
  is_level_sensor_error?: boolean;      // Firmware canonical key (compat)
  is_sensor_error: boolean;             // Ultrasonic sensor failure
  is_flow_sensor_error: boolean;
  is_overflow_error: boolean;
  last_fault_code: FaultCode | string;
  last_fault_message: string;

  // Operational state
  is_idle_mode: boolean;                 // slow-poll mode active
  is_sleeping: boolean;                  // scheduled sleep active
  emergency_stop_latched: boolean;
  manual_desired: boolean;
  bypass_level_sensor: boolean;
  bypass_flow_sensor: boolean;
  manual_runtime_warning: boolean;       // ~90% of max runtime in MANUAL; pump still on until hard cutoff at limit

  // Sensor health
  remote_sensor_stable: boolean;         // 3 consecutive valid frames
  level_fresh: boolean;                  // age < staleness threshold
  level_sensor_health_pct: number;       // 0–100
  level_estimate_active: boolean;
  level_last_valid_age_sec?: number;
  estimated_level_pct?: number;
  remote_level_discard_count: number;

  // Timers
  countdown_active?: boolean;
  countdown_remaining_sec: number;

  // Flow stats
  flow_volume_added_l: number;

  // Connectivity
  wifi_rssi: number;

  // System
  uptime_minutes: number;
  last_boot_reason: string;
  debug_log_level: LogLevel;
  total_pump_cycles: number;
  total_pump_run_min: number;

  // Diagnostics
  ultrasonic_cycles_ok: number;
  ultrasonic_cycles_timeout: number;
  ultrasonic_last_good_cm: number;
  free_heap_bytes: number;
  min_free_heap_bytes?: number;         // Firmware canonical key (compat)
  min_free_heap_observed_bytes: number;
  max_alloc_heap_bytes?: number;
  firebase_consecutive_failures: number;
  firebase_last_error: string;
  cloud_last_control_ok_age_sec?: number;
  cloud_control_poll_stale?: boolean;
}

/**
 * /pump_system/control — read by ESP32 every 3 seconds, written by dashboard
 */
export interface PumpControl {
  mode: ControlMode;
  manual_desired: boolean;
  emergency_stop: boolean;              // one-shot
  reset_stop: boolean;                  // one-shot
  clear_error: boolean;                 // one-shot
  countdown_start: boolean;             // one-shot
  countdown_stop: boolean;              // one-shot
  countdown_duration_min: number;
  countdown_add_time: boolean;          // one-shot
  countdown_add_min: number;
  bypass_level_sensor: boolean;
  bypass_flow_sensor: boolean;
  reboot_request_id: number;
}

/**
 * /pump_system/config/device — read by ESP32 every 30 seconds, written by dashboard
 */
export interface DeviceConfig {
  tank_empty_cm: number;
  tank_full_cm: number;
  pump_start_level: number;             // must be < pump_stop_level
  pump_stop_level: number;
  dry_run_threshold_lpm: number;        // default 1.0
  dry_run_timeout_sec: number;
  max_pump_runtime_min: number;
  flow_calibration_factor: number;
  debug_log_level: LogLevel;
  sleep_enabled: boolean;
  sleep_start_hour: number;             // 0–23 PHT
  sleep_end_hour: number;
  sleep_emergency_level: number;
  sensor_failure_threshold: number;
  idle_sensor_interval_ms: number;
  idle_firebase_interval_ms: number;
  auto_bypass_on_sensor_fail: boolean;
  auto_bypass_delay_sec: number;
  level_sensor_failure_threshold?: number;
}

/**
 * /pump_system/config/notifications_by_user/$uid
 */
export interface NotificationPrefs {
  enabled: boolean;
  email: string;
  pushEnabled: boolean;
  fcmTokens: { [deviceId: string]: string };
  dryRunAlert: boolean;
  lowLevelAlert: boolean;
  lowLevelThreshold: number;
  pumpStartedAlert: boolean;
  overflowAlert: boolean;
}

/**
 * Convenience: null-safe status with all fields defaulted
 * Use when a component needs guaranteed non-null values
 */
export const DEFAULT_STATUS: PumpStatus = {
  is_running: false,
  flow_rate_lpm: 0,
  run_mode: 'AUTO_STANDBY',
  pump_cooldown_remaining_sec: 0,
  is_error: false,
  is_sensor_error: false,
  is_flow_sensor_error: false,
  is_overflow_error: false,
  last_fault_code: '',
  last_fault_message: '',
  is_idle_mode: false,
  is_sleeping: false,
  emergency_stop_latched: false,
  manual_desired: false,
  bypass_level_sensor: false,
  bypass_flow_sensor: false,
  manual_runtime_warning: false,
  remote_sensor_stable: false,
  level_fresh: false,
  level_sensor_health_pct: 0,
  level_estimate_active: false,
  remote_level_discard_count: 0,
  countdown_active: false,
  countdown_remaining_sec: 0,
  flow_volume_added_l: 0,
  wifi_rssi: 0,
  uptime_minutes: 0,
  last_boot_reason: '',
  debug_log_level: 2,
  total_pump_cycles: 0,
  total_pump_run_min: 0,
  ultrasonic_cycles_ok: 0,
  ultrasonic_cycles_timeout: 0,
  ultrasonic_last_good_cm: 0,
  free_heap_bytes: 0,
  min_free_heap_bytes: 0,
  min_free_heap_observed_bytes: 0,
  firebase_consecutive_failures: 0,
  firebase_last_error: '',
  cloud_last_control_ok_age_sec: 0,
  cloud_control_poll_stale: false,
};
export interface PumpSnapshot {
  status: PumpStatus;
  control: PumpControl;
  updatedAt: number;
}

/**
 * Backward-compatibility wrappers for untransformed UI components
 */
export type NotificationConfig = NotificationPrefs;

export interface HistoryEntry {
  time: string;
  level: number;
  flow: number;
  [key: string]: unknown;
}

export interface HistoryEvent {
  time: string;
  type: string;
  message?: string;
  runMode?: string;
  prevRunMode?: string;
  faultCode?: string;
  details?: unknown;
}

export const DEFAULT_DEVICE_CONFIG: DeviceConfig = {
  tank_empty_cm: 200,
  tank_full_cm: 30,
  pump_start_level: 20,
  pump_stop_level: 90,
  dry_run_threshold_lpm: 1.0,
  dry_run_timeout_sec: 30,
  max_pump_runtime_min: 120,
  flow_calibration_factor: 7.5,
  debug_log_level: 2,
  sleep_enabled: false,
  sleep_start_hour: 22,
  sleep_end_hour: 5,
  sleep_emergency_level: 10,
  sensor_failure_threshold: 3,
  idle_sensor_interval_ms: 10000,
  idle_firebase_interval_ms: 30000,
  auto_bypass_on_sensor_fail: false,
  auto_bypass_delay_sec: 60,
  level_sensor_failure_threshold: 3,
};

export const DEFAULT_NOTIFICATION_CONFIG: NotificationPrefs = {
  enabled: true,
  email: '',
  pushEnabled: true,
  fcmTokens: {},
  dryRunAlert: true,
  lowLevelAlert: true,
  lowLevelThreshold: 20,
  pumpStartedAlert: true,
  overflowAlert: true,
};

