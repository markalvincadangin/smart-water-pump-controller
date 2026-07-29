# Tasks: Manual and Countdown MVP

**Branch**: `007-manual-countdown-mvp` | **Date**: 2026-07-29

**Input**: Generated from `specs/007-manual-countdown-mvp/plan.md`

## Phase 1: Setup

- [ ] T001 [P] Create `firmware/master_node/src/config/feature_config.h` defining `FEATURE_SENSOR_SERVICE` (false) and `FEATURE_AUTO_MODE` (false).
- [ ] T002 [P] Create `firmware/master_node/src/core/app/pump_command.h` defining `CommandType` enum and `PumpCommand` struct.

## Phase 3: Implementation [US1] Independent Local Control

- [ ] T003 [US1] Update `firmware/master_node/src/state/state.h` to define `PumpState` enum (IDLE, MANUAL, COUNTDOWN, ERROR) and add `currentState` global.
- [ ] T004 [US1] Update `firmware/master_node/src/firebase_control.cpp` and `.h` to decouple execution by returning a `PumpCommand` struct instead of calling `setPump()` directly.
- [ ] T005 [US1] Update `setup()` in `firmware/master_node/src/main.cpp` for explicit Boot Safety: `pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);`.
- [ ] T006 [US1] Update `loop()` in `firmware/master_node/src/main.cpp` to include a strict `switch(currentState)` block that drives relay states and countdown timers using `elapsedMillis32()`.
- [ ] T007 [US1] Wrap RS485 initialization and polling in `firmware/master_node/src/main.cpp` with `#if FEATURE_SENSOR_SERVICE`.
- [ ] T008 [US1] Update `firmware/master_node/src/rs485_master.cpp` to respect `FEATURE_SENSOR_SERVICE` and prevent hardware UART ownership when disabled.

## Phase 4: Polish & Integration

- [ ] T009 Run `cd firmware/master_node && pio run` to ensure successful compilation.
- [ ] T010 Commit changes with `feat(firmware): implement Manual/Countdown MVP state machine`.
