# [PROJECT_NAME] Constitution
<!-- SmartFlow naming convention: "[ProjectName] Constitution" -->

## Core Principles

<!--
  SMARTFLOW GUIDANCE: This project governs a safety-critical physical system
  (relay-controlled pump). Your first principle MUST address fail-safe direction.
  Ask: "If the firmware encounters an unhandled fault, which physical state is safe?"
  For SmartFlow: de-energized relay (pump OFF) is safe. State this explicitly.
-->

### [PRINCIPLE_1_NAME] *(NON-NEGOTIABLE)*
<!-- SmartFlow pattern: "I. Fail Toward [SAFE_STATE]" -->
[PRINCIPLE_1_DESCRIPTION]
<!--
  Cover:
  - Which hardware state is safe under fault/ambiguity
  - Which function/pin controls safety state
  - Whether watchdog, timeout, and unhandled exceptions are covered
-->

### [PRINCIPLE_2_NAME] *(NON-NEGOTIABLE)*
<!-- SmartFlow pattern: "II. [PROTECTION_NAME] Lockout" -->
[PRINCIPLE_2_DESCRIPTION]
<!--
  Cover:
  - What condition triggers lockout
  - What the system does immediately (state flag + safe-state actuator call)
  - What the ONLY valid exit condition is (explicit command vs. timeout)
  - Whether this survives refactors
-->

### [PRINCIPLE_3_NAME] *(NON-NEGOTIABLE)*
<!-- SmartFlow pattern: "III. [SECOND_PROTECTION_NAME]" -->
[PRINCIPLE_3_DESCRIPTION]

### [PRINCIPLE_4_NAME]
<!-- SmartFlow pattern: "IV. Hardware Independence" — software safety != hardware safety -->
[PRINCIPLE_4_DESCRIPTION]
<!--
  Cover:
  - Hardware safety systems present (e.g., thermal overload relay)
  - Why firmware must not assume sole protection responsibility
  - What firmware must never bypass
-->

### [PRINCIPLE_5_NAME]
<!-- SmartFlow pattern: "V. Sensor Freshness Gate & Reachable E-Stop" -->
[PRINCIPLE_5_DESCRIPTION]
<!--
  For IoT systems: cover data freshness requirements before automated actuation,
  and ensure emergency stop / error clear paths cannot be blocked by normal code paths.
-->

### [PRINCIPLE_6_NAME]
<!-- SmartFlow pattern: "VI. Backward Compatibility & Additive Contracts" -->
[PRINCIPLE_6_DESCRIPTION]
<!--
  For systems with serial protocols and cloud schemas:
  - Protocol change rules (new fields must be optional with safe defaults)
  - Schema change rules (additive only, no removal/rename without migration)
-->

---

## Technical Constraints & Module Governance

<!--
  SmartFlow pattern: enumerate modules, relay control invariant, timing invariant.
  Replace with your project's equivalent hard constraints.
-->

1. **Repository Scope**: List each software module and its path.
2. **[ACTUATOR] Control Invariant**: Identify the single authorized actuator control function.
   Direct hardware writes outside that function are prohibited.
3. **[TIMING / RESOURCE] Invariant**: Identify any wrap-safe or resource-safe patterns
   that must be used everywhere (e.g., monotonic timers, memory allocator).

---

## Governance

- **Supremacy**: This Constitution supersedes all informal team practices, feature requests,
  or unratified documentation.
- **Compliance**: All Pull Requests, SpecKit plans (`/speckit-plan`), and task breakdowns
  (`/speckit-tasks`) MUST explicitly verify adherence to the non-negotiable principles.
- **Amendments**: Any change to these principles requires formal review, physical/system
  risk assessment, updated field documentation, and an explicit version bump.

**Version**: [CONSTITUTION_VERSION] | **Ratified**: [RATIFICATION_DATE] | **Last Amended**: [LAST_AMENDED_DATE]
