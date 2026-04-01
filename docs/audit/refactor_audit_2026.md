# SmartFlow - System Refactor Phase 0 Audit Report

Document version: 1.2
Date: 2026-03-31
Status: COMPLETE - Phase 0 exit criteria met (revalidated against current source)

This report supersedes assumptions in .plans/smartflow_refactor_plan_v2.md.

## 0.1 Source File Inventory

Scope required by the plan:
- firmware/arduino_smart_water_pump_controller/
- firmware/platformio_smart_water_pump_controller/src/
- firmware/arduino_sensor_node/
- firmware/platformio_sensor_node/src/
- dashboard/app/
- dashboard/components/
- dashboard/lib/

### 0.1.1 ESP32 Master (Arduino)
- firmware/arduino_smart_water_pump_controller/01_config.ino
- firmware/arduino_smart_water_pump_controller/02_rs485_comm.ino
- firmware/arduino_smart_water_pump_controller/03_safety_pump.ino
- firmware/arduino_smart_water_pump_controller/04_persistence.ino
- firmware/arduino_smart_water_pump_controller/05_connectivity_cloud.ino
- firmware/arduino_smart_water_pump_controller/arduino_smart_water_pump_controller.ino
- firmware/arduino_smart_water_pump_controller/secrets.h.example
- firmware/arduino_smart_water_pump_controller/smart_water_pump_controller_shared.h

Primary responsibilities:
- Entry point and scheduler: arduino_smart_water_pump_controller.ino
- Runtime config and globals: 01_config.ino
- RS-485 master link and frame parser: 02_rs485_comm.ino
- Safety and run-mode state machine: 03_safety_pump.ino
- NVS persistence and crash-loop handling: 04_persistence.ino
- WiFi/Firebase IO and schema handlers: 05_connectivity_cloud.ino

### 0.1.2 ESP32 Master (PlatformIO)
- firmware/platformio_smart_water_pump_controller/src/main.cpp
- firmware/platformio_smart_water_pump_controller/src/secrets.h
- firmware/platformio_smart_water_pump_controller/src/secrets.h.example
- firmware/platformio_smart_water_pump_controller/src/config/config.h
- firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.cpp
- firmware/platformio_smart_water_pump_controller/src/connectivity/connectivity_cloud.h
- firmware/platformio_smart_water_pump_controller/src/persistence/persistence.cpp
- firmware/platformio_smart_water_pump_controller/src/persistence/persistence.h
- firmware/platformio_smart_water_pump_controller/src/rs485/rs485_comm.cpp
- firmware/platformio_smart_water_pump_controller/src/rs485/rs485_comm.h
- firmware/platformio_smart_water_pump_controller/src/safety/safety_pump.cpp
- firmware/platformio_smart_water_pump_controller/src/safety/safety_pump.h
- firmware/platformio_smart_water_pump_controller/src/state/state.cpp
- firmware/platformio_smart_water_pump_controller/src/state/state.h
- firmware/platformio_smart_water_pump_controller/src/utils/app_logger.cpp
- firmware/platformio_smart_water_pump_controller/src/utils/app_logger.h
- firmware/platformio_smart_water_pump_controller/src/utils/crc16_modbus.cpp
- firmware/platformio_smart_water_pump_controller/src/utils/crc16_modbus.h

### 0.1.3 NodeMCU Sensor Node (Arduino)
- firmware/arduino_sensor_node/01_config.ino
- firmware/arduino_sensor_node/02_sensors.ino
- firmware/arduino_sensor_node/03_rs485_slave.ino
- firmware/arduino_sensor_node/arduino_sensor_node.ino
- firmware/arduino_sensor_node/sensor_node_shared.h

Primary responsibilities:
- Entry point and transport mode selection: arduino_sensor_node.ino
- Sensor sampling, ISR handling, plausibility and hysteresis: 02_sensors.ino
- RS-485 slave protocol framing and stall timeout reset: 03_rs485_slave.ino

### 0.1.4 NodeMCU Sensor Node (PlatformIO)
- firmware/platformio_sensor_node/src/main.cpp
- firmware/platformio_sensor_node/src/config/config.h
- firmware/platformio_sensor_node/src/config/secrets_ota.h
- firmware/platformio_sensor_node/src/config/secrets_ota.h.example
- firmware/platformio_sensor_node/src/ota/ota_wifi.h
- firmware/platformio_sensor_node/src/ota/ota_wifi_ota.cpp
- firmware/platformio_sensor_node/src/ota/ota_wifi_stubs.cpp
- firmware/platformio_sensor_node/src/rs485/rs485_slave.cpp
- firmware/platformio_sensor_node/src/rs485/rs485_slave.h
- firmware/platformio_sensor_node/src/sensors/sensors.cpp
- firmware/platformio_sensor_node/src/sensors/sensors.h
- firmware/platformio_sensor_node/src/state/state.cpp
- firmware/platformio_sensor_node/src/state/state.h
- firmware/platformio_sensor_node/src/utils/crc16_modbus.cpp
- firmware/platformio_sensor_node/src/utils/crc16_modbus.h
- firmware/platformio_sensor_node/src/utils/syslog_helper.cpp
- firmware/platformio_sensor_node/src/utils/syslog_helper.h

### 0.1.5 Dashboard (App, Components, Lib)
- dashboard/app/error.tsx
- dashboard/app/global-error.tsx
- dashboard/app/globals.css
- dashboard/app/layout.tsx
- dashboard/app/manifest.ts
- dashboard/app/page.tsx
- dashboard/app/api/firebase-messaging-sw/route.ts
- dashboard/app/api/health/route.ts
- dashboard/app/login/page.tsx

- dashboard/components/ActivityPanel.tsx
- dashboard/components/AlertsCard.tsx
- dashboard/components/AppIcon.tsx
- dashboard/components/AuthGuard.tsx
- dashboard/components/CollapsibleSection.tsx
- dashboard/components/ControlPanel.tsx
- dashboard/components/CooldownTimer.tsx
- dashboard/components/DashboardHistorySection.tsx
- dashboard/components/DeviceConfigSettings.tsx
- dashboard/components/DiagnosticsCard.tsx
- dashboard/components/ErrorBoundary.tsx
- dashboard/components/Header.tsx
- dashboard/components/HistoryChart.tsx
- dashboard/components/IdleModeBadge.tsx
- dashboard/components/InfoTooltip.tsx
- dashboard/components/InstallPrompt.tsx
- dashboard/components/LogLevelControl.tsx
- dashboard/components/Logo.tsx
- dashboard/components/NotificationSettings.tsx
- dashboard/components/OverflowMenu.tsx
- dashboard/components/PumpStatusCard.tsx
- dashboard/components/RemoteDiscard.tsx
- dashboard/components/StatCard.tsx
- dashboard/components/TankLevelCard.tsx
- dashboard/components/ThemeProvider.tsx
- dashboard/components/ThemeToggle.tsx
- dashboard/components/ToastHost.tsx

- dashboard/lib/audit.ts
- dashboard/lib/auth.ts
- dashboard/lib/constants.ts
- dashboard/lib/controlContract.ts
- dashboard/lib/deviceId.ts
- dashboard/lib/faultCodes.ts
- dashboard/lib/fcm.ts
- dashboard/lib/firebase.ts
- dashboard/lib/pumpActions.ts
- dashboard/lib/time.ts
- dashboard/lib/toast.ts
- dashboard/lib/types.ts
- dashboard/lib/useAuditEvents.ts
- dashboard/lib/useDeviceConfig.ts
- dashboard/lib/useIsAdmin.ts
- dashboard/lib/useMediaQuery.ts
- dashboard/lib/useNotificationConfig.ts
- dashboard/lib/usePendingControl.ts
- dashboard/lib/usePresence.ts
- dashboard/lib/usePumpData.ts
- dashboard/lib/utils.ts
- dashboard/lib/validation.ts

### 0.1.6 TODO/FIXME/HACK/TEMP marker scan
No explicit TODO/FIXME/HACK/TEMP tokens found in Phase 0 scope directories.

## 0.2 Pin Assignment Verification

Reference source of truth: hardware/wiring_notes.md (section "FINAL Pin Assignments (Production)").

| Signal | Firmware Constant(s) | Firmware GPIO | Hardware Doc GPIO | Match |
|---|---|---:|---:|---|
| Relay output | RELAY_PIN | 4 | 4 | YES |
| RS-485 TX (ESP32) | RS485_TX_PIN | 17 | 17 | YES |
| RS-485 RX (ESP32) | RS485_RX_PIN | 25 | 25 | YES |
| RS-485 DE/RE (ESP32) | RS485_DE_RE_PIN | 5 | 5 | YES |
| RS-485 DE/RE (NodeMCU) | PIN_RS485_DE_RE | 14 | 14 | YES |
| Flow input (NodeMCU) | PIN_FLOW_INPUT | 13 | 13 | YES |
| Ultrasonic TRIG (NodeMCU) | PIN_US_TRIG | 5 | 5 | YES |
| Ultrasonic ECHO (NodeMCU) | PIN_US_ECHO | 16 | 16 | YES |
| RS-485 UART TX (NodeMCU) | UART0 TX | 1 | 1 | YES |
| RS-485 UART RX (NodeMCU) | UART0 RX | 3 | 3 | YES |

Pin verification result: no discrepancies found.

## 0.3 Firebase Schema Audit

### 0.3.1 /pump_system/status (written by ESP32)
Observed in firmware/arduino_smart_water_pump_controller/05_connectivity_cloud.ino pushFirebaseStatus():
- water_level_percent (guarded; omitted until >= 0)
- is_running
- flow_rate_lpm
- is_error
- is_level_sensor_error
- is_flow_sensor_error
- is_overflow_error
- bypass_level_sensor
- auto_bypass_active
- is_sleeping
- is_idle_mode
- wifi_rssi
- last_boot_reason
- manual_runtime_warning
- bypass_flow_sensor
- uptime_minutes
- free_heap_bytes
- min_free_heap_bytes
- max_alloc_heap_bytes
- min_free_heap_observed_bytes
- firebase_consecutive_failures
- firebase_last_error
- ultrasonic_cycles_ok
- ultrasonic_cycles_timeout
- ultrasonic_last_good_cm
- flow_discard_max_sane
- flow_stuck_high_events
- remote_level_discard_count
- manual_desired
- emergency_stop_latched
- remote_sensor_stable
- level_fresh
- run_mode
- countdown_remaining_sec
- pump_cooldown_remaining_sec
- last_fault_code (conditional)
- last_fault_message (conditional)
- estimated_level_pct (conditional)
- level_estimate_active
- flow_volume_added_l
- level_last_valid_age_sec
- level_sensor_health_pct
- total_pump_cycles
- total_pump_run_min
- debug_log_level

### 0.3.2 /pump_system/control (read by ESP32)
Observed in readFirebaseControl():
- mode
- manual_desired
- emergency_stop
- reset_stop
- manual_stop (legacy)
- countdown_start
- countdown_duration_min
- countdown_add_time
- countdown_add_min
- manual_start (legacy)
- bypass_level_sensor
- bypass_flow_sensor
- clear_error
- reboot_request_id

### 0.3.3 /pump_system/config/device (read by ESP32)
Observed in readDeviceConfigFromFirebase():
- tank_empty_cm
- tank_full_cm
- pump_start_level
- pump_stop_level
- dry_run_threshold_lpm
- dry_run_timeout_sec
- flow_calibration_factor
- max_pump_runtime_min
- sleep_enabled
- sleep_start_hour
- sleep_end_hour
- sleep_emergency_level
- level_sensor_failure_threshold
- sensor_failure_threshold (legacy fallback)
- idle_sensor_interval_ms
- idle_firebase_interval_ms
- auto_bypass_on_sensor_fail
- auto_bypass_delay_sec
- debug_log_level

### 0.3.4 Firmware/Dashboard mismatch analysis
Confirmed aligned fields:
- run_mode includes AUTO_COOLDOWN and MANUAL_COOLDOWN
- pump_cooldown_remaining_sec present
- manual_runtime_warning present
- is_idle_mode present
- debug_log_level present in status and device config
- remote_level_discard_count present
- bypass_flow_sensor runtime control path present

Open mismatches:
- None open after continuation fix pass (2026-03-31):
  - N-06 resolved by status key normalization in dashboard ingestion (is_level_sensor_error -> is_sensor_error).
  - N-07 resolved by adding firmware canonical heap key compatibility in PumpStatus typing/normalization.

## 0.4 Dashboard Stack Confirmation

- Framework: Next.js 14.2.35 with App Router
- Language: TypeScript
- Firebase SDK: firebase ^11.0.2
- Auth model: Firebase Auth (Email/Password + Google)
- PWA: enabled (manifest in dashboard/app/manifest.ts)
- Theme color: #185FA5, background #F1EFE8
- Error boundaries present: dashboard/app/error.tsx and dashboard/app/global-error.tsx
- Key implemented dashboard panels/components include controls, diagnostics, alerts, cooldown timer, remote discard display, log-level control, and history chart.

## 0.5 Known Bug Verification

| Bug ID | Description | Status | Evidence Summary |
|---|---|---|---|
| C-01 | Missing void setup() declaration in main .ino | FIXED | setup() exists in arduino_smart_water_pump_controller.ino |
| C-02 | waterLevelPct initialized to 0 before first valid frame | FIXED | waterLevelPct = -1 in 01_config.ino and guarded Firebase push |
| H-01 | No log verbosity levels | FIXED | LOG/LOG_SN level macros and runtime ceiling present |
| H-02 | Plausibility discard had no counter/promotion | FIXED | snLevelDiscardCount + warning + error promotion implemented |
| H-03 | Flow discard debug used zeroed global instead of local | FIXED | local disc copy is used consistently in logging/logic |
| H-04 | Flow error flag non-hysteretic | FIXED | assert/clear dwell hysteresis counters implemented |
| H-05 | Overflow stops pump in MANUAL | FIXED | MANUAL runtime warning without forced stop |
| H-06 | Crash loop counter clear window too short | FIXED | success-based clear on Firebase push plus 180s fallback |
| H-07 | No AUTO_COOLDOWN mode while off-timer active | FIXED | AUTO_COOLDOWN and MANUAL_COOLDOWN logic in safety state machine |
| M-01 | Overlapping level timestamps | FIXED | levelLastUpdateMs used as freshness gate; levelLastValidMs retained for health telemetry |
| M-02 | cfgBypassFlowSensor lacked Firebase runtime control | FIXED | bypass_flow_sensor read path and persistence present |
| M-03 | No RS-485 inter-byte timeout reset | FIXED | 20ms partial-frame stall reset in slave receiver |
| M-05 | runMode initialized to OFF | FIXED | runMode defaults to AUTO_STANDBY |
| M-06 | is_idle_mode not pushed to Firebase | FIXED | is_idle_mode explicitly written in status payload |

Newly identified bugs in this audit:
- N-06 (High): status key mismatch is_level_sensor_error vs is_sensor_error in dashboard typing/consumption. RESOLVED (2026-03-31 continuation).
- N-07 (Low): status key min_free_heap_bytes is not represented in dashboard PumpStatus model. RESOLVED (2026-03-31 continuation).

## 0.6 ISR Safety Audit

NodeMCU sensor node:
- volatile qualifiers confirmed for ISR-shared variables:
  - flowPulseCount
  - flowPulseDiscardCount
  - flowLastPulseUs
- Atomic read/reset pattern confirmed in main loop sensor service:
  - noInterrupts(); copy and reset counters; interrupts();
- ISR writes are not consumed unsafely outside protected sections.

ESP32 master:
- No local flow-pulse ISR path in primary control logic (flow arrives over RS-485).
- No unsafe ISR read pattern found in audited master path.

ISR safety result: PASS.

## 0.7 Revised Scope for Phases 1-7

Based on current source state:
- Previously listed core bugs C-01, C-02, H-01..H-07, M-01, M-02, M-03, M-05, M-06 are already fixed in code.
- Remaining implementation scope should be narrowed to unresolved findings and integration validation.

Updated phase focus:
- Phase 1-6: treat as maintenance/hardening only where evidence shows open gaps.
- Outstanding code work from this audit:
  - No remaining schema drift from N-06/N-07 after continuation fixes.
- Phase 7: full integration and field validation remains the critical gate.

## Phase 0 Exit Criterion Checklist

- 0.1 Full source inventory: YES
- 0.2 Pin assignment verification: YES
- 0.3 Firebase schema extraction + mismatch analysis: YES
- 0.4 Dashboard stack confirmation: YES
- 0.5 Known bug verification (C-01..M-06): YES
- 0.6 ISR safety audit: YES
- 0.7 Revised scope documented: YES

Phase 0 is complete with current-source evidence.
