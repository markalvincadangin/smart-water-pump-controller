# SmartFlow Release Documentation

## Version v1.0.0 (First Stable Release)

This directory contains the formal release documentation set for SmartFlow v1.0.0.

| Field | Value |
|---|---|
| Release tier | Stable |
| Release designation | First stable release |
| Documentation scope | Firmware, dashboard, RTDB contract, deployment, UX behavior |
| Product name | SmartFlow |

## Audience

- Deployment and operations engineers
- Firmware and dashboard maintainers
- Security and safety reviewers
- Release and incident coordinators

## Document Set

- [release-notes.md](release-notes.md): release summary, compatibility, and limitations.
- [release_manifest.md](release_manifest.md): release metadata, readiness checklist, and required sign-offs.
- [system-overview.md](system-overview.md): architecture and end-to-end control/data flow.
- [deploy.md](deploy.md): deployment and commissioning procedure.
- [firmware.md](firmware.md): ESP32/ESP8266 firmware behavior and safety model.
- [dashboard.md](dashboard.md): dashboard architecture and operator behavior.
- [dashboard-ux-spec.md](dashboard-ux-spec.md): safety-oriented UX contract.
- [rtdb-contract.md](rtdb-contract.md): schema and write-ownership contract.

## Governance

- Canonical evolving specs are maintained in `docs/specs/`.
- This release folder is a versioned snapshot and should only be changed for factual corrections.

