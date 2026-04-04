# SmartFlow Production Launch Runbook

Use this runbook for controlled release to production.

## Release gates

Do not proceed to the next gate until the current gate passes.

1. Access control validated
2. Deployment completed
3. Health endpoint green
4. Notifications path validated
5. First-hour telemetry stable

## Gate 1: Access control validation

Run [admin_bootstrap_and_rule_simulation.md](admin_bootstrap_and_rule_simulation.md) completely.

Minimum pass criteria:

- At least one admin UID exists at `pump_system/config/admins/{uid} = true`
- Admin can write `pump_system/control/mode` with values `AUTO`, `MANUAL`, `COUNTDOWN`
- Non-admin cannot write privileged control keys

## Gate 2: Deploy dashboard and backend

1. Deploy target branch through your CI/CD pipeline.
2. Confirm release artifacts completed without failed jobs.
3. Confirm required secrets/env are present for the target environment.

## Gate 3: Health verification

1. Validate dashboard health endpoint:

```bash
./scripts/post-deploy-health-check.sh https://YOUR_DASHBOARD_URL
```

2. Expected result: HTTP `200` with `firebase: initialized`.
3. If HTTP `503`, stop and use [health_check_503_remediation.md](health_check_503_remediation.md).

## Gate 4: Notification path verification

Run [notifications_setup.md](notifications_setup.md) validation section.

Minimum pass criteria:

- Function trigger executes from `pump_system/status` update
- At least one configured alert is delivered (email and/or push)
- No fatal notification errors in Functions logs

## Gate 5: First-hour telemetry checks

Monitor for 60 minutes:

- `pump_system/status` updates continue at expected cadence
- `run_mode` and `mode` transitions are coherent with operator actions
- `emergency_stop_latched` and fault flags behave correctly under test conditions
- `min_free_heap_observed_bytes` remains stable (no sustained leak trend)
- `firebase_consecutive_failures` remains low and recovers after transient failures

## Launch sign-off record

Capture and store:

- Launch UTC timestamp
- Verified admin UID
- Health endpoint verification timestamp
- Notification verification timestamp
- Heap watermark at T+60
- Operator on-call handoff owner

## Incident fallback

If any gate fails after deployment, execute [rollback_runbook.md](rollback_runbook.md).
