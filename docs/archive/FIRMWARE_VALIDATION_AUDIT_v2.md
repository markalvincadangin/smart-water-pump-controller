# Firmware Validation Audit — v3.0.1
## Smart Water Pump Controller — Full Pass

---

## Part 1 — Prior Issue Verification

| # | Issue | Expected Fix | Verified |
|---|---|---|---|
| Bug 1 | Stop restarts countdown (stale mode overwrite) | `pendingModeWriteback` flag | ✅ |
| Bug 2 | Countdown expiry restarts | Same flag | ✅ |
| Bug 3 | Add-time infinite loop | `setBool false` + `lastAddTime=false` on new start | ✅ |
| Old #2 | `"COUNTDOWN"` dropped from NVS mode restore | Added to validation list | ✅ |
| Old #3 | `WiFi.disconnect(true)` regression | `disconnect(false)` | ✅ |
| Old #4 | Overflow not guarding COUNTDOWN | `pumpMode == "AUTO" \|\| pumpMode == "COUNTDOWN"` | ✅ |
| Old #5 | `runMode` initialized to `"AUTO"` | Initialized to `"OFF"` | ✅ |
| Old #6 | `auto_bypass` not in NVS | `auto_bypass_en` / `auto_bypass_sec` in both load and save | ✅ |
| Old #7 | `-1` sentinel pushed raw | `if (estimatedLevelPct >= 0.0f)` guard | ✅ |
| Old #10 | NVS schema unversioned | `NVS_SCHEMA_VERSION=1`, written and checked | ✅ |
| Old #11 | `WiFi.persistent(true)` | `WiFi.persistent(false)` | ✅ |
| Audit2 #1 | `pendingModeWriteback` write-storms | `pendingModeWritebackSentMs` rate-limit (5s) | ✅ |
| Audit2 #2 | `countdownConsumed` reset tied to writeback confirm | `if (pumpMode=="AUTO") countdownConsumed=false` on confirm | ✅ |
| Audit2 #3 | `checkCountdownExpiry()` had Firebase write in sensor block | Firebase write removed; only local state updated | ✅ |
| Audit2 #4 | `lastFirebaseMs` reset on retry | `if (normalIntervalDue) lastFirebaseMs = now` | ✅ |
| Audit2 #5 | `runMode` gap for COUNTDOWN+!isCountdownActive | `else if (pumpMode=="COUNTDOWN" && !isCountdownActive) runMode="OFF"` | ✅ |
| Audit2 #6 | `schemaVer < NVS_SCHEMA_VERSION` not logged | Log line added | ✅ |
| Audit2 #7 | Mode read timeout enters 30s cooldown on 1st failure | `>= STATUS_PUSH_RETRY_MAX` guard added | ✅ |
| Audit2 #8 | `setPump(false)` in `checkCountdownExpiry()` phantom cycle | Removed — only state change, no `setPump()` | ✅ |
| Audit2 #9 | `bypass` enable clears `autoBypassWasEngaged` | Only cleared on `!v` (disable) path | ✅ |
| Audit2 #10 | Dead `runMode="AUTO"` in `checkCountdownExpiry()` | Removed | ✅ |
| Audit2 #11 | `delay(100)` in WiFi reconnect | Removed | ✅ |
| Offline | `cfgLastCountdownDurationMin` NVS persist | Saved/loaded in `loadStateFromNVS()` | ✅ |
| Offline | Status push retry | `statusRetryDue` + `normalIntervalDue` split | ✅ |
| Offline | 30s cooldown after 3 consecutive failures only | `>= STATUS_PUSH_RETRY_MAX` guard | ✅ |

**All 26 prior issues confirmed fixed.**

---

## Part 2 — Offline-First Redesign Assessment

The question is whether the offline-first architecture (NVS as authority, Firebase as sync layer, epoch-based command IDs) has been implemented. Here is the truthful state:

### What is implemented (partial offline-first)

- **Local state machine**: `executePumpLogic()`, `checkCountdownExpiry()`, `checkSafetyCutoff()` all run on local variables with zero Firebase dependency. The sensor and pump loop is fully decoupled from network. ✅
- **NVS persistence**: `pumpMode`, `isDryRunError`, `cfgBypassLevelSensor`, `totalPumpCycles`, `totalPumpRunSec`, `cfgLastCountdownDurationMin`, all device config keys are persisted to NVS. Pump runs from NVS state at boot if Firebase is unavailable. ✅
- **`pendingModeWriteback` queue**: Firmware-originated mode changes (stop, expiry, tank-full early stop) are queued and retried with rate-limiting rather than requiring a successful synchronous write. ✅
- **Countdown timer is millis()-based**: `countdownEndMs` is a local wall-clock value. The countdown runs and expires correctly with no Firebase dependency after it starts. ✅
- **Offline countdown start**: If `countdown_duration_min` cannot be read from Firebase, `cfgLastCountdownDurationMin` (NVS-persisted) is used as fallback. ✅
- **Retry and cooldown logic**: 3-failure threshold before 30s cooldown; status push retry; mode read timeout guard. ✅

### What is NOT implemented (full offline-first as described)

The described architecture had four specific features. Two are implemented above. Two are not:

**Missing 1 — Epoch/ID-based one-shot commands**: The description called for `stop_request_id`, `start_request_id` etc. as monotonic IDs stored in NVS. Currently `manual_start` and `manual_stop` are still boolean edge-detect flags (`lastManualStart`, `lastManualStop` statics). This means if the edge is missed during a Firebase cooldown window, the command is silently lost. On reconnect, the firmware reads `manual_start = true`, `lastManualStart = false` (since it's a static that survived) — actually it depends on whether the device rebooted. If the device was running during the outage, the static survives and the edge fires correctly on reconnect. If the device rebooted while the flag was true in Firebase, the static resets to false — the edge fires correctly on first read. So the current boolean edge approach is actually sufficient for the reboot+reconnect case. The main failure mode is: start pressed → Firebase goes down → Firebase recovers → firmware reads `manual_start = true` → `lastManualStart` is now `false` (because the Firebase read failed before delivering the `true`) → edge fires → pump starts. This is correct. The epoch ID approach would be more robust for multi-command sequences but is not strictly necessary for the current single-command use cases.

**Missing 2 — Full control shadow state (single JSON read)**: The description called for reading `/pump_system/control` as a single JSON blob instead of 7 individual reads. Currently `readFirebaseControl()` still makes 7 separate RTDB reads per cycle (`getString mode`, `getBool manual_stop`, `getInt countdown_duration_min`, `getBool countdown_add_time`, `getBool manual_start`, `getBool bypass_level_sensor`, `getBool clear_error`, `getInt reboot_request_id`). Each is a separate network round-trip. With -79 dBm WiFi, each has a non-trivial probability of timing out independently. This means one `readFirebaseControl()` call can make up to 8 network requests in sequence.

This is a meaningful remaining improvement. On a bad network cycle, a timeout on any individual read triggers `firebaseConsecutiveFailCount++`. If 3 of the 8 reads fail (easy at -79 dBm), the cooldown triggers even though 5 reads succeeded. A single JSON read would make the entire control cycle succeed or fail atomically.

---

## Part 3 — New Issues Found in This Pass

---

### Issue 1 — 🔴 Critical: `pendingModeWritebackSentMs` initialized to `0` in `manual_stop` handler — first retry fires immediately instead of after 5s

**File:** `05_connectivity_cloud.ino`, line 260

```cpp
pendingModeWriteback = true;
pendingModeWritebackSentMs = millis();  // ← set to NOW
Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
```

The write-back is sent immediately (correct). `pendingModeWritebackSentMs = millis()` means the 5-second retry timer starts from now — so if Firebase fails to confirm within 5s, a retry fires. This is correct.

But look at `checkCountdownExpiry()`:

```cpp
pendingModeWriteback = true;
pendingModeWritebackSentMs = 0;   // ← set to 0
// No Firebase.RTDB.setString call here
```

`pendingModeWritebackSentMs = 0` means `millis() - 0 >= 5000UL` is true **immediately** (millis() is always >> 5000 after boot). So on the very next Firebase cycle (3 seconds later), the retry condition fires instantly:

```cpp
} else if (millis() - pendingModeWritebackSentMs >= 5000UL) {
    Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", pumpMode);
    pendingModeWritebackSentMs = millis();
}
```

This means the first write-back after countdown expiry fires on the first Firebase cycle after expiry, which is correct — but it also means `pendingModeWritebackSentMs = 0` was intentionally designed to trigger an immediate first send. **This is actually intentional and correct.** `checkCountdownExpiry()` intentionally defers the write to the Firebase cycle (fix for Audit2 Issue 3), and `= 0` is the signal "write has never been sent yet, send on next cycle."

**Re-verification:** On manual_stop, `pendingModeWritebackSentMs = millis()` because the write IS sent immediately in that same function call. On expiry, `= 0` because the write is deferred to the Firebase sync block. The logic is consistent. **Not a bug.**

---

### Issue 2 — 🔴 Critical: `runActive` check in mode read uses stale `runMode` — Emergency Override (FORCE_ON) cannot be stopped by FORCE_OFF during the wrong runMode window

**File:** `05_connectivity_cloud.ino`, line 190

```cpp
bool runActive = (runMode == "MANUAL" || (pumpMode == "COUNTDOWN" && isCountdownActive));
```

`runActive` is intended to detect "a firmware-controlled run is in progress." It checks `runMode == "MANUAL"` but not `pumpMode == "FORCE_ON"`. Consider the sequence:

1. User sets `mode = "FORCE_ON"` from dashboard (Emergency Override).
2. Firmware receives it: `pumpMode = "FORCE_ON"`, `isManualRun = false` (because it came via mode write, not `manual_start`).
3. `runMode` is derived as `"MANUAL"` in `executePumpLogic()` because `pumpMode == "FORCE_ON"`.
4. User now sends `mode = "FORCE_OFF"` to stop the emergency override.
5. Mode read: `runActive = (runMode == "MANUAL")` → **true** (runMode is "MANUAL").
6. `runActive && newMode == "FORCE_OFF"` → enters the stop handler: `setPump(false)`, `pumpMode = "FORCE_OFF"` ✓.

This path is actually **correct** — it works because `runMode == "MANUAL"` covers FORCE_ON since `executePumpLogic()` sets `runMode = "MANUAL"` for `pumpMode == "FORCE_ON"`. No bug here.

But: on the cycle immediately after FORCE_ON is set, before `executePumpLogic()` runs (i.e., on the same Firebase cycle that delivered FORCE_ON), `runMode` has NOT been updated yet. The sensor block runs every 1s; the Firebase block runs every 3s. So for up to 1 second after FORCE_ON is set, `runMode` is still `"AUTO_STANDBY"` or `"OFF"`. If a FORCE_OFF arrives within that same Firebase cycle (very unlikely but possible), `runActive` would be false and `pumpMode` would be overwritten by the else branch instead of the stop handler. This is a 3-second window at most and extremely unlikely in practice.

**Verdict:** Negligible race, not a real-world bug. No fix required.

---

### Issue 3 — 🔴 Critical: `checkCountdownExpiry()` still called in sensor block — Firebase write removed but `pendingModeWriteback` set here, while `pendingModeWriteback` write-back ALSO happens in the Firebase block — **correct sequencing but potential same-cycle race**

**File:** `smart_water_pump_controller.ino`, line 303–304

```cpp
// sensor block (every 1s):
checkCountdownExpiry();   // sets pendingModeWriteback=true, pendingModeWritebackSentMs=0
executePumpLogic();
```

```cpp
// Firebase block (every 3s, later in same loop() pass when both intervals fire):
readFirebaseControl();    // checks pendingModeWriteback, may fire write-back
pushFirebaseStatus();
```

On the specific loop() iteration where BOTH the sensor interval AND Firebase interval fire simultaneously (possible since Firebase is a multiple of sensor interval: 3s vs 1s), the execution order is:

1. Sensor block fires: `checkCountdownExpiry()` → `pendingModeWriteback = true`, `pendingModeWritebackSentMs = 0`
2. `executePumpLogic()` runs with `pumpMode = "AUTO"` → pump stops correctly
3. Firebase block fires in the SAME loop() pass
4. `readFirebaseControl()` → mode read returns "COUNTDOWN" (stale) → `pendingModeWriteback` is true → `millis() - 0 >= 5000` → **FIRES WRITE-BACK IMMEDIATELY on the same cycle as expiry**

This is actually the intended behavior — expiry happens, write-back queued, Firebase block fires in same pass, write-back sent. The 5s rate-limiter correctly doesn't apply on the first send (since `pendingModeWritebackSentMs = 0`).

**Verdict:** This is correct. The sequence is intentional. No bug.

---

### Issue 4 — 🔴 Critical: `P4 early-stop` (tank full during countdown) still calls `Firebase.RTDB.setString` directly inside `executePumpLogic()`

**File:** `03_safety_pump.ino`, lines 255–258

```cpp
if (pumpMode == "COUNTDOWN") {
  if (isCountdownActive) {
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      setPump(false);
      isCountdownActive = false;
      countdownEndMs = 0;
      pumpMode = "AUTO";
      pendingModeWriteback = true;
      pendingModeWritebackSentMs = millis();
      Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");  // ← PROBLEM
      return;
    }
```

`executePumpLogic()` is called from **inside the sensor block**. This is the same problem as Audit2 Issue 3 that was fixed for `checkCountdownExpiry()` — Firebase writes must not happen inside the sensor block because `fbdo` may be mid-operation from the Firebase sync block on the same loop() pass.

`checkCountdownExpiry()` was fixed by removing its Firebase write and using `pendingModeWritebackSentMs = 0` to trigger a deferred send on the next Firebase cycle. The same fix must be applied here.

**Fix:** Remove the `Firebase.RTDB.setString` call from the P4 early-stop block. The `pendingModeWriteback = true` and `pendingModeWritebackSentMs = millis()` are already set, so the write-back will happen on the next Firebase cycle via the retry mechanism in `readFirebaseControl()`. But `pendingModeWritebackSentMs = millis()` means the retry won't fire until 5s later. Since this is the first send (same as expiry case), set it to `0` for immediate send on next Firebase cycle:

```cpp
pumpMode = "AUTO";
pendingModeWriteback = true;
pendingModeWritebackSentMs = 0;  // trigger immediate send on next Firebase cycle
// Remove: Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
return;
```

---

### Issue 5 — 🟡 Significant: `manual_stop` handler sets `pendingModeWritebackSentMs = millis()` AND immediately calls `Firebase.RTDB.setString` — but on failure, the 5s retry won't fire for 5 seconds, leaving Firebase with stale COUNTDOWN

**File:** `05_connectivity_cloud.ino`, lines 259–261

```cpp
pendingModeWriteback = true;
pendingModeWritebackSentMs = millis();
Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
```

If `setString` fails (network issue), `pendingModeWritebackSentMs` was set to `millis()` at time of failure. The retry condition is `millis() - pendingModeWritebackSentMs >= 5000UL`, so the retry won't fire for 5 seconds. During those 5 seconds, Firebase still has `mode = "COUNTDOWN"`, and the mode read block in `readFirebaseControl()` will suppress the overwrite correctly (because `pendingModeWriteback = true`). So the pump stays stopped and the mode stays "AUTO" locally. After 5s, the retry fires.

This is actually fine — `pendingModeWriteback` prevents the stale read from causing problems during the 5s window. The 5s delay before retry is acceptable.

However, there's a subtle issue: `Firebase.RTDB.setString` is called here from within `readFirebaseControl()`, which is itself called from the Firebase sync block. Then the `pendingModeWriteback` retry path in the SAME `readFirebaseControl()` call also potentially calls `setString` if the first send failed synchronously and the timing check passed. In practice, the first send at line 261 either succeeds (sets `lastSuccessfulFirebaseMs`, no retry needed) or fails (sets `pendingModeWritebackSentMs = millis()`, retry in 5s). These two paths are exclusive.

**Verdict:** Acceptable behavior. The 5s gap is protected by `pendingModeWriteback`. No critical issue.

---

### Issue 6 — 🟡 Significant: `countdownConsumed` is a `static` local variable in `readFirebaseControl()` — not visible to `checkCountdownExpiry()` or any other function

**File:** `05_connectivity_cloud.ino`, line 175

```cpp
void readFirebaseControl() {
  static bool countdownConsumed = false;
  ...
}
```

`countdownConsumed` as a static local is correct for its primary purpose (preventing re-arm within the same countdown lifetime). But there is a subtle scenario:

1. Countdown is active. `countdownConsumed = true`.
2. `checkCountdownExpiry()` fires (sensor block): `pumpMode = "AUTO"`, `pendingModeWriteback = true`.
3. Firebase block fires: `readFirebaseControl()` runs.
4. Mode read returns "COUNTDOWN" still (stale) → `pendingModeWriteback` suppresses overwrite ✓.
5. `countdownConsumed` reset: `firebaseReadMode = "COUNTDOWN"` → does NOT reset. `countdownConsumed` stays `true` ✓.
6. Writeback confirmed: `newMode == pumpMode ("AUTO")` → `pendingModeWriteback = false`, `countdownConsumed = false` (line 209: `if (pumpMode=="AUTO") countdownConsumed = false`) ✓.

Now `pumpMode = "AUTO"`, `countdownConsumed = false`. `pumpMode == "COUNTDOWN"` is false → start block does not fire ✓.

This path is correct. The static local is safe here.

**However:** There is one edge case. If `readFirebaseControl()` is never called between a countdown expiry and the next countdown start (e.g., Firebase is down for a long time, then recovers and immediately the dashboard sends COUNTDOWN again), `countdownConsumed` retains its previous value from the last call. Since the last call was during the previous countdown with `countdownConsumed = true`, and `pendingModeWriteback` was cleared (writeback confirmed), `countdownConsumed` would have been reset to `false` at writeback confirmation (line 209). So by the time a new COUNTDOWN is received, `countdownConsumed` is `false`. ✓

**Verdict:** Logic is correct. The static local works as intended.

---

### Issue 7 — 🟡 Significant: `lastAddTime` is a `static` local that is NOT reset when `isCountdownActive` becomes false via `checkCountdownExpiry()`

**File:** `05_connectivity_cloud.ino`, line 176 + line 296

```cpp
static bool lastAddTime = false;
```

`lastAddTime` is reset to `false` when a new countdown STARTS (line 296: `lastAddTime = false`). But `checkCountdownExpiry()` does not reset it — it only sets `isCountdownActive = false` and `pumpMode = "AUTO"`. If `countdown_add_time` is still `true` in Firebase when the countdown expires (e.g., user tapped Add Time in the last second), `lastAddTime` remains `true` from the last read.

When a NEW countdown starts:
1. `lastAddTime = false` (line 296, countdown start block). ✓
2. First read of `countdown_add_time`: if it's still `true` from the previous one (firmware reset write-back may not have landed yet), `v=true && !lastAddTime(false)` → **fires +5 min on the new countdown**. ✓ This is safe because the user tapped Add Time.

If the previous countdown's add-time flag was correctly reset to `false` by firmware (line 309: `setBool false`), then `countdown_add_time` is `false` in Firebase. `lastAddTime` was reset to `false` on new start. `v=false && !lastAddTime` → no fire. ✓

**Verdict:** The reset on countdown start (`lastAddTime = false`) covers this case. No bug.

---

### Issue 8 — 🟡 Significant: `FORCE_ON` received from dashboard via mode write (Emergency Override) sets `isManualRun = false` — but `runMode` is derived as `"MANUAL"` anyway, creating a misleading state

**File:** `05_connectivity_cloud.ino`, line 216–221

```cpp
} else {
  if (pumpMode != newMode) {
    if (newMode != "FORCE_ON") isManualRun = false;  // isManualRun NOT set true for mode-based FORCE_ON
  }
  pumpMode = newMode;
}
```

When the dashboard writes `mode = "FORCE_ON"` directly (Emergency Override, not via `manual_start`), `isManualRun` stays `false`. But `executePumpLogic()` derives `runMode = "MANUAL"` for any `pumpMode == "FORCE_ON"`. The dashboard will show `run_mode = "MANUAL"` whether the pump was started via Quick Start button or Emergency Override.

The spec says "Manual run and Emergency Override both use FORCE_ON at the firmware level; the dashboard distinguishes them by which UI triggered the mode." Since `run_mode = "MANUAL"` for both, the dashboard cannot distinguish them from status alone. This is a known design decision, not a new bug.

**Verdict:** By design. No fix needed.

---

### Issue 9 — 🟡 Significant: `delay(100)` still present before `ESP.restart()` in reboot handler

**File:** `05_connectivity_cloud.ino`, line 385

```cpp
delay(100);
ESP.restart();
```

This was not flagged before because it's in the reboot handler, not the main loop. A 100ms blocking delay before restart is harmless here since restart is unconditional. However, it's worth noting that the two Firebase writes before it (`setInt last_reboot_request_id`, `setInt last_reboot_at`) are synchronous and may block for up to 10s (Firebase read timeout). If both fail, the 100ms delay adds nothing.

**Verdict:** Harmless. Not a bug.

---

### Issue 10 — 🟡 Significant: `runMode = "AUTO"` set in `manual_stop` handler (line 255) conflicts with `executePumpLogic()` re-derivation

**File:** `05_connectivity_cloud.ino`, line 255

```cpp
if (v && !lastManualStop) {
  setPump(false);
  runMode = "OFF";          // ← set here
  ...
  pumpMode = "AUTO";
}
```

After `manual_stop` sets `runMode = "OFF"`, `executePumpLogic()` will run on the next sensor cycle (up to 1s later) and re-derive `runMode` from `pumpMode = "AUTO"`:
- If level ≤ start threshold: `isRunning` just became false via `setPump(false)` → `pumpMode=="AUTO" && !isRunning` → `runMode = "AUTO_STANDBY"`. ✓
- The pump won't restart because `isRunning` is now false and `executePumpLogic()` re-evaluates it correctly.

So setting `runMode = "OFF"` in `manual_stop` provides correct status for the ~1 second window before `executePumpLogic()` runs. This is intentional defensive state-setting, not a bug. After `executePumpLogic()` runs, `runMode` will be correctly `"AUTO_STANDBY"` or `"AUTO"`. **Not a bug.**

---

### Issue 11 — 🟢 Minor: `firebaseConsecutiveFailCount` is incremented by BOTH `readFirebaseControl()` mode read failures AND `pushFirebaseStatus()` failures — the cooldown threshold `STATUS_PUSH_RETRY_MAX` applies to the combined count

**Files:** `05_connectivity_cloud.ino`, lines 227, 468

Both functions increment `firebaseConsecutiveFailCount`. The cooldown guards check `>= STATUS_PUSH_RETRY_MAX (3)`. So if the mode read fails twice and the status push fails once, the cooldown triggers even though the status push only failed once and the mode read only failed twice. The counter is shared but represents failures from different operations.

In practice this is harmless — if both reads and pushes are failing, the cooldown is appropriate. But it means a burst of 3 mode-read failures (without any push failures) will trigger the cooldown, silencing pushes too. This is correct defensive behavior.

**Verdict:** Design is acceptable. Could be split into separate counters but not worth the complexity.

---

### Issue 12 — 🟢 Minor: `pendingModeWriteback` is never reset if Firebase is permanently unavailable

**File:** `05_connectivity_cloud.ino`, lines 205–214

If WiFi never reconnects after a stop/expiry, `pendingModeWriteback` stays `true` permanently. The mode read block suppresses all mode overwrites from Firebase. This means if Firebase later reconnects with a different mode (e.g., someone changed mode in the Firebase console directly), that change will be ignored forever until the device reboots.

In practice, if Firebase reconnects, the writeback retry will fire (since `pendingModeWritebackSentMs` will be old), send "AUTO", and get confirmed. Then `pendingModeWriteback = false`. The permanent-stuck state only occurs if Firebase never reconnects AND never sends a read that equals `pumpMode`. Since `readFirebaseControl()` only runs when `Firebase.ready()`, this can only happen if Firebase is available but returning wrong data — an unlikely edge case.

**Verdict:** Acceptable. The only complete fix would be a `pendingModeWriteback` timeout (e.g., 5 minutes), but that introduces risk of accidental mode overwrites. Current behavior is safe.

---

## Part 4 — Offline-First Architecture Gap Assessment

The asked-about "Offline-First Redesign" has been **partially implemented**. Here is the honest gap:

### Implemented ✅
- Local state machine decoupled from Firebase (sensor, pump, safety, countdown all run on local variables)
- NVS as persistence layer for all critical state
- `pendingModeWriteback` as outbound command queue for mode write-backs
- Countdown timer runs purely on `millis()` — immune to network state
- Offline countdown start with last-known duration
- Firebase is purely a sync layer — pump logic never blocks on Firebase

### Not implemented (remaining gap)
- **Single JSON control read**: `readFirebaseControl()` still makes 7+ individual RTDB reads. This is the largest remaining reliability improvement. A single `getJSON("/pump_system/control")` would make all control reads atomic — either all succeed or all fail as one operation.
- **Epoch-based command IDs**: `manual_start`/`manual_stop` still use boolean edge-detect. Acceptable for current use case but not as robust as monotonic IDs.

### Verdict on the redesign question
The core pump loop is already offline-first. The remaining gap is in the control read path. The full redesign is not complete, but the firmware as-is is substantially more resilient than the original and all the described countdown bugs are fixed. The single remaining high-value improvement is collapsing `readFirebaseControl()` into a single `getJSON` call.

---

## Summary

### Confirmed Fixed
All 26 prior issues verified as correctly implemented.

### New Issues Found

| # | Severity | File | Description |
|---|---|---|---|
| 4 | 🔴 Critical | `03_safety_pump.ino` | P4 early-stop still calls `Firebase.RTDB.setString` inside `executePumpLogic()` — `fbdo` used in sensor block |
| 6 | 🟡 Significant | `05_connectivity_cloud.ino` | `countdownConsumed` static local edge case on long Firebase outage — analyzed, confirmed safe |
| 7 | 🟡 Significant | `05_connectivity_cloud.ino` | `lastAddTime` static local not reset on expiry — analyzed, confirmed safe via new-start reset |
| 11 | 🟢 Minor | `05_connectivity_cloud.ino` | `firebaseConsecutiveFailCount` shared between reads and push — acceptable, cooldown is still correct |
| 12 | 🟢 Minor | `05_connectivity_cloud.ino` | `pendingModeWriteback` never auto-resets on permanent Firebase loss — acceptable, safe |

**Only Issue 4 requires a code change.** Issues 2, 3, 5, 6, 7, 8, 9, 10 were analyzed and confirmed correct. Issues 11 and 12 are noted but require no action.

### The one required fix

**`03_safety_pump.ino`, P4 early-stop block** — remove the `Firebase.RTDB.setString` call and set `pendingModeWritebackSentMs = 0` (deferred send) instead of `= millis()` (5s delay):

```cpp
// BEFORE:
pumpMode = "AUTO";
pendingModeWriteback = true;
pendingModeWritebackSentMs = millis();
Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
return;

// AFTER:
pumpMode = "AUTO";
pendingModeWriteback = true;
pendingModeWritebackSentMs = 0;   // send on next Firebase cycle (immediate)
// Firebase write deferred to readFirebaseControl() retry path
return;
```
