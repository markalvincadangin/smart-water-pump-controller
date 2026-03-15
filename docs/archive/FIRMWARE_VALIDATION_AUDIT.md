# Firmware Validation Audit
## Smart Water Pump Controller v3.0.0 — Post-Fix Review

---

## Verification Matrix — Prior Issues

| # | Issue | Status | Notes |
|---|---|---|---|
| Bug 1 | Stop restarts countdown (stale mode overwrite) | ✅ Fixed | `pendingModeWriteback` flag implemented |
| Bug 2 | Countdown expiry restarts (same root cause) | ✅ Fixed | Same flag covers this path |
| Bug 3 | Add-time infinite loop (no firmware write-back) | ✅ Fixed | `setBool false` + `lastAddTime = false` on new start |
| Old #2 | `"COUNTDOWN"` dropped from NVS restore | ✅ Fixed | Added to validation list in `loadStateFromNVS()` |
| Old #3 | `WiFi.disconnect(true)` regression | ✅ Fixed | `WiFi.disconnect(false)` confirmed in loop |
| Old #4 | Overflow not guarding COUNTDOWN | ✅ Fixed | `!(pumpMode == "AUTO" || pumpMode == "COUNTDOWN")` |
| Old #5 | `runMode` initialized to `"AUTO"` | ✅ Fixed | Initialized to `"OFF"` in `01_config.ino:128` |
| Old #6 | `auto_bypass` not saved/loaded in NVS | ✅ Fixed | `auto_bypass_en` / `auto_bypass_sec` in both load and save |
| Old #7 | `-1` sentinel pushed raw to Firebase | ✅ Fixed | `if (estimatedLevelPct >= 0.0f)` guard added |
| Old #10 | NVS schema unversioned | ✅ Fixed | `NVS_SCHEMA_VERSION = 1`, written and checked |
| Old #11 | `WiFi.persistent(true)` flash wear | ✅ Fixed | `WiFi.persistent(false)` in `connectWiFi()` |
| Offline | `cfgLastCountdownDurationMin` NVS persist | ✅ Fixed | Saved on countdown start, loaded in `loadStateFromNVS()` |
| Offline | Status push retry (3 retries before cooldown) | ✅ Fixed | `statusRetryDue` logic in loop |
| Offline | 30s cooldown only after 3 consecutive timeouts | ✅ Fixed | `firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX` guard |

**All 14 previously identified issues are confirmed fixed.**

---

## New Issues Found in This Audit Pass

---

### Issue 1 — 🔴 Critical: `pendingModeWriteback` re-sends write on EVERY cycle while pending

**File:** `05_connectivity_cloud.ino`, lines 205–210

```cpp
if (pendingModeWriteback) {
  if (newMode == pumpMode) {
    pendingModeWriteback = false;
  } else {
    Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", pumpMode);
  }
}
```

When Firebase still reports the old mode (propagation lag, typically 1–3 cycles at 3s each), this fires a new `setString` write on **every Firebase cycle** until Firebase confirms the new value. With a 3s cycle, that's potentially 3–6 redundant writes during normal propagation, and many more on a bad network. Each write consumes a Firebase RTDB operation and increases exposure to rate-limiting.

**Fix:** Add a `pendingModeWritebackSentMs` timestamp. Only re-send if more than 5 seconds have elapsed since the last send attempt:

In `01_config.ino`, add:
```cpp
unsigned long pendingModeWritebackSentMs = 0;
```

In `smart_water_pump_controller_shared.h`, add:
```cpp
extern unsigned long pendingModeWritebackSentMs;
```

Replace the else branch:
```cpp
} else if (millis() - pendingModeWritebackSentMs >= 5000UL) {
  Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", pumpMode);
  pendingModeWritebackSentMs = millis();
}
```

Also reset `pendingModeWritebackSentMs = 0` when `pendingModeWriteback` is cleared (both on confirmation and on any path that sets `pendingModeWriteback = false`).

---

### Issue 2 — 🔴 Critical: `countdownConsumed` reset races with `pendingModeWriteback` — Countdown can re-arm after Stop under specific timing

**File:** `05_connectivity_cloud.ino`, lines 261–290

The `pendingModeWriteback` flag prevents the mode read from overwriting `pumpMode` with a stale `"COUNTDOWN"`. However, the `countdownConsumed` reset at line 265 fires independently based only on `firebaseReadMode`:

```cpp
if (firebaseReadMode.length() > 0 && firebaseReadMode != "COUNTDOWN") {
  countdownConsumed = false;
}
```

The intended sequence after Stop:
1. `manual_stop` → `pumpMode = "AUTO"`, `pendingModeWriteback = true`, Firebase write queued
2. Firebase still reports `"COUNTDOWN"` → `pendingModeWriteback` suppresses mode overwrite ✓
3. Firebase delivers `"AUTO"` → `countdownConsumed = false`, `pendingModeWriteback` cleared ✓
4. `pumpMode == "AUTO"` → countdown start block doesn't fire ✓

**The race:** if the user sends a NEW `mode = "COUNTDOWN"` from the dashboard immediately after tapping Stop (within the propagation window), the sequence becomes:

1. `manual_stop` → `pumpMode = "AUTO"`, `pendingModeWriteback = true`
2. Firebase delivers new `"COUNTDOWN"` (the new dashboard write)
3. Mode read: `pendingModeWriteback` is true, `newMode == "COUNTDOWN"`, `pumpMode == "AUTO"` → `newMode != pumpMode` → write-back re-sends `"AUTO"` (correct)
4. `countdownConsumed` reset: `firebaseReadMode = "COUNTDOWN"` → does NOT reset → `countdownConsumed` stays `true` ✓
5. Firebase confirms `"AUTO"` (firmware's write lands) → `countdownConsumed = false`, `pendingModeWriteback = false`
6. But now the user's intended new `"COUNTDOWN"` has been overwritten. The countdown never starts.

This is correct behaviour (the stop wins), but the user gets no feedback. Not a safety bug, but a UX correctness issue when rapid re-start after stop is attempted.

**More dangerous path:** If `countdownConsumed` is `false` AND `pumpMode` gets set back to `"COUNTDOWN"` by the mode read (if `pendingModeWriteback` ever clears prematurely), the start block fires with the old `cfgLastCountdownDurationMin` even though the user intended to stop. This is the same restart bug but via a different path through the new flag.

**Fix:** When `pendingModeWriteback` is active and Firebase confirms the written value (line 207: `newMode == pumpMode`), also explicitly set `countdownConsumed = false` only if `pumpMode == "AUTO"` (to ensure a clean slate for the next fresh COUNTDOWN command):

```cpp
if (newMode == pumpMode) {
  pendingModeWriteback = false;
  pendingModeWritebackSentMs = 0;
  if (pumpMode == "AUTO") countdownConsumed = false;  // ADD THIS
  Serial.println("[FIREBASE] Mode write-back confirmed.");
}
```

This makes `countdownConsumed` reset happen at the right moment — when the firmware is sure its own `"AUTO"` write landed — rather than relying on the next independent Firebase read.

---

### Issue 3 — 🔴 Critical: `checkCountdownExpiry()` is called inside the **sensor block** but contains a Firebase write

**File:** `smart_water_pump_controller.ino`, lines 304–305

```cpp
checkSafetyCutoff();
checkCountdownExpiry();   // ← called every sensorInterval (1s normally)
executePumpLogic();
```

`checkCountdownExpiry()` in `05_connectivity_cloud.ino` does:
```cpp
Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
```

Calling a Firebase RTDB write from inside the sensor block (which runs every 1 second) while the Firebase sync block (which runs every 3 seconds) also uses `fbdo` is **not safe with Firebase-ESP-Client**. The library is not designed for concurrent use of the same `FirebaseData` object from interleaved call sites. On the cycle where both the sensor block's `checkCountdownExpiry()` fires AND the Firebase sync block runs, both will attempt to use `fbdo` simultaneously within the same loop iteration.

In practice this mostly works because `loop()` is single-threaded, but the interleaving within a single loop() pass creates the following call sequence on an expiry+firebase cycle:

```
loop():
  sensor block fires → checkCountdownExpiry() → fbdo.setString("AUTO")  [fbdo used]
  firebase block fires → readFirebaseControl() → fbdo.getString("mode")   [fbdo used again]
  firebase block continues → pushFirebaseStatus() → fbdo.setJSON(status) [fbdo used again]
```

Three `fbdo` operations in one loop() pass with no reset between them is risky and has caused subtle state corruption in Firebase-ESP-Client when `fbdo` is reused without clearing between operations.

**Fix:** Move `checkCountdownExpiry()` **out of the sensor block** and into the Firebase sync block, called before `readFirebaseControl()`:

In `smart_water_pump_controller.ino`, **remove** the call from the sensor block (lines 304–305) and add it inside the Firebase sync block:

```cpp
if (firebaseCooldownUntilMs == 0 && WiFi.status() == WL_CONNECTED && Firebase.ready()) {
  checkCountdownExpiry();        // ← move here, before readFirebaseControl()
  if (lastDeviceConfigMs == 0 || ...) { readDeviceConfigFromFirebase(); }
  readFirebaseControl();
  pushFirebaseStatus();
}
```

The expiry logic itself (the `millis() >= countdownEndMs` check and local state changes) is safe anywhere. Only the Firebase write needs to be in the Firebase block. If Firebase is unavailable when the countdown expires, the local state still transitions correctly (`pumpMode = "AUTO"`, pump stops) and `pendingModeWriteback = true` ensures the write-back is retried on the next successful Firebase cycle.

---

### Issue 4 — 🟡 Significant: `statusRetryDue` resets `lastFirebaseMs` unconditionally, corrupting the normal interval

**File:** `smart_water_pump_controller.ino`, lines 312–315

```cpp
bool statusRetryDue = (statusPushRetryCount > 0 && ...);
if (now - lastFirebaseMs >= firebaseInterval || statusRetryDue) {
  lastFirebaseMs = now;   // ← always updated, even on a retry-triggered entry
```

When `statusRetryDue` triggers this block (e.g. 1 second after a failed push), `lastFirebaseMs` is reset to `now`. This means the next **normal** Firebase cycle is delayed by a full `firebaseInterval` (3s) from the retry timestamp, not from the last successful cycle. If retries happen 3 times over 3 seconds, the normal interval effectively shifts forward by 3 seconds each time — the dashboard sees a 6–9 second gap instead of 3 seconds during recovery.

**Fix:** Only reset `lastFirebaseMs` when the **normal interval** triggered the block, not when a retry triggered it:

```cpp
bool normalIntervalDue = (now - lastFirebaseMs >= firebaseInterval);
bool statusRetryDue    = (statusPushRetryCount > 0 &&
                          statusPushRetryCount < STATUS_PUSH_RETRY_MAX &&
                          now - statusPushRetryMs >= STATUS_PUSH_RETRY_MS);

if (normalIntervalDue || statusRetryDue) {
  if (normalIntervalDue) lastFirebaseMs = now;   // only reset on normal interval
  ...
}
```

---

### Issue 5 — 🟡 Significant: `runMode` derivation has a gap for `COUNTDOWN` + `!isCountdownActive`

**File:** `03_safety_pump.ino`, lines 198–214

```cpp
if (isDryRunError || isOverflowError)             runMode = "OFF";
else if (pumpMode == "FORCE_OFF")                 runMode = "OFF";
else if (pumpMode == "FORCE_ON")                  runMode = "MANUAL";
else if (pumpMode == "COUNTDOWN" && isCountdownActive) runMode = "COUNTDOWN";
else if (pumpMode == "AUTO" && isRunning)         runMode = "AUTO";
else if (pumpMode == "AUTO" && !isRunning)        runMode = "AUTO_STANDBY";
else if (!isRunning)                              runMode = "OFF";
else                                              runMode = "AUTO";
```

When `pumpMode == "COUNTDOWN"` and `isCountdownActive == false` (the brief window between expiry/stop and Firebase write-back confirmation), none of the explicit branches match. It falls through to `else if (!isRunning)` → `"OFF"` or `else` → `"AUTO"`. This causes the dashboard to briefly flicker between `"COUNTDOWN"` → `"OFF"` → `"AUTO_STANDBY"` during normal expiry.

The `pendingModeWriteback` flag prevents `pumpMode` from being overwritten, but `runMode` still derives from the current `pumpMode` value. When `pumpMode` is stuck at `"COUNTDOWN"` with `isCountdownActive = false`, the runMode flicker is visible on the dashboard.

**Fix:** Add an explicit branch for this transitional state:

```cpp
else if (pumpMode == "COUNTDOWN" && !isCountdownActive) runMode = "OFF";
```

Insert this immediately after the `isCountdownActive` branch. This makes the dashboard show a stable `"OFF"` during the write-back window instead of flickering.

---

### Issue 6 — 🟡 Significant: `NVS_SCHEMA_VERSION` check is one-directional

**File:** `04_persistence.ino`, lines 44–47

```cpp
if (schemaVer > NVS_SCHEMA_VERSION) {
  Serial.println("[NVS] Schema version from newer firmware. Using defaults.");
  return;
}
```

This correctly rejects NVS written by a newer firmware. But it does **not** handle the case where `schemaVer < NVS_SCHEMA_VERSION` — i.e., NVS was written by an older firmware that didn't know about `auto_bypass_en` or `cd_dur_min`. In that case, the old NVS is loaded as-is, and the new keys are simply missing (they get their defaults from `prefs.getBool("auto_bypass_en", false)` etc., which works correctly due to the default argument). So this is **not a crash**, but it means a device upgraded from pre-v3.0 firmware silently uses defaults for new keys without logging it.

Not a safety issue since defaults are safe, but worth noting. A future migration path (e.g. clearing old keys) should be added when `NVS_SCHEMA_VERSION` is incremented.

**Current state:** Safe as-is. Add a log line for the `schemaVer < NVS_SCHEMA_VERSION` case:

```cpp
if (schemaVer < NVS_SCHEMA_VERSION) {
  Serial.printf("[NVS] Schema v%d loaded into firmware v%d — new fields use defaults.\n",
                schemaVer, NVS_SCHEMA_VERSION);
  // No return — continue loading; new keys fall back to default args in prefs.getX()
}
```

---

### Issue 7 — 🟡 Significant: `mode` read timeout in `readFirebaseControl()` enters 30s cooldown for ALL error types

**File:** `05_connectivity_cloud.ino`, lines 234–238

```cpp
} else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
  unsigned long now = millis();
  firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 30000UL);
  Serial.println("[FIREBASE] Network timeout; cooling down 30s.");
}
```

This is in `readFirebaseControl()` (the **mode read** specifically), not in `pushFirebaseStatus()`. A single timeout reading the mode key enters a 30-second cooldown. During that cooldown, ALL Firebase operations stop — including `readFirebaseControl()`, `pushFirebaseStatus()`, and `readDeviceConfigFromFirebase()`. The dashboard goes dark for 30 seconds on any single mode-read timeout.

By contrast, the fix applied to `pushFirebaseStatus()` correctly only enters cooldown after `STATUS_PUSH_RETRY_MAX` (3) consecutive failures. The same protection was not applied to the mode read in `readFirebaseControl()`.

**Fix:** Apply the same consecutive-failure guard to the mode read timeout:

```cpp
} else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
  if (firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX) {
    unsigned long now = millis();
    firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 30000UL);
    Serial.println("[FIREBASE] Mode read timeout; cooling down 30s.");
  } else {
    Serial.printf("[FIREBASE] Mode read timeout; retrying (%d/%d).\n",
                  (int)firebaseConsecutiveFailCount, STATUS_PUSH_RETRY_MAX);
  }
}
```

---

### Issue 8 — 🟡 Significant: `setPump(false)` called by `checkCountdownExpiry()` before `executePumpLogic()` — double stop on same cycle

**File:** `05_connectivity_cloud.ino`, lines 156–167 + `smart_water_pump_controller.ino` loop

When the countdown expires:
1. `checkCountdownExpiry()` calls `setPump(false)` and sets `pumpMode = "AUTO"`
2. On the same sensor cycle, `executePumpLogic()` runs immediately after
3. `executePumpLogic()` P4 branch: `pumpMode == "COUNTDOWN"` is now false (was just changed to `"AUTO"`) → skips P4
4. Reaches P5 AUTO hysteresis: `!isRunning && waterLevelPct <= cfgPumpStartLevel` → **starts pump again if level is low**

This means if the tank level is ≤ 30% when the countdown expires, the pump stops and immediately restarts in AUTO mode within the same sensor cycle. This is actually **correct behaviour** — the countdown ended, AUTO takes over, and AUTO says to run. But it may surprise the user who expected the pump to stop at countdown end.

However, there is a subtle double-stop bug: `setPump(false)` is called in `checkCountdownExpiry()`, which updates `pumpOnSinceMs` and `totalPumpRunSec`. Then if `executePumpLogic()` immediately calls `setPump(true)`, the cycle counter increments again (`totalPumpCycles++`). The pump goes: stop → start in the same millisecond. This creates a phantom "cycle" in the telemetry that didn't correspond to a real pump on/off.

**Fix:** `checkCountdownExpiry()` should not call `setPump(false)` directly. It should only update state (`isCountdownActive`, `countdownEndMs`, `pumpMode`, `pendingModeWriteback`) and let `executePumpLogic()` handle the actual relay state on the same cycle. This is the correct separation of concerns.

Change `checkCountdownExpiry()`:
```cpp
void checkCountdownExpiry() {
  if (!isCountdownActive || pumpMode != "COUNTDOWN") return;
  if (millis() >= countdownEndMs) {
    Serial.println("[COUNTDOWN] Timer expired. Reverting to AUTO mode.");
    isCountdownActive = false;
    countdownEndMs = 0;
    // Do NOT call setPump(false) here — let executePumpLogic() decide based on new pumpMode
    pumpMode = "AUTO";
    runMode  = "AUTO_STANDBY";  // pre-set to avoid flicker; executePumpLogic() will correct
    pendingModeWriteback = true;
    // Firebase write moved to Firebase sync block (Issue 3 fix)
  }
}
```

---

### Issue 9 — 🟡 Significant: `bypass_level_sensor` control read clears `autoBypassActive` and `autoBypassWasEngaged` even when set to `true`

**File:** `05_connectivity_cloud.ino`, lines 330–338

```cpp
if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/bypass_level_sensor")) {
  bool v = fbdo.boolData();
  if (v != cfgBypassLevelSensor) {
    cfgBypassLevelSensor = v;
    autoBypassActive = false;         // ← cleared regardless of direction
    autoBypassWasEngaged = false;     // ← cleared regardless of direction
    ...
  }
}
```

When a user manually enables bypass (`v = true`) while auto-bypass is already active (`autoBypassActive = true`), this clears `autoBypassWasEngaged`. The result: when the sensor later recovers, `checkLevelSensorFailure()` checks `autoBypassWasEngaged` to decide whether to auto-disable bypass:

```cpp
if (autoBypassWasEngaged) {
  cfgBypassLevelSensor = false;  // auto-disable
  ...
}
```

Since `autoBypassWasEngaged` was cleared, the sensor recovery does NOT auto-disable bypass. The operator has to manually disable it. This is arguably correct (manual override takes precedence), but it silently defeats the auto-recovery behavior without any log message.

The dangerous opposite: if the user manually sets `bypass = false` while `autoBypassWasEngaged` was true (they are overriding the auto-bypass), `autoBypassWasEngaged` gets cleared too. Now if the sensor fails again, `cfgAutoBypassOnSensorFail` will re-trigger auto-bypass correctly since `cfgBypassLevelSensor` is false and `autoBypassWasEngaged` is false.

**Fix:** Only clear `autoBypassActive` and `autoBypassWasEngaged` when bypass is being **disabled** (v = false), not when being enabled:

```cpp
if (v != cfgBypassLevelSensor) {
  cfgBypassLevelSensor = v;
  if (!v) {
    // Manual disable — clear auto-bypass tracking
    autoBypassActive     = false;
    autoBypassWasEngaged = false;
  }
  Serial.printf("[FIREBASE] Bypass level sensor: %s\n", v ? "ON" : "OFF");
}
```

---

### Issue 10 — 🟢 Minor: `runMode = "AUTO"` set in `checkCountdownExpiry()` is immediately overwritten by `executePumpLogic()`

**File:** `05_connectivity_cloud.ino`, line 164

```cpp
runMode = "AUTO";
```

`executePumpLogic()` re-derives `runMode` from scratch at its top. Setting it in `checkCountdownExpiry()` has no lasting effect — it's overwritten 1–2 lines later when `executePumpLogic()` runs. This line is harmless dead code currently.

With the fix from Issue 8 (not calling `setPump(false)` in `checkCountdownExpiry()`), this line also becomes irrelevant. Remove it to avoid confusion:

```cpp
// Remove: runMode = "AUTO";
// executePumpLogic() will derive it correctly from pumpMode = "AUTO"
```

---

### Issue 11 — 🟢 Minor: `delay(100)` during WiFi reconnect blocks the sensor cycle

**File:** `smart_water_pump_controller.ino`, line 164

```cpp
WiFi.disconnect(false);
delay(100);   // ← blocking
WiFi.mode(WIFI_STA);
```

This 100ms delay runs inside `loop()` on every WiFi reconnect attempt. Since reconnect attempts happen up to every 5 seconds (backoff), and the sensor interval is 1 second, this creates a 100ms blind spot per attempt. More importantly, it blocks the WDT reset path (though WDT is 120s so not a risk here). It's minor but worth removing:

```cpp
WiFi.disconnect(false);
WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
```

The 100ms was likely added to let the WiFi stack settle, but `WiFi.begin()` is non-blocking and handles this internally.

---

## Summary

### Previously Fixed Issues — All Confirmed ✅
14 of 14 fixes verified as correctly implemented.

### New Issues Found in This Pass

| # | Severity | File | Description |
|---|---|---|---|
| 1 | 🔴 Critical | `05_connectivity_cloud.ino` | `pendingModeWriteback` re-sends Firebase write every cycle — needs rate limiting |
| 2 | 🔴 Critical | `05_connectivity_cloud.ino` | `countdownConsumed` reset should be tied to writeback confirmation, not independent Firebase read |
| 3 | 🔴 Critical | `smart_water_pump_controller.ino` | `checkCountdownExpiry()` called in sensor block but contains Firebase write — `fbdo` reuse risk |
| 4 | 🟡 Significant | `smart_water_pump_controller.ino` | `lastFirebaseMs` reset on retry corrupts normal 3s interval timing |
| 5 | 🟡 Significant | `03_safety_pump.ino` | `runMode` has no branch for `COUNTDOWN` + `!isCountdownActive` — dashboard flickers |
| 6 | 🟡 Significant | `04_persistence.ino` | `schemaVer < NVS_SCHEMA_VERSION` case not logged — silent on firmware upgrades |
| 7 | 🟡 Significant | `05_connectivity_cloud.ino` | Mode read timeout enters 30s cooldown on first failure — same bug that was fixed in pushStatus |
| 8 | 🟡 Significant | `05_connectivity_cloud.ino` | `setPump(false)` in `checkCountdownExpiry()` creates phantom telemetry cycle if AUTO restarts pump |
| 9 | 🟡 Significant | `05_connectivity_cloud.ino` | `bypass_level_sensor` write clears `autoBypassWasEngaged` even on enable — defeats auto-recovery |
| 10 | 🟢 Minor | `05_connectivity_cloud.ino` | `runMode = "AUTO"` in `checkCountdownExpiry()` is dead code — overwritten by `executePumpLogic()` |
| 11 | 🟢 Minor | `smart_water_pump_controller.ino` | `delay(100)` during WiFi reconnect blocks sensor cycle unnecessarily |

### Priority Fix Order
1. **Issue 3** first — `fbdo` reuse between sensor block and Firebase block is the most structurally risky. Moving `checkCountdownExpiry()` into the Firebase block also simplifies Issues 1, 2, and 8.
2. **Issues 1 + 2** together — both involve `pendingModeWriteback` and `countdownConsumed`; fix them in the same edit.
3. **Issue 7** — single mode-read timeout should not cause 30s dashboard blackout.
4. **Issue 4** — retry logic corrupting normal interval.
5. **Issues 5, 8, 9** — state correctness fixes.
6. **Issues 6, 10, 11** — minor cleanups.
