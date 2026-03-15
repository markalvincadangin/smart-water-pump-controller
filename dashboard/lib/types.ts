// lib/types.ts
// Firebase RTDB data structures — must match the ESP32 firmware exactly.

/** /pump_system/status — ESP32 → Cloud (pushed every 3s) */
export interface PumpStatus {
  water_level_percent: number;   // 0 – 100
  is_running: boolean;
  flow_rate_lpm: number;   // Litres per minute
  is_error: boolean;  // true = dry-run lockout active
  /** v3.0: level (ultrasonic) sensor failure. Older firmware may send is_sensor_error instead. */
  is_level_sensor_error?: boolean;
  /** v3.0: flow sensor stuck-high failure. */
  is_flow_sensor_error?: boolean;
  /** @deprecated Use is_level_sensor_error / is_flow_sensor_error. Kept for backward compat with older firmware. */
  is_sensor_error?: boolean;
  is_overflow_error: boolean;  // Phase 1: max runtime exceeded
  is_sleeping?: boolean;  // Phase 3: scheduled sleep active
  wifi_rssi: number;   // Phase 2: WiFi signal strength (dBm)
  last_boot_reason: string;  // Phase 2: e.g. 'Power-on', 'Task watchdog'
  uptime_minutes?: number; // Phase 5: Uptime counter (prevents 49-day rollover)
  // Phase 7 manual + v3.0 COUNTDOWN (replaces timed run)
  run_mode?: "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL" | "COUNTDOWN";
  countdown_remaining_sec?: number;
  last_fault_code?: string;
  last_fault_message?: string;
  /** v3.0: maintenance bypass active — level sensor ignored */
  bypass_level_sensor?: boolean;
  /** v3.0: true when bypass was auto-enabled by firmware (sensor failure); distinct from manual bypass */
  auto_bypass_active?: boolean;
  /** @deprecated Use bypass_level_sensor only. Kept for backward compat with older firmware. */
  is_maintenance_active?: boolean;
  /** v3.0: flow-based level estimate when sensor failed/bypass */
  estimated_level_pct?: number;
  level_estimate_active?: boolean;
  flow_volume_added_l?: number;
  level_last_valid_age_sec?: number;
  level_sensor_health_pct?: number;
  total_pump_cycles?: number;
  total_pump_run_min?: number;
  free_heap_bytes?: number;
  min_free_heap_bytes?: number;
  max_alloc_heap_bytes?: number;
  min_free_heap_observed_bytes?: number;
  firebase_consecutive_failures?: number;
  firebase_last_error?: string;
  ultrasonic_last_good_cm?: number;
  ultrasonic_cycles_ok?: number;
  ultrasonic_cycles_timeout?: number;
  flow_discard_max_sane?: number;
  flow_stuck_high_events?: number;
}

/** /pump_system/control — Cloud → ESP32 */
export interface PumpControl {
  mode: "AUTO" | "FORCE_ON" | "FORCE_OFF" | "COUNTDOWN";
  clear_error: boolean;
  reboot_request_id?: number;
  manual_start?: boolean;
  manual_stop?: boolean;
  /** v3.0 COUNTDOWN: duration in minutes when starting (1–120). */
  countdown_duration_min?: number;
  /** v3.0 COUNTDOWN: one-shot to add time; firmware reads this when countdown_add_time is true (1–120 min). */
  countdown_add_min?: number;
  /** v3.0 COUNTDOWN: one-shot to add time to running countdown (amount in countdown_add_min if set). */
  countdown_add_time?: boolean;
  /** v3.0: maintenance — ignore level sensor for start/stop; flow guard still active. */
  bypass_level_sensor?: boolean;
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
  /** Enable push notifications to phone/browser (FCM). Stored per device. */
  pushEnabled?: boolean;
  /** FCM tokens: deviceId -> token. Cloud Functions sends push to these. */
  fcmTokens?: Record<string, string>;
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
  /** Preferred key; firmware accepts level_sensor_failure_threshold or sensor_failure_threshold */
  level_sensor_failure_threshold?: number;  // 3–20
  sensor_failure_threshold: number;  // 3–20, legacy; use level_sensor_failure_threshold when writing
  idle_sensor_interval_ms: number;   // 5000–60000, slow-poll when tank ≥90%
  idle_firebase_interval_ms: number; // 10000–120000
  auto_bypass_on_sensor_fail?: boolean;
  auto_bypass_delay_sec?: number;    // 10–300
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
  auto_bypass_on_sensor_fail: false,
  auto_bypass_delay_sec: 60,
};
