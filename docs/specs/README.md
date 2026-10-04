---
status: current
last-reviewed: 2026-10-04
source: hand-authored
---

# SmartFlow Specifications (Current)

This folder contains the current specifications for SmartFlow domain implementations in this repository.
The overarching cross-system contract governing interactions between ESP32 Firmware, Firebase RTDB, Cloud Functions, and the Android client is defined in [`docs/specifications/smartflow-system-contract.md`](../specifications/smartflow-system-contract.md) (v1.1).
Each file in this folder is the single source of truth for its domain. Do not duplicate content across files — cross-reference instead.

## Specification Set

| File | Domain | Source of truth for |
|------|--------|---------------------|
| [`../specifications/smartflow-system-contract.md`](../specifications/smartflow-system-contract.md) | Cross-system contract | Authoritative system-wide contract (v1.1), canonical event vocabulary, state lifecycle, RTDB `.validate` rules, and safety boundaries |
| [`firmware.md`](./firmware.md) | Firmware architecture | System architecture, build targets, GPIO assignments, RS-485 overview, control modes, RTDB schema (status + control fields) |
| [`firmware_operational_rules.md`](./firmware_operational_rules.md) | Firmware behavior | Mode model, decision priority, emergency stop lifecycle, safety rules, one-shot command semantics, WiFi/restart/safe-mode behavior |
| [`rs485_protocol.md`](./rs485_protocol.md) | Wire protocol | RS-485 frame format, CRC, timing, field semantics, master acceptance rules |
| [`app.md`](./app.md) | Android app / UX | App navigation, provisioning, ownership, RTDB consumption, command lifecycle, and safety-critical UX rules |
| [`coding_standards.md`](./coding_standards.md) | Code quality / architecture | Firmware structural layers, BARR-C:2018 style rules, naming conventions, and file placement |

## Content Ownership Map

The following content types have exactly one home. Do not add them elsewhere:

| Content type | Owner |
|---|---|
| Master cross-boundary contract & safety rules | `docs/specifications/smartflow-system-contract.md` |
| RTDB field definitions (names, types, valid values) | `firmware.md` |
| GPIO / pin assignments | `firmware.md` → Hardware Interface section |
| Firmware state machine and behavioral rules | `firmware_operational_rules.md` |
| RS-485 framing, CRC, timing | `rs485_protocol.md` |
| Android app RTDB reads, command model, and UX rules | `app.md` |
| Firmware directory structure and coding standards | `coding_standards.md` |
| Safety non-negotiables (fail-toward-OFF, lockout semantics) | `.specify/memory/constitution.md` — not duplicated into specs |

## Document Governance

- **Source of truth:** [`docs/specifications/smartflow-system-contract.md`](../specifications/smartflow-system-contract.md) plus this folder (`docs/specs/`) and supporting material under `docs/` (operations, audit, ADR) as referenced here. Agents and contributors MUST treat these files as canonical for current implementation behavior.
- **Feature development:** ALWAYS use the Spec Kit lifecycle (`/speckit-specify` → clarify if needed → `/speckit-plan` → `/speckit-tasks` → `/speckit-implement`). Artifacts live under `specs/[###-feature-name]/` — never as new files in this folder.
- **Edits:** All changes are in-place edits to existing files. Do not create new spec files for topics that fit an existing owner — add a section or subsection instead.
- **When code changes behavior:** Update the owning spec file here in the same PR/change set.
- **Historical and superseded documentation:** `docs/archive/`
- **Architectural reference plans** (shipped work): `docs/plans/` — read-only design artifacts, not active spec-kit workflows
- **Active known issues:** `docs/audit/firmware_known_issues_2026-04-02.md`
- **Field runbooks:** `docs/operations/`
