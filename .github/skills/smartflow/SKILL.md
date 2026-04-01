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
- Firmware master: firmware/arduino_smart_water_pump_controller/
- Firmware sensor node: firmware/arduino_sensor_node/
- Dashboard: dashboard/
- Protocol and release docs: docs/ and .plan/

## State/Schema Expectations
- run_mode values include AUTO_STANDBY, AUTO, AUTO_COOLDOWN, MANUAL_ON, MANUAL_OFF, MANUAL_COOLDOWN, COUNTDOWN, STOPPED.
- Newer fields may include pump_cooldown_remaining_sec, manual_runtime_warning, bypass_flow_sensor, is_idle_mode, debug_log_level, remote_level_discard_count.
- Keep parser compatibility for frames where LDSC may be absent.

## When editing
- Use existing naming/style conventions in touched files.
- Preserve deployment artifacts and audit traceability.
- If request is operational/manual only, update runbooks/records rather than inventing firmware changes.
