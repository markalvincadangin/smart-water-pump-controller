# Smart Water Pump Controller — Enhancement Plan v3

**Date:** March 2026
**Structure:** Phase-by-phase. Each phase is self-contained (firmware + dashboard) and can be implemented independently.

---

## Bugs & Critical Fixes (Pre-Phase)

These must be fixed regardless of which phases are implemented.

| # | Severity | Issue | Fix |
|---|----------|-------|-----|
| 1 | **CRITICAL** | `FLOW_CALIBRATION_FACTOR = 1.0` — should be ~7.5 per YF-G1 datasheet. Flow readings 7.5× inflated, weakening dry-run protection. | Change default to `7.5f`. Verify with bucket test. Tunable via Firebase `flow_calibration_factor`. **Note:** YF-G1 variants may differ (some cite 4.8); always verify with a physical bucket + stopwatch test. |
| 2 | **MEDIUM** | Ultrasonic timeout (`-1`) silently keeps last known level. No failure flag, no dashboard visibility. | Fixed in Phase 1. |
| 3 | **LOW** | `pumpMode` not persisted to NVS — reboot loses FORCE_OFF, defaults to AUTO. | Fixed in Phase 2. |

---

## Phase 1 — Safety & Sensor Reliability

> **Goal:** Prevent the system from operating on bad data. Detect sensor failures, add overflow protection, and harden sensor readings.

### Firmware Changes

#### 1A. Sensor Failure Detection

- **Consecutive ultrasonic failure counter** — after **5 consecutive** timeouts, set `isSensorError = true` (hardcoded to 5 in Phase 1; becomes configurable via `sensor_failure_threshold` in Phase 4)
- **Safe fallback in AUTO mode** — pump **OFF** on sensor error (prevents overflow on stale data)
- `FORCE_ON` still works (operator responsibility); dry-run protection still active
- Auto-recovery: valid reading resumes → clear error automatically
- **Flow sensor sanity checks:**
  - Pump OFF + flow > 2 LPM for 5s → `isFlowSensorError = true` (stuck/noisy sensor)
  - Flow > 100 LPM → discard reading (physically impossible for 1.5HP pump)
- **Rate-of-change guard:** level jump > 30% in 1 second → hold previous value (signal glitch)

#### 1B. Overflow Protection

**Current gap:** If sensor fails during fill, pump runs indefinitely → tank overflow → water damage.

- **Max continuous runtime cutoff** — if pump runs > `max_pump_runtime_min` (default: **120 min**) in AUTO mode without reaching stop level → **stop pump + flag error**
  - Configurable via Firebase `max_pump_runtime_min` (range: 30–480)
  - Does NOT apply to `FORCE_ON` (manual override is operator's responsibility)
  - Reset by: `clear_error`, or pump stops normally within the time limit
  - Serial: `[ERROR] Max runtime exceeded. Pump stopped. Possible overflow or sensor failure.`
- Push `is_overflow_error` to Firebase status

#### 1C. Ultrasonic Sensor Hardening (JSN-SR04T)

- **5-sample median filter** — take 5 readings (60ms apart ≈ 300ms total), use median. Eliminates noise spikes.
- **EMA smoothing** — `level = 0.3 × newReading + 0.7 × level`. Smooths inter-cycle noise.
- **Float-based percentage** — replace integer `map()` with: `100.0f * (emptyCm - dist) / (emptyCm - fullCm)`. More precise.
- **Distance validation** — discard readings < 2cm or > 200cm (physically impossible)
- **Timeout increase** — `ULTRASONIC_TIMEOUT_MS`: 30 → 50ms (margin for 40m CAT6 signal attenuation)

#### 1D. Flow Calibration Factor Fix

- Change `FLOW_CALIBRATION_FACTOR` default: `1.0f` → `7.5f`
- Update comments: `Q (L/min) = F (Hz) / 7.5`
- **Note:** YF-G1 calibration varies ±10% by manufacturer/batch. 7.5 is the most common datasheet value; some variants use 4.8. Always verify with a bucket + stopwatch test. The `flow_calibration_factor` is tunable via Firebase without reflashing.

### Dashboard Changes

- **`types.ts`** — Add to `PumpStatus`: `is_sensor_error: boolean`, `is_overflow_error: boolean`
- **`StatusBar.tsx`** — Show warning badge when `is_sensor_error` or `is_overflow_error` is true
- **`DeviceConfigSettings.tsx`** — Add `max_pump_runtime_min` field (number input, 30–480)
- **`types.ts`** — Add to `DeviceConfig`: `max_pump_runtime_min: number` (default: 120)

### Verification

1. Compile clean
2. Disconnect ultrasonic ECHO → after ~5s confirm `[ERROR] Sensor failure`, pump OFF in AUTO
3. Reconnect → error auto-clears
4. Run pump > 120 min in AUTO → confirm max runtime cutoff triggers
5. Check Firebase for `is_sensor_error`, `is_overflow_error`
6. Verify flow reading accuracy with bucket test → adjust `flow_calibration_factor` via Firebase

---

## Phase 2 — System Resilience & State Persistence

> **Goal:** Survive crashes, power loss, and WiFi outages gracefully.

### Firmware Changes

#### 2A. Hardware Watchdog (TWDT)

- `#include <esp_task_wdt.h>`
- `esp_task_wdt_init(15, true)` — 15-second timeout, panic-reboot on hang
- `esp_task_wdt_add(NULL)` in `setup()`, `esp_task_wdt_reset()` at top of `loop()`
- If loop hangs > 15s → auto-reboot → pump starts OFF (safe)

Why 15s: Firebase can block 3–10s on poor WiFi. 5s = false reboots. 30s = too long.

#### 2B. Crash Loop Detection

- On each `setup()`: read `last_boot_time` and `recent_boot_count` from NVS
- If `now - last_boot_time > 5 min`: reset `recent_boot_count = 0` (long uptime, not a crash loop)
- Write `last_boot_time = now`, increment `recent_boot_count`
- If `recent_boot_count >= 5` → **safe mode**: pump OFF, skip Firebase, print to Serial only
- **Safe mode timeout:** auto-clear after 1 hour (or next full power cycle). This prevents being stuck in safe mode permanently if a transient issue resolves.
- Prevents a firmware bug from hammering the relay

#### 2C. NVS State Persistence

- Persist `pumpMode` and `isDryRunError` to NVS **on change** (not every loop)
- Persist `waterLevelPct` for post-reboot diagnostics — to reduce NVS wear, **only write when level changed by ≥ 5%** since last write, or at a **5-minute interval** (whichever comes first). This reduces writes from ~525K/year to ~100K/year.
- On boot: load last known state → log: `[BOOT] Last state: Level=85%, Mode=AUTO, Pump=ON`
- **Do NOT auto-start pump** — just log context

#### 2D. Boot Reason Logging

- Use `esp_reset_reason()` → log human-readable reason (power-on, watchdog, crash, software)
- Push `last_boot_reason` (string) to Firebase `/pump_system/status/`

#### 2E. Startup Stabilization

- 5-second delay after GPIO init, before first sensor read
- Allows sensors to settle after power-up (JSN-SR04T needs ~250ms warm-up)

#### 2F. WiFi Reconnect (Exponential Backoff)

- Replace fixed 15s retry with: **5s → 10s → 20s → 40s → 60s** (cap), ±2s random jitter
- Full cycle: `WiFi.disconnect(true)` → `WiFi.begin(SSID, PASS)` → non-blocking wait
- Add `WiFi.setAutoReconnect(true)` in `setup()`
- After WiFi reconnects: set `firebaseNeedsReinit = true` to refresh stale token

#### 2G. Connectivity Telemetry

- Add `wifi_rssi` (int, dBm) to Firebase status pushes
- Track `lastSuccessfulFirebaseMs` — if > 5 min since last sync → log warning
- Log RSSI on reconnect and every 60s to Serial

### Dashboard Changes

- **`types.ts`** — Add to `PumpStatus`: `wifi_rssi: number`, `last_boot_reason: string`
- **`StatusBar.tsx`** — Show WiFi signal indicator (based on RSSI ranges), boot reason on hover
- **Notification: `overflowAlert`** — Add overflow alert type to `NotificationSettings.tsx` and Cloud Function

### Verification

1. Compile clean
2. Boot → Serial shows: boot reason, WDT init, 5s delay, WiFi connect
3. Unplug USB briefly → reboot → confirm NVS loads last mode (e.g., FORCE_OFF persists)
4. Power-cycle router → observe exponential backoff (5s, 10s, 20s…) in Serial
5. Check Firebase for `wifi_rssi`, `last_boot_reason`

---

## Phase 3 — Scheduled Sleep & ESP32 Protection

> **Goal:** Extend ESP32 hardware lifespan over years. Reduce heat, power draw, flash wear, and WiFi radio duty during idle hours.

### Why This Matters

| Factor | Continuous 24/7 | With scheduled sleep |
|--------|-----------------|---------------------|
| Current draw | ~80mA constant | ~20mA during sleep window (Light Sleep) |
| Heat | Constant — degrades capacitors over years | Reduced 6-8 hrs/day |
| WiFi radio | Active 24/7 | Power-save during sleep |
| NVS flash writes | Every 60s = ~525K/year | Paused during sleep → ~350K/year |
| Firebase RTDB ops | ~28,800 reads/day + ~28,800 writes/day | ~40% reduction |

### Firmware Changes

#### 3A. NTP Time Sync

- `configTime(8 * 3600, 0, "pool.ntp.org")` — Philippine Standard Time (GMT+8), no DST
- Call in `setup()` after WiFi connects
- `getLocalTime(&timeinfo)` to get current hour
- If WiFi never connects → time unknown → sleep mode **disabled** (safe fallback)
- Internal RTC drifts ~1–2s/day — acceptable for hour-level scheduling

#### 3B. Scheduled Sleep Mode

- **New Firebase config fields** (dashboard-configurable):
  - `sleep_enabled` (bool, default: `false`)
  - `sleep_start_hour` (int, 0–23, default: `23`)
  - `sleep_end_hour` (int, 0–23, default: `5`)
  - `sleep_emergency_level` (int, 0–100, default: `5`)

- **Behavior during scheduled sleep window:**
  - AUTO mode **suppressed** — pump will NOT auto-start
  - ESP32 enters **Light Sleep** between sensor reads using `esp_light_sleep_start()`
    - CPU pauses, RAM retained, wakes on timer
    - WiFi enters modem-sleep (DTIM-based power save — stays associated with AP)
    - Wake every `idle_sensor_interval_ms` (default: 30s) to read sensors + check Firebase
  - All safety protections remain active (dry-run, sensor failure, overflow cutoff)
  - **FORCE_ON always works** — if user sends FORCE_ON from dashboard during sleep, pump starts
  - **Emergency override** — if level ≤ `sleep_emergency_level` (default 5%), bypass sleep → pump ON
  - On emergency wake: log `[SLEEP] Emergency override: level at X%`
  - Dry-run protection still runs if pump is forced on during sleep

- **Condition-based idle** (outside sleep hours): when pump OFF and level ≥ 90% for > 5 min → slow-poll (10s sensor, 30s Firebase). This applies 24/7 as a separate optimization.

- **Wake from sleep:**
  - When current hour exits the sleep window → resume normal 1s/3s intervals
  - Log: `[SLEEP] Waking up — resuming normal operation`

#### 3C. Light Sleep Implementation

```
// Pseudocode for sleep cycle
if (sleepEnabled && isInSleepWindow(currentHour) && !emergencyOverride) {
    // Read sensors once
    readSensors();
    checkSafety();
    pushFirebaseIfReady();

    // Configure wake timer
    esp_sleep_enable_timer_wakeup(idleSensorIntervalMs * 1000ULL);  // µs

    // Enter light sleep — CPU pauses, RAM retained, WiFi in modem-sleep
    esp_light_sleep_start();

    // Execution resumes here after wake
    esp_task_wdt_reset();  // Feed watchdog immediately after wake
}
```

> **Deep sleep is NOT used.** It loses all RAM, disconnects WiFi fully, and requires cold-boot reconnection (~3–5s). Light Sleep retains RAM and WiFi association.

### Dashboard Changes

- **`types.ts`** — Add to `DeviceConfig`:
  - `sleep_enabled: boolean` (default: `false`)
  - `sleep_start_hour: number` (default: `23`)
  - `sleep_end_hour: number` (default: `5`)
  - `sleep_emergency_level: number` (default: `5`)
- **`DeviceConfigSettings.tsx`** — Add "Sleep Schedule" section:
  - Toggle: Enable sleep mode
  - Two dropdowns: Start hour, End hour (0–23, formatted as 12h with AM/PM)
  - Number input: Emergency override level (0–20%)
- **`StatusBar.tsx`** — Show "😴 Sleep Mode" indicator when ESP32 is in scheduled sleep
- **`types.ts` → `PumpStatus`** — Add `is_sleeping: boolean`

### Verification

1. Compile clean
2. Set `sleep_enabled: true`, `sleep_start_hour` to current hour via Firebase
3. Serial: `[SLEEP] Entering scheduled sleep` — confirm slow polling
4. Confirm current draw drops (multimeter on 5V line: ~80mA → ~20mA)
5. Set `sleep_emergency_level: 50`, with tank below 50% → confirm pump starts despite sleep
6. Send FORCE_ON from dashboard during sleep → confirm pump starts
7. Wait for sleep window to end → confirm `[SLEEP] Waking up` and normal operation resumes

---

## Phase 4 — Firebase Config Expansion & Dashboard Polish

> **Goal:** Wire all new parameters into the Firebase config read path. Ensure the dashboard can configure everything without reflashing.

### Firmware Changes

#### 4A. Expand `readDeviceConfigFromFirebase()`

Add parsing, validation, and NVS persistence for all new config fields:

| Firebase Key | Type | Range | Default |
|-------------|------|-------|---------|
| `max_pump_runtime_min` | int | 30–480 | 120 |
| `sleep_enabled` | bool | — | false |
| `sleep_start_hour` | int | 0–23 | 23 |
| `sleep_end_hour` | int | 0–23 | 5 |
| `sleep_emergency_level` | int | 0–100 | 5 |
| `sensor_failure_threshold` | int | 3–20 | 5 |
| `idle_sensor_interval_ms` | int | 5000–60000 | 30000 |
| `idle_firebase_interval_ms` | int | 10000–120000 | 30000 |

Validation: if any field invalid → keep current value, don't reject the entire config.

#### 4B. NVS Schema Expansion

Add NVS keys for new config fields so they persist across reboot when offline.

### Dashboard Changes

- **`DeviceConfigSettings.tsx`** — Full config form with all parameters grouped:
  - **Tank Calibration:** empty cm, full cm
  - **Pump Thresholds:** start level, stop level, max runtime
  - **Safety:** dry-run threshold, dry-run timeout, flow calibration, sensor failure threshold
  - **Sleep Schedule:** enabled, start hour, end hour, emergency level
  - **Advanced:** idle sensor interval, idle Firebase interval
- **`useDeviceConfig.ts`** — Update to read/write all new fields
- **Form validation** — client-side validation matching firmware ranges

### Verification

1. Change every config field via dashboard → confirm ESP32 picks up within 30s (Serial)
2. Power-cycle ESP32 → confirm all config persists from NVS
3. Set invalid values in Firebase Console directly → confirm firmware rejects gracefully

---

## Phase 5 — Uptime Counter & Dashboard Bug Fixes

> **Goal:** Track total connection stability without suffering from the standard 49.7-day `millis()` rollover, and fix all hardcoded threshold values in the UI.

### Firmware Changes
1. **Accurate Uptime:** Use `esp_timer_get_time()` to calculate `uptime_minutes` since boot. This avoids the standard 32-bit `millis()` rollover limit.
2. **Firebase Push:** Append `uptime_minutes` to the 3-second `/pump_system/status` broadcast payload.

### Dashboard Changes
1. **Dynamic UI Rendering:** Fix the `StatCard` labels inside `page.tsx` that previously used hardcoded text (e.g., `Auto start ≤ 30% · stop ≥ 100%`) to read live values from `useDeviceConfig()` (path: `/pump_system/config/device`).
2. **Correct Warning Thresholds:** The "Dry-Run risk" warning in the flow stat card now dynamically reacts to `config.dry_run_threshold_lpm` rather than a flat `0.5LPM` assumption.
3. **Uptime Indicator:** Incorporate the `uptime_minutes` value into `StatusBar.tsx`, placed gracefully beside the "ESP32 online" text using formatting logic (e.g., `up 2h 15m`).

### Dashboard Inconsistencies & Fixes (Documented)

| # | Issue | Location | Fix |
|---|-------|----------|-----|
| 1 | **StatCard threshold label not updating** — After changing pump start/stop in DeviceConfigSettings, the label "Auto start ≤ 30% · stop ≥ 100%" stayed at default values. | `page.tsx` StatCard, `useDeviceConfig.ts` | Add optimistic update in `saveConfig`: after Firebase write succeeds, call `setConfig(merged)` so UI reflects changes immediately without waiting for `onValue` round-trip. |
| 2 | **Dry-Run lockout message hardcoded "30s"** — ModeControls showed "No flow detected for 30s" regardless of configured `dry_run_timeout_sec`. | `ModeControls.tsx` | Add `dryRunTimeoutSec` prop from `config.dry_run_timeout_sec`; render `{dryRunTimeoutSec}s` in the message. |
| 3 | **Sleep schedule description hardcoded "30s poll"** — Text said "30s poll" even when `idle_sensor_interval_ms` was different (e.g. 10s). | `DeviceConfigSettings.tsx` | Use dynamic value: `{Math.round((form.idle_sensor_interval_ms ?? 10000) / 1000)}s poll`. |
| 4 | **ESP32 config fetch "every 30s"** — Footer text hardcodes "every 30s"; firmware may vary. | `DeviceConfigSettings.tsx` line 237 | Left as-is (documentation); config fetch interval is firmware-specific. |

### Verification
1. Ensure the dashboard `StatCards` match whatever is set in `DeviceConfigSettings`. Edit a setting and verify the card text updates **immediately** after save.
2. Verify that "up Xm" appears next to the online status in the navigation bar.
3. Change `dry_run_timeout_sec` in DeviceConfigSettings → trigger dry-run error → verify ModeControls shows the configured seconds.

---

## Configurability Summary

### Dashboard-configurable (no reflash):

| Parameter | Firebase Key | Default | Phase |
|-----------|-------------|---------|-------|
| Tank empty distance | `tank_empty_cm` | 122 | Existing |
| Tank full distance | `tank_full_cm` | 8 | Existing |
| Pump start threshold | `pump_start_level` | 30 | Existing |
| Pump stop threshold | `pump_stop_level` | 100 | Existing |
| Dry-run flow threshold | `dry_run_threshold_lpm` | 0.5 | Existing |
| Dry-run timeout | `dry_run_timeout_sec` | 30 | Existing |
| Flow calibration factor | `flow_calibration_factor` | 7.5 | Fix |
| Max pump runtime | `max_pump_runtime_min` | 120 | Phase 1 |
| Sleep enabled | `sleep_enabled` | false | Phase 3 |
| Sleep start hour | `sleep_start_hour` | 23 | Phase 3 |
| Sleep end hour | `sleep_end_hour` | 5 | Phase 3 |
| Sleep emergency level | `sleep_emergency_level` | 5 | Phase 3 |
| Sensor failure threshold | `sensor_failure_threshold` | 5 | Phase 4 |
| Idle sensor interval | `idle_sensor_interval_ms` | 30000 | Phase 4 |
| Idle Firebase interval | `idle_firebase_interval_ms` | 30000 | Phase 4 |

### Hardcoded (safety — never remotely configurable):

| Parameter | Value | Reason |
|-----------|-------|--------|
| GPIO pins | 4, 5, 18, 34 | Hardware-dependent |
| WDT timeout | 15s | Safety-critical |
| Relay active-low | `LOW = ON` | Hardware-dependent |
| Boot behavior | Pump OFF | Must never auto-start |
| Credentials | `secrets.h` | Security |

---

---

## Phase 6 — Dashboard UX, Push Notifications & PWA (March 2026)

> **Goal:** Improve usability, add phone/browser push notifications (like YouTube, Facebook), and make the dashboard installable as a mobile app.

### 6A. Firebase Data Optimization

- **Assessment document** — `docs/FIREBASE_OPTIMIZATION.md` documents current RTDB usage patterns and long-term suitability
- **Verdict:** Current architecture is well-optimized for single-pump use; no changes required for normal operation
- **Recommendations:** Optional improvements for multi-device scaling or offline persistence

### 6B. Dashboard Settings UX

- **InfoTooltip component** — Reusable popover showing help text on hover (desktop) or tap (mobile)
- **DeviceConfigSettings** — Tooltips on every section and key field: Tank Calibration, Pump Thresholds, Safety, Sleep Schedule, Advanced
- **NotificationSettings** — Tooltips on delivery methods, alert types, and push configuration
- **Modern approach:** No additional UI library; lightweight, accessible Info icon + popover pattern

### 6C. Push Notifications (FCM)

- **Firebase Cloud Messaging** — Alerts sent directly to phone/browser, like YouTube/Facebook/Instagram
- **Dashboard:** "Enable push on this device" button in Notification Settings; stores FCM token in `notifications_by_user/{uid}/fcmTokens/{deviceId}`
- **Cloud Functions:** Sends push to all stored FCM tokens for each user (in addition to email when configured)
- **Service worker:** Dynamic `/firebase-messaging-sw.js` served by API route with injected Firebase config
- **Setup:** `NEXT_PUBLIC_FIREBASE_VAPID_KEY` from Firebase Console → Cloud Messaging → Web Push certificates

### 6D. Progressive Web App (PWA)

- **Installable app** — Users can add the dashboard to the home screen on Android, iOS, and desktop
- **Manifest** — `app/manifest.ts` with name, icons, theme colors, display standalone
- **Service worker** — `@ducanh2912/next-pwa` generates `sw.js` for caching and installability
- **Install prompt** — `InstallPrompt` component shows banner when `beforeinstallprompt` fires
- **Icons** — 72×72, 192×192, 512×512 in `dashboard/public/icons/`

### Verification

1. Tooltips appear on hover/tap in Device and Notification settings
2. Push: Set VAPID key → Enable push in dashboard → Save → Trigger alert → Receive push on device
3. PWA: Deploy over HTTPS → On mobile, "Add to Home Screen" or install icon appears
4. Firebase optimization doc exists and describes current usage

---

## Recommendations (Future — not in current phases)

1. **Multi-SSID** — `WiFiMulti` for backup WiFi AP
2. **LED indicator** — Solid/blink for system state
3. **Error history** — Circular buffer in NVS

*(Note: OTA Updates were evaluated but discarded due to the 1.25MB firmware size occupying 95% of the default partition scheme. Adding it requires custom minimal-SPIFFS partitioning. Since most parameters are now dynamically configurable via the dashboard, frequent remote flashing is unnecessary.)*

---

*Plan version 5. Phase 6 added — March 2026.*

---

## Phase 7 — Smart Manual & Timed Runs

**Goal:** Add a safer, smarter way to run the pump manually or for a fixed time **even when the current sensor is unavailable or unreliable**, while keeping all existing flow‑sensor and level protections.

This phase is now implemented in this repo as an **additive** extension (existing deployments remain compatible because the new Firebase keys are optional and safe to ignore).

### 7.1 High‑Level Behavior

- **Manual “run until I stop”** from the dashboard:
  - Operator explicitly starts the pump (similar to FORCE_ON today) but with a clear **MANUAL** label and explanation.
  - ESP32 keeps all protections active: flow dry‑run, overflow, sensor failure, sleep/emergency logic.
  - Operator stops the run from the dashboard, or the firmware stops it automatically on safety fault.
- **Timed run**:
  - Operator chooses a duration (e.g. 5/10/15/30/60 min).
  - ESP32 starts the pump and **auto‑stops** after the duration expires, or earlier on safety fault.
- **Compatibility with AUTO:**
  - AUTO mode (start/stop by tank% thresholds) continues to behave exactly as today.
  - Manual/timed runs are **short‑lived overrides** that return to AUTO afterward.
- **Current sensor optional**:
  - Design does not depend on current sensing; if/when a current sensor is re‑enabled, additional fault codes can be added without changing the high‑level contract.

### 7.2 Firmware State Model (Additive)

Existing `control.mode` values remain unchanged:

| Field | Current values | Meaning |
|-------|----------------|---------|
| `/pump_system/control/mode` | `"AUTO" \| "FORCE_ON" \| "FORCE_OFF"` | Automation, manual override on, manual override off |

Phase 7 **adds** a separate notion of **run mode** inside the firmware, without overwriting `mode`:

- Internal enum (conceptual):
  - `RUN_OFF`
  - `RUN_AUTO` (current AUTO behavior)
  - `RUN_MANUAL_UNTIL_STOP`
  - `RUN_TIMED`
- New runtime fields:
  - `pumpRunMode` (enum above)
  - `runStartMillis` (unsigned long)
  - `runRequestedDurationMs` (unsigned long, 0 for manual‑until‑stop)

Within the existing non‑blocking loop:

- On each sensor tick:
  - Read level, flow, and run all existing safety checks (`checkSafetyCutoff()`).
  - Execute pump logic:
    - `RUN_AUTO`: same as today (start at `pump_start_level`, stop at `pump_stop_level`, respect sleep/emergency).
    - `RUN_MANUAL_UNTIL_STOP`: pump ON, but **never** bypass safety; any safety fault stops the pump and returns to `RUN_OFF`.
    - `RUN_TIMED`: pump ON; when `millis() - runStartMillis >= runRequestedDurationMs`, stop pump and return to `RUN_AUTO` (or `RUN_OFF` if previously not in AUTO), unless a safety fault already stopped it earlier.
- Any safety fault (dry‑run, overflow, sensor failure, future current faults) forces:
  - Pump OFF
  - `pumpRunMode = RUN_OFF`
  - Error flags pushed to Firebase as today.

Sleep/idle logic (Phase 3) remains unchanged; runs that start during sleep either:

- Use **emergency override** rules (low level), or
- Are considered **operator‑forced** (manual/timed) and still respect dry‑run and flow safety.

### 7.3 Firebase Schema Extensions (Planned)

To avoid breaking existing dashboards and firmware, Phase 7 uses **additive** fields in both `status` and `control`. These are safe to ignore for older code:

| Path | Field | Type | Notes |
|------|-------|------|-------|
| `/pump_system/status` | `run_mode` | `"AUTO" \| "MANUAL" \| "TIMED" \| "OFF"` | Human‑readable mirror of internal `pumpRunMode` for the dashboard. |
| `/pump_system/status` | `run_remaining_sec` | number | Seconds until auto‑stop when in timed run; `0` otherwise. |
| `/pump_system/status` | `last_fault_code` | string | Optional stable code such as `"DRY_RUN"`, `"OVERFLOW"`, `"SENSOR"`, `"CURRENT"`. |
| `/pump_system/status` | `last_fault_message` | string | Human‑readable description (for dashboard banner). |
| `/pump_system/control` | `manual_start` | boolean | When toggled `true`, requests a **manual‑until‑stop** run; firmware clears it after processing. |
| `/pump_system/control` | `timed_start_sec` | number | Duration in seconds for timed run. Non‑zero value triggers a timed run; firmware copies & clears it atomically. |
| `/pump_system/control` | `manual_stop` | boolean | Immediate stop request; firmware turns pump off and clears flags. |

Notes:

- Existing fields (`mode`, `clear_error`, `reboot_request_id`) keep their semantics and remain the primary way to request AUTO/FORCE_ON/FORCE_OFF and clear errors.
- For backwards compatibility, an initial implementation can treat:
  - `manual_start` as equivalent to setting `mode = "FORCE_ON"` plus a tracked run mode.
  - `manual_stop` as equivalent to `mode = "FORCE_OFF"` or returning to `"AUTO"`, depending on previous state.

### 7.4 Dashboard UX (Organized Controls)

Dashboard changes are kept small and aligned with the existing `ModeControls` and Device Config patterns:

- **New control group:** “Run pump” with two primary actions:
  - **Quick start (Manual)** — sends `manual_start = true`. Label explains: “Runs now, stops when you press Stop or when safety triggers.”
  - **Timed run…** — opens a small dialog to pick a duration (e.g. 5/10/15/30/60 minutes), then writes `timed_start_sec`. Text explains that the ESP32 will **auto‑stop** at the end and still has full protections.
- **Stop button:** Visible whenever `run_mode` is `"MANUAL"` or `"TIMED"`:
  - Sends `manual_stop = true`.
  - Disabled while a stop request is already in flight (use `usePendingControl` style feedback).
- **Status display:**
  - Show `Run mode: AUTO / Manual / Timed / Off`.
  - If timed: show a countdown “Auto‑stop in mm:ss” derived from `run_remaining_sec`.
- **Config & safety:**
  - No new config fields are required to use manual/timed runs.
  - Existing **max runtime**, sleep, and dry‑run thresholds still apply and are documented as the safety envelope for both manual and timed runs.

### 7.5 Safety & Compatibility Notes

- Manual/timed runs **never bypass**:
  - Dry‑run shutdown (flow threshold + timeout).
  - Overflow cutoff (`max_pump_runtime_min`).
  - Sensor failure handling (sensor error ⇒ safe OFF).
  - Emergency low‑level overrides and sleep rules from Phase 3.
- The design is safe to roll out in small steps:
  1. Implement internal `pumpRunMode` and timed/remaining logic purely in firmware, without new Firebase fields (for bench testing).
  2. Add `run_mode` and `run_remaining_sec` to status (dashboard can start reading them).
  3. Add control fields and wire a simple dashboard “Timed run” button.
- Current sensor can later set additional fault codes without changing this contract, keeping the system compatible across hardware revisions.

This phase gives you a clear, documented path to safer manual and timed control, even with the current‑sensor hardware still in flux, while staying consistent with the existing v2.4.0 architecture and plans.
