# SmartFlow Rollback Runbook

Use this runbook when a release causes production regression.

## Rollback strategy selection

Choose the smallest rollback scope that restores service:

1. Dashboard only
2. Database rules only
3. Functions only
4. Full stack rollback

## Pre-checks

- Confirm incident started after latest deployment.
- Identify last known good commit/tag.
- Confirm access to Firebase project and dashboard host.
- Confirm who is incident commander and who approves rollback.

## A. Dashboard-only rollback

Use when UI/auth/client regressions occur and backend is healthy.

1. Redeploy previous known-good dashboard build in hosting platform.
2. Or restore dashboard code from known-good commit and redeploy.
3. Verify `/api/health` returns 200.
4. Verify sign-in and control-mode writes (`AUTO`, `MANUAL`, `COUNTDOWN`).

## B. Database-rules rollback

Use when permissions are broken or over-permissive.

1. Restore prior `database.rules.json` from known-good commit.
2. Deploy:

```bash
firebase deploy --only database
```

3. Verify admin control writes and non-admin denial behavior.
4. Verify ESP32 status writes continue.

## C. Functions-only rollback

Use when notification or server-side trigger logic regresses.

1. Restore `functions/` from known-good commit.
2. Build and deploy:

```bash
cd functions
npm ci
npm run build
cd ..
firebase deploy --only functions
```

3. Verify function invocation and logs.

## D. Full-stack rollback

Use when multiple layers are broken.

1. Checkout known-good commit/tag in rollback branch.
2. Deploy in this order:
   - `firebase deploy --only database`
   - `firebase deploy --only functions` (if needed)
   - dashboard deployment
3. Verify health, auth, control writes, and status stream.

## Post-rollback validation checklist

- Dashboard health endpoint is 200.
- Admin can control mode and critical commands.
- Non-admin restrictions work as expected.
- ESP32 status updates are live in RTDB.
- Notification pipeline works for at least one test event.

## Incident closure requirements

- Record rollback timestamp and target commit.
- Document user impact window.
- Document root cause and forward-fix plan.
- Open follow-up issue for preventive controls.
