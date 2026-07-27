---
status: draft          # draft | current | deprecated
version: 0.1
last-reviewed:         # YYYY-MM-DD
source: hand-authored  # hand-authored | auto-generated
---

# Feature Specification: [FEATURE NAME]

**Feature Branch**: `[###-feature-name]`

**Created**: [DATE]

**Status**: Draft

**Input**: User description: "$ARGUMENTS"

## Constitution Check *(mandatory gate)*

<!--
  Before writing any requirement, verify this feature does not conflict with
  the SmartFlow constitution (.specify/memory/constitution.md).
  Check each applicable principle:
-->

| Principle | Applicable? | Notes |
|-----------|-------------|-------|
| I. Fail Toward Pump OFF | <!-- Yes/No --> | <!-- If Yes: describe all fault paths and their relay disposition --> |
| II. Dry-Run Lockout | <!-- Yes/No --> | <!-- If Yes: confirm clear_error is the only exit, not timeout --> |
| III. Overflow Protection | <!-- Yes/No --> | <!-- If Yes: verify all three modes (AUTO/MANUAL/COUNTDOWN) are covered --> |
| IV. TOR Independence | <!-- Yes/No --> | <!-- If Yes: confirm software protection does not gate hardware cutout --> |
| V. Sensor Freshness / E-Stop | <!-- Yes/No --> | <!-- If Yes: confirm readFirebaseControl() cannot block reset_stop path --> |
| VI. Backward Compatibility | <!-- Yes/No --> | <!-- If Yes: confirm new RS-485 fields are optional; RTDB changes are additive --> |

---

## User Scenarios & Testing *(mandatory)*

<!--
  IMPORTANT: User stories should be PRIORITIZED as user journeys ordered by importance.
  Each user story/journey must be INDEPENDENTLY TESTABLE - meaning if you implement just ONE of them,
  you should still have a viable MVP (Minimum Viable Product) that delivers value.

  Assign priorities (P1, P2, P3, etc.) to each story, where P1 is the most critical.
  Think of each story as a standalone slice of functionality that can be:
  - Developed independently
  - Tested independently
  - Deployed independently
  - Demonstrated to users independently
-->

### User Story 1 - [Brief Title] (Priority: P1)

[Describe this user journey in plain language]

**Why this priority**: [Explain the value and why it has this priority level]

**Independent Test**: [Describe how this can be tested independently - e.g., "Can be fully tested by [specific action] and delivers [specific value]"]

**Acceptance Scenarios**:

1. **Given** [initial state], **When** [action], **Then** [expected outcome]
2. **Given** [initial state], **When** [action], **Then** [expected outcome]

---

### User Story 2 - [Brief Title] (Priority: P2)

[Describe this user journey in plain language]

**Why this priority**: [Explain the value and why it has this priority level]

**Independent Test**: [Describe how this can be tested independently]

**Acceptance Scenarios**:

1. **Given** [initial state], **When** [action], **Then** [expected outcome]

---

[Add more user stories as needed, each with an assigned priority]

### Edge Cases

<!--
  ACTION REQUIRED: Fill with project-specific edge cases.
  For firmware: sensor loss, RS-485 timeout, RTDB disconnection, watchdog reset.
  For dashboard: offline mode, stale data, Firebase auth expiry, PWA install state.
  For functions: RTDB write failure, duplicate trigger, auth token missing.
-->

- What happens when [boundary condition]?
- How does system handle [error scenario]?

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST [specific capability]
- **FR-002**: System MUST [specific capability]
- **FR-003**: Users MUST be able to [key interaction]

*Mark unclear requirements:*

- **FR-XXX**: System MUST [NEEDS CLARIFICATION: specify what is unclear]

### Key Entities *(include if feature involves data)*

- **[Entity]**: [What it represents, key attributes, relationships]

---

<!-- ============================================================
  MODULE SECTIONS — include only the sections that apply.
  Delete or comment out inapplicable sections entirely.
  Don't leave empty sections with placeholder text.
============================================================ -->

<!-- if applicable: Firmware touches firmware/master_node/ or firmware/sensor_node/ -->
## Firmware Behavior *(if applicable)*

<!--
  Describe behavior at the C++/Arduino level. Reference actual function names,
  state machine transitions, and RS-485 frame fields where relevant.
  Relay control: ALL changes MUST go through setPump(bool). Direct digitalWrite(RELAY_PIN) is prohibited.
  Timing: MUST use elapsedMillis32() / millisDeadlineReached() — raw millis() arithmetic is non-compliant.
-->

### State Machine Impact
- [Which states are added/modified/removed]
- [Transition conditions and their relay disposition]

### RS-485 Protocol Impact *(if applicable)*
- New or modified frame fields: [field name, byte position, type, default if absent]
- Parser changes: all new fields MUST default safely if missing (backward compat)

### Safety Invariants
- [ ] All fault paths exit with `setPump(false)`
- [ ] No new `digitalWrite(RELAY_PIN, ...)` calls added outside `setPump()`
- [ ] All new timing uses wrap-safe helpers

---

<!-- if applicable: touches Firebase RTDB schema or Cloud Functions in functions/ -->
## Firebase Contract *(if applicable)*

<!--
  All RTDB schema changes MUST be additive only.
  No existing control flags or status nodes may be removed or renamed.
  New nodes must be optional in all reader/writer implementations.
-->

### RTDB Schema Changes
```json
// New or modified nodes only. Mark each as [NEW] or [MODIFIED].
{
  "devices": {
    "[device_id]": {
      // [NEW] example_field: describe type, default, and which component writes it
    }
  }
}
```

### Cloud Functions Changes *(if applicable)*
- Function name(s) affected: [list]
- Trigger: [RTDB path / HTTP / scheduled]
- Breaking changes: NONE (or document migration path)

---

<!-- if applicable: touches dashboard/ (Next.js PWA) -->
## Dashboard UX *(if applicable)*

<!--
  Reference actual component paths in dashboard/src/.
  Note: 4 tests in __tests__/lib/usePumpData.test.tsx have a pre-existing failure
  (mock uses set, source uses update) — don't mistake this for a regression.
-->

### User-Facing Changes
- [Screens/components affected]
- [New UI states and their conditions]

### Offline / PWA Behavior
- [What works offline vs. requires connectivity]

### Test Coverage
- New tests in `dashboard/__tests__/`: [describe]
- Pre-existing failure scope: does this change affect `usePumpData`? [Yes/No — if Yes, note separately]

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: [Measurable metric — be specific, not "works correctly"]
- **SC-002**: [Measurable metric]
- **SC-003**: [Measurable metric]

### Validation Commands

```bash
# Dashboard
cd dashboard && npm test && npm run validate

# Cloud Functions
cd functions && npm run build && npm test

# Firmware (requires hardware — document expected serial output instead)
# pio run -e [env] --target upload && pio device monitor
```

---

## Assumptions

<!--
  Document assumptions made when the feature description was underspecified.
  Flag anything that requires hardware, credentials, or physical access.
-->

- [Assumption about scope — e.g., "firmware change is master node only, not sensor node"]
- [Assumption about dependencies — e.g., "RTDB rules already permit the new node path"]
- [Hardware dependency — e.g., "requires physical ESP32+relay setup to fully validate"]
