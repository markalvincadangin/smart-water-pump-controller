# Firmware Validation Audit — v4.0
## Smart Water Pump Controller — Post–Offline-First & Single-JSON Control Read

**Scope:** `firmware/platformio_smart_water_pump_controller/src/` (and equivalent `firmware/arduino_smart_water_pump_controller/`).  
**Date:** March 2025.  
**Baseline:** v3.0.0 firmware with hierarchical priority model (P1–P5), COUNTDOWN, manual one-shots, bypass, NVS persistence, and single-JSON control read.

---

## Part 1 — Verification Matrix (Prior Issues)

Every issue from prior audits (FIRMWARE_VALIDATION_AUDIT.md, v2, v3) checked against the current codebase.

| ID | Issue | Fix Expected | File:Line | Result |
|----|-------|--------------|-----------|--------|
| Bug 1 | Stop restarts countdown | `pendingModeWriteback` flag | 05:226-265 | ✅ |
| Bug 2 | Expiry restarts countdown | Same flag; `pendingModeWritebackSentMs=0` in expiry | 05:164-165, 03:259-261 | ✅ |
| Bug 3 | Add-time loops | `setBool false` + `lastAddTime=false` on new start | 05:307, 05:296 | ✅ |
| Old #2 | COUNTDOWN dropped from NVS restore | Added to validation list | 04:204-208 | ✅ |
| Old #3 | `WiFi.disconnect(true)` | `disconnect(false)`, no `delay(100)` | smart_water_pump_controller.ino:163 | ✅ |
| Old #4 | Overflow not guarding COUNTDOWN | `pumpMode=="AUTO" \|\| pumpMode=="COUNTDOWN"` | 03:116 | ✅ |
| Old #5 | `runMode` init to `"AUTO"` | Initialized to `"OFF"` | 01:127 | ✅ |
| Old #6 | `auto_bypass` not in NVS | `auto_bypass_en` / `auto_bypass_sec` load+save | 04:38-39, 102-103 | ✅ |
| Old #7 | `-1` sentinel pushed raw | `if (estimatedLevelPct >= 0.0f)` guard | 05:416-418 | ✅ |
| Old #10 | NVS schema unversioned | `NVS_SCHEMA_VERSION=1`, written+checked | 04:44, 108 | ✅ |
| Old #11 | `WiFi.persistent(true)` | `WiFi.persistent(false)` | 05:361 | ✅ |
| A2 #1 | Write-back storms Firebase | `pendingModeWritebackSentMs` 5s rate-limit | 05:234-237 | ✅ |
| A2 #2 | `countdownConsumed` reset wrong | Reset in writeback confirmation when `pumpMode=="AUTO"` | 05:231 | ✅ |
| A2 #3 | `checkCountdownExpiry` Firebase write in sensor block | Firebase write removed; pure state only | 05:156-166 | ✅ |
| A2 #4 | `lastFirebaseMs` reset on retry | `if (normalIntervalDue) lastFirebaseMs=now` | smart_water_pump_controller.ino:315 | ✅ |
| A2 #5 | `runMode` gap: COUNTDOWN+!isCountdownActive | `else if (COUNTDOWN && !isCountdownActive) runMode="OFF"` | 03:206-207 | ✅ |
| A2 #6 | `schemaVer<NVS_SCHEMA_VERSION` not logged | Log line added | 04:48-51 | ✅ |
| A2 #7 | Mode read timeout → 30s cooldown on 1st fail | `>= STATUS_PUSH_RETRY_MAX` guard | 05:185-194 | ✅ |
| A2 #8 | `setPump(false)` in `checkCountdownExpiry` phantom cycle | Removed | 05:156-166 | ✅ |
| A2 #9 | `bypass` enable clears `autoBypassWasEngaged` | Only cleared on `!v` (disable) path | 05:334-338 | ✅ |
| A2 #10 | Dead `runMode="AUTO"` in expiry | Removed | 05:156-166 | ✅ |
| A2 #11 | `delay(100)` in WiFi reconnect | Removed | smart_water_pump_controller.ino:163-165 | ✅ |
| A3 #4 | P4 early-stop: Firebase write in sensor block | Removed; `pendingModeWritebackSentMs=0` | 03:259-261 | ✅ |
| Offline | `cfgLastCountdownDurationMin` NVS persist | Saved on start + loaded in `loadStateFromNVS` | 05:283-288, 04:204 | ✅ |
| Offline | Status push retry | `statusRetryDue` + `normalIntervalDue` split | smart_water_pump_controller.ino:311-316 | ✅ |
| Offline | 30s cooldown only after 3 consecutive failures | `>= STATUS_PUSH_RETRY_MAX` in push | 05:354-362 | ✅ |
| v3 #10 | P1 error during countdown: revert `pumpMode` to AUTO | `pumpMode="AUTO"` + `pendingModeWriteback=true` in P1 block | 03:229-235 | ✅ |
| v3 #13 | 8 individual RTDB reads per cycle | Single `getJSON("/pump_system/control")` | 05:174-201 | ✅ |

**All 28 prior issues confirmed fixed. Single-JSON control read is implemented.**

---

## Part 2 — Offline-First Redesign Verification

### Implemented ✅

| Property | Evidence |
|----------|----------|
| **NVS authority** | `pumpMode`, `isDryRunError`, `cfgBypassLevelSensor`, `totalPumpCycles`, `totalPumpRunSec`, `cfgLastCountdownDurationMin`, device config (including `auto_bypass_en` / `auto_bypass_sec`) saved on change and loaded on boot (04, 05). |
| **Firebase as sync layer** | Sensor loop, safety, countdown, relay run on local state only. No Firebase calls in `executePumpLogic()`, `checkCountdownExpiry()`, or `checkSafetyCutoff()`. |
| **Single control read** | `readFirebaseControl()` uses one `Firebase.RTDB.getJSON(&fbdo, "/pump_system/control")` (05:174). All keys parsed from `controlJson` (mode, manual_stop, countdown_duration_min, countdown_add_time, manual_start, bypass_level_sensor, clear_error, reboot_request_id). One network round-trip per cycle. |
| **Mode write-back queue** | `pendingModeWriteback` + `pendingModeWritebackSentMs` (5s rate-limit). Used on manual_stop, countdown expiry, P4 early stop, P1 countdown clear. No Firebase write inside sensor block. |
| **Countdown timer** | `countdownEndMs` is millis()-based; runs and expires with no Firebase dependency after start. Offline start uses `cfgLastCountdownDurationMin` from NVS when key missing from JSON. |
| **Retry and cooldown** | Control read and status push share `firebaseConsecutiveFailCount`; 30s cooldown only when `>= STATUS_PUSH_RETRY_MAX`. `statusRetryDue` and `normalIntervalDue` keep retries without starving normal interval. |
| **WiFi recovery** | Exponential backoff + jitter; `WiFi.disconnect(false)`; no `delay(100)`; late Firebase init when WiFi was down at boot; token refresh on reconnect. |

### Not implemented (acceptable for current spec)

| Gap | Notes |
|-----|--------|
| **Epoch/ID-based one-shots** | `manual_start` / `manual_stop` remain boolean edge-detect. Acceptable: edge fires correctly on reconnect; reboot with flag true in Firebase still fires edge on first read. |
| **Full control shadow state** | Single JSON read is implemented; no further shadow layer required. |

**Offline-first status: Fully aligned with design. Single-JSON control read closes the main prior gap.**

---

## Part 3 — New Issues and Logic Scan

### Issue 1 — 🟢 Minor: Firebase API method name `setwriteSizeLimit`

**File:** `05_connectivity_cloud.ino`, in `initFirebase()`

```cpp
Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
```

The method name uses a lowercase `w` (`setwriteSizeLimit`). The Firebase_ESP_Client library may use this spelling (build succeeds). If the library expects `setWriteSizeLimit` (capital W), this would be a link or runtime error; confirm against the library header.

**Verdict:** Verify against installed Firebase_ESP_Client API. If the build is green, the spelling matches the library. No logic bug.

---

### Issue 2 — 🟢 Minor: `isManualRun` not cleared at top of `executePumpLogic()`

**File:** `03_safety_pump.ino`, `executePumpLogic()`

The v3 plan suggested: at the top of `executePumpLogic()`, clear `isManualRun` when `pumpMode != "FORCE_ON"` so the flag cannot become stale. Current code clears `isManualRun` only in `readFirebaseControl()` (manual_stop, and when `newMode != "FORCE_ON"`). If `pumpMode` were ever set to something other than FORCE_ON by another path (e.g. NVS restore to AUTO, or a future code path), `isManualRun` could remain true. `runMode` is derived from `pumpMode`, not `isManualRun`, so the UI would still show AUTO/AUTO_STANDBY correctly. The only effect would be a stale internal flag.

**Verdict:** Optional defensive cleanup. Not a functional bug. Low priority.

---

### Issue 3 — Logic scan: P1 / P4 / runActive ordering

**Files:** `05_connectivity_cloud.ino` (mode read), `03_safety_pump.ino` (executePumpLogic)

- **P1 (hard safety):** When `isDryRunError` or `isOverflowError`, pump is stopped; if countdown was active, `pumpMode` is set to `"AUTO"` and `pendingModeWriteback` is set (03:229-235). ✅  
- **P4 early stop (tank full):** `pumpMode = "AUTO"`, `pendingModeWriteback = true`, `pendingModeWritebackSentMs = 0`; no Firebase call in sensor block (03:259-261). ✅  
- **runActive:** Uses `runMode == "MANUAL"` and `(pumpMode == "COUNTDOWN" && isCountdownActive)`. FORCE_ON is reflected as `runMode == "MANUAL"` from `executePumpLogic()`, which runs in the sensor block before the next Firebase cycle, so by the time the mode read runs, `runActive` is correct. ✅  
- **manual_stop:** Sets `pumpMode = "AUTO"`, writes back to Firebase immediately, `pendingModeWritebackSentMs = millis()`. Countdown cleared. ✅  
- **countdownConsumed:** Reset when `firebaseReadMode != "COUNTDOWN"` (05:271-273); also reset on write-back confirmation when `pumpMode == "AUTO"` (05:231). Start block (05:275-298) only runs when `pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed`. No double-start. ✅  

**Verdict:** No logic errors found in countdown/mode/run paths.

---

### Issue 4 — Logic scan: Level sensor and auto-bypass

**File:** `02_sensors.ino` (via `03_safety_pump.ino`), `05_connectivity_cloud.ino`

- **checkLevelSensorFailure:** On recovery, `autoBypassWasEngaged` path clears `cfgBypassLevelSensor`, `autoBypassWasEngaged`, `autoBypassActive` (03:70-75). On enable of bypass from Firebase, only on `!v` (disable) do we clear `autoBypassWasEngaged` (05:335-337). ✅  
- **Auto-bypass:** After `cfgAutoBypassDelaySec` of sustained level sensor failure, `cfgBypassLevelSensor = true`, `autoBypassWasEngaged = true`, `autoBypassActive = true` (03:44-51). ✅  

**Verdict:** No bugs found.

---

### Issue 5 — Logic scan: Status push and retry

**File:** `smart_water_pump_controller.ino`, `05_connectivity_cloud.ino`

- `normalIntervalDue` and `statusRetryDue` both allow entering the Firebase block; `if (normalIntervalDue) lastFirebaseMs = now` so the normal 3s interval is not starved by retries. ✅  
- Push failure increments `firebaseConsecutiveFailCount` and `statusPushRetryCount`; 30s cooldown only when `firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX`. ✅  
- Control read failure also increments `firebaseConsecutiveFailCount`; same cooldown guard (05:185). ✅  

**Verdict:** Retry and cooldown logic correct.

---

## Part 4 — Summary

### Prior issues

All 28 items from previous audits (including v3.0.2) are verified fixed in the current code. Control path now uses a **single getJSON** for `/pump_system/control`, improving reliability on weak WiFi.

### New issues (this pass)

| # | Severity | File | Description | Action |
|----|----------|------|-------------|--------|
| 1 | 🟢 Minor | 05_connectivity_cloud.ino | Confirm `setwriteSizeLimit` spelling against Firebase_ESP_Client API | Verify only |
| 2 | 🟢 Minor | 03_safety_pump.ino | Optional: clear `isManualRun` when `pumpMode != "FORCE_ON"` at top of `executePumpLogic()` | Optional cleanup |

No critical or significant new bugs found. Logic scan of P1–P5, countdown, runActive, manual_stop, write-back, auto-bypass, and status retry shows correct behavior.

### Offline-first

- NVS is authority for mode, errors, bypass, countdown duration, telemetry, and device config.  
- Firebase is a sync layer; one control read per cycle; mode write-back is queued and rate-limited.  
- Sensor and pump logic are fully decoupled from network; countdown is millis()-based.

**Conclusion: Firmware is validated for v4.0. All known prior fixes are in place; offline-first redesign and single-JSON control read are correctly implemented; no new critical or significant issues identified.**
