# SmartFlow Safety Operations

Safety controls in SmartFlow are layered: electrical protection, firmware protection, and operator procedure.

## Non-negotiable rules

- Do not work on energized 220V wiring.
- Verify isolation before touching mains-side components.
- Keep earth/ground continuity intact before commissioning.
- Do not bypass TOR/contactor protections in operation.

## Hardware safety baseline

- MCB -> contactor -> TOR chain is installed and correctly rated.
- TOR current dial matches motor full-load current specification.
- Enclosure grounding and bonding are complete.
- Cable strain relief and ingress protection are intact.

## Firmware safety baseline

- Dry-run lockout enabled and tested.
- Overflow timeout protection enabled and tested.
- Emergency stop latching behavior verified.
- Sensor-failure handling biases pump to safe state.
- Startup state defaults to pump OFF.

## Pre-energization checklist

1. Complete wiring inspection and tug test.
2. Confirm relay polarity assumptions are correct for deployed hardware.
3. Confirm sensor and RS-485 cabling continuity.
4. Confirm dashboard control writes are restricted to admins.
5. Confirm emergency-stop and clear/recover workflow with dry test.

## Live-operation checklist

1. Verify tank has valid water source before run.
2. Observe first startup cycle on-site.
3. Confirm expected flow and level movement.
4. Validate no persistent fault flags in status.
5. Keep rollback procedure available during launch window.

## Safety references

- Root hardware and commissioning procedures in `README.md`
- Incident response: [rollback_runbook.md](rollback_runbook.md)
- Diagnostics: [troubleshooting.md](troubleshooting.md)
