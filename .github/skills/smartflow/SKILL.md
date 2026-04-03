---
name: smartflow
description: "Use when working on SmartFlow (ESP32 + NodeMCU + RS-485 + Firebase + Next.js dashboard), including firmware fixes, dashboard changes, protocol/schema updates, deployment runbooks, and safety behavior. Trigger keywords: SmartFlow, ESP32, NodeMCU, RS-485, dry-run, overflow, run_mode, Firebase RTDB, JSN-SR04T, YF-G1, C-01, C-02, H-02, H-03, H-04, H-05, H-06, H-07, M-01, M-02, M-03, M-05, M-06."
---

# SmartFlow Skill

## Goal
Provide safe, phase-aware implementation guidance for SmartFlow repository tasks across firmware, dashboard, docs, and deployment artifacts.

## Core Rules
1. Safety first: fail toward pump OFF, never ON.
2. Never weaken dry-run lockout, overflow protection, E-stop semantics, or TOR independence.
3. Keep Firebase and RS-485 changes backward compatible (additive fields, optional parser fields).
4. Prefer minimal, scoped edits; avoid unrelated refactors.
5. Validate changes with project-appropriate checks (typecheck/build for dashboard, compile expectations for firmware edits).

## Project Awareness
- Firmware master (active): firmware/platformio_smart_water_pump_controller/
- Firmware sensor node (active): firmware/platformio_sensor_node/
- Legacy Arduino sketches (reference only): firmware/arduino_*/
- Dashboard: dashboard/
- Protocol, audit, and release docs: docs/ and .plan/

## Validation Preferences
- Firmware: compile with PlatformIO before concluding changes.
	- Master: `firmware/platformio_smart_water_pump_controller` (env: `esp32dev`)
	- Sensor: `firmware/platformio_sensor_node` (default NodeMCU env)
- Dashboard: run type/build checks when practical.

## State/Schema Expectations
- run_mode values include AUTO_STANDBY, AUTO, AUTO_COOLDOWN, MANUAL_ON, MANUAL_OFF, MANUAL_COOLDOWN, COUNTDOWN, STOPPED.
- RTDB control should include: mode, manual_desired, emergency_stop, reset_stop, clear_error, countdown_start, countdown_stop, countdown_add_time, countdown_add_min, bypass_level_sensor, bypass_flow_sensor, reboot_request_id.
- RTDB status should include: countdown_active, countdown_remaining_sec, emergency_stop_latched, bypass_level_sensor, bypass_flow_sensor, run_mode, pump_cooldown_remaining_sec, manual_runtime_warning, is_idle_mode, debug_log_level, remote_level_discard_count.
- RTDB config/device should include calibrated values such as flow_calibration_factor and tank_full_cm.
- Current expected pre-flash-safe control/config values:
	- `/pump_system/control/emergency_stop = false`
	- `/pump_system/control/countdown_stop = false`
	- `/pump_system/control/bypass_level_sensor = false`
	- `/pump_system/control/bypass_flow_sensor = false`
	- `/pump_system/control/mode = "AUTO"`
	- `/pump_system/config/device/flow_calibration_factor = 7.5`
	- `/pump_system/config/device/tank_full_cm = 30`
- Keep parser compatibility for frames where LDSC may be absent.

## Firmware Notes
- Keep `LOG_COMPILE_FLOOR = LOG_LEVEL_VERBOSE` in master config so runtime Firebase log-level control remains available.
- Preserve fail-safe behavior: any fault path must bias to `setPump(false)`.
- Sensor node uses hardened RS-485 payload with `LDSC` and CRC; retain backward compatibility where practical.

## When editing
- Use existing naming/style conventions in touched files.
- Preserve deployment artifacts and audit traceability.
- If request is operational/manual only, update runbooks/records rather than inventing firmware changes.
