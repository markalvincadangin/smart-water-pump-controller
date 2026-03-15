# PRR Critical Blockers — Remediation Summary

This document summarizes the remediation work performed to move the Smart Water Pump Controller to **Go** status per the [Production Readiness Review](../PRODUCTION_READINESS_REVIEW.md).

---

## 1. Testing Framework & Coverage (Gate 1)

### Dashboard
- **Config:** `dashboard/jest.config.js`, `dashboard/jest.setup.js`
- **Scripts:** `npm run test`, `npm run test:watch`, `npm run test:coverage`; `validate` now includes `npm run test`
- **Gold-standard tests:**
  - `dashboard/__tests__/lib/controlContract.test.ts` — `VALID_CONTROL_MODES`, `isValidControlMode`, `sanitizeControlMode` (pump_system/control/mode data contract: AUTO, FORCE_ON, FORCE_OFF, COUNTDOWN)
  - `dashboard/__tests__/lib/usePendingControl.test.tsx` — pending mode clearing and toast on confirmation
  - `dashboard/__tests__/lib/usePumpData.test.tsx` — Firebase mocks; setMode, startCountdown duration clamping
- **Shared contract:** `dashboard/lib/controlContract.ts` — `VALID_CONTROL_MODES`, `isValidControlMode()`, `sanitizeControlMode()` for v3.0 mode validation

### Functions
- **Config:** `functions/jest.config.js`
- **Scripts:** `npm run test`, `npm run test:coverage`
- **Extracted module:** `functions/src/notifications.ts` — `canSend(db, uid, type)`, `recordSent(db, uid, type)`, `THROTTLE_SEC` (injectable `db` for tests)
- **Gold-standard tests:** `functions/src/__tests__/notifications.test.ts` — throttle logic (canSend true/false by time, recordSent update, THROTTLE_SEC value)
- **Build:** `tsconfig.json` excludes `src/__tests__` so test code is not emitted to `lib/`

---

## 2. Security Hardening (Gate 2)

### Database Rules
- **File:** `database.rules.json` (repo root)
- **Change:** All admin write rules now use **only** `root.child('pump_system/config/admins').child(auth.uid).val() === true`. Hardcoded Firebase UIDs removed.
- **Bootstrap:** To grant admin, set `pump_system/config/admins/{uid}` to `true` in Firebase Console or via a one-time script. No UIDs in rules.

### Dependency Patching Plan
- **File:** `docs/operations/DEPENDENCY_PATCHING_PLAN.md`
- **Content:** Step-by-step plan to resolve 7 high-severity dashboard npm audit findings (Next.js, flatted, serialize-javascript/PWA, LHCI) without breaking the build. Overrides and upgrade order documented.

---

## 3. Deployment & Disaster Recovery (Gate 4)

### Rollback Runbook
- **File:** `docs/operations/ROLLBACK_RUNBOOK.md`
- **Content:** Point of no return; rollback procedures for Dashboard only, Database rules only, Full stack (dashboard + rules), and Functions only. Prerequisites and post-rollback steps included.

### Health API
- **File:** `dashboard/app/api/health/route.ts`
- **Behavior:** `GET /api/health` returns **200** only when Firebase is effectively initializable with valid env (non-placeholder `NEXT_PUBLIC_FIREBASE_PROJECT_ID` and `NEXT_PUBLIC_FIREBASE_API_KEY`). Returns **503** with `"firebase": "not_initialized"` otherwise. Use for liveness/readiness and post-deploy checks.

---

## 4. Build Integrity (Gate 3)

### Firmware version pinning
- **File:** `firmware/platformio_smart_water_pump_controller/platformio.ini`
- **Change:** `lib_deps` pinned to:
  - `mobizt/Firebase Arduino Client Library for ESP8266 and ESP32@4.4.17`
  - `bblanchon/ArduinoJson@6.21.6`
- **Verification:** `pio run` succeeds with pinned versions.

---

## Verification Commands

```bash
# Dashboard: lint, test, build
cd dashboard && npm run lint && npm run test && npm run build

# Functions: test, build
cd functions && npm run test && npm run build

# Firmware
cd firmware/platformio_smart_water_pump_controller && pio run

# Health (after dashboard is running with valid .env.local)
curl -s http://localhost:3000/api/health
```

---

## Post-Remediation PRR Status

| Blocker | Status |
|--------|--------|
| Unit/integration tests & coverage | **Resolved** — Jest in dashboard and functions; gold-standard tests for control contract, usePendingControl, usePumpData, canSend/recordSent. |
| Hardcoded UIDs in database rules | **Resolved** — Rules use only `pump_system/config/admins/{uid}`. |
| Rollback procedure | **Resolved** — Rollback runbook in `docs/operations/ROLLBACK_RUNBOOK.md`. |
| Health checks | **Resolved** — `/api/health` implemented. |
| Build reproducibility (firmware) | **Resolved** — platformio.ini pins library versions. |
| Dashboard npm high-severity vulns | **Documented** — `DEPENDENCY_PATCHING_PLAN.md`; execute per plan for full mitigation. |

---

## Operational Launch

- **Launch runbook:** [LAUNCH_SEQUENCE_RUNBOOK.md](LAUNCH_SEQUENCE_RUNBOOK.md) — Execute Phase 1 (bootstrap + rule confirmation) before Phase 2 (pipeline). If health returns 503, halt and follow [HEALTH_CHECK_503_REMEDIATION.md](HEALTH_CHECK_503_REMEDIATION.md).
- **First admin:** Run `scripts/bootstrap-admin.js` with your Google UID and service account key; then confirm a control write from the dashboard.

---

## Production Launch Log (Historical Record)

**System version:** v3.0 (certified zero-failure launch).  
**Fill in after completing the [Launch Sequence Runbook](LAUNCH_SEQUENCE_RUNBOOK.md).** Do not proceed to pipeline/telemetry until Phase 1 (admin bootstrap + rule confirmation) is verified.

| Field | Value |
|-------|--------|
| **System version** | v3.0 |
| **Final production deployment (UTC)** | _e.g. 2025-03-15 14:30:00 UTC — when deploy workflow completed and health check passed_ |
| **Verified admin UID** | _UID bootstrapped and used to confirm control write in Phase 1.2_ |
| **Health check** | _200 OK at _______________ (time)_ |
| **Function verification** | _Low level / Dry run notification verified at _______________ (time)_ |
| **Status stream (first 60 min)** | _Verified at _______________ (time)_ |
| **min_free_heap_observed_bytes (T+60 min)** | _Value from dashboard/status_ |
| **Notes** | _Any incidents, rollbacks, or follow-ups_ |
