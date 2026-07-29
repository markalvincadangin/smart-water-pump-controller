# Documentation Index

This directory is the **documentation source of truth** for SmartFlow alongside feature work tracked under [`../specs/`](../specs/) via **Spec Kit**.

- **Current specs** (`./specs/`) — canonical for the current implementation ([`specs/README.md`](./specs/README.md)):
  - [specs/firmware.md](./specs/firmware.md)
  - [specs/firmware_operational_rules.md](./specs/firmware_operational_rules.md)
  - [specs/rs485_protocol.md](./specs/rs485_protocol.md)
  - [specs/coding_standards.md](./specs/coding_standards.md)
- **Feature specs (Spec Kit):** [`../specs/`](../specs/) — always use the Spec Kit lifecycle for non-trivial changes; do not add feature specs under `docs/specs/`.
- **Architecture**:
  - Current architecture references are maintained in [specs/firmware.md](./specs/firmware.md), [operations/firmware_config_from_database.md](./operations/firmware_config_from_database.md), and [operations/rs485_tank_link.md](./operations/rs485_tank_link.md).
  - No standalone `docs/architecture_redesign_vNext.md` document is maintained in this repository.
- **Releases** (`./releases/`)
  - `v1.0.0/` is the first stable release documentation set.
  - No other stable release sets are retained in this repository.
- **Operations** (`./operations/`)
  - [troubleshooting.md](./operations/troubleshooting.md), [safety.md](./operations/safety.md), [notifications_setup.md](./operations/notifications_setup.md), [firmware_config_from_database.md](./operations/firmware_config_from_database.md), [rs485_tank_link.md](./operations/rs485_tank_link.md).
- **Assets** (`./assets/`)
  - `diagrams/` — system diagrams (e.g. Diagram.png, Diagram.pdf).
  - `manuals/` — Master_Manual.pdf, Hardware_Documentation.pdf, Software_Firmware_Documentation.pdf.
- **Archive** (`./archive/`)
  - Superseded docs (DEPLOY_GUIDE, DASHBOARD_DESIGN_v2, FIRMWARE_DASHBOARD_DESIGN_v2, ENHANCEMENT_PLAN) kept for reference only.

Authoritative specs for the current code live under `./specs/`.

