# Release Manifest - SmartFlow v1.0.0

## Release Identity

| Field | Value |
|---|---|
| Product | SmartFlow |
| Version | v1.0.0 |
| Stability tier | Stable |
| Release role | First stable release |
| Documentation root | docs/releases/v1.0.0 |

## Scope

This release package covers:

- Controller firmware behavior and safety model
- Sensor-node telemetry and RS-485 communication
- Dashboard control and operator UX expectations
- Firebase RTDB contract and authorization boundaries
- Deployment and commissioning workflow

## Required Acceptance Checks

- Dashboard builds and validates in target environment.
- Firmware builds with pinned dependencies.
- RS-485 link commissioning completed on target installation.
- Emergency stop, dry-run lockout, and overflow lockout verified.
- Admin bootstrap and control-write permissions verified.

## Sign-off Matrix

| Area | Owner | Status |
|---|---|---|
| Firmware safety | _TBD_ | Pending |
| Dashboard operations UX | _TBD_ | Pending |
| RTDB rules and auth | _TBD_ | Pending |
| Deployment readiness | _TBD_ | Pending |

## Notes

- This manifest is a release-governance artifact.
- Canonical evolving specs remain under docs/specs/.
