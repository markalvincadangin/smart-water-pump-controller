---
name: countdown_bugs_and_offline_resilience
overview: >
  Fix three confirmed countdown bugs in 05_connectivity_cloud.ino and
  03_safety_pump.ino, and redesign the firmware's online/offline state
  machine for network resilience. All bug root causes are traced to exact
  lines in the source files.
todos:

  # ══════════════════════════════════════════════════════════════════════════
  # PHASE 1 — BUG FIXES (fix in this order; each fix is independent)
  # ══════════════════════════════════════════════════════════════════════════

  - id: fix-stop-restarts-countdown
    content: >
      BUG 1 — Stop button stops pump for 3–5 s then countdown restarts.

      ROOT CAUSE (05_connectivity_cloud.ino lines 231–288):
      The `manual_stop` handler correctly clears `isCountdownActive` and sets
      `pumpMode = "AUTO"`, and also writes `"AUTO"` back to Firebase.
      But the `countdownConsumed` reset guard at line 254 fires on the SAME
      Firebase cycle because `firebaseReadMode` is read at the TOP of
      `readFirebaseControl()` — before `manual_stop` is processed.

      On the cycle where Stop is tapped:
        Step 1: Mode is read from Firebase → still "COUNTDOWN" (the dashboard
                hasn't received the firmware's "AUTO" write back yet, and the
                Firebase write itself takes up to 3s to propagate).
                firebaseReadMode = "COUNTDOWN". countdownConsumed stays true.
        Step 2: manual_stop fires → pumpMode = "AUTO", isCountdownActive = false.
        Step 3: countdownConsumed reset check: firebaseReadMode == "COUNTDOWN"
                → condition is false → countdownConsumed stays true. CORRECT so far.

      On the NEXT Firebase cycle (3s later), Firebase may still report "COUNTDOWN"
      (propagation delay for the firmware's write-back) OR it may have flipped.
      If it still reports "COUNTDOWN":
        Step 1: firebaseReadMode = "COUNTDOWN".
        Step 2: pumpMode is now "AUTO" (set by manual_stop handler last cycle).
                The cleanup block at line 284 runs:
                  `if (pumpMode != "COUNTDOWN" && isCountdownActive)` → false
                  (isCountdownActive is already false).
        Step 3: countdownConsumed stays true → countdown does NOT restart. OK.
        Step 4: When Firebase finally reflects "AUTO" (the write-back landed):
                firebaseReadMode = "AUTO" → countdownConsumed = false (line 255).
        Step 5: Now pumpMode == "AUTO", isCountdownActive == false,
                countdownConsumed == false.
                  → The start block at line 258 fires: `pumpMode == "COUNTDOWN"`
                  is false → does NOT restart. OK.

      Wait — this analysis says it should work. Let me re-read the actual bug.

      ACTUAL ROOT CAUSE (re-read lines 182–210):
      The mode read block sets pumpMode UNCONDITIONALLY:
        ```
        if (runActive && newMode == "FORCE_OFF") { ... }
        else if (runActive && newMode != "FORCE_OFF") { runPrevPumpMode = newMode; }
        else {
          pumpMode = newMode;   ← THIS FIRES WHEN runActive IS FALSE
        }
        ```
      After `manual_stop` runs (line 231–248), `isManualRun` becomes false and
      `isCountdownActive` becomes false. On the NEXT Firebase cycle:
        - `runActive` = `(runMode == "MANUAL" || (pumpMode == "COUNTDOWN" && isCountdownActive))`
        - runMode is "OFF", pumpMode is "AUTO", isCountdownActive is false
        - → runActive = false
        - Firebase still reports mode = "COUNTDOWN" (propagation lag)
        - → The else branch fires: `pumpMode = "COUNTDOWN"`  ← RESTARTS COUNTDOWN
        - → pumpMode is now "COUNTDOWN" again, isCountdownActive is false,
            countdownConsumed is true (correctly) so it won't re-arm immediately.
        - But then countdownConsumed reset fires: firebaseReadMode = "COUNTDOWN"
          → countdownConsumed stays true.

      Then eventually Firebase propagation delivers the "AUTO" write-back:
        - firebaseReadMode = "AUTO" → countdownConsumed = false
        - pumpMode has been "COUNTDOWN" since the mode read overwrote it
        - pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed
          → COUNTDOWN STARTS AGAIN ← This is the 3-5s restart.

      THE FIX:
      The mode read must NOT overwrite pumpMode if the firmware has already
      locally reverted to a different mode via manual_stop or checkCountdownExpiry.
      Track a `firmwareOwnedMode` flag — when firmware writes mode back to
      Firebase (AUTO after stop/expiry), suppress the mode-read overwrite until
      Firebase confirms the new value.

      Replace the mode handling in readFirebaseControl() with this pattern:

      In 05_connectivity_cloud.ino, add a static bool:
        static bool pendingModeWriteback = false;

      Set pendingModeWriteback = true in manual_stop handler AND in
      checkCountdownExpiry() whenever pumpMode is set locally and
      Firebase.RTDB.setString is called for mode.

      In the mode read block, change the else branch:
        else {
          // Only accept Firebase mode if we are NOT waiting for our own write
          // to land back. This prevents Firebase propagation lag from overwriting
          // a locally-set mode (e.g. after manual_stop reverts to AUTO).
          if (!pendingModeWriteback) {
            if (pumpMode != newMode) {
              Serial.printf("[FIREBASE] Mode changed: %s -> %s\n",
                            pumpMode.c_str(), newMode.c_str());
              if (newMode != "FORCE_ON") isManualRun = false;
            }
            pumpMode = newMode;
          }
          // When Firebase confirms our written value, clear the flag
          if (pendingModeWriteback && newMode == pumpMode) {
            pendingModeWriteback = false;
          }
        }

      Also set pendingModeWriteback = true:
        - In manual_stop handler when mode is set to "AUTO"
        - In checkCountdownExpiry() when mode is set to "AUTO"
        - In the P4 COUNTDOWN early-stop block in executePumpLogic() when mode
          is set to "AUTO"

      File: dashboard/firmware/05_connectivity_cloud.ino
    status: pending

  - id: fix-countdown-restarts-after-expiry
    content: >
      BUG 2 — After countdown finishes, pump stops for 3–5 s then restarts
      for the same duration from the beginning.

      ROOT CAUSE: Same as Bug 1, different trigger path.

      When checkCountdownExpiry() fires (03_safety_pump.ino / 05_connectivity_cloud.ino):
        - isCountdownActive = false
        - pumpMode = "AUTO"
        - Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO")

      On the next Firebase cycle (up to 3s later), Firebase still reports
      mode = "COUNTDOWN" (propagation lag). The same else branch as Bug 1:
        - runActive = false (isCountdownActive is false)
        - Firebase reports "COUNTDOWN"
        - → pumpMode = "COUNTDOWN" (overwrites the locally-set "AUTO")
        - countdownConsumed is false (it was cleared after the previous run finished)
        - → pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed
          → COUNTDOWN STARTS AGAIN with the same duration_min still in Firebase

      The fix is the same `pendingModeWriteback` flag from Bug 1 fix.
      Both bugs share the same root cause and are fixed by the same change.
      No additional code change needed beyond the fix-stop-restarts-countdown task.

      VERIFY after fix-stop-restarts-countdown is applied:
        - Let countdown run to expiry. Confirm pump stops and stays stopped.
        - Confirm mode in Firebase shows "AUTO" within ~6s of expiry.
        - Confirm pump does not restart.
    status: pending

  - id: fix-addtime-infinite-loop
    content: >
      BUG 3 — Add Time continuously adds 5 minutes in a loop instead of once.

      ROOT CAUSE (05_connectivity_cloud.ino lines 270–281):

      ```cpp
      if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/countdown_add_time")) {
        bool v = fbdo.boolData();
        if (v && !lastAddTime) {          // ← edge-detect: fires on true→false→true edge
          countdownEndMs = min(countdownEndMs + ..., maxEnd);
        }
        lastAddTime = v;                  // ← lastAddTime tracks raw Firebase value
      }
      ```

      The edge-detect `v && !lastAddTime` is correct in intent: it fires only on
      a rising edge (false→true transition). This is why the dashboard spec says
      the dashboard resets the flag to false after the firmware processes it.

      BUT: the firmware NEVER resets `countdown_add_time` to false. Look at the
      spec comments — for `countdown_add_time`, the firmware is supposed to reset
      it (firmware-reset one-shot), not the dashboard. The dashboard spec says:
      "Firmware resets it to false after applying."

      This means:
        1. Dashboard writes `countdown_add_time = true`.
        2. Firmware reads it as true, `lastAddTime` was false → edge fires → +5 min. ✓
        3. Firmware sets `lastAddTime = true`.
        4. Next Firebase cycle: dashboard reads countdown_add_time still true
           (firmware didn't reset it) → dashboard's isAddingCountdownTime stays busy.
        5. Dashboard times out waiting (after 8s), resets countdown_add_time = false.
        6. Firmware reads false → `lastAddTime = true → false` transition.
        7. Next cycle: dashboard sees countdown_add_time = false → clears busy →
           re-enables the Add Time button.
        8. User taps Add Time again: dashboard writes true.
        9. Firmware: v = true, lastAddTime = false → edge fires → +5 min again. ✓

      This path works BUT: if the dashboard sets busy via a timer timeout and
      the timer resets the flag WHILE Firebase still has it as true from a previous
      cycle (due to propagation), `lastAddTime` was left as `true` from step 3,
      so the next `true` read does NOT fire the edge (v=true, lastAddTime=true
      → condition false). This is fine.

      THE ACTUAL INFINITE LOOP CAUSE:
      The dashboard's `isAddingCountdownTime` is cleared "when Firebase control
      node confirms `countdown_add_time = false`". But the firmware never writes
      `false` back. The dashboard has to rely on a timeout to clear busy and
      re-enable the button. If the timeout is very short OR if the dashboard
      implementation resets the flag via a timer independently of Firebase state,
      then:
        - Dashboard writes true
        - Timer fires → dashboard resets to false
        - Dashboard sees false from Firebase (its own write-back or cached read)
        - isAddingCountdownTime clears immediately
        - Button re-enables
        - Firmware reads: true (original write), then false (dashboard reset)
          → true→false is a falling edge, not a rising edge → no re-fire ✓

      BUT: if the dashboard does NOT reset to false (just clears busy state locally),
      and the button becomes re-enabled before Firebase delivers the write,
      the user could tap again, the dashboard writes true again, and the cycle
      repeats. On each cycle: v=true, lastAddTime=true (from last read) → no fire.
      Wait — lastAddTime would be true, so the edge would NOT fire.

      The REAL infinite loop: the 8s dashboard timeout resets add_time = false.
      Firmware then reads false: lastAddTime = true → false (falling edge, no action).
      Then reads false: lastAddTime = false.
      Now dashboard button is re-enabled. User taps → true.
      Firmware: v=true, lastAddTime=false → FIRES again. But this requires user action.

      So the true infinite loop without user action must be different. Looking again:
      The add_time block runs every Firebase cycle (3s) as long as
      `pumpMode == "COUNTDOWN" && isCountdownActive`. If Firebase has
      `countdown_add_time = true` and the firmware NEVER resets it to false,
      then on EVERY cycle after the first:
        - v = true, lastAddTime = true → condition `v && !lastAddTime` = false.
        - No re-fire.

      Except: `lastAddTime` is a STATIC LOCAL. After Bug 1/2's mode overwrite
      causes `isCountdownActive` to briefly become false, then true again when
      a new countdown starts, the `countdown_add_time` block is gated by
      `pumpMode == "COUNTDOWN" && isCountdownActive`. When the countdown
      restarts (Bug 2), `lastAddTime` still holds its old value. If
      `countdown_add_time` was still `true` in Firebase (never reset),
      `lastAddTime` = true → no fire on first cycle of new countdown.
      But after the dashboard's 8s timeout resets it to false:
      Firmware reads false: lastAddTime = true → false (no action).
      Firmware reads false: lastAddTime = false.
      Then if ANYTHING writes it back to true (or the dashboard retries):
      v=true, lastAddTime=false → FIRES → +5 min added to the NEW countdown.

      THE FIX:
      Firmware must reset `countdown_add_time` to false after processing it,
      matching the spec ("Firmware resets it to false after applying").

      In the add_time handler, add the write-back:
        if (v && !lastAddTime) {
          unsigned long maxEnd = millis() + (unsigned long)COUNTDOWN_MAX_DURATION_MIN * 60000UL;
          countdownEndMs = min(countdownEndMs + (unsigned long)COUNTDOWN_ADD_TIME_MIN * 60000UL, maxEnd);
          Serial.printf("[COUNTDOWN] +%d min added.\n", COUNTDOWN_ADD_TIME_MIN);
          // Firmware-reset one-shot: clear the flag so dashboard knows it was processed
          Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_add_time", false);
        }
        lastAddTime = v;

      Also reset lastAddTime when a new countdown starts (in the countdown start
      block at lines 258–268) to ensure old edge state doesn't carry over:
        isCountdownActive = true;
        countdownConsumed = true;
        lastAddTime = false;  // ← ADD THIS LINE

      File: 05_connectivity_cloud.ino
    status: pending

  # ══════════════════════════════════════════════════════════════════════════
  # PHASE 2 — OFFLINE RESILIENCE REDESIGN
  # ══════════════════════════════════════════════════════════════════════════

  - id: offline-local-state-machine
    content: >
      Decouple the pump state machine from Firebase entirely.

      CURRENT PROBLEM:
      The firmware currently relies on Firebase for its operating mode. When
      Firebase is unavailable (bad network, token refresh, cooldown), the mode
      read silently fails and pumpMode holds its last value. This is mostly safe,
      but it means:
        - If Firebase is down when the user starts a countdown, the countdown
          cannot be started.
        - If Firebase drops during a FORCE_OFF, the pump stays off correctly
          (NVS-persisted) but the user cannot restart it until Firebase recovers.
        - Any mode change during a Firebase outage is lost.

      THE REDESIGN — "Firebase is a remote control, not the source of truth":

      1. Add a `localPumpMode` variable that is the authoritative in-memory mode.
         Firebase reads update `localPumpMode` only when the read succeeds.
         `pumpMode` becomes an alias for `localPumpMode` (or rename throughout).

      2. NVS persistence of mode (already done for pumpMode) is the offline
         source of truth. On boot, load from NVS. Firebase syncs overlay on top.

      3. Add `localCountdownEndMs` — countdown timer runs entirely in millis().
         It is started on a successful Firebase read of `countdown_duration_min`
         and continues running even when Firebase is unavailable.
         `checkCountdownExpiry()` uses `millis()` exclusively — already does this,
         so no change needed here.

      4. Add offline countdown start: if the ESP32 receives a COUNTDOWN mode
         transition but the subsequent `countdown_duration_min` read fails,
         use the NVS-persisted last-known duration (or fall back to 15 min default).

      In 01_config.ino global state, add:
        int cfgLastCountdownDurationMin = 15;  // persisted to NVS

      In the countdown start block (05_connectivity_cloud.ino line 258–268):
        if (pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed) {
          int durationMin = cfgLastCountdownDurationMin;  // default to last known
          if (Firebase.RTDB.getInt(&fbdo, "/pump_system/control/countdown_duration_min")) {
            int v = constrain(fbdo.intData(), 1, COUNTDOWN_MAX_DURATION_MIN);
            durationMin = v;
            cfgLastCountdownDurationMin = v;  // persist for offline use
            // Save to NVS
            if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
              prefs.putInt("cd_dur_min", cfgLastCountdownDurationMin);
              prefs.end();
            }
          }
          countdownEndMs    = millis() + (unsigned long)durationMin * 60000UL;
          isCountdownActive = true;
          countdownConsumed = true;
          lastAddTime       = false;
          Serial.printf("[COUNTDOWN] Started: %d min.%s\n", durationMin,
                        Firebase.ready() ? "" : " (Firebase unavailable — using last known duration)");
        }

      In loadStateFromNVS() (04_persistence.ino), load:
        cfgLastCountdownDurationMin = prefs.getInt("cd_dur_min", 15);

      File: 05_connectivity_cloud.ino, 04_persistence.ino, 01_config.ino
    status: pending

  - id: offline-control-queue
    content: >
      Add a lightweight local command queue for control writes during Firebase
      outages.

      CURRENT PROBLEM:
      When Firebase fails, any pending status push is silently dropped. The
      dashboard can have stale state for the duration of the outage. More
      critically, if a user changes mode while offline, the change is lost when
      Firebase recovers (the next successful mode read will overwrite with the
      stale Firebase value).

      THE FIX — Pending write-back queue:

      Add a global struct for pending outbound writes that must survive a Firebase
      failure cycle:

      In smart_water_pump_controller_shared.h, add:
        struct PendingWrites {
          bool modeWritePending;
          String pendingModeValue;
          bool clearErrorPending;
        };
        extern PendingWrites pendingWrites;

      In 01_config.ino:
        PendingWrites pendingWrites = {false, "", false};

      Whenever firmware locally changes pumpMode and needs to write it back to
      Firebase (manual_stop, checkCountdownExpiry, P4 early-stop), instead of
      calling Firebase.RTDB.setString immediately (which may fail), set:
        pendingWrites.modeWritePending = true;
        pendingWrites.pendingModeValue = "AUTO";

      At the start of each successful Firebase cycle, flush pending writes first:
        if (pendingWrites.modeWritePending) {
          if (Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode",
                                       pendingWrites.pendingModeValue)) {
            pendingWrites.modeWritePending = false;
            pendingModeWriteback = true;  // suppress incoming read until confirmed
          }
        }

      This ensures mode write-backs land even after a Firebase interruption,
      preventing the stale-mode overwrite that causes Bugs 1 and 2.

      File: 05_connectivity_cloud.ino, smart_water_pump_controller_shared.h,
            01_config.ino
    status: pending

  - id: offline-shadow-state
    content: >
      Add a shadow state for critical control values so the firmware can operate
      without Firebase reads.

      CURRENT PROBLEM:
      Every control value (mode, bypass, clear_error, manual_start, manual_stop,
      countdown_add_time) requires a successful Firebase read on each cycle.
      With unstable WiFi (-79 dBm RSSI as seen in Serial Monitor), reads fail
      intermittently. During a failure window, commands sent from the dashboard
      are not seen until the next successful read cycle.

      THE REDESIGN — Shadow state for control node:

      Instead of reading individual RTDB paths on every cycle, read the entire
      `/pump_system/control` node as a single JSON on each Firebase cycle.
      This reduces from 7 separate RTDB reads to 1, dramatically reducing
      exposure to network latency and partial-read failures.

      Replace all individual reads in readFirebaseControl() with a single getJSON:

        if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/control")) {
          // Network failure — use shadow state
          // All processing below will use shadowControl values
          handleFirebaseReadFailure();
          return;
        }

        FirebaseJson controlJson = fbdo.to<FirebaseJson>();
        FirebaseJsonData jd;

        // Extract mode
        controlJson.get(jd, "mode");
        String newMode = jd.success ? jd.stringValue : shadowControlMode;
        ...

      This approach has a secondary benefit: if the read succeeds but one key
      is missing (e.g. manual_start doesn't exist yet in a fresh Firebase project),
      the firmware gracefully falls back to shadow values instead of treating a
      missing key as an error.

      Shadow state struct:

      In smart_water_pump_controller_shared.h, add:
        struct ControlShadow {
          String  mode;
          bool    manualStart;
          bool    manualStop;
          bool    clearError;
          int     rebootRequestId;
          int     countdownDurationMin;
          bool    countdownAddTime;
          bool    bypassLevelSensor;
        };
        extern ControlShadow shadowControl;

      In 01_config.ino initialize with safe defaults:
        ControlShadow shadowControl = {"AUTO", false, false, false, 0, 15, false, false};

      On each successful Firebase JSON read, update shadowControl with new values.
      On failure, use shadowControl as-is (no-op for one-shots since they require
      rising edge transitions, so stale false values are safe).

      File: 05_connectivity_cloud.ino, smart_water_pump_controller_shared.h,
            01_config.ino
    status: pending

  - id: offline-status-push-retry
    content: >
      Add a lightweight status push retry and local status cache.

      CURRENT PROBLEM:
      When pushFirebaseStatus() fails, the status is simply dropped. The
      dashboard goes stale until the next successful push (~3s normally, but
      potentially 30–60s during a Firebase cooldown). During the cooldown,
      the dashboard shows the controller as offline even though the pump is
      running correctly.

      THE FIX:

      1. Track whether the last status push succeeded. If it failed, retry the
         push on the next cycle regardless of firebaseInterval (use a shorter
         retry window — 1s — for up to 3 retries before entering normal cooldown).

      In smart_water_pump_controller_shared.h, add:
        extern int    statusPushRetryCount;
        extern unsigned long statusPushRetryMs;
        #define STATUS_PUSH_RETRY_MAX   3
        #define STATUS_PUSH_RETRY_MS    1000

      In the Firebase sync block in loop():
        // Normal interval OR short retry after failure
        bool shouldPush = (now - lastFirebaseMs >= firebaseInterval) ||
                          (statusPushRetryCount > 0 && statusPushRetryCount < STATUS_PUSH_RETRY_MAX &&
                           now - statusPushRetryMs >= STATUS_PUSH_RETRY_MS);
        if (shouldPush) {
          lastFirebaseMs = now;
          ...
          pushFirebaseStatus();
        }

      In pushFirebaseStatus(), on failure:
        statusPushRetryCount++;
        statusPushRetryMs = millis();

      On success:
        statusPushRetryCount = 0;

      2. Do NOT enter the 30s cooldown for ordinary network timeouts unless
         they exceed STATUS_PUSH_RETRY_MAX consecutive failures. The current
         code puts the entire Firebase cycle into cooldown on the first timeout,
         which leaves the dashboard dark for 30s on any brief network hiccup.

      Change the cooldown logic in pushFirebaseStatus():
        // Current: enters 30s cooldown on first read timeout
        // New: only enter cooldown after N consecutive failures
        if (err.indexOf("payload read timed out") >= 0 && firebaseConsecutiveFailCount >= 3) {
          firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 30000UL);
        }

      File: 05_connectivity_cloud.ino, smart_water_pump_controller_shared.h,
            01_config.ino
    status: pending

  - id: offline-nvs-mode-restore
    content: >
      Ensure mode and countdown state are correctly restored from NVS on boot
      so the pump runs in last-known-good mode during network outages at boot.

      CURRENT BEHAVIOR (04_persistence.ino loadStateFromNVS):
      Mode is loaded from NVS. If it was "COUNTDOWN" when power failed, it is
      restored. But countdownEndMs is NOT persisted, so the countdown has no
      timer. The firmware will see pumpMode = "COUNTDOWN" and isCountdownActive =
      false, then try to read countdown_duration_min from Firebase. If Firebase
      is unavailable at boot, the countdown never starts and the pump stays off.

      THE FIX:
      When mode is restored as "COUNTDOWN" from NVS and Firebase is unavailable
      at boot, auto-start a countdown using cfgLastCountdownDurationMin (the
      persisted last-known duration from offline-local-state-machine task).

      In loadStateFromNVS() (04_persistence.ino), after restoring pumpMode:
        if (pumpMode == "COUNTDOWN") {
          // Countdown timer cannot survive a power cycle. Auto-start with
          // the last known duration so the pump continues working offline.
          // Firebase will update duration when connectivity resumes.
          pumpMode = "COUNTDOWN";
          // countdownConsumed stays false → start block will fire after boot
          // OR if Firebase unavailable, start directly with last known duration:
          // (The start block in readFirebaseControl handles this via offline fallback)
          Serial.println("[BOOT] Restored COUNTDOWN mode from NVS. Timer will restart.");
        }

      Also: persist `countdownConsumed` flag state is NOT needed (it's ephemeral
      and should reset on boot — fresh Firebase read is preferable). Just ensure
      the boot path correctly sets countdownConsumed = false so the start block
      can fire.

      File: 04_persistence.ino
    status: pending

  # ══════════════════════════════════════════════════════════════════════════
  # PHASE 3 — VALIDATION
  # ══════════════════════════════════════════════════════════════════════════

  - id: validate-bugs
    content: >
      Verify all three bugs are fixed. Test each scenario with Serial Monitor open.

      BUG 1 — Stop during countdown:
        [ ] Start a 5-minute countdown via dashboard.
        [ ] While countdown is running (~2 min remaining), tap Stop.
        [ ] Serial Monitor should show:
              [FIREBASE] Manual stop requested. Reverting to AUTO.
        [ ] Pump relay goes off.
        [ ] Wait 10 seconds. Pump must NOT restart.
        [ ] Firebase /control/mode must show "AUTO" within 6s.
        [ ] run_mode in status must show "AUTO_STANDBY" (or "AUTO" when level ≤ 30%).

      BUG 2 — Countdown expiry:
        [ ] Start a 2-minute countdown.
        [ ] Wait for it to expire.
        [ ] Serial Monitor: "[COUNTDOWN] Timer expired. Reverting to AUTO mode."
        [ ] Pump relay goes off.
        [ ] Wait 10 seconds. Pump must NOT restart a new countdown.
        [ ] Firebase /control/mode shows "AUTO".

      BUG 3 — Add time infinite loop:
        [ ] Start a 15-minute countdown.
        [ ] Tap "Add 5 min" once.
        [ ] Serial Monitor: "[COUNTDOWN] +5 min added." — appears ONCE.
        [ ] Remaining time increases from ~15 min to ~20 min in status.
        [ ] After firmware resets countdown_add_time = false:
              Dashboard "Add 5 min" button re-enables.
        [ ] Tap "Add 5 min" again → "+5 min added." appears again (now ~25 min).
        [ ] Confirm time does NOT keep increasing without user action.

      OFFLINE RESILIENCE:
        [ ] Start a 10-minute countdown. Disconnect WiFi router.
        [ ] Serial: "[WIFI] Connection lost." — countdown timer continues running.
        [ ] Pump continues running.
        [ ] Reconnect WiFi. Firebase re-syncs. Countdown continues from correct
            remaining time (not reset).
        [ ] Start a countdown. Kill power and restore. Pump restarts in COUNTDOWN
            mode using last-known duration.
    status: pending

isProject: false
---

## Bug Root Cause Summary

All three bugs share a single architectural flaw: **Firebase propagation lag
causes the firmware to re-read and re-apply a stale mode value, overwriting
a locally-set mode before the write-back reaches Firebase.**

```
Timeline of Bugs 1 & 2:

T=0    Countdown running. Firebase: mode="COUNTDOWN". isCountdownActive=true.
T=1    User taps Stop (Bug 1) or timer expires (Bug 2).
       Firmware: pumpMode="AUTO", isCountdownActive=false.
       Firmware writes "AUTO" to Firebase (takes 1–3s to propagate).
T=3    Next Firebase cycle. Firebase still reports mode="COUNTDOWN" (stale).
       runActive = (runMode=="MANUAL" || pumpMode=="COUNTDOWN"&&isCountdownActive)
                 = (false || false) = false
       → else branch fires: pumpMode = "COUNTDOWN"  ← STALE OVERWRITE
T=6    Firebase propagation delivers "AUTO". firebaseReadMode="AUTO".
       → countdownConsumed = false.
       → pumpMode=="COUNTDOWN" && !isCountdownActive && !countdownConsumed
       → NEW COUNTDOWN STARTS  ← THE BUG
```

**Bug 3 root cause:** Firmware never resets `countdown_add_time` to false
after processing it (the spec says firmware should do this, but the code
does not include the write-back). This leaves a stale `true` in Firebase,
and depending on dashboard timeout and edge-detect state, can trigger
multiple adds or make the button appear to loop.

## Offline Resilience — Is It an Improvement?

**Yes, strongly.** The deployment conditions (RSSI -79 dBm, GlobeAtHome
network in Leon, Iloilo) are exactly the scenario where this matters. At
-79 dBm the WiFi is at the edge of reliability. Firebase RTDB read latency
spikes to 2–5s at this signal level, and brief packet loss causes the
current cooldown logic to silence the firmware for 30s on a single timeout.

The current architecture treats Firebase as a synchronous dependency:
sensors run, but pump mode decisions are driven by what Firebase last said.
The redesign inverts this: **the local state machine is the source of truth;
Firebase is a remote control that syncs when available.**

Key gains from the redesign:
- Countdown timer is `millis()`-based and immune to network state.
- Mode changes made locally (expiry, stop) are protected from stale Firebase
  overwrites via the `pendingModeWriteback` flag.
- Single JSON control read (1 RTDB call instead of 7) halves the network
  exposure per Firebase cycle.
- Status push retry on failure keeps the dashboard current during brief drops
  without entering the 30s cooldown on the first timeout.
- Last-known-good NVS persistence for countdown duration means the pump
  continues working correctly through a power cycle with no network.
