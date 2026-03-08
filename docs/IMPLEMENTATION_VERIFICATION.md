# Implementation Verification — ENHANCEMENT_PLAN.md v5

**Date:** March 2026  
**Scope:** Pre-Phase + Phases 1–6 + Dashboard Inconsistencies

---

## Pre-Phase: Critical Bug Fixes

| # | Issue | Expected | Implemented | Status |
|---|-------|----------|-------------|--------|
| 1 | Flow calibration factor | 1.0 → 7.5, tunable via Firebase | `FLOW_CALIBRATION_FACTOR = 7.5f`, `flow_calibration_factor` in Firebase/NVS | ✅ |
| 2 | Ultrasonic timeout handling | Phase 1 | Phase 1 sensor failure detection | ✅ |
| 3 | pumpMode NVS persistence | Phase 2 | Phase 2 NVS state persistence | ✅ |

---

## Phase 1 — Safety & Sensor Reliability

### Firmware

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| 1A. Consecutive ultrasonic failure | 5 timeouts → `isSensorError` | `checkSensorFailure()`, configurable via `cfgSensorFailureThreshold` (Phase 4) | ✅ |
| 1A. Safe fallback AUTO | Pump OFF on sensor error | `executePumpLogic()` — sensor error → pump OFF | ✅ |
| 1A. FORCE_ON works | Yes | FORCE_ON checked before sensor error | ✅ |
| 1A. Auto-recovery | Valid reading clears error | `checkSensorFailure()` clears on valid read | ✅ |
| 1A. Flow pump OFF + >2 LPM 5s | `isFlowSensorError` | `checkFlowSensorStuck()` | ✅ |
| 1A. Flow >100 LPM discard | Discard | `calculateFlowRate()` returns previous | ✅ |
| 1A. Rate-of-change >30% | Hold previous | `readUltrasonicSensor()` rate guard | ✅ |
| 1B. Max runtime cutoff | 120 min default, 30–480 | `checkOverflowProtection()`, `cfgMaxPumpRuntimeMin` | ✅ |
| 1B. FORCE_ON exempt | Yes | Only applies when `pumpMode == "AUTO"` | ✅ |
| 1B. clear_error reset | Yes | `readFirebaseControl()` clears overflow | ✅ |
| 1B. Serial message | `[ERROR] Max runtime exceeded...` | Present | ✅ |
| 1B. Push `is_overflow_error` | Yes | `pushFirebaseStatus()` | ✅ |
| 1C. 5-sample median | Yes | `readUltrasonicSensor()` | ✅ |
| 1C. EMA α=0.3 | Yes | `ULTRASONIC_EMA_ALPHA` | ✅ |
| 1C. Float percentage | `100.0f * (emptyCm - dist) / (emptyCm - fullCm)` | Yes | ✅ |
| 1C. Distance 2–200cm | Discard invalid | `readSingleUltrasonic()` | ✅ |
| 1C. Timeout 30→50ms | Yes | `ULTRASONIC_TIMEOUT_MS 50` | ✅ |
| 1D. Flow default 7.5 | Yes | `FLOW_CALIBRATION_FACTOR 7.5f` | ✅ |
| 1D. Comments | Q = F/7.5 | Yes | ✅ |

### Dashboard

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| types.ts PumpStatus | `is_sensor_error`, `is_overflow_error` | Yes | ✅ |
| StatusBar | Warning badges | SENSOR (amber), OVERFLOW (red) | ✅ |
| DeviceConfigSettings | max_pump_runtime_min 30–480 | Yes | ✅ |
| types.ts DeviceConfig | `max_pump_runtime_min` | Yes | ✅ |

---

## Phase 2 — System Resilience & State Persistence

### Firmware

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| 2A. esp_task_wdt | 15s timeout, panic-reboot | `esp_task_wdt_init(15, true)` | ✅ |
| 2A. Add in setup, reset in loop | Yes | `esp_task_wdt_add(NULL)`, `esp_task_wdt_reset()` | ✅ |
| 2B. Crash loop | 5 boots in 5 min → safe mode | `checkCrashLoop()` | ✅ |
| 2B. Reset if last_boot > 5 min | Yes | `lastBootTime > CRASH_LOOP_WINDOW_SEC*1000` | ✅ |
| 2B. Safe mode 1h timeout | Yes | `SAFE_MODE_TIMEOUT_MS` | ✅ |
| 2B. Power cycle clear | Yes | `now < 5000` clears | ✅ |
| 2C. Persist mode/error on change | Yes | `persistStateToNVS()` | ✅ |
| 2C. Level ≥5% or 5 min | Yes | `NVS_LEVEL_DELTA_THRESHOLD`, `NVS_LEVEL_INTERVAL_MS` | ✅ |
| 2C. Boot log last state | Yes | `loadStateFromNVS()` | ✅ |
| 2C. Do NOT auto-start | Yes | Pump OFF on boot | ✅ |
| 2D. esp_reset_reason | Human-readable | `getBootReasonString()` | ✅ |
| 2D. Push last_boot_reason | Yes | `pushFirebaseStatus()` | ✅ |
| 2E. 5s startup delay | Yes | `delay(5000)` after GPIO | ✅ |
| 2F. Exponential backoff | 5s→60s, ±2s jitter | `wifiBackoffMs` | ✅ |
| 2F. WiFi.disconnect(true) | Yes | Yes | ✅ |
| 2F. WiFi.setAutoReconnect(true) | Yes | In `connectWiFi()` | ✅ |
| 2F. firebaseNeedsReinit | Yes | On reconnect | ✅ |
| 2G. wifi_rssi | Yes | Pushed to Firebase | ✅ |
| 2G. lastSuccessfulFirebaseMs | >5 min warning | Yes | ✅ |
| 2G. RSSI log 60s | Yes | `lastRssiLogMs` | ✅ |

### Dashboard

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| types.ts PumpStatus | `wifi_rssi`, `last_boot_reason` | Yes | ✅ |
| StatusBar | RSSI indicator, boot reason tooltip | Yes | ✅ |
| overflowAlert | NotificationSettings + Cloud Function | Dashboard ✅, **Cloud Function ❌** | ⚠️ See Gap |

---

## Phase 3 — Scheduled Sleep & ESP32 Protection

### Firmware

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| 3A. configTime PHT | 8*3600, pool.ntp.org | Yes | ✅ |
| 3A. getLocalTime | Current hour | Yes | ✅ |
| 3A. WiFi fail → sleep disabled | Yes | `ntpSynced` only when time valid | ✅ |
| 3B. sleep_enabled, start, end, emergency | Yes | Firebase config, NVS | ✅ |
| 3B. AUTO suppressed | Yes | `executePumpLogic()` when `isSleeping` | ✅ |
| 3B. Light Sleep | esp_light_sleep_start() | Yes | ✅ |
| 3B. Wake every 30s | SLEEP_WAKE_INTERVAL_MS | Yes | ✅ |
| 3B. FORCE_ON works | Yes | Checked before sleep branch | ✅ |
| 3B. Emergency override | Level ≤ emergency → bypass | Yes | ✅ |
| 3B. Condition-based idle | Pump OFF, level ≥90%, 5 min | Yes | ✅ |
| 3B. Idle intervals | 10s sensor, 30s Firebase | cfgIdleSensorIntervalMs, cfgIdleFirebaseIntervalMs | ✅ |
| 3B. Wake log | `[SLEEP] Waking up` | Yes | ✅ |
| 3C. WDT reset after wake | Yes | `esp_task_wdt_reset()` | ✅ |

### Dashboard

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| types.ts DeviceConfig | sleep_enabled, start, end, emergency | Yes | ✅ |
| DeviceConfigSettings | Sleep Schedule section | Yes | ✅ |
| StatusBar | 😴 Sleep indicator | Yes | ✅ |
| PumpStatus is_sleeping | Yes | Yes | ✅ |

**Note:** Plan Phase 3 says "Emergency override level (0–20%)" in Dashboard; Config Summary says 0–100. Implementation uses 0–100 (matches Config Summary). ✅

---

## Phase 4 — Firebase Config Expansion

### Firmware

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| 4A. sensor_failure_threshold | 3–20, default 5 | Yes | ✅ |
| 4A. idle_sensor_interval_ms | 5000–60000, default 30000* | 10000 (matches Phase 3 "10s") | ✅ |
| 4A. idle_firebase_interval_ms | 10000–120000, default 30000 | Yes | ✅ |
| 4A. Validation per-field | Keep current if invalid | Yes | ✅ |
| 4B. NVS keys | sens_thresh, idle_sens_ms, idle_fb_ms | Yes | ✅ |

*Phase 4 table says idle_sensor default 30000, but Phase 3 says "10s sensor, 30s Firebase". Implementation uses 10000/30000 — consistent with Phase 3. ✅

### Dashboard

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| Grouped form | Tank, Pump, Safety, Sleep, Advanced | Yes | ✅ |
| sensor_failure_threshold | 3–20 | Yes | ✅ |
| idle_sensor_interval_ms | 5000–60000 | Yes | ✅ |
| idle_firebase_interval_ms | 10000–120000 | Yes | ✅ |
| useDeviceConfig | All fields | Yes | ✅ |

---

## Configurability Summary Check

| Parameter | Firebase Key | Default (Plan) | Default (Impl) | Status |
|-----------|-------------|----------------|---------------|--------|
| idle_sensor_interval_ms | idle_sensor_interval_ms | 30000 | 10000 | ⚠️ Plan says 30k, Phase 3 says 10s. Impl matches Phase 3. |

All other parameters match.

---

## Hardcoded Safety Check

| Parameter | Expected | Implemented | Status |
|-----------|----------|-------------|--------|
| GPIO | 4, 5, 18, 34 | Yes | ✅ |
| WDT | 15s | Yes | ✅ |
| Relay | LOW = ON | Yes | ✅ |
| Boot | Pump OFF | Yes | ✅ |
| Credentials | secrets.h | Yes | ✅ |

---

## Phase 5 — Uptime Counter & Dashboard Bug Fixes

### Firmware

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| Accurate uptime | esp_timer_get_time(), avoid millis rollover | `uptimeMinutes = esp_timer_get_time() / 60000000ULL` | ✅ |
| Push uptime_minutes | Firebase status payload | `statusJson.set("uptime_minutes", uptimeMinutes)` | ✅ |

### Dashboard

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| StatCard dynamic labels | pump_start_level, pump_stop_level from config | `config?.pump_start_level ?? 30`, `config?.pump_stop_level ?? 100` | ✅ |
| Dry-Run threshold dynamic | config.dry_run_threshold_lpm | `config?.dry_run_threshold_lpm ?? 0.5` in flow StatCard | ✅ |
| Uptime indicator | StatusBar "up Xm" / "up Xh Ym" | `uptimeMinutes` formatted in StatusBar.tsx | ✅ |
| Optimistic config update | StatCard updates immediately after save | `setConfig(merged)` in useDeviceConfig.saveConfig | ✅ |
| ModeControls dry-run timeout | Dynamic dryRunTimeoutSec | `dryRunTimeoutSec={config?.dry_run_timeout_sec ?? 30}` | ✅ |
| Sleep description poll | Dynamic from idle_sensor_interval_ms | `{Math.round((form.idle_sensor_interval_ms ?? 10000) / 1000)}s poll` | ✅ |

### Dashboard Inconsistencies (from ENHANCEMENT_PLAN.md Phase 5 table)

| # | Issue | Fix Applied | Status |
|---|-------|-------------|--------|
| 1 | StatCard threshold label not updating | Optimistic update in saveConfig | ✅ |
| 2 | ModeControls hardcoded "30s" | dryRunTimeoutSec prop | ✅ |
| 3 | DeviceConfigSettings "30s poll" | Dynamic from form.idle_sensor_interval_ms | ✅ |
| 4 | ESP32 config "every 30s" | Left as-is (documentation) | ✅ |

---

## Phase 6 — Dashboard UX, Push Notifications & PWA

### 6A. Firebase Data Optimization

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| Optimization doc | Assessment of RTDB usage | `docs/FIREBASE_OPTIMIZATION.md` | ✅ |
| Long-term suitability | Documented | Current architecture assessed as suitable | ✅ |

### 6B. Dashboard Settings UX

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| InfoTooltip component | Hover/tap popover | `dashboard/components/InfoTooltip.tsx` | ✅ |
| DeviceConfigSettings tooltips | All sections + key fields | Tank Calibration, Pump Thresholds, Safety, Sleep, Advanced | ✅ |
| NotificationSettings tooltips | Delivery methods, alert types | Yes | ✅ |

### 6C. Push Notifications (FCM)

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| FCM token request | "Enable push on this device" | `lib/fcm.ts`, NotificationSettings.tsx | ✅ |
| Token storage | `fcmTokens` in RTDB | `notifications_by_user/{uid}/fcmTokens/{deviceId}` | ✅ |
| Cloud Functions push | Send to FCM tokens | `sendPush()` in `functions/src/index.ts` | ✅ |
| Service worker | Dynamic SW with Firebase config | `app/api/firebase-messaging-sw/route.ts` + rewrite | ✅ |
| VAPID env | NEXT_PUBLIC_FIREBASE_VAPID_KEY | .env.local.example, DEPLOY_GUIDE | ✅ |

### 6D. Progressive Web App (PWA)

| Item | Expected | Implemented | Status |
|------|----------|-------------|--------|
| Manifest | name, icons, theme | `app/manifest.ts` | ✅ |
| Service worker | next-pwa | @ducanh2912/next-pwa in next.config.js | ✅ |
| Install prompt | beforeinstallprompt banner | `components/InstallPrompt.tsx` | ✅ |
| Icons | 72, 192, 512px | `dashboard/public/icons/` | ✅ |

---

## Gaps / Corrections Needed

### 1. Cloud Function — overflowAlert ✅ (Fixed)

**Plan:** "Add overflow alert type to NotificationSettings.tsx **and Cloud Function**"

**Fix applied:** Added overflow alert handling in `functions/src/index.ts`: when `after.is_overflow_error` is true and `config.overflowAlert ?? true`, send email with subject "Overflow Protection Triggered". Uses `canSend(uid, "overflow")` and `recordSent(uid, "overflow")` for throttling.

---

## Summary

| Phase | Status | Notes |
|-------|--------|------|
| Pre-Phase | ✅ | Flow factor fixed |
| Phase 1 | ✅ | All items implemented |
| Phase 2 | ✅ | Cloud Function overflow alert added |
| Phase 3 | ✅ | All items implemented |
| Phase 4 | ✅ | All items implemented |
| Phase 5 | ✅ | Uptime, dynamic StatCards, dashboard bug fixes |
| Phase 6 | ✅ | UX tooltips, FCM push, PWA installable app |

---

## Build & Lint Status

| Component | Status | Notes |
|-----------|--------|------|
| Dashboard TypeScript | ✅ `tsc --noEmit` passes | No type errors |
| Dashboard Linter | ✅ No errors | ReadLints clean |
| Dashboard Next.js build | ✅ | Build succeeds (Phase 6 additions verified) |
| Cloud Functions | ✅ `npm run build` passes | Push notifications wired |
| Firmware (PlatformIO) | — | `pio run` not in PATH; code verified via grep |

---

## Document References

| Topic | File |
|-------|------|
| Firebase optimization assessment | `docs/FIREBASE_OPTIMIZATION.md` |
| Push notifications setup | `docs/NOTIFICATIONS_SETUP.md` (see Push Notifications section) |
