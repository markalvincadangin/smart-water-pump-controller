# SmartFlow Troubleshooting

Use this guide for common production issues.

## Quick triage order

1. Safety state: is emergency stop or hard fault active?
2. Cloud health: does `/api/health` return 200?
3. RTDB flow: is `pump_system/status` updating?
4. Control path: can admin write `pump_system/control/mode`?
5. Field link: is RS-485 stable between ESP32 and NodeMCU?

## Symptom -> action table

| Symptom | Likely cause | Immediate action |
|--------|---------------|------------------|
| Dashboard health returns 503 | Missing/placeholder Firebase env vars | Follow [health_check_503_remediation.md](health_check_503_remediation.md) |
| Dashboard shows stale status | ESP32 offline, Wi-Fi down, or Firebase auth issue | Check controller serial logs and Wi-Fi, then verify RTDB write activity |
| Mode changes fail with permission denied | UID not bootstrapped as admin | Run [admin_bootstrap_and_rule_simulation.md](admin_bootstrap_and_rule_simulation.md) |
| Pump does not start in AUTO | Level above start threshold, active fault, or cooldown | Check `run_mode`, `is_error`, `emergency_stop_latched`, cooldown seconds |
| Unexpected dry-run lockout | No/low flow, plumbing issue, or threshold too strict | Verify physical flow path, then review dry-run config |
| Sensor-related fault | Ultrasonic/flow sensor issue or RS-485 instability | Check sensor wiring, RS-485 link, and `remote_sensor_stable` |
| Notifications not delivered | Functions secret/env not set, user prefs disabled, or throttling | Re-run [notifications_setup.md](notifications_setup.md) checks |

## Useful paths

- `pump_system/status`
- `pump_system/control`
- `pump_system/config/device`
- `pump_system/config/admins`
- `pump_system/config/notifications_by_user/{uid}`

## Escalation guideline

Escalate immediately if:

- Pump behavior conflicts with expected safety logic.
- Repeated lockouts happen without physical root cause.
- Status stream is unavailable for more than 5 minutes during active operation.
- Rollback is required.

For rollback, use [rollback_runbook.md](rollback_runbook.md).
