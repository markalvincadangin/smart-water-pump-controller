---
status: current
version: 1.0
last-reviewed: 2026-07-29
source: auto-generated
---

# Feature Specification: Manual and Countdown MVP

**Feature Branch**: `007-manual-countdown-mvp`

**Created**: 2026-07-29

**Status**: Draft

**Input**: User description: "As part of restructuring the SmartFlow project using a Specification-Driven Development (SDD) approach, I want the next specification to focus exclusively on completing and validating the Manual and Countdown operating modes..."

**Milestone Definition**: Upon completion of this specification, the Master Node SHALL function as a fully independent local pump controller capable of Manual and Countdown operation without requiring any Sensor Node, RS485 communication, or automatic sensing.

## Clarifications

### Session 2026-07-29
- Q: How should Auto Mode unavailability be specified? → A: FR-005 updated to explicitly require rejecting requests to enter Auto Mode while preserving the logic in the codebase.
- Q: How should Sensor node communication be specified? → A: FR-003 updated to specify that the Sensor Communication Service SHALL remain disabled.
- Q: What explicitly is deferred from this release? → A: Created "Out of Scope" section and "Feature Availability" table.
- Q: What are the expected operating conditions? → A: Added explicit "Assumptions" section detailing hardware and connectivity states.
- Q: How is a reboot during a Countdown handled? → A: Added an Edge Case specifying that countdowns are cancelled and the pump remains OFF after a reboot.
- Q: How is the inactive sensor service verified? → A: SC-003 updated to measure task scheduling rather than CPU cycles.

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | All timeouts, watchdog resets, and overflow faults must de-energize the relay (`setPump(false)`). |
| II. Dry-Run Lockout | Yes | Flow sensing is deferred, so dry-run may not actively trigger, but the architectural exit path via `clear_error` must remain intact. |
| III. Overflow Protection | Yes | Both Manual and Countdown modes must enforce `cfgMaxPumpRuntimeMin` and lock out if exceeded. |
| IV. TOR Independence | Yes | Software control does not bypass the hardware TOR. |
| V. Sensor Freshness / E-Stop | Yes | Although Auto mode is inactive, `readFirebaseControl()` and E-stop reachability must never be blocked. |
| VI. Backward Compatibility | Yes | Deferring Auto mode is a runtime or build-time constraint; the RTDB schema and underlying logic structure are preserved. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Manual Control MVP (Priority: P1)

A user wants to turn the water pump on and off manually via the dashboard or mobile application without relying on any automatic sensor data.

**Why this priority**: Manual control is the absolute baseline requirement for any pump controller. It validates the relay driver, network connectivity, and RTDB command processing.

**Independent Test**: Can be fully tested by sending a manual START command, verifying the physical relay energizes, and then sending a STOP command to verify it de-energizes.

**Acceptance Scenarios**:

1. **Given** the pump is OFF and idle, **When** the user sends a Manual START command, **Then** the relay energizes and the state updates to RUNNING in MANUAL mode.
2. **Given** the pump is RUNNING in MANUAL mode, **When** the user sends a STOP command, **Then** the relay de-energizes immediately and the state returns to idle.

---

### User Story 2 - Countdown Timer (Priority: P1)

A user wants to run the pump for a specific duration (e.g., 30 minutes) and have it shut off automatically without requiring level sensors.

**Why this priority**: Countdown provides immediate, safe automation value without the complexity of RS485 or external sensor hardware dependencies.

**Independent Test**: Can be fully tested by starting a short countdown (e.g., 1 minute) and verifying the pump stops automatically exactly when the timer expires.

**Acceptance Scenarios**:

1. **Given** the pump is OFF, **When** the user starts a Countdown for 10 minutes, **Then** the relay energizes and begins tracking elapsed time.
2. **Given** the pump is RUNNING in Countdown mode, **When** the 10-minute timer expires, **Then** the relay de-energizes and the state returns to idle.
3. **Given** the pump is RUNNING in Countdown mode, **When** the user sends a manual STOP command before expiration, **Then** the timer is aborted and the relay de-energizes immediately.

---

### User Story 3 - Overflow Protection (Priority: P1)

The system must protect against unbounded runtimes to prevent flooding, regardless of the active mode.

**Why this priority**: Safety requirement mandated by the Constitution (Principle III).

**Independent Test**: Can be tested by configuring a short `cfgMaxPumpRuntimeMin` (e.g., 2 minutes), starting Manual mode, and verifying the system trips after 2 minutes.

**Acceptance Scenarios**:

1. **Given** the pump is RUNNING in Manual mode, **When** the continuous runtime exceeds `cfgMaxPumpRuntimeMin`, **Then** the relay de-energizes and the system enters the `isOverflowError` lockout state.
2. **Given** the system is in `isOverflowError` lockout, **When** the user sends a manual START command, **Then** the command is rejected until an explicit `clear_error` is received.

---

### Edge Cases

- What happens if the Wi-Fi connection drops during a Countdown? The local firmware must continue tracking the timer and stop the pump when it expires, regardless of cloud connectivity.
- What happens to Dry-Run protection since flow sensors are deferred? The dry-run detection logic should be bypassed or naturally inactive since no flow data is polled, ensuring it does not falsely trigger and lock out the pump.
- What happens if a power failure or reboot occurs during an active Countdown? After reboot, all active countdowns are cancelled and the pump remains OFF until a new command is received (aligning with the "Fail Toward Pump OFF" principle).

---

## Out of Scope

This specification intentionally excludes:

* Auto Mode
* RS485 protocol
* Sensor Node firmware
* Ultrasonic processing
* Flow measurement
* Dry-run detection
* Automatic water level management

### Feature Availability

| Feature     | Status    |
| ----------- | --------- |
| Manual      | Available |
| Countdown   | Available |
| Auto        | Deferred  |
| RS485       | Deferred  |
| Sensor Node | Deferred  |

---

## Assumptions

* Relay hardware is functional.
* Wi-Fi and Firebase connectivity are available.
* The pump hardware is protected by the Thermal Overload Relay (TOR).
* Countdown timing uses the system monotonic clock.
* Sensor hardware may be physically disconnected.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Master Node MUST operate the relay in Manual mode, staying ON until explicitly stopped or maximum runtime is reached.
- **FR-002**: The Master Node MUST operate the relay in Countdown mode, staying ON until the requested duration expires or it is explicitly stopped.
- **FR-003**: The Sensor Communication Service SHALL remain disabled. The Master Node SHALL neither establish nor attempt communication with the Sensor Node.
- **FR-004**: The Master Node MUST NOT poll, wait for, or process any ultrasonic or flow sensor data.
- **FR-005**: The Master Node SHALL reject any request to enter Auto Mode while this specification is active. The Auto Mode implementation and interfaces SHALL remain in the codebase but SHALL NOT be initialized or executed during runtime.
- **FR-006**: The system MUST enforce the maximum runtime threshold (`cfgMaxPumpRuntimeMin`) across all active modes and enter a locked-out overflow error state if exceeded.
- **FR-007**: The local timer for Countdown mode and Overflow protection MUST use wrap-safe arithmetic (`elapsedMillis32`).

---

## Firmware Behavior *(if applicable)*

### State Machine Impact
- **Inactive States**: Any state transitions related to `AUTO` mode, sensor polling, or RS485 communication timeouts are functionally disabled or unreachable.
- **Active States**: `MANUAL`, `COUNTDOWN`, `IDLE`, and `ERROR` (specifically overflow).

### Safety Invariants
- [x] All fault paths exit with `setPump(false)`
- [x] No new `digitalWrite(RELAY_PIN, ...)` calls added outside `setPump()`
- [x] All new timing uses wrap-safe helpers

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The Master Node executes Manual Mode on/off commands with less than 500ms latency from RTDB command receipt.
- **SC-002**: The Master Node executes Countdown Mode and de-energizes the relay within 1 second of the exact expected expiration time.
- **SC-003**: No Sensor Service task is scheduled while this specification is active.
- **SC-004**: When runtime exceeds `cfgMaxPumpRuntimeMin`, the Master Node forces the relay OFF and publishes the `isOverflowError` state to the RTDB immediately.

### Validation Commands

```bash
# Firmware (requires hardware — verify serial output and relay clicks)
cd firmware/master_node
pio run -e esp32dev_ota --target upload && pio device monitor

# Expected Validation Steps:
# 1. Start Manual mode via RTDB/App -> Verify relay clicks ON.
# 2. Stop Manual mode via RTDB/App -> Verify relay clicks OFF.
# 3. Start 1-minute Countdown -> Verify relay clicks OFF automatically after 60s.
# 4. Lower MaxRuntime to 2 mins, start Manual -> Verify error lockout after 120s.
```
