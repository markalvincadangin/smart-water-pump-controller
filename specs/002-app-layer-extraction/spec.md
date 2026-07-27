---
status: draft
version: 0.1
last-reviewed: 2026-07-27
source: auto-generated
---

# Feature Specification: app-layer-extraction

**Feature Branch**: `002-app-layer-extraction`

**Created**: 2026-07-27

**Status**: Draft

**Input**: User description: "Refactor high-level decision-making from safety_pump.cpp into a new src/app/ layer, consistent with the layered, single-responsibility organization."

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | Relocating the state machine to `app/` MUST NOT compromise the hardware safety layer's ability to default to OFF during a cutoff. `setPump(bool)` should remain protected. |
| II. Dry-Run Lockout | Yes | `checkDryRunProtection` MUST remain isolated in the safety/hardware-oriented layer, maintaining independent lockout capability. |
| III. Overflow Protection | Yes | `checkOverflowProtection` MUST similarly remain isolated in the safety layer, unaffected by application mode switching. |
| IV. TOR Independence | No | Hardware limits are untouched. |
| V. Sensor Freshness / E-Stop | Yes | The Application layer must properly evaluate the freshness gate (`allowStartFromSensors`) and E-Stop latches provided by the safety layer before engaging any automated run. |
| VI. Backward Compatibility | Yes | Mode semantics (`MANUAL`, `COUNTDOWN`, `AUTO`), telemetry strings, and RTDB payload shapes MUST remain perfectly untouched. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Mode Integrity Post-Extraction (Priority: P1)
As an operator, I expect the pump to behave exactly as before in AUTO, MANUAL, and COUNTDOWN modes, despite the logic being relocated.
**Why this priority**: The goal is 100% structural parity. Any change in mode behavior is a regression.
**Independent Test**: Trigger an AUTO start, MANUAL start, and COUNTDOWN start. Verify state transitions occur normally and telemetry updates properly.

### User Story 2 - Safety Precedence (Priority: P1)
As an operator, I expect hardware safety logic (e.g., dry-run, comms loss) to continue overriding any application mode request.
**Why this priority**: Decoupling the App layer from the Safety layer could introduce a bug where the App layer attempts to override a safety latch.
**Independent Test**: Simulate a `COMM_LOSS` (freshness fail) during a running COUNTDOWN. Verify the pump immediately fails-safe and logs the fault, overriding the application intent.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Move all application-level mode evaluation (e.g. `MANUAL`, `COUNTDOWN`, `AUTO`, `SLEEP`, `IDLE` state handling) from `safety_pump.cpp` into a new application layer module (e.g., `src/app/pump_app.cpp`).
- **FR-002**: Leave hardware-oriented safety and sensor gating functions (`checkDryRunProtection`, `checkOverflowProtection`, `checkLevelSensorFailure`, `checkFlowSensorStuck`, `checkSafetyCutoff`) completely in `safety_pump.cpp`.
- **FR-003**: `safety_pump.cpp` MUST not contain any conditional checks explicitly relying on `pumpMode == "AUTO"`, `MANUAL`, or `COUNTDOWN`. It should evaluate hardware safety agnostically.
- **FR-004**: Relocate `checkCountdownExpiry()` from `connectivity_cloud.cpp` into the new application layer, as countdown expiration is business logic, not a cloud transport concern.
- **FR-005**: Ensure `setPump(bool)` remains the single entry point for relay control (Constitution II.2), exposed cleanly to the App layer.
- **FR-006**: The refactor MUST NOT change the exact wording, conditions, or logging triggers of telemetry (e.g., `lastFaultCode`, `lastFaultMessage`, `isCountdownActive` resetting on trips).

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: `safety_pump.cpp` contains zero strings matching `"AUTO"`, `"MANUAL"`, or `"COUNTDOWN"` (excluding comments).
- **SC-002**: A new `src/app/pump_app.cpp` module successfully centralizes the state machine.
- **SC-003**: The firmware successfully compiles using PlatformIO on both `master_node` (ESP32) and `sensor_node` (NodeMCU) with zero signature mismatch errors.
- **SC-004**: A thorough line-by-line diff review of `pump_app.cpp` confirms that the fault reporting suppression bug (from the previous cycle) was not reintroduced and all fault assignments are unconditional where appropriate.

### Validation Commands

```bash
cd firmware/master_node && pipx run platformio run -e esp32dev
cd firmware/sensor_node && pipx run platformio run -e nodemcuv2
```
