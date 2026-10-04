# SmartFlow Firmware Coding Standards

This document defines the structural and stylistic rules for the SmartFlow firmware. It acts as an extension to `CONTRIBUTING.md`.

## 1. Architectural Layering Model

The firmware is organized into a layered architecture to separate business logic from hardware specifics. The strict rule is **dependencies point downward only**: higher layers may call lower layers, but lower layers must never call upward into higher layers.

- **Application / Core Layer (`core/app/`, `state/`)**: Pure decision logic, state machines, and business rules (e.g., mode evaluation, command processing in `pump_app.cpp`). Knows *when* to operate the pump, but delegates hardware manipulation to lower layers.
- **Safety Layer (`safety/`)**: Independent safety evaluators (`safety_pump.cpp`) enforcing hard invariants (E-Stop, Dry-Run, Maximum Runtime). Safety logic strictly overrides application intent.
- **Services Layer (`services/`)**: Domain services (e.g., `water_level_service.cpp`) aggregating sensor readings and maintaining metric calculations.
- **Cloud & Network Middleware (`cloud/`, `network/`, `persistence/`, `ota/`)**: Cloud sync (`cloud_manager.cpp`, `device_shadow.cpp`), Wi-Fi and BLE provisioning, OTA updates, and NVS persistence.
- **Driver Layer (`drivers/`, `rs485/`)**: Logical drivers managing communication bus and peripheral orchestration (`pump_driver.cpp`, `sensor_driver.cpp`, `rs485_comm.cpp`).
- **HAL (Hardware Abstraction Layer) (`hal/`)**: Direct pin and peripheral interface abstraction (`pump_hal.cpp`, `flow_meter_hal.cpp`, `ultrasonic_hal.cpp`).

## 2. Naming and Style Conventions

We adopt the **BARR-C:2018 (Barr Group's Embedded C Coding Standard)** as our baseline, specifically focusing on the following conventions:

- **Variables and Functions**: `camelCase` (e.g., `waterLevelPct`, `executePumpLogic()`).
- **Constants and Macros**: `UPPER_SNAKE_CASE` (e.g., `MIN_PUMP_OFF_TIME_MS`, `RELAY_PIN`).
- **Types and Structs**: `PascalCase` (e.g., `PumpState`, `OperatingMode`).
- **Canonical Fault Codes**: UPPER_SNAKE_CASE string literals matching contract definitions (e.g., `"MAX_RUNTIME"`, `"DRY_RUN"`).
- **Scoping**: Variables must be declared in the narrowest possible scope. Global variables must be minimized and clearly justified.
- **Braces**: K&R style, mandatory braces for all `if`/`else`/`while` blocks, even for single-line statements (to prevent safety-critical macro injection bugs).

## 3. File Organization Rules

- **Single Responsibility Principle**: Each `.cpp`/`.h` pair must have exactly one responsibility (e.g., "manage the RS-485 bus" or "evaluate dry-run safety gates"). Do not mix hardware I/O and application policy in the same file.
- **File Size Guidelines**: Files should ideally remain under 300–400 lines. If a file exceeds this size, it likely violates the single responsibility principle and should be split.
- **Headers**: All headers must use `#pragma once` (or traditional include guards) and include only what they directly depend on.

## 4. Where New Code Should Go

When adding new features in `firmware/master_node/src/`, place them in the appropriate layer:

- **New Control Mode / Policy Logic**: `core/app/pump_app.cpp` or `state/state.cpp`
- **New Safety Constraint or Fault Rule**: `safety/safety_pump.cpp`
- **New Sensor Hardware Driver**: `drivers/sensor_driver.cpp` or dedicated file in `drivers/`
- **New Hardware Pin Interaction (GPIO)**: `hal/`
- **New Cloud Sync / RTDB Shadow Field**: `cloud/device_shadow.cpp` and `cloud/cloud_manager.cpp`
- **New Hardware Pins / Static Constants**: `config/config.h` or `config/hardware.h`
- **Orchestration / Setup**: `main.cpp` (Keep this file free of raw business logic; use it only for `setup()` wiring and `loop()` orchestration).
