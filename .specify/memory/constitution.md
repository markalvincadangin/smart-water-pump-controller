# SmartFlow Constitution

## Core Principles

### I. Fail Toward Pump OFF (NON-NEGOTIABLE)
SmartFlow is a physical safety-critical system automating a 1.5 HP jet pump. Every fault path, timeout, unexpected state, watchdog reset, or unhandled exception MUST transition the relay to its de-energized state (`setPump(false)` / RELAY_PIN HIGH). Software must NEVER bias toward pump ON under ambiguity or error conditions.

### II. Dry-Run Lockout Protection
Dry-run protection prevents pump destruction when water flow fails. If flow rate falls below `cfgDryRunThresholdLpm` for longer than `cfgDryRunTimeoutSec` while the pump is requested ON:
1. `isDryRunError` MUST be set to `true`.
2. `setPump(false)` MUST be called immediately.
3. Lockout MUST persist until an explicit `clear_error` command is processed. This protection path must survive all refactors.

### III. Overflow Runtime Protection
Overflow protection prevents flooding and water loss. If cumulative or continuous pump runtime exceeds `cfgMaxPumpRuntimeMin` in ANY mode (AUTO, MANUAL, or COUNTDOWN):
1. `isOverflowError` MUST be set to `true`.
2. `setPump(false)` MUST be executed immediately.
3. Lockout MUST persist until explicitly reset via `clear_error`.

### IV. Hardware Thermal Overload Relay (TOR) Independence
Software protections supplement, but never replace, hardware physical safety. The LR2-D13 Thermal Overload Relay (set to 8–9 A) operates independently at the contactor hardware level. Firmware must never assume sole protection responsibility or bypass physical hardware cutouts.

### V. Sensor Level Freshness Gate & Reachable E-Stop
- **Freshness Gate**: Before any automated pump start decision, sensor data freshness MUST be verified via `(levelLastUpdateMs > 0) && elapsedMillis32(nowMs, levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS`. The `> 0` check is mandatory to prevent false-fresh reads at boot.
- **E-Stop Reachability**: `readFirebaseControl()` and control polling MUST NEVER return early or exit in a manner that blocks `reset_stop` or `clear_error` from being evaluated and executed while safety latches are active.

### VI. Backward Compatibility & Additive Contracts
RS-485 serial communication frames and Firebase RTDB schema modifications MUST maintain backward compatibility:
- New serial payload fields must be optional in parser implementations (e.g., default `LDSC` to 0 if missing).
- RTDB schema changes must be additive only; no existing valid control flags or status telemetry nodes may be removed or broken without multi-version migration.

---

## Technical Constraints & Monorepo Governance

1. **Repository Scope**: A single root constitution governs all software components within `smartflow`:
   - `firmware/`: ESP32 Master and NodeMCU Sensor Node C++/PlatformIO projects.
   - `app/`: Native Android application built with Kotlin and Jetpack Compose.
   - `functions/`: Firebase Cloud Functions backend.
   - `docs/` & `scripts/`: System runbooks, specs, and administrative scripts.
2. **Relay Control Invariant**: `setPump(bool)` is the single authorized entry point for relay state changes. Direct `digitalWrite(RELAY_PIN, ...)` calls elsewhere in code are prohibited.
3. **Wrap-Safe Timing**: Monotonic timing in firmware MUST use wrap-safe arithmetic (`elapsedMillis32()`, `millisDeadlineReached()`, `addMillisSaturated()`). Raw `millis()` additions or subtractions are non-compliant.

---

## Governance

- **Supremacy**: This Constitution supersedes all informal team practices, feature requests, or unratified documentation across firmware, Android application, and cloud functions.
- **Compliance**: All Pull Requests, SpecKit plans (`/speckit.plan`), and task breakdowns (`/speckit.tasks`) MUST explicitly verify adherence to these non-negotiables.
- **Amendments**: Any change to these principles requires formal review, physical risk assessment, updated field documentation, and an explicit version bump.

**Version**: 1.0.0 | **Ratified**: 2026-07-25 | **Last Amended**: 2026-07-25

### VII. C++ Naming Conventions
To maintain consistency across the C++ embedded projects (`firmware/master_node/` and `sensor_node/`), the following standard conventions MUST be used:
- **Files & Directories**: `snake_case` (e.g., `pump_hal.cpp`, `water_level_service.cpp`).
- **Classes & Structs**: `PascalCase` (e.g., `PumpHal`, `WaterLevelService`).
- **Functions & Methods**: `camelCase` (e.g., `executeLogic`, `init`).
- **Variables**: `camelCase` (e.g., `waterLevelPct`, `isRunning`).
- **Constants & Macros**: `UPPER_SNAKE_CASE` (e.g., `PIN_RELAY`, `TANK_EMPTY_CM`).
