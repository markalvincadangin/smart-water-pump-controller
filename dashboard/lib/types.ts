// lib/types.ts
// Firebase RTDB data structures — must match the ESP32 firmware exactly.

/** /pump_system/status — ESP32 → Cloud (pushed every 3s) */
export interface PumpStatus {
  water_level_percent: number;   // 0 – 100
  is_running: boolean;
  flow_rate_lpm: number;   // Litres per minute
  is_error: boolean;  // true = dry-run lockout active
  is_sensor_error: boolean;  // Phase 1: ultrasonic/flow sensor failure
  is_overflow_error: boolean;  // Phase 1: max runtime exceeded
  is_sleeping?: boolean;  // Phase 3: scheduled sleep active
  wifi_rssi: number;   // Phase 2: WiFi signal strength (dBm)
  last_boot_reason: string;  // Phase 2: e.g. 'Power-on', 'Task watchdog'
  uptime_minutes?: number; // Phase 5: Uptime counter (prevents 49-day rollover)
}

/** /pump_system/control — Cloud → ESP32 */
export interface PumpControl {
  mode: "AUTO" | "FORCE_ON" | "FORCE_OFF";
  clear_error: boolean;
}

/** Combined snapshot used by the dashboard UI */
export interface PumpSnapshot {
  status: PumpStatus;
  control: PumpControl;
  updatedAt: number;  // Date.now() of last received update
}

/** One entry in the local history chart */
export interface HistoryEntry {
  time: string;  // HH:MM:SS
  level: number;
  flow: number;
}

/** /pump_system/config/notifications_by_user/{uid} — per-user notification settings (Dashboard ↔ Cloud Function) */
export interface NotificationConfig {
  enabled: boolean;
  email: string;
  dryRunAlert: boolean;
  lowLevelAlert: boolean;
  lowLevelThreshold: number;  // 0–50
  pumpStartedAlert: boolean;
  overflowAlert: boolean;  // Phase 2: max runtime overflow
}

export const DEFAULT_NOTIFICATION_CONFIG: NotificationConfig = {
  enabled: false,
  email: "",
  dryRunAlert: true,
  lowLevelAlert: true,
  lowLevelThreshold: 20,
  pumpStartedAlert: true,
  overflowAlert: true,
};

/** /pump_system/config/device — Calibration & thresholds (dashboard/ESP32). Write = same UIDs as control/mode. */
export interface DeviceConfig {
  tank_empty_cm: number;
  tank_full_cm: number;
  pump_start_level: number;
  pump_stop_level: number;
  dry_run_threshold_lpm: number;
  dry_run_timeout_sec: number;
  flow_calibration_factor: number;
  max_pump_runtime_min: number;  // Phase 1: max AUTO runtime before overflow error
  // Phase 3: Scheduled sleep
  sleep_enabled: boolean;
  sleep_start_hour: number;  // 0–23
  sleep_end_hour: number;   // 0–23
  sleep_emergency_level: number;  // 0–100, bypass sleep if level ≤ this
  // Phase 4: Advanced
  sensor_failure_threshold: number;  // 3–20, consecutive ultrasonic timeouts
  idle_sensor_interval_ms: number;   // 5000–60000, slow-poll when tank ≥90%
  idle_firebase_interval_ms: number; // 10000–120000
}

export const DEFAULT_DEVICE_CONFIG: DeviceConfig = {
  tank_empty_cm: 122,
  tank_full_cm: 8,
  pump_start_level: 30,
  pump_stop_level: 100,
  dry_run_threshold_lpm: 0.5,
  dry_run_timeout_sec: 30,
  flow_calibration_factor: 7.5,
  max_pump_runtime_min: 120,
  sleep_enabled: false,
  sleep_start_hour: 23,
  sleep_end_hour: 5,
  sleep_emergency_level: 5,
  sensor_failure_threshold: 5,
  idle_sensor_interval_ms: 10000,
  idle_firebase_interval_ms: 30000,
};
