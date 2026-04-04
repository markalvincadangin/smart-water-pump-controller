# PRR Critical Blockers — Remediation Summary

This document summarizes historical remediation work that moved SmartFlow toward production readiness.

---

## 1. Testing Framework & Coverage (Gate 1)

### Dashboard
- **Config:** `dashboard/jest.config.js`, `dashboard/jest.setup.js`
- **Scripts:** `npm run test`, `npm run test:watch`, `npm run test:coverage`; `validate` now includes `npm run test`
- **Gold-standard tests:**
  - `dashboard/__tests__/lib/controlContract.test.ts` — `VALID_CONTROL_MODES`, `isValidControlMode`, `sanitizeControlMode` (pump_system/control/mode data contract: AUTO, MANUAL, COUNTDOWN)
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
- **File:** `docs/operations/dependency_patching_plan.md`
- **Content:** Step-by-step plan to resolve 7 high-severity dashboard npm audit findings (Next.js, flatted, serialize-javascript/PWA, LHCI) without breaking the build. Overrides and upgrade order documented.

---

## 3. Deployment & Disaster Recovery (Gate 4)

### Rollback Runbook
- **File:** `docs/operations/rollback_runbook.md`
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
| Rollback procedure | **Resolved** — Rollback runbook in `docs/operations/rollback_runbook.md`. |
| Health checks | **Resolved** — `/api/health` implemented. |
| Build reproducibility (firmware) | **Resolved** — platformio.ini pins library versions. |
| Dashboard npm high-severity vulns | **Documented** — `dependency_patching_plan.md`; execute per plan for full mitigation. |

---

## Operational Launch

- **Launch runbook:** [launch_sequence_runbook.md](launch_sequence_runbook.md) — Execute Phase 1 (bootstrap + rule confirmation) before Phase 2 (pipeline). If health returns 503, halt and follow [health_check_503_remediation.md](health_check_503_remediation.md).
- **First admin:** Run `scripts/bootstrap-admin.js` with your Google UID and service account key; then confirm a control write from the dashboard.

---

## Production Launch Log (Historical Record)

**System version:** v1.0.0 (first stable release).  
**Fill in after completing the [Launch Sequence Runbook](launch_sequence_runbook.md).** Do not proceed to pipeline/telemetry until Phase 1 (admin bootstrap + rule confirmation) is verified.

| Field | Value |
|-------|--------|
| **System version** | v1.0.0 |
| **Final production deployment (UTC)** | _2026-03-15 — firmware flashed; dashboard control and status stream verified_ |
| **Verified admin UID** | _Fill in your Firebase Auth UID (dashboard user admin@example.com)_ |
| **Health check** | _Confirm 200 OK on production DASHBOARD_URL/api/health_ |
| **Function verification** | _Optional: low level / dry run notification_ |
| **Status stream (first 60 min)** | _Verified — ESP32 writing to pump_system/status every ~3s; Firebase sync OK_ |
| **min_free_heap_observed_bytes (T+60 min)** | _Check dashboard/status telemetry when available_ |
| **Notes** | _First boot: NVS pump_cfg not found (defaults used); state namespace mode NOT_FOUND until first persist. Ultrasonic 5 consecutive timeouts -> sensor error path observed; bypass enabled from dashboard. AUTO, MANUAL run/stop, COUNTDOWN, +5 min add, dry-run lockout, clear error, mode write-back confirmed — all exercised successfully._ |
