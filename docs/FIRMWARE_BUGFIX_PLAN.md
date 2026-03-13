# Firmware Bug Fix Plan

This document plans fixes for five issues identified in a full firmware scan. Locations and line numbers refer to [firmware/smart_pump_controller/smart_pump_controller.ino](firmware/smart_pump_controller/smart_pump_controller.ino).

---

## Bug 1 — WiFi reconnection wipes credentials (Critical)

**Severity:** Critical — router restart or brief outage can prevent reconnection until power cycle.

**Location:** `loop()`, ~line 1261.

**Current code:**
```cpp
WiFi.disconnect(true);  // Full disconnect (clear stored config)
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

**Problem:** `WiFi.disconnect(true)` turns the WiFi radio off and **erases** the stored SSID/password. After that, `WiFi.setAutoReconnect(true)` (set in `connectWiFi()`) has nothing to use. Each retry again calls `disconnect(true)`, so credentials are repeatedly wiped and re-applied; reconnection only succeeds when a retry happens to coincide with the router being ready.

**Fix:** Use session-only disconnect and update the comment so it doesn't imply credentials are cleared:
```cpp
WiFi.disconnect(false);  // Session disconnect only — keeps credentials and radio
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

**Files:** `firmware/smart_pump_controller/smart_pump_controller.ino` (one line + comment).

---

## Bug 2 — Task WDT double-init warning (Cosmetic)

**Severity:** Low — log noise only; WDT is functional.

**Location:** `setup()`, ~lines 1186–1199.

**Current code:** Full `#if ESP_ARDUINO_VERSION >= 3` / `#else` block that calls `esp_task_wdt_init()` then `esp_task_wdt_add(NULL)`.

**Problem:** The Arduino-ESP32 core already initializes the Task Watchdog before `setup()`. A second `esp_task_wdt_init()` triggers:
```text
E (12327) task_wdt: esp_task_wdt_init(517): TWDT already initialized
```

**Fix:** Remove the WDT **init** call; only **register** the current task with the already-running TWDT:
- Delete the entire `#if` / `#else` / `#endif` block that contains `esp_task_wdt_init(...)`.
- Keep only:
  - `esp_task_wdt_add(NULL);  // Register loopTask with existing TWDT`
  - Serial message: `"[INIT] Watchdog: task registered."` (do not print a timeout value; the core controls it).

**Files:** `firmware/smart_pump_controller/smart_pump_controller.ino`.

---

## Bug 3 — Crash loop detection depends on other NVS writes (Logic)

**Severity:** Medium — can prevent boot count from resetting after long stable runs in edge cases.

**Location:** `persistStateToNVS()` (~1078–1109) and `checkCrashLoop()` (~1002–1050).

**Current behavior:** `last_boot_ms` (used as “uptime at last NVS write”) is only written when `persistStateToNVS()` actually writes something — i.e. when `modeChanged`, `dryRunChanged`, or `levelNeedsWrite` is true. Level is written at most every 5 min (NVS_LEVEL_INTERVAL_MS) or on 5% level change, so in practice `last_boot_ms` is often updated every 5 minutes. If level and mode and dry-run never change for long enough (e.g. config change to longer level interval), `last_boot_ms` might not advance, so `lastBootTime > CRASH_LOOP_WINDOW_SEC*1000` would never be true and the boot count would never reset.

**Fix:** Update uptime for crash-loop detection on a **dedicated timer**, independent of mode/level/dry-run:

1. **Add** a global and constant near other timing globals (e.g. after `lastLevelWriteMs`):
   - `unsigned long lastUptimeWriteMs = 0;`
   - `#define NVS_UPTIME_INTERVAL_MS 60000UL  // Write uptime every 60s for crash loop detection`

2. **In `persistStateToNVS()`:**
   - Compute `bool uptimeNeedsWrite = (now - lastUptimeWriteMs >= NVS_UPTIME_INTERVAL_MS);`
   - Change the early return to:  
     `if (!modeChanged && !dryRunChanged && !levelNeedsWrite && !uptimeNeedsWrite) return;`
   - Inside the `prefs.begin()` block, after existing writes, add:
     - `if (uptimeNeedsWrite) { prefs.putULong("last_boot_ms", now); lastUptimeWriteMs = now; }`

3. **In `setup()`:** In the timing init block (with `lastSensorMs`, `lastFirebaseMs`, etc.), set `lastUptimeWriteMs = nowInit` so the first loop iteration does not immediately trigger an uptime write.

This limits NVS wear (one extra write at most every 60s) and guarantees crash-loop logic sees an updated uptime after 60s of stable operation.

**Files:** `firmware/smart_pump_controller/smart_pump_controller.ino`.

---

## Bug 4 — `isFlowSensorError` never clears while pump is running (Logic)

**Severity:** Medium — dashboard shows sensor error for the whole pump run after a stuck-high event.

**Location:** `checkFlowSensorStuck()`, ~454–465.

**Current code (simplified):**
```cpp
} else {
  if (isFlowSensorError && isRunning) {
    // Only clear stuck-high error when pump starts (flow is expected)
    // ← EMPTY: never clears isFlowSensorError
  } else if (!isRunning && flowRateLpm <= FLOW_STUCK_THRESHOLD_LPM) {
    if (isFlowSensorError) Serial.println("[INFO] Flow sensor recovered...");
    isFlowSensorError = false;
  }
  flowStuckTimerActive = false;
  flowStuckStartMs = 0;
}
```

**Problem:** When the pump is ON, the first branch is taken but does nothing, so `isFlowSensorError` stays true. It only clears when the pump is OFF and flow is low. So after a stuck-high detection, the dashboard keeps showing a sensor error for the entire time the pump runs.

**Fix:** In the `else` branch (flow is “normal”: either pump off with low flow, or pump on with any flow), always clear the stuck-high error and reset timer:
```cpp
} else {
  if (isFlowSensorError) {
    Serial.println("[INFO] Flow sensor recovered. Stuck-high error cleared.");
    isFlowSensorError = false;
  }
  flowStuckTimerActive = false;
  flowStuckStartMs = 0;
}
```

Remove the empty `if (isFlowSensorError && isRunning)` and the `else if (!isRunning && ...)`; a single “flow is normal” path is enough.

**Files:** `firmware/smart_pump_controller/smart_pump_controller.ino`.

---

## Bug 5 — `prevWaterLevelPct` and first-reading behavior (Non-issue)

**Severity:** None.

**Location:** Global `prevWaterLevelPct = 0`, and `readUltrasonicSensor()` rate-of-change guard.

**Finding:** The guard is `if (prevWaterLevelPct > 0 && delta > LEVEL_RATE_OF_CHANGE_MAX)`. So when `prevWaterLevelPct == 0` (e.g. first cycle), the block is skipped and no false rate-of-change block occurs. No code change needed.

---

## Summary and implementation order

| # | Severity | One-line description | Fix |
|---|----------|----------------------|-----|
| 1 | Critical | WiFi reconnect uses `disconnect(true)` and wipes credentials | Use `WiFi.disconnect(false)` before `WiFi.begin()` in loop |
| 2 | Low | WDT double-init logs an error every boot | Remove `esp_task_wdt_init()`; keep only `esp_task_wdt_add(NULL)` and log |
| 3 | Medium | Crash loop uptime only written when mode/level/dry-run change | Add 60s uptime write timer and write `last_boot_ms` in `persistStateToNVS()` |
| 4 | Medium | Flow sensor stuck-high error never clears while pump is ON | In `checkFlowSensorStuck()` else branch, always clear `isFlowSensorError` and reset timer |
| 5 | — | Rate-of-change guard with `prevWaterLevelPct` | No change |

**Recommended order:** 1 → 4 → 3 → 2 (fix reconnection first, then visible sensor-error behavior, then crash-loop robustness, then log cleanup).
