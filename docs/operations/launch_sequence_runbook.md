# Production Launch Sequence Runbook
## SRE / On-Call Incident Commander — Final Launch

**Use this runbook to execute the production bootstrap, pipeline, health audit, and first-60-min telemetry.**  
**Do not proceed to Phase 2 until Phase 1 (Admin Access) is verified. If health returns 503, halt and follow [HEALTH_CHECK_503_REMEDIATION.md](HEALTH_CHECK_503_REMEDIATION.md).**

---

## Phase 1 — Production Bootstrap Sequence

### 1.1 First Admin Provisioning

**Prerequisites:** Firebase project with Realtime Database and Authentication (Google) enabled. Database rules already deployed (`firebase deploy --only database`). Your primary Google account exists in Authentication → Users.

1. **Get your primary Google UID**
   - Firebase Console → **Authentication** → **Users**
   - Copy the **User UID** of the account you use for the dashboard (e.g. `ZScmJXQg2tZSVDiZ1YxTUxN36xw1`).

2. **Obtain a service account key** (if not already)
   - Firebase Console → **Project settings** (gear) → **Service accounts** → **Generate new private key**.
   - Save the JSON file securely (e.g. `serviceAccountKey.json`). Do not commit it.

3. **Run the bootstrap script**
   ```bash
   cd scripts
   npm install
   node bootstrap-admin.js --keyfile /path/to/serviceAccountKey.json YOUR_PRIMARY_GOOGLE_UID
   ```
   Replace `/path/to/serviceAccountKey.json` and `YOUR_PRIMARY_GOOGLE_UID` with your values.

4. **Expected output**
   ```
   Admin bootstrap OK: pump_system/config/admins/YOUR_UID = true
   ```

5. **Optional:** In Firebase Console → Realtime Database → Data, confirm `pump_system/config/admins/YOUR_UID` is `true`.

---

### 1.2 Rule Confirmation (Manual Write)

**Gate:** Do not proceed to Phase 2 until this step succeeds.

1. Open the **production dashboard** in a browser (or `http://localhost:3000` if testing locally with correct env).
2. Sign in with the **same Google account** whose UID you bootstrapped.
3. Perform a **manual write** to `pump_system/control/mode`:
   - Change mode (e.g. switch to **FORCE_OFF** or **AUTO** or start a **COUNTDOWN**).
4. **Expected:** The write succeeds; the dashboard shows the new mode and (if applicable) a success toast. No permission-denied error in the browser console.
5. **If the write fails (permission denied):**
   - Re-check that `pump_system/config/admins/{your-uid}` is `true` in the database.
   - Re-check that you are signed in with the same Google account whose UID was bootstrapped.
   - Do not proceed to Phase 2 until control writes work.

**Record for handover:** Your verified admin UID: `________________` (fill in and add to PRR_REMEDIATION_SUMMARY.md Launch Log after Phase 4).

---

## Phase 2 — Pipeline Execution & Health Audit

### 2.1 Deployment Trigger

1. Ensure the current branch is the **stable** branch you intend to deploy (e.g. `main`).
2. **Push to trigger the workflow:**
   ```bash
   git push origin main
   ```
   (Or merge a PR into `main` and push.)
3. Open **GitHub** → **Actions** → **Deploy** workflow run for this push.

**Prerequisites for a green run:** Repository secrets set: `FIREBASE_TOKEN`, `VERCEL_TOKEN`, `VERCEL_ORG_ID`, `VERCEL_PROJECT_ID`. Variable `DASHBOARD_URL` set to your production dashboard URL (e.g. `https://your-app.vercel.app`).

---

### 2.2 Live Health Verification

1. **In the same workflow run**, go to the **Post-deploy health check** job.
2. **Expected:** The job runs a request to `$DASHBOARD_URL/api/health` and succeeds (HTTP 200). Logs show e.g. `Health check OK: HTTP 200`.
3. **If the job fails with 503:**
   - **Halt.** Do not proceed to Phase 3.
   - Follow **[HEALTH_CHECK_503_REMEDIATION.md](HEALTH_CHECK_503_REMEDIATION.md)** (env vars, redeploy, re-run health).
   - Optionally run locally: `./scripts/post-deploy-health-check.sh https://your-dashboard.vercel.app`
4. **Optional local check** (from repo root):
   ```bash
   ./scripts/post-deploy-health-check.sh https://YOUR_PRODUCTION_DASHBOARD_URL
   ```
   Expect: `OK: Health check HTTP 200`.

---

### 2.3 Cloud Function Verification (Notifications)

**Goal:** Confirm that `functions/src/notifications.ts` logic sends a notification when a trigger condition occurs.

1. **Option A — Low level**
   - In Firebase Console → Realtime Database, temporarily set `pump_system/status/water_level_percent` to a value **≤** your configured low-level threshold (e.g. 20).
   - Ensure at least one user has notifications enabled with an email and **Low level** alert on (dashboard → bell icon).
   - Wait for the function to run (status write triggers `onStatusChange`). Check **Firebase Console → Functions → Logs** for the function run.
   - **Expected:** An email (or push) is sent for the low-level alert (subject to 15-min throttle per user/type). Inbox or Functions logs confirm delivery or Resend success.

2. **Option B — Dry run**
   - If you can safely trigger a dry-run condition on the ESP32 (or by writing `pump_system/status/is_error: true` in the database for testing), the function should send a dry-run alert to users who have it enabled.
   - Check Functions logs and inbox as above.

**Record:** Function verification completed at: `________________` (timestamp).

---

## Phase 3 — Post-Launch Telemetry (First 60 Minutes)

**Gate:** Proceed only after Phase 1 (admin write confirmed) and Phase 2 (health 200, pipeline green).

### 3.1 Status Stream (ESP32 → Firebase)

1. **Where to look:** Dashboard **live** view, or Firebase Console → Realtime Database → `pump_system/status`.
2. **Expected (per README / v3 spec):** The ESP32 writes to `pump_system/status` approximately **every 3 seconds** (sensor interval 1s, Firebase interval 3s). Fields such as `water_level_percent`, `is_running`, `flow_rate_lpm`, `uptime_minutes` update regularly.
3. **Check:** Over 1–2 minutes, confirm multiple updates (e.g. timestamps or changing values). If no updates, verify ESP32 is powered, WiFi connected, and Firebase credentials correct (Serial Monitor or dashboard status).

**Record:** Status stream verified at: `________________`.

---

### 3.2 Memory / Heap Monitor

1. **Where to look:** Dashboard shows telemetry fields from `pump_system/status`; or Firebase Console → `pump_system/status` → `min_free_heap_observed_bytes` (and related heap fields if present).
2. **Expected:** With pinned firmware (Firebase Arduino Client 4.4.17, ArduinoJson 6.21.6), no sustained drop in free heap over the first 60 minutes. `min_free_heap_observed_bytes` may decrease slightly then stabilize; it should not trend to zero or very low values (e.g. &lt; 50k) under normal operation.
3. **If you see a clear downward trend or very low heap:** Note the value and time; consider a firmware review or rollback of the last firmware change. Document in the Launch Log.

**Record:** Heap check at T+60 min: `min_free_heap_observed_bytes` = `________________`.

---

## Phase 4 — Final Handover

1. **Update the Launch Log** in `docs/operations/PRR_REMEDIATION_SUMMARY.md`:
   - **Production launch (UTC):** date and time of successful completion of Phase 2 (health 200, pipeline green).
   - **Verified admin UID:** the UID you bootstrapped and used to confirm control write in Phase 1.2.
   - Optionally: Function verification timestamp, status stream verification time, heap value at T+60.

2. **Communicate:** Notify stakeholders that production launch is complete and the first 60-minute telemetry gate has passed (or document any open follow-ups).

3. **Retain runbook:** Keep this runbook and [ROLLBACK_RUNBOOK.md](ROLLBACK_RUNBOOK.md) handy for the first 24–48 hours in case of incidents.

---

## Quick Reference

| Phase | Gate | On failure |
|-------|------|------------|
| 1.1 | Bootstrap script exits 0 | Fix key/UID and re-run; confirm path in DB. |
| 1.2 | Dashboard control write succeeds | Do not proceed; fix admin list or auth. |
| 2.2 | Health check 200 | Halt; follow HEALTH_CHECK_503_REMEDIATION.md. |
| 3 | Status stream + heap stable | Document; escalate if heap leak or no status. |
