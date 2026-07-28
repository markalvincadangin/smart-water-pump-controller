---
status: current
last-reviewed: 2026-07-25
source: hand-authored
---

# SmartFlow Specifications (Current)

This folder contains the current, non-versioned specifications for SmartFlow as implemented in this repository.
Each file is the single source of truth for its domain. Do not duplicate content across files — cross-reference instead.

## Specification Set

| File | Domain | Source of truth for |
|------|--------|---------------------|
| [`firmware.md`](./firmware.md) | Firmware architecture | System architecture, build targets, GPIO assignments, RS-485 overview, control modes, RTDB schema (status + control fields) |
| [`firmware_operational_rules.md`](./firmware_operational_rules.md) | Firmware behavior | Mode model, decision priority, emergency stop lifecycle, safety rules, one-shot command semantics, WiFi/restart/safe-mode behavior |
| [`rs485_protocol.md`](./rs485_protocol.md) | Wire protocol | RS-485 frame format, CRC, timing, field semantics, master acceptance rules |
| [`dashboard.md`](./dashboard.md) | Dashboard / UX | Dashboard system responsibilities, RTDB consumption rules, control UX contract, safety-critical display rules |

## Content Ownership Map

The following content types have exactly one home. Do not add them elsewhere:

| Content type | Owner |
|---|---|
| RTDB field definitions (names, types, valid values) | `firmware.md` |
| GPIO / pin assignments | `firmware.md` → Hardware Interface section |
| Firmware state machine and behavioral rules | `firmware_operational_rules.md` |
| RS-485 framing, CRC, timing | `rs485_protocol.md` |
| Dashboard RTDB reads and UX rules | `dashboard.md` |
| Safety non-negotiables (fail-toward-OFF, lockout semantics) | `.specify/memory/constitution.md` — not duplicated into specs |

## Document Governance

- **Source of truth:** This folder (`docs/specs/`) plus supporting material under `docs/` (operations, audit, ADR) as referenced here. Agents and contributors MUST treat these files as canonical for current implementation behavior.
- **Feature development:** ALWAYS use the Spec Kit lifecycle (`/speckit-specify` → clarify if needed → `/speckit-plan` → `/speckit-tasks` → `/speckit-implement`). Artifacts live under `specs/[###-feature-name]/` — never as new files in this folder.
- **Edits:** All changes are in-place edits to existing files. Do not create new spec files for topics that fit an existing owner — add a section or subsection instead.
- **When code changes behavior:** Update the owning spec file here in the same PR/change set.
- **Historical and superseded documentation:** `docs/archive/`
- **Architectural reference plans** (shipped work): `docs/plans/` — read-only design artifacts, not active spec-kit workflows
- **Active known issues:** `docs/audit/firmware_known_issues_2026-04-02.md`
- **Field runbooks:** `docs/operations/`
