# Implementation Plan: app-layer-extraction

## User Review Required
> [!IMPORTANT]
> The `checkOverflowProtection()` function currently contains a pre-warning specifically for `MANUAL` mode (logging at 90% of max runtime). Moving mode-awareness out of `safety_pump.cpp` requires changing this without leaking mode logic downward.
> **Approved Solution**: `checkOverflowProtection()` will return an `OverflowStatus` struct containing `{ SafetyDecision decision; bool nearThreshold; }`. The App layer will read `nearThreshold` and emit the `MANUAL`-specific warning if applicable, keeping the safety layer purely factual (comparing runtime against a threshold).
> Additionally, the entry guard `if (!isRunning || !(pumpMode == "AUTO" ...))` will be replaced with a simple `if (!isRunning)` check. Overflow tracking will purely follow physical runtime, universally applying the protection regardless of the application mode string.

## Proposed Changes

### src/app
#### [NEW] pump_app.h
- Define `void app_executePumpLogic();`
- Define `void app_checkCountdownExpiry();`

#### [NEW] pump_app.cpp
- Move the entire `executePumpLogic()` function here, renaming it to `app_executePumpLogic()`.
- Move `checkCountdownExpiry()` here, renaming it to `app_checkCountdownExpiry()`.
- Implement mode-specific evaluation using the `OverflowStatus.nearThreshold` (passed up via `SafetyStatus`) to trigger manual warnings.
- Include necessary headers (`state.h`, `safety_pump.h`, `app_logger.h`).

### src/safety
#### [MODIFY] safety_pump.h
- Remove `executePumpLogic()`.
- Add `OverflowStatus` and `SafetyStatus` structs:
  ```cpp
  struct OverflowStatus {
      SafetyDecision decision;
      bool nearThreshold;
  };
  struct SafetyStatus {
      SafetyDecision decision;
      bool overflowNearThreshold;
  };
  ```
- Update `checkOverflowProtection()` to return `OverflowStatus`.
- Update `checkSafetyCutoff()` to return `SafetyStatus`.

#### [MODIFY] safety_pump.cpp
- Remove `executePumpLogic()`.
- Modify `checkOverflowProtection()`:
  - Replace `if (!isRunning || !(pumpMode == "AUTO"...))` with `if (!isRunning) { pumpAutoStartTracking = false; pumpAutoStartMs = 0; return {SafetyDecision::OK, false}; }`.
  - Calculate `nearThreshold` (elapsed >= 90% of maxRuntime) and return it in the struct.
  - Remove all mode-specific strings (e.g. `"MANUAL"`, `"AUTO"`, `"COUNTDOWN"`).
- Modify `checkSafetyCutoff()` to return `SafetyStatus` incorporating the `overflowNearThreshold` flag.
- Ensure no mode-conditional logic remains in the file.

### src/connectivity
#### [MODIFY] connectivity_cloud.h
- Remove `checkCountdownExpiry()`.

#### [MODIFY] connectivity_cloud.cpp
- Remove `checkCountdownExpiry()` implementation.

### src/main.cpp
#### [MODIFY] main.cpp
- Include `app/pump_app.h`.
- Update `loop()` to call `app_checkCountdownExpiry()` and `app_executePumpLogic()`.

### docs/specs
#### [MODIFY] firmware_operational_rules.md
- Update the invariant text from `executePumpLogic()` to `app_executePumpLogic()` to reflect that the app layer is the sole authorized caller of `setPump(bool)`.

## Verification Plan

### Automated Tests
- `pipx run platformio run -e esp32dev` for `master_node`.
- `pipx run platformio run -e nodemcuv2` for `sensor_node`.

### Manual Verification
- **Diff-Review Step**: I will explicitly output the extracted blocks and modified `safety_pump.cpp` bounds for visual verification *before* asking for a "converged/done" approval, specifically ensuring the fault-code telemetry remains unsuppressed and unconditional for `COUNTDOWN`.
