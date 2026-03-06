// lib/types.ts
// Firebase RTDB data structures — must match the ESP32 firmware exactly.

/** /pump_system/status — ESP32 → Cloud (pushed every 3s) */
export interface PumpStatus {
  water_level_percent: number;   // 0 – 100
  is_running:          boolean;
  flow_rate_lpm:       number;   // Litres per minute
  is_error:            boolean;  // true = dry-run lockout active
}

/** /pump_system/control — Cloud → ESP32 */
export interface PumpControl {
  mode:        "AUTO" | "FORCE_ON" | "FORCE_OFF";
  clear_error: boolean;
}

/** Combined snapshot used by the dashboard UI */
export interface PumpSnapshot {
  status:    PumpStatus;
  control:   PumpControl;
  updatedAt: number;  // Date.now() of last received update
}

/** One entry in the local history chart */
export interface HistoryEntry {
  time:  string;  // HH:MM:SS
  level: number;
  flow:  number;
}

/** /pump_system/config/notifications_by_user/{uid} — per-user notification settings (Dashboard ↔ Cloud Function) */
export interface NotificationConfig {
  enabled:           boolean;
  email:             string;
  dryRunAlert:       boolean;
  lowLevelAlert:     boolean;
  lowLevelThreshold: number;  // 0–50
  pumpStartedAlert:  boolean;
}

export const DEFAULT_NOTIFICATION_CONFIG: NotificationConfig = {
  enabled:           false,
  email:             "",
  dryRunAlert:       true,
  lowLevelAlert:     true,
  lowLevelThreshold: 20,
  pumpStartedAlert:  true,
};

/** /pump_system/config/device — Calibration & thresholds (dashboard/ESP32). Write = same UIDs as control/mode. */
export interface DeviceConfig {
  tank_empty_cm:          number;
  tank_full_cm:           number;
  pump_start_level:      number;
  pump_stop_level:       number;
  dry_run_threshold_lpm: number;
  dry_run_timeout_sec:    number;
  flow_calibration_factor: number;
}

export const DEFAULT_DEVICE_CONFIG: DeviceConfig = {
  tank_empty_cm:           122,
  tank_full_cm:            8,
  pump_start_level:        30,
  pump_stop_level:         100,
  dry_run_threshold_lpm:   0.5,
  dry_run_timeout_sec:     30,
  flow_calibration_factor: 1.0,
};
