# Emergency Rollback Procedure
## Smart Water Pump Controller — Dashboard & Firebase

**Purpose:** Step-by-step rollback when a deployment causes failures. Execute in order; do not skip the "Point of No Return" check.

---

## Point of No Return

- **Dashboard (Vercel/host):** Once the new deployment is "Live", traffic is already on the new build. Rollback = redeploy previous deployment or revert Git and redeploy.
- **Firebase Database Rules:** Rules take effect **immediately** on `firebase deploy --only database`. There is no gradual rollout. Rollback = redeploy previous rules from Git.
- **Firebase Functions:** New function code is live after `firebase deploy --only functions`. Rollback = redeploy previous version from Git.

**Dynamic admin rules (current setup):** Control writes are allowed only for UIDs listed at `pump_system/config/admins/{uid} = true`. There are no hardcoded UIDs in rules. After **any** rules deploy (including a rollback), ensure at least one admin exists: if you roll back to an older rules file that also used dynamic admins, the same requirement applies. If you roll back to rules that used hardcoded UIDs, no bootstrap is needed for those UIDs, but consider re-hardening (remove UIDs and bootstrap admins) as soon as possible. See `docs/operations/ADMIN_BOOTSTRAP_AND_RULE_SIMULATION.md`.

**Before any rollback:** Confirm the incident is caused by the last deployment (symptoms match changed areas). If unsure, prefer fixing forward unless safety is at risk.

---

## Prerequisites

- Git repository with history (no force-push that removed the last good commit).
- Firebase CLI: `firebase use <project-id>` and `firebase login`.
- Access to the dashboard hosting platform (e.g. Vercel dashboard or CLI).
- Last-known-good commit or tag (e.g. `v1.0.0` or `abc1234`).
- **With dynamic admin rules:** At least one UID must be set at `pump_system/config/admins/{uid} = true` after any rules deploy (including rollback) or no one can write to control. See `docs/operations/ADMIN_BOOTSTRAP_AND_RULE_SIMULATION.md`.

---

## Rollback A: Dashboard Only

Use when the **dashboard** is broken (UI, auth, or Firebase client errors) and rules/functions are unchanged.

1. **Identify last good version**
   - From Git: `git log --oneline -10` and note the commit hash before the bad deploy.
   - Or use the hosting platform’s list of deployments and pick the previous successful one.

2. **Revert and redeploy (Git-based)**
   - `git checkout <last-good-commit> -- dashboard/`
   - Commit: `git commit -m "Rollback: dashboard to <commit>"` and push, **or**
   - On Vercel: use the dashboard → Deployments → ⋮ on the previous deployment → "Promote to Production".

3. **Verify**
   - Open the dashboard URL; sign in and confirm status/control load.
   - Call `GET /api/health`; expect `200` and `"firebase": "initialized"`.

4. **If using Git revert:** Push the rollback commit and let CI redeploy, or trigger deploy manually.

---

## Rollback B: Firebase Database Rules Only

Use when **rules** are wrong (e.g. admins locked out, or overly permissive) and dashboard/functions code is fine.

1. **Revert rules in repo**
   - `git checkout <last-good-commit> -- database.rules.json`
   - Or manually restore the previous `database.rules.json` from Git history.

2. **Deploy rules**
   - From repo root: `firebase deploy --only database`
   - Confirm in Firebase Console → Realtime Database → Rules that the rules match the file.

3. **Verify**
   - Sign in to the dashboard; try a control write (e.g. mode change or manual stop) if you have admin.
   - Confirm ESP32 can still write status (password provider) if applicable.

4. **Commit the rollback** (if you changed the file): `git add database.rules.json && git commit -m "Rollback: database rules" && git push`

---

## Rollback C: Full Stack (Dashboard + Rules)

Use when both dashboard and rules were updated and the release is failing.

1. **Revert Git to last good tag/commit**
   - `git checkout <last-good-commit>`
   - Or create a rollback branch: `git checkout -b rollback-YYYYMMDD <last-good-commit>`

2. **Deploy database rules first**
   - `firebase deploy --only database`
   - Verify in Firebase Console.

3. **Deploy dashboard**
   - If using Vercel: push the rollback branch and promote, or trigger deploy from the rollback commit.
   - If manual: `cd dashboard && npm ci && npm run build && npm run start` (or your host’s deploy command).

4. **Verify**
   - Dashboard loads and `/api/health` returns 200.
   - Control writes and status reads work for admins and ESP32.

5. **Optional: deploy functions**  
   If you also reverted functions code: `firebase deploy --only functions`.

---

## Rollback D: Firebase Functions Only

Use when **Cloud Functions** (e.g. notifications) are failing and dashboard/rules are fine.

1. **Revert functions code**
   - `git checkout <last-good-commit> -- functions/`

2. **Rebuild and deploy**
   - `cd functions && npm ci && npm run build && cd ..`
   - `firebase deploy --only functions`

3. **Verify**
   - Trigger a condition that invokes the function (e.g. status change) and confirm logs in Firebase Console → Functions → Logs.

---

## Post-Rollback

- Create an incident note: what was rolled back, from which commit to which commit, and why.
- Schedule a post-mortem to fix the root cause and re-deploy with a safe change.
- If you had to restore **hardcoded UIDs** in rules temporarily, treat that as a security exception and remove them again as soon as admin list is fixed.

---

## Quick Reference

| Component   | Rollback action |
|------------|------------------|
| Dashboard  | Redeploy previous build (Vercel/host) or `git checkout <commit> -- dashboard/` and push. |
| DB Rules   | `git checkout <commit> -- database.rules.json` then `firebase deploy --only database`. |
| Functions  | `git checkout <commit> -- functions/` then `npm run build` and `firebase deploy --only functions`. |
