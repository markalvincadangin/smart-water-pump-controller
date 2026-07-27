# SmartFlow Operations Manual

This folder contains production operations runbooks for SmartFlow.

## Audience

- On-call engineers
- Operators and maintainers
- Release and incident coordinators

## Quick Start

1. Review [safety.md](safety.md) before any hardware or live-pump action.
2. Bootstrap first admin and validate control permissions using [admin_bootstrap_and_rule_simulation.md](admin_bootstrap_and_rule_simulation.md).
3. Execute launch steps in [launch_sequence_runbook.md](launch_sequence_runbook.md).
4. If health fails, use [health_check_503_remediation.md](health_check_503_remediation.md).
5. If release behavior regresses, execute [rollback_runbook.md](rollback_runbook.md).

## Document Map

- [admin_bootstrap_and_rule_simulation.md](admin_bootstrap_and_rule_simulation.md): First-admin bootstrap and rule validation.
- [launch_sequence_runbook.md](launch_sequence_runbook.md): Production launch sequence and first-hour checks.
- [health_check_503_remediation.md](health_check_503_remediation.md): Dashboard health endpoint failure response.
- [firmware_config_from_database.md](firmware_config_from_database.md): Implemented RTDB device-config contract and operational behavior.
- [rs485_tank_link.md](rs485_tank_link.md): RS-485 tank-node link protocol and wiring baseline.
- [notifications_setup.md](notifications_setup.md): Email and push notification deployment and validation.
- [rollback_runbook.md](rollback_runbook.md): Controlled rollback procedures.
- [troubleshooting.md](troubleshooting.md): Symptoms and corrective actions.
- [safety.md](safety.md): Electrical and operational safety controls.
- [dependency_patching_plan.md](dependency_patching_plan.md): Ongoing dependency and security maintenance workflow.

## Source-of-Truth Notes

- Firmware contracts: `firmware/master_node/src/**` and `firmware/sensor_node/src/**`
- Dashboard contracts: `dashboard/lib/types.ts` and `dashboard/app/api/health/route.ts`
- Access control rules: `database.rules.json`

When docs conflict with code, update docs to match code and capture the change in the next release notes.
