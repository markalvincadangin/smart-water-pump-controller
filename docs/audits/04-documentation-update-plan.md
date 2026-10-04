---
status: complete
version: 1.0
last-reviewed: 2026-10-04
source: WP-03 documentation and diagram audit
---
# WP-04 — Documentation & Diagram Update Plan

## 1. Purpose
This plan converts WP-03 findings into an ordered, low-risk documentation migration. The objective is to make SmartFlow documentation internally consistent without rewriting useful material unnecessarily.

WP-04 is a planning artifact. It does not change implementation code or perform the documentation migration itself.

## 2. Guiding Rules
1. The WP-02 system contract is the cross-system authority.
2. Domain specifications remain responsible for implementation-specific detail.
3. README documents the project at a useful overview level; it is not a second specification.
4. Historical material remains available but must not appear current.
5. A documented value must be labeled as an implementation/configuration value, engineering target, or physically verified value.
6. Safety claims must not imply physical verification that has not happened.
7. Documentation changes must not silently change firmware behavior.
8. Each documentation PR should be small enough to review and revert independently.

## 3. Target Documentation Hierarchy
```text
                    SmartFlow
                       │
             System Contract (WP-02)
                       │
        ┌──────────────┼──────────────┐
        ▼              ▼              ▼
    Firmware         Firebase       Android
     specs            contract        spec
        │
        ▼
   RS-485 protocol

Safety Constitution → non-negotiable engineering rules
Deployment Safety → physical commissioning / verification
Audits → evidence of what was reviewed
Verification → evidence of what was actually tested
```

## 4. Migration Order
1. Keep WP-02 contract as the cross-system authority.
2. Update docs/specs/README.md governance.
3. Add explicit contract references to firmware and Android specs.
4. Update docs/specs/firmware_operational_rules.md.
5. Update DEPLOYMENT_SAFETY.md.
6. Reconcile .specify/memory/constitution.md terminology.
7. Ensure MAX_RUNTIME, TANK_FULL, and OVERFLOW are not conflated.
8. Reconcile docs/specs/firmware.md with the actual device shadow.
9. Remove or explicitly label obsolete /pump_system/* paths.
10. Ensure desired/reported/telemetry/events paths agree with WP-02.
11. Rewrite firmware/README.md around the actual PlatformIO structure.
12. Update docs/specs/app.md to reference WP-02 and document current Android state semantics.
13. Update root README.md and CONTRIBUTING.md.
14. Update/create the authoritative architecture and safety diagrams.

## 5. Exact File Change Plan
### docs/specs/README.md — UPDATE
Add the relationship to docs/specifications/smartflow-system-contract.md and distinguish cross-system authority from domain authority. Define the roles of audits, verification, and archive material.

### docs/specs/firmware.md — UPDATE
Reconcile RTDB schema, desired/reported shadow, telemetry, events, mode names, configuration values, and RS-485 references against WP-02 and actual implementation. Do not copy the whole system contract.

### docs/specs/firmware_operational_rules.md — UPDATE, HIGH PRIORITY
Replace obsolete Firebase paths after implementation verification. Reconcile MANUAL, COUNTDOWN, AUTO, E-stop, dry-run, maximum runtime, tank-full, sensor freshness, minimum OFF-time, one-shot commands, and recovery semantics. Use MAX_RUNTIME where the condition is maximum runtime rather than OVERFLOW.

### docs/specs/rs485_protocol.md — KEEP / VERIFY
Verify the protocol against actual firmware serial initialization, frame format, CRC, baud rate, timing, field names, and master/node responsibilities.

### docs/specs/app.md — UPDATE
Reference WP-02 and explicitly document desired-vs-reported semantics, command lifecycle, stale telemetry, ownership, notification semantics, safety-fault rendering, and the MVP mode boundary.

### DEPLOYMENT_SAFETY.md — UPDATE, HIGH PRIORITY
Correct stale dry-run and maximum-runtime values. Label them as current firmware configuration values pending physical commissioning. Separate software validation, electrical verification, and physical commissioning.

### .specify/memory/constitution.md — UPDATE SAFETY TERMINOLOGY ONLY
Keep all safety principles. Rename the runtime-protection concept to Maximum Runtime Protection and use MAX_RUNTIME for the runtime fault. Reserve OVERFLOW for a genuine high-high/overflow condition if implemented.

### firmware/README.md — UPDATE
Rewrite onboarding around PlatformIO, master/sensor projects, build commands, configuration, directory structure, and OTA development. Remove or explicitly label obsolete Arduino-era instructions.

### README.md — UPDATE, PUBLIC-FACING
Keep project purpose, real-world problem, system overview, hardware, capabilities, visuals, architecture overview, status, and links. Remove detailed normative values better owned by specifications. Explicitly state that MVP is Manual + Countdown + safety protections and AUTO remains under development/physical validation.

### CONTRIBUTING.md — UPDATE
Align workflow to main → short-lived focused branch → pull request → review/tests → main. Retain conventional commits.

## 6. Diagram Implementation Plan
### Diagram 1 — Physical Architecture
Show the 660 L tank, sensors, ESP8266, RS-485, ESP32, relay, contactor, pump, physical E-stop, Firebase, and Android. Keep physical architecture separate from software semantics.

### Diagram 2 — Logical Control/Data Flow
```text
Android → Firebase desired → ESP32 → safety evaluation → physical pump control
                                      ├→ shadow/reported
                                      ├→ telemetry
                                      └→ events
                                              ↓
                                           Firebase
                                              ↓
                                           Android
```
Highlight: Desired state is intent. Reported state is actual device state.

### Diagram 3 — Safety Decision Flow
```text
Request → E-stop? → Hard safety fault? → Sensor/communication valid?
        → Tank-full? → Minimum OFF-time? → Mode policy → Pump command
```
Do not show AUTO as part of MVP acceptance.

### Diagram 4 — Firebase Device Contract
```text
devices/{deviceId}
├── ownership
├── metadata
├── pairing
├── shadow
│   ├── desired
│   └── reported
├── telemetry
├── settings
├── status
├── diagnostics
└── events

users/{uid}
├── devices
└── notification_prefs
```

## 7. Documentation Cross-Link Plan
System contract links to firmware specification, Android specification, RS-485 protocol, notification specification when created, and verification reports.
Firmware specification links to system contract, operational rules, and RS-485 protocol.
Android specification links to system contract and notification specification.
Deployment safety links to the constitution, firmware operational rules, and MVP verification.
README links to system contract, deployment safety, firmware docs, Android docs, and verification status.

## 8. Documentation Migration Rules
KEEP if accurate, useful, and owned by that document.
MOVE if the content belongs to another authoritative document.
UPDATE if the concept is correct but values, paths, or names are stale.
ARCHIVE if it explains a genuinely historical architecture or implementation.
REMOVE if it is duplicated without value, misleading, obsolete, or contradicted by current implementation with no migration purpose.

## 9. Validation Before Each Documentation Edit
Before changing a normative value, verify it against implementation: dry-run threshold and max runtime from config.h; countdown limits from desired-state validation; RTDB paths from Firebase repository/cloud manager; event codes from firmware and Cloud Functions; notification fields from Cloud Functions and Android; RS-485 baud from serial initialization; ownership paths from security rules/bootstrap functions.

Documentation must follow implementation reality unless a deliberate implementation change is being made separately.

## 10. Commit Strategy
1. docs(governance): establish documentation source hierarchy
2. docs(firmware): align operational rules with system contract
3. docs(firebase): reconcile device shadow and RTDB paths
4. docs(safety): align deployment checklist and fault terminology
5. docs(android): align app specification with device contract
6. docs(firmware): update firmware onboarding
7. docs(contributing): align branch and PR workflow
8. docs(readme): refresh project overview
9. docs(diagrams): add authoritative system and safety flows

Each commit should be independently reviewable.

## 11. Acceptance Test
- No current document says 0.5 L/min unless explicitly historical.
- No current document says 15 sec dry-run timeout unless explicitly historical.
- No current document says 45 min max runtime unless explicitly historical.
- Current RS-485 baud is consistently documented.
- Current Firebase paths are consistently documented.
- AUTO is explicitly non-MVP/in-development.
- System contract is clearly cross-system authority.
- Firmware, Android, constitution, audits, and verification have distinct ownership.
- Physical architecture, logical flow, safety flow, and Firebase tree diagrams are current.
- Diagrams do not imply unverified physical behavior.
- Documentation changes are reviewable and implementation code is not mixed into documentation commits.

## 12. Implementation Boundary
WP-04 does not authorize firmware behavior changes, Android behavior changes, Firebase schema migrations, safety threshold changes, notification implementation changes, or AUTO completion.

## 13. Recommended Execution After WP-04
```text
WP-04 Documentation Plan
          ↓
Documentation migration
          ↓
WP-05 Firmware/MVP corrections
          ↓
WP-06 Firebase/backend audit
          ↓
WP-07 Android architecture audit
          ↓
WP-08 Android UI/UX audit
          ↓
WP-09 Notification audit
          ↓
Implementation + verification
```

**Status: COMPLETE.**