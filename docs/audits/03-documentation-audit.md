---
status: complete
version: 1.0
last-reviewed: 2026-10-04
source: repository audit against docs/specifications/smartflow-system-contract.md
---

# WP-03 — Documentation & Diagram Audit

## 1. Purpose

This audit compares the repository's current documentation against the proposed SmartFlow System Contract established in WP-02.

The goal is not to rewrite documentation yet. The goal is to identify:

- conflicting sources of truth
- stale values and paths
- duplicate specifications
- MVP/AUTO scope confusion
- missing diagrams or missing diagram updates
- documents that should be retained, merged, archived, or removed

No implementation code is changed by this work package.

---

## 2. Audit Verdict

**Documentation is useful but currently has multiple generations of architecture and behavior mixed together.**

The repository has a strong set of safety, firmware, Android, and RS-485 documents, but they do not consistently describe the same system revision.

The most important problems are:

1. Root README contains stale safety values.
2. Deployment safety checklist contains stale safety acceptance values.
3. Firmware operational rules still describe legacy `/pump_system/*` cloud paths.
4. Current firmware specification and WP-02 contract use `/devices/{deviceId}/*`.
5. Android domain models contain defaults that conflict with the intended MVP safety contract.
6. AUTO is presented as a current feature even though it is not part of the validated MVP boundary.
7. RS-485 documentation is current at 115200 baud, while root documentation still says 9600.
8. Multiple documents claim to be authoritative for overlapping areas.
9. The repository's documented architecture does not yet have one clear system-level diagram showing desired → firmware decision → reported state → Android.
10. Documentation governance still references a `develop` workflow while the current working strategy is short-lived branches targeting `main`.

These are documentation/system-contract issues, not evidence that every implementation path is broken.

---

## 3. Source-of-Truth Hierarchy

After WP-02, the intended hierarchy should be:

1. **SmartFlow System Contract**
   - Cross-component device/cloud/application contract.
2. **Firmware Operational Rules**
   - Firmware-specific behavior and state-machine rules.
3. **Firmware Specification**
   - Firmware architecture, hardware interfaces, implemented schema details.
4. **RS-485 Protocol**
   - Wire-level protocol only.
5. **Android Application Specification**
   - Android UX, provisioning, state interpretation, and client behavior.
6. **Constitution**
   - Non-negotiable engineering/safety principles.
7. **Deployment Safety**
   - Physical commissioning and verification procedure.
8. **README**
   - User-facing overview; should summarize, not become a second specification.
9. **Historical/archive documents**
   - Reference only; explicitly non-authoritative.

The repository currently has overlapping claims that do not consistently follow this hierarchy.

---

## 4. Document Audit Matrix

| Document | Status | Decision | Main issue |
|---|---|---|---|
| `docs/specifications/smartflow-system-contract.md` | New | KEEP / AUTHORITATIVE | Cross-system contract |
| `docs/specs/firmware.md` | Current | KEEP / UPDATE | Needs alignment with system contract and actual serialized shadow |
| `docs/specs/firmware_operational_rules.md` | Current | KEEP / UPDATE | Contains legacy RTDB paths and outdated operational wording |
| `docs/specs/rs485_protocol.md` | Current | KEEP | Appears aligned with current 115200 protocol; cross-check exact implementation |
| `docs/specs/app.md` | Current | KEEP / UPDATE | Good UX/security rules but needs explicit contract reference and current state schema |
| `docs/specs/README.md` | Current | UPDATE | Governance predates WP-02 and defines an incomplete source-of-truth model |
| `DEPLOYMENT_SAFETY.md` | Current | KEEP / UPDATE | Safety values conflict with firmware |
| `firmware/README.md` | Current | UPDATE / SIMPLIFY | Contains stale architecture/setup paths and legacy Arduino structure |
| root `README.md` | Current | UPDATE / SIMPLIFY | Multiple stale values and AUTO treated as current feature |
| `.specify/memory/constitution.md` | Ratified | KEEP / REVIEW | Strong safety principles, but terminology such as OVERFLOW needs reconciliation |
| `CONTRIBUTING.md` | Current | UPDATE | Branch workflow conflicts with current main + short-lived branch approach |
| `docs/audits/01-mvp-safety-audit.md` | Audit | KEEP | Historical audit record; not a specification |
| historical `docs/archive/` content | Historical | ARCHIVE | Keep only when clearly labeled historical |
| old `specs/###-...` feature artifacts | Historical/feature | REVIEW / ARCHIVE | Must not compete with current specs |

---

## 5. Critical Documentation Drift

### 5.1 Dry-run protection

Root README currently states:

- threshold: **0.5 L/min**
- timeout: **15 seconds**

Current firmware/configuration and newer documentation use:

- threshold: **1.0 L/min**
- timeout: **30 seconds**

**Decision:** 1.0 L/min / 30 seconds is the current implementation value. It still requires physical validation.

### 5.2 Maximum runtime

Root README and deployment checklist currently state:

- **45 minutes**

Current firmware configuration/specification uses:

- **120 minutes**

The WP-02 contract records 120 minutes as the current implementation default, not as a physically validated safety claim.

### 5.3 RS-485 baud rate

Root README states:

- **9600 baud**

Current firmware specification and RS-485 protocol specify:

- **115200 8N1**

This is a direct documentation contradiction.

### 5.4 AUTO threshold

Root README describes:

- start around 20%
- stop around 90%

Current firmware may contain configurable thresholds and newer documentation describes the implemented control model.

AUTO must nevertheless be presented as **development/non-MVP** until independently validated.

### 5.5 Legacy Firebase paths

`firmware_operational_rules.md` describes:

- `/pump_system/control`
- `/pump_system/status`
- `/pump_system/config/device`

The current system contract and firmware specification use:

- `/devices/{deviceId}/shadow/desired`
- `/devices/{deviceId}/shadow/reported`
- `/devices/{deviceId}/settings`
- `/devices/{deviceId}/telemetry`

This is one of the highest-priority documentation corrections because a developer following the operational rules could implement against the wrong API.

### 5.6 Fault terminology

The constitution and firmware operational rules use `OVERFLOW` for maximum runtime protection.

The actual condition described is maximum continuous runtime.

The contract therefore distinguishes:

- `MAX_RUNTIME`
- `TANK_FULL`
- `OVERFLOW`

Documentation should use the same vocabulary.

---

## 6. MVP Scope Drift

Several documents describe SmartFlow as having three production operating modes:

- AUTO
- MANUAL
- COUNTDOWN

For the current project audit, this is misleading.

The accepted MVP scope is:

- MANUAL
- COUNTDOWN
- safety protections

AUTO is implemented as an evolving foundation but has not completed the required engineering/physical validation.

### Required documentation language

Use:

> **MVP:** Manual control + Countdown control + safety protections.

And:

> **AUTO:** Development/in-progress capability. Not part of current MVP acceptance.

Do not remove AUTO implementation merely to make the documentation cleaner.

---

## 7. Safety Documentation Audit

The repository has unusually strong safety documentation, but some safety documents have diverged from implementation.

### Good coverage

The following concepts are documented consistently enough to retain:

- fail toward OFF
- independent hardware thermal overload protection
- E-stop
- dry-run lockout
- maximum runtime protection
- sensor freshness gating
- RS-485 validation
- minimum OFF-time
- NVS/persistence
- physical commissioning warnings

### Required corrections

The documentation must distinguish:

- software safety vs physical safety
- configured defaults vs physically validated limits
- maximum runtime vs tank overflow
- maintenance bypass vs normal operating mode
- desired state vs actual reported state

No documentation should imply that software has been physically verified when WP-01 explicitly found those tests outstanding.

---

## 8. Diagram Audit

### 8.1 Existing high-level diagram

The root README has a useful physical architecture diagram showing:

- tank sensors
- ESP8266
- RS-485
- ESP32
- relay/contactor
- pump
- Firebase
- BLE

This should be retained, but updated to reflect the current cloud/device contract.

### 8.2 Missing authoritative logical diagram

A new system-level logical diagram is needed:

```text
Android
   │
   │ desired commands
   ▼
Firebase RTDB
   │
   ├── shadow/desired
   │
   ▼
ESP32 Controller
   │
   ├── validate intent
   ├── evaluate safety
   ├── control relay
   │
   ├──────────────► shadow/reported
   │
   ├──────────────► telemetry
   │
   └──────────────► events
            │
            ▼
        Firebase
            │
            ▼
         Android
```

The key message should visually communicate:

**Android/Firebase request → ESP32 safety decision → physical state → reported state.**

### 8.3 Missing safety decision diagram

A dedicated safety/control flow diagram should show:

```text
Command / Mode Intent
        │
        ▼
Emergency Stop?
   ├─ YES → OFF / LATCH
   └─ NO
        │
        ▼
Hard Safety Fault?
   ├─ YES → OFF / LOCKOUT
   └─ NO
        │
        ▼
Sensor / RS-485 Valid?
   ├─ NO → OFF / START BLOCK
   └─ YES
        │
        ▼
Minimum OFF Time?
   ├─ ACTIVE → WAIT
   └─ CLEAR
        │
        ▼
MANUAL / COUNTDOWN policy
        │
        ▼
Pump command
```

### 8.4 Missing Firebase contract diagram

A compact RTDB tree should be added to the documentation:

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
```

This should replace scattered legacy path descriptions.

---

## 9. README Audit

The root README is visually strong and useful as a portfolio-facing introduction, but it currently tries to be both marketing overview and engineering specification.

### KEEP

- project overview
- real-world problem
- physical architecture
- hardware summary
- repository structure
- safety philosophy
- links to detailed documentation
- deployment/prototype status

### REMOVE FROM README

Avoid detailed normative values that can drift:

- exact dry-run threshold
- exact dry-run timeout
- exact maximum runtime
- detailed Firebase path names
- detailed mode semantics
- exact RS-485 timing

These belong in specifications.

### UPDATE

The README should instead link to:

- system contract
- firmware specification
- operational rules
- RS-485 protocol
- Android specification
- deployment safety
- verification reports

---

## 10. Firmware README Audit

`firmware/README.md` contains significant legacy material.

Examples include an older Arduino IDE project structure and credential instructions that do not clearly match the current PlatformIO-first architecture.

It also contains legacy RTDB references such as `/pump_system/config/device`.

### Decision

**Keep as firmware onboarding documentation, but rewrite around the actual current PlatformIO projects.**

The old Arduino sketch structure should either be explicitly labeled historical/legacy or removed if those files are no longer maintained.

---

## 11. Android Documentation Audit

`docs/specs/app.md` contains useful current requirements:

- durable account requirement
- ownership rules
- provisioning handoff
- server-authoritative ownership
- diagnostics
- Material 3
- state idempotency
- pending command behavior

It should remain.

However, it needs to reference the WP-02 system contract and explicitly define:

- desired state is intent
- reported state is authoritative for UI
- telemetry freshness is separate from Firebase connectivity
- safety faults are machine-readable
- notification behavior follows event codes

---

## 12. Constitution Audit

The constitution is valuable and should remain authoritative for safety principles.

However, the following terminology should be reconciled:

`OVERFLOW` currently represents maximum runtime protection.

The constitution should eventually use `MAX_RUNTIME` for the software condition and reserve `OVERFLOW` for an actual high-high/overflow condition.

This is a semantic correction, not a weakening of the safety rule.

The constitution's fail-OFF, E-stop, sensor freshness, relay authority, and wrap-safe timing rules should remain intact.

---

## 13. Contribution Workflow Audit

`CONTRIBUTING.md` currently defines:

```text
feature → develop → main
```

The current repository workflow being used for SmartFlow is:

```text
main
  └── short-lived focused branch
        └── PR
              └── main
```

### Decision

Update CONTRIBUTING.md to match the actual workflow.

There is no need to introduce a permanent `develop` branch unless the project genuinely adopts a release-train workflow.

---

## 14. Documentation Governance Problem

The current `docs/specs/README.md` says that `docs/specs/` is the source-of-truth collection and that each domain file owns its contract.

WP-02 introduces a cross-system contract that necessarily spans:

- firmware
- Firebase
- Android
- events
- notifications

Therefore the governance model needs to distinguish:

### Cross-system contract

`docs/specifications/smartflow-system-contract.md`

### Domain specifications

`docs/specs/`

### Safety constitution

`.specify/memory/constitution.md`

### Verification

`docs/verification/`

### Audits

`docs/audits/`

### Historical material

`docs/archive/`

This prevents the system contract from being duplicated inside firmware and Android documents.

---

## 15. Recommended Documentation Structure

Target structure:

```text
docs/
├── audits/
│   ├── 01-mvp-safety-audit.md
│   ├── 02-system-contract-audit.md
│   ├── 03-documentation-audit.md
│   ├── 04-firebase-audit.md
│   └── 05-android-audit.md
│
├── specifications/
│   ├── smartflow-system-contract.md
│   ├── control-safety-specification.md
│   └── notification-specification.md
│
├── verification/
│   ├── mvp-verification-report.md
│   └── final-system-verification.md
│
├── specs/
│   ├── firmware.md
│   ├── firmware_operational_rules.md
│   ├── rs485_protocol.md
│   └── app.md
│
├── operations/
│   └── ...
│
└── archive/
    └── ...
```

The existing files should be migrated incrementally rather than duplicated.

---

## 16. Priority Matrix

### P0 — Must correct before implementation documentation is trusted

- Replace stale dry-run values in README/deployment docs.
- Replace stale 45-minute runtime references.
- Resolve 9600 vs 115200 RS-485 documentation.
- Remove/mark legacy `/pump_system/*` paths.
- Establish the system contract as the cross-system authority.
- Clearly mark AUTO as non-MVP/in-development.

### P1 — Correct during documentation update

- Normalize fault terminology.
- Align Android specification with system contract.
- Align firmware operational rules with current device shadow paths.
- Update firmware README for PlatformIO-first architecture.
- Update CONTRIBUTING workflow.
- Add system architecture/data-flow diagram.
- Add safety decision-flow diagram.
- Add Firebase RTDB tree diagram.

### P2 — Polish

- Reduce duplicate technical detail in root README.
- Improve cross-links.
- Add version/review metadata consistently.
- Separate portfolio-facing explanation from normative engineering documentation.
- Add verification-status indicators to safety claims.

---

## 17. What Should NOT Be Done

Do not:

- rewrite every document from scratch
- duplicate the system contract into firmware.md and app.md
- delete useful historical documents simply because they are old
- remove AUTO code solely to simplify documentation
- claim physical safety verification based on static code inspection
- add diagrams that imply behavior not verified by code
- preserve legacy RTDB paths merely for documentation compatibility if they are no longer implemented

---

## 18. Acceptance Criteria

WP-03 is complete when:

- all major current documentation has been compared with WP-02
- stale values and paths are identified
- authoritative ownership is defined
- AUTO/MVP boundary is explicit
- diagram gaps are identified
- each major document has a KEEP/UPDATE/ARCHIVE/REMOVE decision
- no implementation code is changed as part of this audit

**Status: COMPLETE.**

---

## 19. Recommended Next Work Package

**WP-04 — Documentation & Diagram Update Plan**

WP-04 should turn this audit into an ordered implementation plan containing:

1. exact files to modify
2. exact sections to update
3. diagrams to create/update
4. migration order
5. cross-reference changes
6. acceptance checks
7. commit/branch structure

Only after WP-04 should the repository's documentation be materially rewritten.
