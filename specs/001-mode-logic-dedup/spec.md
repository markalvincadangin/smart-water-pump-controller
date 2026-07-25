---
status: draft
version: 0.1
last-reviewed: 2026-07-26
source: auto-generated
---

# Feature Specification: mode-logic-dedup

**Feature Branch**: `[001-mode-logic-dedup]`

**Created**: 2026-07-26

**Status**: Draft

**Input**: User description: "Option 3 dedup: Deduplicate the shared minimum-off-time and safety-gate logic between MANUAL and COUNTDOWN modes in the firmware code and operational rules document, without changing the `pumpMode` enum (schema). Also, convert `checkOverflowProtection()` and `checkDryRunProtection()` to return a decision status rather than executing a hardware action (calling `setPump()`) directly."

## Constitution Check *(mandatory gate)*

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | Yes | The centralized executor for the new `SafetyDecision` return type MUST default to `setPump(false)` when a safety trip is detected. |
| II. Dry-Run Lockout | Yes | The refactored `checkDryRunProtection()` MUST still block operation and require `clear_error` for recovery. |
| III. Overflow Protection | Yes | The shared logic MUST consistently apply the maximum runtime constraints across AUTO, MANUAL, and COUNTDOWN modes. |
| IV. TOR Independence | No | Hardware relay limits remain untouched. |
| V. Sensor Freshness / E-Stop | Yes | The freshness gate logic is explicitly being deduplicated, it MUST remain a mandatory block for automated decisions. |
| VI. Backward Compatibility | Yes | The `pumpMode` enum values in RTDB must NOT be changed. |

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Maintain Timer End State (Priority: P1)

As an operator, when a COUNTDOWN timer expires, the pump should turn off but the mode should remain COUNTDOWN (idle).

**Why this priority**: Preserves the distinct UX state that differentiates a completed timer from a manual stop, which was the core reason for choosing Option 3.

**Independent Test**: Trigger a 1-minute countdown. Observe that the pump stops at 0 but the dashboard still reads "Countdown" (not MANUAL).

**Acceptance Scenarios**:

1. **Given** pumpMode is COUNTDOWN and timer expires, **When** the state machine evaluates the next loop, **Then** pump is commanded OFF and runMode remains COUNTDOWN.

---

### User Story 2 - Maintain Hardware Lockouts (Priority: P1)

As a safety-critical system, if flow drops below the threshold, the pump must lock out in any mode.

**Why this priority**: We are changing the safety checks to return a decision rather than acting directly; the ultimate action MUST still reliably occur.

**Independent Test**: Trigger a dry-run condition in AUTO, MANUAL, and COUNTDOWN. Verify the pump stops and `isDryRunError` sets in all three.

**Acceptance Scenarios**:

1. **Given** flow drops below threshold, **When** `checkDryRunProtection()` returns a STOP decision, **Then** the main loop catches it and executes `setPump(false)`.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST consolidate duplicate minimum-off-time and sensor freshness evaluation logic from the `MANUAL` and `COUNTDOWN` blocks in `executePumpLogic()` into a shared, mode-agnostic pre-evaluation step.
- **FR-002**: `checkOverflowProtection()` MUST return a `SafetyDecision` (or similar status enum) instead of calling `setPump(false)` itself.
- **FR-003**: `checkDryRunProtection()` MUST return a `SafetyDecision` (or similar status enum) instead of calling `setPump(false)` itself.
- **FR-004**: The main `executePumpLogic()` orchestrator MUST interpret the returned `SafetyDecision`s and execute `setPump(false)` if any trip is signaled.
- **FR-005**: The `pumpMode` valid values (AUTO, MANUAL, COUNTDOWN) MUST remain identical, breaking no downstream integrations.
- **FR-006**: The `firmware_operational_rules.md` document MUST be updated to reflect the deduplicated shared gate logic for MANUAL and COUNTDOWN.

---

## Firmware Behavior *(if applicable)*

### State Machine Impact
- **executePumpLogic()**: Will be refactored to check safety decisions first via the return values of `checkSafetyCutoff()`, `checkDryRunProtection()`, and `checkOverflowProtection()`. It will evaluate freshness and cooldown rules in a single block before branching into mode-specific intent (AUTO/MANUAL/COUNTDOWN).
- **Safety Functions**: Stripped of direct `setPump()` calls. They will now return an enum, e.g., `enum class SafetyDecision { OK, STOP_DRYRUN, STOP_OVERFLOW };`.

### Safety Invariants
- [x] All fault paths exit with `setPump(false)`
- [x] No new `digitalWrite(RELAY_PIN, ...)` calls added outside `setPump()`
- [x] All new timing uses wrap-safe helpers

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of direct `setPump(false)` calls are removed from `checkDryRunProtection()` and `checkOverflowProtection()`.
- **SC-002**: Duplicate blocks for sensor freshness checks and minimum-off-time enforcement in `executePumpLogic()` are reduced to a single shared block.
- **SC-003**: The firmware compiles and passes `pio check` with no new high-severity defects.
- **SC-004**: The RTDB schema and mode enums remain fully backward compatible.

### Validation Commands

```bash
# Firmware (requires hardware — document expected serial output instead)
pio check -e esp32dev --fail-on-defect high
```

---

## Assumptions

- We are operating on the `master_node` C++ codebase, and changes will only affect `master_node/src/safety/safety_pump.cpp`.
