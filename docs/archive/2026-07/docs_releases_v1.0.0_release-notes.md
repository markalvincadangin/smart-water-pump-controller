# Release Notes - SmartFlow v1.0.0

## Release Classification

- Stability: Stable
- Release type: First stable release
- Scope: Firmware, dashboard, RTDB contract, deployment runbook, and safety UX

## Summary

SmartFlow v1.0.0 is the first stable release of the distributed pump-control system:

- ESP32 master controller for pump actuation, safety enforcement, and cloud sync.
- ESP8266 (NodeMCU V2) sensor node for ultrasonic/flow telemetry over RS-485.
- Next.js dashboard (v15) for operator visibility and admin-gated control intents.
- Firebase RTDB contract and rules aligned to additive, safety-first operation.

## Included Capabilities

### Firmware (ESP32 master)

- Policy modes: `AUTO`, `MANUAL`, `COUNTDOWN`.
- Emergency stop latch via `emergency_stop` and `reset_stop` one-shots.
- Dry-run and overflow lockout handling with fail-safe pump-off behavior.
- RS-485 frame validation (STX/ETX + CRC16 + sequence-aware handling).
- NVS-backed persistence for config and critical state.

### Firmware (ESP8266 sensor node)

- Non-blocking ultrasonic acquisition and filtering.
- ISR-based flow pulse counting and L/min conversion.
- Framed RS-485 response payload with `DIST`, `FLOW`, `ERR`, `SEQ`, and `CRC`.

### Dashboard

- Typed control/status model aligned to firmware behavior.
- Admin-gated writes using `pump_system/config/admins/{uid}`.
- Safety-state UX for e-stop, lockouts, stale level, and unstable sensor link.

### RTDB and Rules

- No hardcoded UIDs in rules.
- Additive contract model under `/pump_system/*`.
- Clear write ownership boundaries for controller, admins, and user-scoped preferences.

## Compatibility Notes

- Legacy policy values `FORCE_ON` and `FORCE_OFF` are not valid operating modes for this release.
- Legacy control fields may remain present for compatibility, but standard operation is based on `mode`, `manual_desired`, and countdown/e-stop one-shots.

## Validation Status

- Build and behavior require hardware-in-loop validation for final commissioning.
- Dashboard quality checks should be executed with `npm run validate` in `dashboard/`.
- Firmware reproducibility depends on pinned PlatformIO dependencies.

## Known Limitations

- RS-485 noise tolerance and power transient resilience require on-site verification.
- Local Lighthouse checks can vary by host environment; CI/Linux is preferred for repeatable performance gates.

## Upgrade Guidance

- This release is intended as the baseline stable version.
- Future releases should preserve backward-compatible RTDB and RS-485 extensions.

