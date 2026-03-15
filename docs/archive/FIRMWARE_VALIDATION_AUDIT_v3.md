# Firmware Validation Audit — v3.0.2
## Smart Water Pump Controller — Full Pass After Latest Fixes

---

## Part 1 — Verification Matrix

Every issue from every prior audit pass, checked against the actual code.

| ID | Issue | Fix Expected | File:Line | Result |
|---|---|---|---|---|
| Bug 1 | Stop restarts countdown | `pendingModeWriteback` flag | 05:259 | ✅ |
| Bug 2 | Expiry restarts countdown | Same flag, `pendingModeWritebackSentMs=0` in expiry | 05:165-166 | ✅ |
| Bug 3 | Add-time loops | `setBool false` + `lastAddTime=false` on new start | 05:309, 05:296 | ✅ |
| Old #2 | COUNTDOWN dropped from NVS restore | Added to validation list | 04:207 | ✅ |
| Old #3 | `WiFi.disconnect(true)` | `disconnect(false)`, no `delay(100)` | main:163 | ✅ |
| Old #4 | Overflow not guarding COUNTDOWN | `pumpMode=="AUTO"||pumpMode=="COUNTDOWN"` | 03:118 | ✅ |
| Old #5 | `runMode` init to `"AUTO"` | Initialized to `"OFF"` | 01:128 | ✅ |
| Old #6 | `auto_bypass` not in NVS | `auto_bypass_en`/`auto_bypass_sec` load+save | 04:38-39,102-103 | ✅ |
| Old #7 | `-1` sentinel pushed raw | `if (estimatedLevelPct >= 0.0f)` guard | 05:441 | ✅ |
| Old #10 | NVS schema unversioned | `NVS_SCHEMA_VERSION=1`, written+checked | 04:44,108 | ✅ |
| Old #11 | `WiFi.persistent(true)` | `WiFi.persistent(false)` | 05:497 | ✅ |
| A2 #1 | Write-back storms Firebase | `pendingModeWritebackSentMs` 5s rate-limit | 05:211-214 | ✅ |
| A2 #2 | `countdownConsumed` reset wrong | Reset inside writeback confirmation when `pumpMode=="AUTO"` | 05:209 | ✅ |
| A2 #3 | `checkCountdownExpiry` had Firebase write in sensor block | Firebase write removed, pure state change only | 05:158-168 | ✅ |
| A2 #4 | `lastFirebaseMs` reset on retry | `if (normalIntervalDue) lastFirebaseMs=now` | main:315 | ✅ |
| A2 #5 | `runMode` gap: COUNTDOWN+!isCountdownActive | `else if (COUNTDOWN && !isCountdownActive) runMode="OFF"` | 03:206-207 | ✅ |
| A2 #6 | `schemaVer<NVS_SCHEMA_VERSION` not logged | Log line added | 04:48-51 | ✅ |
| A2 #7 | Mode read timeout → 30s cooldown on 1st fail | `>= STATUS_PUSH_RETRY_MAX` guard | 05:238 | ✅ |
| A2 #8 | `setPump(false)` in `checkCountdownExpiry` phantom cycle | Removed — pure state change only | 05:158-168 | ✅ |
| A2 #9 | `bypass` enable clears `autoBypassWasEngaged` | Only cleared on `!v` (disable) path | 05:344-347 | ✅ |
| A2 #10 | Dead `runMode="AUTO"` in expiry | Removed | 05:158-168 | ✅ |
| A2 #11 | `delay(100)` in WiFi reconnect | Removed | main:163-165 | ✅ |
| A3 #4 | P4 early-stop: `Firebase.RTDB.setString` in sensor block | Removed; `pendingModeWritebackSentMs=0` | 03:257 | ✅ |
| Offline | `cfgLastCountdownDurationMin` NVS persist | Saved on start + loaded in `loadStateFromNVS` | 05:283-288, 04:204 | ✅ |
| Offline | Status push retry | `statusRetryDue` + `normalIntervalDue` split | main:311-315 | ✅ |
| Offline | 30s cooldown only after 3 consecutive failures | `>= STATUS_PUSH_RETRY_MAX` guard in push | 05:479 | ✅ |

**All 27 prior issues confirmed fixed.**

---

## Part 2 — New Issues Found

---

### Issue 1 — 🔴 Critical: `countdownConsumed` reset at line 274 still fires BEFORE `manual_stop` is processed

**File:** `05_connectivity_cloud.ino`, execution order lines 249–276

```
readFirebaseControl() execution order:
  1. Mode read (line 183)     → firebaseReadMode set
  2. manual_stop (line 250)   → pumpMode = "AUTO" set HERE
  3. countdownConsumed reset (line 274) → checks firebaseReadMode (not pumpMode)
  4. Countdown start block (line 278)   → checks pumpMode
```

The `countdownConsumed` reset at line 274 fires based on `firebaseReadMode`, which was captured at step 1 (before `manual_stop` ran). Consider:

- Countdown running. Firebase: `mode = "COUNTDOWN"`. `countdownConsumed = true`.
- User taps Stop. Dashboard writes `manual_stop = true` AND `mode = "AUTO"` in the same dashboard action.
- Firebase cycle: mode read returns `"AUTO"` (dashboard already wrote it). `firebaseReadMode = "AUTO"`.
- Step 2: `manual_stop` fires → `pumpMode = "AUTO"`, `pendingModeWriteback = true`.
- Step 3: `firebaseReadMode = "AUTO"` (not "COUNTDOWN") → **`countdownConsumed = false`** ← RESETS IMMEDIATELY.
- Step 4: `pumpMode == "COUNTDOWN"` → false (was just set to "AUTO") → start block does not fire. ✓

This path is **safe** because `pumpMode` was already set to "AUTO" by `manual_stop` before the start block check.

**But consider the FORCE_OFF path:** Dashboard writes `mode = "FORCE_OFF"` to stop an active countdown.
- Mode read: `firebaseReadMode = "FORCE_OFF"`.
- `runActive = (pumpMode=="COUNTDOWN" && isCountdownActive)` → true.
- `runActive && newMode == "FORCE_OFF"` → stop handler fires: `pumpMode = "FORCE_OFF"`, `isCountdownActive = false`.
- `countdownConsumed` reset: `firebaseReadMode = "FORCE_OFF"` (not "COUNTDOWN") → **`countdownConsumed = false`**.
- Start block: `pumpMode == "COUNTDOWN"` → false. Does not fire. ✓

The FORCE_OFF path is also safe because `pumpMode` changed from COUNTDOWN in step 1.

**The dangerous scenario:** Dashboard writes `manual_stop = true` but does NOT write `mode = "AUTO"` (relies on firmware to revert). Firebase: `mode` is still `"COUNTDOWN"`. This is the original sequence from the Bug 1 fix.

- Mode read: `firebaseReadMode = "COUNTDOWN"`. `pendingModeWriteback = true` (from previous cycle's manual_stop) → suppresses overwrite. ✓
- `manual_stop`: `lastManualStop = true` from last cycle → edge does not re-fire. ✓
- `countdownConsumed` reset: `firebaseReadMode = "COUNTDOWN"` → does NOT reset. ✓
- Writeback confirmation: when Firebase eventually returns `"AUTO"`, `pendingModeWriteback` clears, `countdownConsumed = false`. ✓

**Verdict:** The logic is correct for all paths. The concern in the prior audit was unfounded. No bug here.

---

### Issue 2 — 🔴 Critical: `pendingModeWriteback` confirmation check clears `countdownConsumed` even when Firebase confirms a NON-AUTO mode

**File:** `05_connectivity_cloud.ino`, line 206–209

```cpp
if (newMode == pumpMode) {
  pendingModeWriteback = false;
  pendingModeWritebackSentMs = 0;
  if (pumpMode == "AUTO") countdownConsumed = false;  // guarded ✓
  Serial.println("[FIREBASE] Mode write-back confirmed.");
}
```

The `if (pumpMode == "AUTO")` guard is present — `countdownConsumed` is only cleared when the confirmed mode is "AUTO". This is correct. If the write-back was for some other mode (e.g., FORCE_OFF), `countdownConsumed` is NOT cleared. ✓

**Verdict:** Correctly implemented. Not a bug.

---

### Issue 3 — 🔴 Critical: P4 early-stop `pendingModeWritebackSentMs = 0` with no Firebase write means first retry waits for NEXT Firebase cycle (up to 3s) — is this acceptable?

**File:** `03_safety_pump.ino`, lines 255–258

```cpp
pumpMode = "AUTO";
pendingModeWriteback = true;
pendingModeWritebackSentMs = 0;   // send on next Firebase cycle
// No Firebase write here
return;
```

With `pendingModeWritebackSentMs = 0`, the retry condition in the writeback block:
```cpp
} else if (millis() - pendingModeWritebackSentMs >= 5000UL) {
```
`millis() - 0` is always >> 5000 after boot, so this fires **immediately on the very first Firebase cycle** after expiry. The earliest that cycle can arrive is 0ms later (if the Firebase interval fires in the same loop() pass) or up to 3s later.

So the write-back is deferred by 0–3 seconds depending on when the Firebase interval next fires. During that window, `pendingModeWriteback = true` suppresses any stale `"COUNTDOWN"` read from overwriting the local `"AUTO"`. The pump is correctly stopped by `executePumpLogic()`. The worst case is 3 seconds before Firebase is updated.

**Verdict:** Correct and acceptable. 3s maximum delay before Firebase write-back, with full local protection during the window. ✓

---

### Issue 4 — 🟡 Significant: `checkCountdownExpiry()` still in sensor block — but now safe since no Firebase call remains inside it

**File:** `smart_water_pump_controller.ino`, line 303–304

```cpp
checkCountdownExpiry();   // sensor block — every 1s
```

`checkCountdownExpiry()` now only modifies local state (`isCountdownActive`, `countdownEndMs`, `pumpMode`, `pendingModeWriteback`, `pendingModeWritebackSentMs`). No Firebase calls. The `fbdo` concern from Audit2 Issue 3 is resolved.

Keeping it in the sensor block means the countdown expires and `pumpMode` switches to `"AUTO"` within 1 second of the timer firing, rather than waiting up to 3 seconds for the Firebase cycle. `executePumpLogic()` then immediately stops the pump. This is correct and better than moving it to the Firebase block.

**Verdict:** ✅ Correct placement. No issue.

---

### Issue 5 — 🟡 Significant: `runActive` check uses `runMode == "MANUAL"` — but `runMode` is derived in `executePumpLogic()` which runs in the SENSOR block, not the Firebase block

**File:** `05_connectivity_cloud.ino`, line 190

```cpp
bool runActive = (runMode == "MANUAL" || (pumpMode == "COUNTDOWN" && isCountdownActive));
```

`executePumpLogic()` is called in the sensor block (every 1s). `readFirebaseControl()` is called in the Firebase block (every 3s). The sequence in a given loop() pass where both intervals fire:

1. Sensor block fires first → `executePumpLogic()` → `runMode` updated.
2. Firebase block fires second → `readFirebaseControl()` → `runActive` checks `runMode`.

Since the sensor block runs before the Firebase block in the same loop() pass (sensor block is `if (now - lastSensorMs >=...)` at line 276, Firebase block is at line 314, both checked sequentially), `runMode` IS current when `runActive` is evaluated. ✓

But on Firebase-only cycles (sensor interval not yet due), `runMode` was last updated in a previous sensor cycle. It could be up to 1s stale. In practice, for MANUAL mode, the pump is either running or not — the 1s staleness doesn't create a bug because `executePumpLogic()` already ran and set the relay correctly.

**Verdict:** Correct. No issue.

---

### Issue 6 — 🟡 Significant: `setPump(false)` called at boot via `setup()` before flow ISR is attached — `pumpOffStartMs` set, `totalPumpRunSec` accumulation logic runs with `pumpOnSinceMs = 0`

**File:** `smart_water_pump_controller.ino`, line 29; `03_safety_pump.ino`, lines 14–16

```cpp
// setup():
setPump(false);   // called before attachInterrupt and before loadStateFromNVS
```

```cpp
// setPump():
if (!on && isRunning && pumpOnSinceMs > 0) {
  totalPumpRunSec += (millis() - pumpOnSinceMs) / 1000UL;
}
```

At boot, `isRunning = false` (initialized in `01_config.ino`), so the runtime accumulation block `(!on && isRunning && pumpOnSinceMs > 0)` does NOT fire. ✓

`pumpOffStartMs = millis()` IS set by `setPump(false)` at boot. This means `pumpOffStartMs` starts non-zero from boot. In `calculateFlowRate()`:

```cpp
if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) > FLOW_PUMP_OFF_ZERO_MS) {
  return 0.0f;
}
```

After 3 seconds (`FLOW_PUMP_OFF_ZERO_MS`), all flow readings are zeroed — correct behavior since the pump has been off since boot. ✓

`totalPumpCycles` is NOT incremented at boot because `on=false` branch does not hit the `if (on && !isRunning)` increment. ✓

**Verdict:** Boot-time `setPump(false)` is safe. No issue.

---

### Issue 7 — 🟡 Significant: `waterLevelPct = 0` at boot — EMA initialization check uses `waterLevelPct == 0` as condition

**File:** `02_sensors.ino`, lines 99–103

```cpp
if (waterLevelEma < 0.1f && waterLevelPct == 0) {
  waterLevelEma = levelFloat;   // cold start: seed EMA directly
} else {
  waterLevelEma = ULTRASONIC_EMA_ALPHA * levelFloat + (1.0f - ULTRASONIC_EMA_ALPHA) * waterLevelEma;
}
```

This seeds the EMA from the first valid reading on boot (when `waterLevelPct == 0` and `waterLevelEma < 0.1`). On subsequent readings, it applies EMA smoothing. **Problem:** `waterLevelPct` is loaded from NVS in `loadStateFromNVS()` — but `waterLevelPct` itself is NOT restored from NVS (only `savedLevel` is read from NVS and used in the Serial.printf log, but NOT assigned to `waterLevelPct`).

Looking at `loadStateFromNVS()` (04:192–222): `savedLevel` is read but not used to set `waterLevelPct`. So `waterLevelPct` always starts as `0` regardless of NVS. This means on every boot, the EMA cold-start condition always fires on the first reading — which is correct behavior (you want to seed EMA from first real reading, not stale NVS data).

**Verdict:** Correct by design. `waterLevelPct = 0` + `waterLevelEma < 0.1f` on boot → EMA seeds from first reading. No issue.

---

### Issue 8 — 🟡 Significant: `firebaseConsecutiveFailCount = 0` reset on successful mode read (line 184) but NOT reset on successful manual_stop/bypass/clear_error reads

**File:** `05_connectivity_cloud.ino`, lines 183–184, 250–268, 323–337, 340–350, 353–369

```cpp
if (Firebase.RTDB.getString(&fbdo, "/pump_system/control/mode")) {
  firebaseConsecutiveFailCount = 0;   // ← reset only here
```

The counter is reset when the mode read succeeds. But if the mode read succeeds and subsequent reads (manual_stop, manual_start, bypass, clear_error) fail, `firebaseConsecutiveFailCount` goes back up. The counter is also reset in `pushFirebaseStatus()` on success (line 458). So the counter represents the tail of consecutive failures — not a lifetime total.

On a bad network cycle where mode read succeeds but 2 subsequent reads fail:
- Mode read success: `firebaseConsecutiveFailCount = 0`
- `manual_stop` getBool failure: `firebaseConsecutiveFailCount++` → 1
- `manual_start` getBool failure: `firebaseConsecutiveFailCount++` → 2
- Threshold is 3 → no cooldown triggered yet. ✓

This is correct behavior. The counter is reset per successful operation and each individual read that fails increments it. The cooldown only fires after 3 consecutive failures without any success.

**Verdict:** Correct. The design intentionally counts failures across all Firebase operations in a cycle.

---

### Issue 9 — 🟡 Significant: `setPump(false)` inside P1 handler in `executePumpLogic()` sets `lastFaultCode` AFTER calling `setPump(false)` — telemetry logged with old fault code for one cycle

**File:** `03_safety_pump.ino`, lines 218–232

```cpp
if (isDryRunError || isOverflowError) {
  if (isDryRunError) {
    lastFaultCode = "DRY_RUN";          // set AFTER relay change
    lastFaultMessage = "...";
  } else {
    lastFaultCode = "OVERFLOW";
    lastFaultMessage = "...";
  }
  setPump(false);
```

Wait — `lastFaultCode` is set BEFORE `setPump(false)` (line 219–225 run before line 227). Re-reading:

```cpp
// P1:
if (isDryRunError || isOverflowError) {
  if (isDryRunError) {           // line 220
    lastFaultCode = "DRY_RUN";  // line 221 ← SET FIRST
    lastFaultMessage = "...";   // line 222
  } else {
    lastFaultCode = "OVERFLOW"; // line 224
    lastFaultMessage = "...";   // line 225
  }
  setPump(false);               // line 227 ← THEN relay
```

`lastFaultCode` is set before `setPump(false)`. ✓ The status push will include the correct fault code.

**Verdict:** Not a bug. Order is correct.

---

### Issue 10 — 🟡 Significant: `runMode` derivation at top of `executePumpLogic()` runs BEFORE P1 clears countdown state — `isCountdownActive` could be true when `runMode = "OFF"` is derived

**File:** `03_safety_pump.ino`, lines 198–232

```cpp
// runMode derivation:
if (isDryRunError || isOverflowError) {
  runMode = "OFF";        // line 199
}
...
// P1 block:
if (isDryRunError || isOverflowError) {
  ...
  setPump(false);
  if (isCountdownActive) {        // line 228
    isCountdownActive = false;    // line 229 ← CLEARED HERE
    countdownEndMs = 0;
  }
  return;
}
```

When a P1 error occurs during a countdown:
1. `runMode` derivation: `isDryRunError` → `runMode = "OFF"` (correct — P1 error overrides).
2. P1 block: `isCountdownActive = false` (clears countdown state).
3. `executePumpLogic()` returns.

On the NEXT call to `executePumpLogic()` (next sensor cycle), `isCountdownActive` is now `false` and `pumpMode` is still `"COUNTDOWN"` (the error doesn't change `pumpMode`). The runMode derivation:
- `COUNTDOWN && !isCountdownActive` → `runMode = "OFF"` ✓ (the new branch added in prior audit).

But wait — when P1 fires, it should also revert `pumpMode` to reflect that the countdown was forcibly terminated. Currently `pumpMode` stays as `"COUNTDOWN"` after a P1 error clears `isCountdownActive`. On subsequent cycles, `executePumpLogic()` hits the P1 path, stops pump, and eventually the user clears the error. After `clear_error`, `pumpMode` is still `"COUNTDOWN"` and `!isCountdownActive` → the countdown start block in `readFirebaseControl()` would re-arm if `countdownConsumed` were false.

Is `countdownConsumed` false at this point? It's a static in `readFirebaseControl()`. It was set `true` when the countdown started. It gets reset to `false` either: (a) when Firebase confirms `"AUTO"` via `pendingModeWriteback`, or (b) when `firebaseReadMode != "COUNTDOWN"`.

After a P1 error: `pumpMode = "COUNTDOWN"`, `pendingModeWriteback = false` (no writeback was set). Firebase still has `mode = "COUNTDOWN"`. `firebaseReadMode = "COUNTDOWN"` on every cycle → `countdownConsumed` stays `true`. When the user clears the error, they typically tap "AUTO" or the mode reverts — but if they don't change mode, `pumpMode` stays `"COUNTDOWN"` and `countdownConsumed` stays `true` indefinitely. The countdown never re-arms. ✓

But if the device reboots after a P1 error (e.g., power cut), `countdownConsumed` resets to `false` (static local, re-initialized on boot). `pumpMode` is restored as `"COUNTDOWN"` from NVS. `isCountdownActive = false`. On the next Firebase cycle: `pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed` → **countdown re-arms with last known duration**.

This is debatable behavior. After a dry-run lockout, the pump restarting in countdown mode on reboot is potentially undesirable — the lockout (`isDryRunError`) was also persisted to NVS and will be restored, so P1 will immediately stop the pump again. Safe, but confusing (pump tries to start, immediately stops with dry-run error).

**Fix:** In P1 handling, set `pendingModeWriteback = true` and `pendingModeWritebackSentMs = 0` to write `"AUTO"` back to Firebase, and set `pumpMode = "AUTO"` locally. This prevents the countdown from re-arming after error and ensures Firebase reflects the correct state.

Add to P1 block in `executePumpLogic()`, after clearing `isCountdownActive`:
```cpp
if (isCountdownActive) {
  isCountdownActive = false;
  countdownEndMs = 0;
  pumpMode = "AUTO";          // ADD: prevent countdown re-arm after error
  pendingModeWriteback = true;
  pendingModeWritebackSentMs = 0;
}
```

---

### Issue 11 — 🟢 Minor: `delay(100)` before `ESP.restart()` in reboot handler (line 385) — two Firebase writes before it may block up to 20s total

**File:** `05_connectivity_cloud.ino`, lines 383–386

```cpp
Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_request_id", lastRebootRequestId);
Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_at", ...);
delay(100);
ESP.restart();
```

Each `setInt` has a 10s read timeout (`Firebase.RTDB.setReadTimeout(&fbdo, 10000)`). If both fail (network issue), the reboot could take up to 20 seconds. The WDT is 120s so no reset risk. The `delay(100)` adds nothing here.

**Verdict:** Harmless. Not a runtime bug.

---

### Issue 12 — 🟢 Minor: `lastSensorTelemetryLogMs` initialized to `0` — first telemetry window resets immediately on boot

**File:** `01_config.ino`, line 77

```cpp
unsigned long lastSensorTelemetryLogMs = 0;
```

On boot, `now - lastSensorTelemetryLogMs >= 60000` → `millis() - 0 >= 60000` is false for the first 60 seconds. So the first telemetry window correctly fires after 60 seconds. ✓

If there's a reboot within 60 seconds (crash loop scenario), this is fine — the telemetry window just resets.

**Verdict:** Correct. No issue.

---

### Issue 13 — 🟢 Minor: Offline-first gap — `readFirebaseControl()` still makes 8 individual RTDB reads per cycle

**File:** `05_connectivity_cloud.ino`, lines 183–388

8 separate `Firebase.RTDB.getString/getBool/getInt` calls:
1. `mode` (getString)
2. `manual_stop` (getBool)
3. `countdown_duration_min` (getInt) — conditional
4. `countdown_add_time` (getBool) — conditional
5. `manual_start` (getBool)
6. `bypass_level_sensor` (getBool)
7. `clear_error` (getBool)
8. `reboot_request_id` (getInt)

Each is a separate network round-trip. At -79 dBm RSSI, each has ~10–15% probability of timeout. With 8 reads, probability of at least one timeout per cycle is `1 - (0.87^8) ≈ 68%`. This means on a weak signal cycle, `firebaseConsecutiveFailCount` will likely increment every cycle, and after 3 cycles the 30s cooldown fires.

A single `getJSON("/pump_system/control")` reduces this to 1 round-trip with one pass/fail outcome. This is the single highest-impact remaining reliability improvement.

**This is not a new bug** — it was identified in the prior audit as the remaining offline-first gap. Documenting here for completeness.

---

## Part 3 — Full Offline-First Architecture Verification

The offline-first design requires four properties. Checking each:

**1. NVS is the authority for critical state across reboots:**
- `pumpMode` ✅ — saved on change, loaded on boot
- `isDryRunError` ✅ — saved on change, loaded on boot
- `cfgBypassLevelSensor` ✅ — saved on change, loaded on boot
- `cfgLastCountdownDurationMin` ✅ — saved on countdown start, loaded on boot
- `totalPumpCycles`, `totalPumpRunSec` ✅ — saved on change
- All device config ✅ — saved when Firebase updates, loaded on boot
- `cfgAutoBypassOnSensorFail`, `cfgAutoBypassDelaySec` ✅ — added in prior fix

**2. Firebase is a sync layer, not the control plane:**
- Sensor loop, safety checks, countdown timer, relay control: all run on local variables ✅
- Firebase reads update local state; Firebase writes reflect local state ✅
- Pump does not wait for Firebase before acting ✅

**3. Control loop is fully decoupled from network:**
- `executePumpLogic()` has zero Firebase calls ✅ (P4 direct write removed in prior audit)
- `checkCountdownExpiry()` has zero Firebase calls ✅ (removed in prior audit)
- `checkSafetyCutoff()` has zero Firebase calls ✅
- Countdown timer runs on `millis()` exclusively ✅

**4. Commands survive network outages:**
- `pendingModeWriteback` queue for mode write-backs ✅
- Offline countdown start with `cfgLastCountdownDurationMin` ✅
- NVS state survives power cycle ✅
- **Boolean edge-detect one-shots** (not epoch IDs) — **partial** ⚠️
  - Acceptable for current use case (edge fires correctly on reconnect for non-rebooted device)
  - Risk: if device reboots while `manual_start = true` in Firebase, static resets, edge fires on first read → pump starts. This is actually correct behavior.

**Overall offline-first status: Substantially implemented. One remaining improvement (single JSON read).**

---

## Summary

### Prior Issues — All Confirmed ✅
All 27 issues from prior audit passes verified as correctly fixed.

### New Issues Found

| # | Severity | File | Description | Action |
|---|---|---|---|---|
| 10 | 🟡 Significant | `03_safety_pump.ino` | P1 error during countdown doesn't revert `pumpMode` to AUTO — countdown may re-arm after power cycle with dry-run still active | Fix: set `pumpMode="AUTO"` and `pendingModeWriteback=true` in P1 countdown-clear block |
| 13 | 🟢 Minor | `05_connectivity_cloud.ino` | 8 individual RTDB reads — reduces to single JSON for major reliability improvement on weak WiFi | Improvement (not a bug) |

**Only Issue 10 requires a code change.** Issues 1–9, 11–12 were analyzed and confirmed correct or harmless.

### The one required fix

**`03_safety_pump.ino`, P1 handler, countdown clear block** — add `pumpMode = "AUTO"` with writeback after clearing countdown on error:

```cpp
// BEFORE (line 228-231):
if (isCountdownActive) {
  isCountdownActive = false;
  countdownEndMs = 0;
}

// AFTER:
if (isCountdownActive) {
  isCountdownActive = false;
  countdownEndMs = 0;
  pumpMode = "AUTO";             // prevent countdown re-arm after error
  pendingModeWriteback = true;
  pendingModeWritebackSentMs = 0;
}
```

This ensures that if a dry-run or overflow error fires during a countdown, both the local mode and the Firebase mode are correctly reverted to AUTO, preventing a confusing restart-then-immediate-lockout cycle after reboot.
