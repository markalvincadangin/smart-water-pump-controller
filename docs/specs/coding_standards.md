# SmartFlow Firmware Coding Standards

This document defines the structural and stylistic rules for the SmartFlow firmware. It acts as an extension to `CONTRIBUTING.md`.

## 1. Architectural Layering Model

The firmware is organized into a layered architecture to separate business logic from hardware specifics. The strict rule is **dependencies point downward only**: higher layers may call lower layers, but lower layers must never call upward into higher layers.

- **Application Layer (`app/`)**: Pure decision logic, state machines, and business rules (e.g., mode evaluation, safety gates). Knows *when* to turn the pump on, but doesn't know *how* to set the GPIO pin.
- **Service Layer / Middleware**: Hardware-agnostic managers and state persistence (e.g., `persistence/`, `connectivity_cloud/`).
- **Driver Layer**: Wrappers around specific hardware peripherals (e.g., `rs485/`, relay drivers, sensor drivers). Translates logical commands into hardware actions.
- **HAL (Hardware Abstraction Layer)**: Provided by the Arduino/PlatformIO framework (`digitalWrite`, `Serial`, `millis()`). We do not write a custom HAL; we use the framework's abstractions.

## 2. Naming and Style Conventions

We adopt the **BARR-C:2018 (Barr Group's Embedded C Coding Standard)** as our baseline, specifically focusing on the following conventions:

- **Variables and Functions**: `camelCase` (e.g., `waterLevelPct`, `executePumpLogic()`).
- **Constants and Macros**: `UPPER_SNAKE_CASE` (e.g., `MIN_PUMP_OFF_TIME_MS`, `RELAY_PIN`).
- **Types and Structs**: `PascalCase` (e.g., `PumpState`).
- **Scoping**: Variables must be declared in the narrowest possible scope. Global variables must be minimized and clearly justified.
- **Braces**: K&R style, mandatory braces for all `if`/`else`/`while` blocks, even for single-line statements (to prevent safety-critical macro injection bugs).

## 3. File Organization Rules

- **Single Responsibility Principle**: Each `.cpp`/`.h` pair must have exactly one responsibility (e.g., "manage the RS-485 bus" or "evaluate dry-run safety gates"). Do not mix hardware I/O and application policy in the same file.
- **File Size Guidelines**: Files should ideally remain under 300-400 lines. If a file exceeds this size, it likely violates the single responsibility principle and should be split.
- **Headers**: All headers must use `#pragma once` (or traditional include guards) and include only what they directly depend on.

## 4. Where New Code Should Go

When adding new features, place them in the appropriate layer:

- **New Control Mode / Policy Logic**: `src/app/state_machine.cpp`
- **New Safety Constraint**: `src/app/safety_gates.cpp`
- **New Sensor Hardware**: `src/sensors/` or `src/driver/` (e.g., `flow_sensor.cpp`)
- **New Cloud Sync Feature**: `src/connectivity_cloud/`
- **New Hardware Pins / Thresholds**: `src/config/config.h`
- **Orchestration / Setup**: `src/main.cpp` (Keep this file free of raw logic; use it only for `setup()` wiring and `loop()` orchestration).
