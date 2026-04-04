# Health Check 503 — Immediate Remediation

If `GET /api/health` returns **503**, the dashboard is reporting that Firebase could not be initialized (missing or placeholder env vars). Halt further rollout and fix as below.

---

## Immediate steps

1. **Do not promote the deployment** to more users or regions until health returns 200.

2. **Confirm environment variables** in the deployment platform (e.g. Vercel → Project → Settings → Environment Variables):
   - `NEXT_PUBLIC_FIREBASE_API_KEY`
   - `NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN`
   - `NEXT_PUBLIC_FIREBASE_DATABASE_URL`
   - `NEXT_PUBLIC_FIREBASE_PROJECT_ID`
   - `NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET`
   - `NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID`
   - `NEXT_PUBLIC_FIREBASE_APP_ID`

   All must be set and **must not** be the placeholders `YOUR_API_KEY` or `YOUR_PROJECT_ID` (or the health endpoint will return 503).

3. **Redeploy** after fixing env vars (e.g. trigger a new Vercel deployment or re-run the deploy job).

4. **Re-run the health check:**
   ```bash
   ./scripts/post-deploy-health-check.sh https://your-dashboard.vercel.app
   ```
   Expect HTTP 200 and `"firebase": "initialized"`.

5. **If 503 persists:** Check build logs to ensure the correct env are available at **build time** (Next.js inlines `NEXT_PUBLIC_*` at build). If the deployment was built without env, rebuild after setting variables and redeploy.

---

## Rollback

If you cannot resolve env quickly, roll back the dashboard per `docs/operations/ROLLBACK_RUNBOOK.md` (Rollback A: Dashboard Only) to the last deployment that was healthy.
