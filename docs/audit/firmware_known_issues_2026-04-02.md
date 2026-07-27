# SmartFlow Firmware Issue Register

Date: 2026-04-02 (last updated: 2026-04-03 Final scan alignment)

Scope:

- Master Node: ESP32 controller firmware in `firmware/master_node/`
- Sensor Node: NodeMCU firmware in `firmware/sensor_node/`

Purpose:

- Record confirmed bugs, logic failures, safety risks, and operational weaknesses in a way that is easy to scan.
- Keep mitigated issues visible so future regressions are easier to spot.

## 1. Severity Scale

- Critical: unsafe pump behavior or loss of safety control.
- High: repeated outages, failed recovery, or serious operator confusion.
- Medium: reliability defect that can disrupt operation or complicate support.
- Low: limited impact, but still worth tracking.

## 2. Master Node (ESP32 Controller)

### 2.1 Confirmed Issues


| ID   | Severity | Issue                                                                                                                         | Impact                                                                                                   | Status                                                                 |
| ---- | -------- | ----------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- |
| M-01 | High     | RS-485 read stalls can delay the main loop and starve Firebase work.                                                          | Control/status sync becomes unreliable under transport pressure.                                         | Mitigated (RS-485 time budget caps blocking when Firebase sync is due) |
| M-02 | High     | Firebase control and status calls are sequential and blocking.                                                                | A transport failure can cascade into repeated cloud timeouts.                                            | Mitigated in latest firmware                                           |
| M-03 | High     | Crash-loop safe mode disables WiFi, Firebase, and sensor initialization until the latch clears or the device is power-cycled. | The controller can remain offline and unreachable.                                                       | Confirmed — open                                                       |
| M-04 | High     | Safe-mode recovery depends on NTP/latch state and an auto-clear timer.                                                        | Recovery can be delayed or inconsistent if time sync does not settle cleanly.                            | Confirmed — open                                                       |
| M-05 | High     | Emergency stop must preserve the current mode and block mode changes while latched.                                           | A bad restart path could reintroduce unsafe pump behavior.                                               | Mitigated in latest firmware                                           |
| M-06 | Medium   | Safety-related state is persisted in NVS, but some runtime timers remain volatile.                                            | A reboot during active operation can reset timing context and complicate recovery logic.                 | Confirmed — open                                                       |
| M-07 | Medium   | Retry/backoff logic can mask the original upstream fault.                                                                     | Diagnosis becomes slower and recovery may appear random.                                                 | Confirmed — open                                                       |
| M-08 | Medium   | WiFi/NTP setup includes blocking waits during startup and reconnect paths.                                                    | Long blocking calls can reduce responsiveness and increase watchdog risk if the environment is unstable. | Confirmed — open                                                       |
| M-09 | High     | Secrets files are present in the firmware tree.                                                                               | Real credentials could leak if files are committed or shared incorrectly.                                | Confirmed risk — process control                                       |


### 2.2 Timing and Arithmetic Bugs


| ID   | Severity | Issue                                                                                                                                                        | Impact                                                                                                              | Status                                                                                                           |
| ---- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| M-13 | Critical | Countdown timer used `if (millis() >= countdownEndMs)` — vulnerable at ~50-day millis() wraparound. Pump could run indefinitely.                             | Timer comparison breaks at rollover: countdown may never expire or fire immediately.                                | Resolved — `millisDeadlineReached()` with `countdownEndMs != 0` guard. Verified in source.                       |
| M-14 | Critical | Multiple unsafe `millis() - X` subtractions across safety_pump, connectivity_cloud, rs485_comm. Safety cutoffs may fail at wraparound.                       | All time-based retries, cooldowns, and safety timeouts vulnerable at ~50-day rollover.                              | Resolved — all sites migrated to `elapsedMillis32()`. Verified in source.                                        |
| M-20 | High     | Firebase auth cooldown used `now + FIREBASE_AUTH_COOLDOWN_MS` without overflow protection. Permanent block risk near ULONG_MAX.                              | Firebase connection permanently blocked if timing wraps near max value.                                             | Resolved — all four cooldown assignments use `addMillisSaturated()`. Verified in source.                         |
| M-21 | High     | RS-485 frame read timeout used unsafe `(millis() - start) < timeoutMs`.                                                                                      | Frame reads hang or timeout prematurely at millis() wraparound.                                                     | Resolved — `elapsedMillis32(millis(), start) < timeoutMs`. Verified in source.                                   |
| M-32 | Medium   | Failure branch of `pollRemoteSensorNodeInternal()` used raw `(now - remoteSensorLastRxMs)` — the only remaining unsafe subtraction after the M-14 fix batch. | Offline detection logic could misfire at millis() wraparound; not a safety path but inconsistent with rest of file. | Resolved — migrated to `elapsedMillis32(now, remoteSensorLastRxMs)` with FIX [M-32] comment. Verified in source. |


### 2.3 Runtime Errors Observed


| ID   | Severity | Observation                                                                                 | Impact                                                                 | Status                                                                           |
| ---- | -------- | ------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| M-10 | Medium   | Firebase RTDB timeout on control/status reads when RS-485 is under load.                    | Dashboard actions can appear delayed or lost.                          | Partially mitigated (M-01 time budget reduces contention; residual risk remains) |
| M-11 | Medium   | RS-485 read-frame timeout bursts during normal polling.                                     | Sensor-node status can oscillate between online and offline.           | Confirmed — open                                                                 |
| M-12 | Low      | PlatformIO test runner can fail in the Unity stage even when the firmware compile succeeds. | Test execution is noisy and not fully reliable as a validation signal. | Confirmed toolchain issue                                                        |


### 2.4 Configuration and Control Logic Bugs


| ID   | Severity | Issue                                                                                                                                                                                                               | Impact                                                                           | Status                                                                                                                                                                                                                                                    |
| ---- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| M-15 | High     | Level freshness check assumed `levelLastUpdateMs > 0` but value is 0 on boot. Raw subtraction `millis() - 0` could return garbage, making level appear fresh before first sensor frame.                             | On startup, pump control decisions could act on invalid level data.              | Resolved — all freshness checks now use `(levelLastUpdateMs > 0) && elapsedMillis32(...)`. Verified at safety_pump.cpp:167, 236; connectivity_cloud.cpp:472.                                                                                              |
| M-16 | High     | Static one-shot flags in `readFirebaseControl()` (`lastEmergencyStop`, `lastCountdownStart`, `lastAddTime`, etc.) use edge-detection pattern. After soft reset, stale values can cause missed or replayed commands. | Control messages lost or duplicated after unplanned reboot.                      | Mostly resolved. Safety and mode commands now rely on Firebase self-clear (`emergency_stop`, `reset_stop`, `countdown_start`) and stale legacy flags were removed. `lastAddTime` is intentionally retained only to de-duplicate repeated `countdown_add_time` convenience pulses. |
| M-17 | Medium   | `Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium")` — appeared to be a camelCase typo.                                                                                                                               | Method call may silently fail.                                                   | Closed — not a bug. This lowercase spelling matches the upstream Firebase ESP Client library's own API. Comment added to source confirms intentional.                                                                                                     |
| M-18 | High     | `countdown_add_min` arithmetic used raw unsigned addition without overflow protection. A zero or negative value from Firebase could wrap the timer.                                                                 | Countdown timer set to undefined value; pump could run past intended duration.   | Resolved — `addMin` validated as `>= 1` before use; addition uses `addMillisSaturated()` with `min(candidate, maxEnd)` cap. Verified in source.                                                                                                           |
| M-19 | Medium   | `countdownRemainSec` returned 0 for both "timer expired" and "timer never started / canceled", making dashboard state ambiguous.                                                                                    | Operator cannot distinguish between a completed countdown and an idle countdown. | Resolved — `countdown_active` boolean field added to Firebase status push (FIX [M-19]). Dashboard can now read `countdown_active=false, remaining=0` vs `countdown_active=true, remaining=0`. Verified in source.                                         |
| M-22 | High     | `readDeviceConfigFromFirebase()` did not detect empty or partial JSON objects. An empty object would pass the `getJSON()` success check and silently leave stale config in place.                                   | Configuration reads silently fail to update fields.                              | Resolved — rate-limited warning log added for the `!allOk` path; required-field validation already rejects partial objects. Verified in source.                                                                                                           |
| M-23 | High     | Same root cause as M-15: `levelLastUpdateMs` initialized to 0 in state.cpp; startup freshness gate incorrectly passes before any RS-485 frame arrives.                                                              | Pump control decisions on startup act on undefined level data.                   | Resolved — same fix as M-15. The `levelLastUpdateMs > 0` guard closes this path across all three freshness check sites.                                                                                                                                   |


### 2.5 Initialization and State Persistence Bugs


| ID   | Severity | Issue                                                                                                             | Impact                             | Status           |
| ---- | -------- | ----------------------------------------------------------------------------------------------------------------- | ---------------------------------- | ---------------- |
| M-06 | Medium   | See 2.1 above — runtime timers (dryRunStartMs, pumpAutoStartMs, pumpOffStartMs) are volatile and reset on reboot. | Timing context lost mid-operation. | Confirmed — open |


### 2.6 New Operational & Diagnostic Findings (Round 2 Audit)


| ID   | Severity | Issue                                                                                                                                                                      | Impact                                                                       | Status                                                                                                                                                                                                                                       |
| ---- | -------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| M-24 | High     | Parse failure logging in rs485_comm.cpp used raw `Serial.print()`, bypassing syslog.                                                                                       | RS-485 parse failures invisible in production logs.                          | Resolved — `LOG(LOG_LEVEL_ERROR, "RS485-ERR", ...)` used. Verified in source.                                                                                                                                                                |
| M-25 | High     | Design conflict: overflow protection applied to MANUAL mode despite original "warning only" intent.                                                                        | Undocumented hard stop in MANUAL mode.                                       | Resolved (Option B) — MANUAL gets 90% warning then hard stop, same as AUTO/COUNTDOWN. Operator docs and QA updated. Intentional documented behavior.                                                                                         |
| M-26 | Medium   | `isLevelFresh` recomputed multiple times per control loop with separate `millis()` calls.                                                                                  | Inconsistent freshness decisions within a single control loop pass.          | Resolved — single `nowMsPump = millis()` snapshot; `levelFreshOk` computed once and reused. Verified in source.                                                                                                                              |
| M-27 | Critical | **OPERATOR LOCKOUT**: `readFirebaseControl()` returned early when e-stop latched, blocking `reset_stop` and `clear_error`. Operator could not recover without power-cycle. | Device trapped in e-stop with no remote recovery path.                       | Resolved — early return removed. E-stop latch now only blocks mode-apply branch; all other fields (`reset_stop`, `clear_error`, bypass toggles) continue processing. Verified in source.                                                     |
| M-28 | Medium   | `pushFirebaseErrorLog()` used monotonic `esp_timer_get_time()` as timestamp instead of NTP wall-clock.                                                                     | Error log timestamps in Firebase do not align with real-world calendar time. | Resolved — NTP epoch used when synced (`ntpEpochSecAtLastSync + deltaSec`), falls back to uptime seconds. Verified in source.                                                                                                                |
| M-29 | Low      | `total_pump_run_min` truncated seconds to minutes without rounding. Sub-minute runtime lost on every push.                                                                 | Cumulative runtime metric drifts over many short pump cycles.                | Resolved — `(totalPumpRunSec + 30UL) / 60UL` rounds to nearest minute. Verified in source.                                                                                                                                                   |
| M-30 | Medium   | `LOG_COMPILE_FLOOR` set to `LOG_LEVEL_INFO`, compiling out DEBUG and VERBOSE levels. Firebase `debug_log_level` runtime tuning broken for those levels.                    | Runtime log verbosity tuning via Firebase had no effect for DEBUG/VERBOSE.   | Resolved — verified in source. `LOG_COMPILE_FLOOR = LOG_LEVEL_VERBOSE` confirmed in master `config.h`. |
| M-31 | High     | `connectWiFi()` blocking loop ran before WDT registration in `setup()`. No watchdog recovery if WiFi hung beyond ~30s.                                                     | Device could hang indefinitely on boot if WiFi environment is unstable.      | Resolved — WDT registered before `connectWiFi()`; `esp_task_wdt_reset()` added inside WiFi retry loop. Verified in source.                                                                                                                   |


### 2.7 Remaining Raw Timing Patterns — Classified Safe

The following `millis() - X` subtractions remain in source but are confirmed non-safety and wrap-safe by type:


| Location                      | Pattern                                                                      | Classification                                                                                                                                                                   |
| ----------------------------- | ---------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `connectivity_cloud.cpp` (×4) | `millis() - t0` for call-duration telemetry                                  | Safe — t0 captured immediately before call; result is always < 15s; telemetry only                                                                                               |
| `persistence.cpp` (×2)        | `now - lastLevelWriteMs`, `now - lastUptimeWriteMs`                          | Safe — NVS write throttles; worst case at wraparound is extra or deferred writes; no safety impact                                                                               |
| `safety_pump.cpp`             | `millis() - pumpOnSinceMs` for runtime accumulation                          | Safe — both are `unsigned long`; C unsigned subtraction is wrap-safe for durations < 49 days; if pump runs past 49 days the accumulated seconds count wraps, which is acceptable |
| `sensors.cpp` (×3)            | `now - lastFlowCalcMs`, `now - lastDbgMs`, `millis() - lastLvlDiscardWarnMs` | Safe — all variables are `uint32_t` or `unsigned long`; intervals are seconds to minutes; unsigned subtraction is inherently wrap-safe                                           |
| `rs485_slave.cpp`             | `millis() - lastByteMs`                                                      | Safe — `lastByteMs` is `static uint32_t`; 20ms interval; unsigned subtraction wrap-safe                                                                                          |


### 2.8 Master Node Notes & Recommendations

- **All Critical items resolved**: M-13, M-14, M-27. No remaining critical-severity open items on master node.
- **All timing wraparound sites resolved**: M-13, M-14, M-20, M-21, M-32. Remaining subtractions are classified safe (see 2.7).
- **M-16 lastAddTime**: Intentionally left edge-detected only for convenience command de-dup. One-shot clear is now always attempted whenever `countdown_add_time=true`.
- **M-30 verified in source**: `LOG_COMPILE_FLOOR = LOG_LEVEL_VERBOSE` is set in master `config.h`; retain field log-level smoke test in validation checklist.
- **Open operational items**: M-03 (safe mode reachability), M-04 (NTP-dependent recovery), M-06 (volatile timers), M-07 (log masking), M-08 (blocking startup), M-09 (secrets hygiene), M-11 (RS-485 oscillation).

### 2.9 New Findings (2026-04-03 Final Scan)

| ID     | Severity | Issue | Impact | Status |
| ------ | -------- | ----- | ------ | ------ |
| NEW-01 | Low | `firebaseCooldownUntilMs = millis() + 10000UL` in WiFi reconnect path (`main.cpp`) uses raw addition and can overflow near 49-day wraparound. | 10s cooldown can be skipped at rollover; cosmetic reliability issue (non-safety). | Open — non-blocking for flash. Recommended fix: `addMillisSaturated(millis(), 10000UL)`. |
| NEW-02 | Low (Cosmetic) | Routine telemetry logs in `main.cpp` were emitted at `LOG_LEVEL_ERROR` instead of `LOG_LEVEL_INFO`. | Healthy operation appears as errors in syslog/Firebase logs; real errors become noisier to triage. | Open — non-blocking for flash. Recommended fix: downgrade those two calls to INFO. |
| NEW-03 | Low (Cosmetic) | Sleep wake target arithmetic uses raw `lastSensorMs + SLEEP_WAKE_INTERVAL_MS` without overflow protection in sleep block. | At rollover, sleep cadence can briefly tighten before self-recovering; no pump safety effect. | Open — non-blocking for flash. Recommended fix: `addMillisSaturated(lastSensorMs, SLEEP_WAKE_INTERVAL_MS)`. |

---

## 3. Sensor Node (NodeMCU)

### 3.1 Confirmed Issues


| ID          | Severity | Issue                                                                                                                                                                                         | Impact                                                                                                              | Status                                                                                                                                                                                                                                              |
| ----------- | -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| S-01        | Medium   | RS-485 slave framing uses a short partial-frame stall reset to avoid hanging on malformed packets.                                                                                            | Incomplete or noisy packets can interrupt polling until the receiver resets.                                        | Confirmed — open                                                                                                                                                                                                                                    |
| S-02        | Medium   | Response turnaround is immediate after valid packet reception.                                                                                                                                | If the Master has not fully switched to RX, the first reply can be lost.                                            | Confirmed risk — open                                                                                                                                                                                                                               |
| S-03        | Medium   | Ultrasonic plausibility filtering can reject large level jumps.                                                                                                                               | Real level changes may be delayed if they look like outliers.                                                       | Confirmed — open                                                                                                                                                                                                                                    |
| S-04        | Medium   | Flow hardening uses aggressive deglitching and diagnostic tuning values.                                                                                                                      | Legitimate pulses may be dropped if the installed sensor behaves differently in the field.                          | Needs field validation                                                                                                                                                                                                                              |
| S-05        | Low      | Sensor-node logic is primarily telemetry-oriented and depends on the Master for final safety action.                                                                                          | A Master-side fault can delay the actual safety response.                                                           | Design limitation                                                                                                                                                                                                                                   |
| S-06 / S-09 | High     | `flowRawEdgeCount` was incremented unconditionally at ISR entry without atomic protection, racing with main-loop reads. Semantics differed from `flowPulseCount` (which was correctly gated). | ISR-main race could corrupt flow diagnostic counter.                                                                | Resolved — `flowRawEdgeCount++` moved inside the deglitch guard alongside `flowPulseCount++`. Both counters now share identical ISR semantics. `noInterrupts()` block in main loop safely covers both. FIX [S-06/S-09] comment in source. Verified. |
| S-07        | Medium   | Sensor node does not explicitly signal "tank full". Master relies on `cfgPumpStopLevel` threshold only.                                                                                       | If sensor reads slightly below stop level when tank is actually full, overflow relies solely on max-runtime cutoff. | Confirmed limitation — design                                                                                                                                                                                                                       |


### 3.2 Timing Issues (Sensor Node)


| ID   | Severity | Issue                                                                                                                                                                                                        | Impact                                                            | Status                |
| ---- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------- | --------------------- |
| S-08 | Medium   | The 20ms partial-frame stall reset is a hardcoded magic number with no documented derivation. At 115200 baud, 20ms equals ~230 byte-times — could be excessive or insufficient depending on line transients. | Frame recovery may be inconsistent or introduce false boundaries. | Confirmed risk — open |


### 3.3 Telemetry & Diagnostic Findings (Round 2 Audit)


| ID   | Severity | Issue                                                                                                                                                                                      | Impact                                                                                                                                          | Status                                                                                                                      |
| ---- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| S-10 | Medium   | `snLevelDiscardCount` is `uint16_t` but cast to `uint8_t` (clamped at 255) before packing into the RS-485 frame. Values above 255 lose granularity in telemetry.                           | `remoteSensorLevelDiscardCount` on Master cannot reliably show aggressive filtering above 255 discards.                                         | Confirmed reporting gap — accepted. Saturation at 255 is better than wrapping. No safety impact.                            |
| S-11 | Medium   | `snLevelDiscardCount` resets to 0 at the start of each 1-second measurement window (`usResetWindow()`). The RS-485 frame value is a per-window snapshot, not a cumulative counter.         | Dashboard displays `remote_level_discard_count` as if cumulative; operators cannot assess long-term sensor filter health from this field alone. | Confirmed semantic confusion — documentation needed. Dashboard and field-service guides must note the per-window semantics. |
| S-12 | Low      | `usMedianValid()` returns index `n/2` (upper median for even windows, e.g. n=4 returns index 2). Undocumented. Slight downward bias on fill-level readings for even-count windows (~1–3%). | Minor overflow detection delay on even-sample windows. Acceptable in practice given 5-sample window (odd).                                      | Confirmed design choice — documentation needed. Add code comment explaining upper-median selection.                         |
| S-13 | Low      | `PIN_FLOW_INPUT` assigned to GPIO12/D6 with a comment in `config.h` marked *"temporary diagnostic reroute"*. The canonical pin was never confirmed or reverted. No comment documents the original pin or rationale for the reroute. | If the reroute was load-bearing (e.g. avoiding a boot-strapping conflict on D3/D7/D8), silent reassignment could break a working sensor node. Field wiring may be inconsistent with source. | Open — requires hardware verification with board in hand. Do not change `config.h` until verified. See `docs/specs/firmware.md` Hardware Interface section. |


### 3.4 Sensor Node Notes & Recommendations

- **S-06/S-09 resolved**: ISR now consistent — `flowRawEdgeCount` and `flowPulseCount` increment on the same accepted-pulse path.
- **S-10, S-11**: No firmware change needed. Dashboard documentation must clarify that `remote_level_discard_count` is a per-window (not cumulative) counter, resetting every ~1 second.
- **S-12**: Add a single-line code comment in `usMedianValid()` documenting the upper-median choice. No behavior change needed.
- **S-08**: Documenting the 20ms stall-reset derivation is recommended before long-term deployment: at 115200 baud, one byte is ~87µs; the 20ms window covers ~230 byte-times, which is intentionally conservative to handle line-ringing on a 40m CAT6 run.
- **S-13**: `PIN_FLOW_INPUT` rerouted to GPIO12/D6 per a comment marked "temporary" in `config.h`; canonical pin never confirmed or reverted. **Do not change without hardware verification.** See `docs/specs/firmware.md` Hardware Interface table.


---

## 4. RTDB Design Structure

The default export shows a single top-level namespace under `/pump_system/` with five main branches.

### 4.1 Top-Level Tree


| Path                        | Purpose                                         | Notes                                                                              |
| --------------------------- | ----------------------------------------------- | ---------------------------------------------------------------------------------- |
| `/pump_system/audit/events` | Append-only audit trail.                        | Stores actor, timestamps, device ID, email, and optional metadata.                 |
| `/pump_system/config`       | Shared configuration and notification settings. | Contains admin lists, device tuning values, and per-user notification preferences. |
| `/pump_system/control`      | Dashboard-to-controller command surface.        | One-shot flags and mode fields are written here by the UI.                         |
| `/pump_system/presence`     | Device/user presence markers.                   | Tracks active device sessions by user and device key.                              |
| `/pump_system/status`       | Controller telemetry and health snapshot.       | Written by the ESP32 and consumed by the dashboard.                                |


### 4.2 `/pump_system/config`


| Subpath                                      | Purpose                                  | Example fields                                                                                                                          |
| -------------------------------------------- | ---------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| `/pump_system/config/admins`                 | Authorized admin UID map.                | `true` flags keyed by Firebase UID.                                                                                                     |
| `/pump_system/config/device`                 | System tuning values.                    | `tank_empty_cm`, `tank_full_cm`, `pump_start_level`, `pump_stop_level`, `dry_run_timeout_sec`, `max_pump_runtime_min`, `sleep_enabled`. |
| `/pump_system/config/notifications`          | Default notification policy.             | `enabled`, `email`, `lowLevelAlert`, `dryRunAlert`, `pumpStartedAlert`.                                                                 |
| `/pump_system/config/notifications_by_user`  | Per-user notification policy and tokens. | User-scoped `fcmTokens`, thresholds, and push flags.                                                                                    |
| `/pump_system/config/notification_last_sent` | Anti-spam timestamps.                    | Last send times for `dryRun`, `lowLevel`, and `pumpStarted`.                                                                            |


### 4.3 `/pump_system/control`


| Field                    | Type    | Meaning                                                |
| ------------------------ | ------- | ------------------------------------------------------ |
| `mode`                   | string  | Main operating mode: `AUTO`, `MANUAL`, or `COUNTDOWN`. |
| `manual_desired`         | boolean | Manual pump intent flag.                               |
| `countdown_start`        | boolean | One-shot trigger to start countdown mode.              |
| `countdown_stop`         | boolean | One-shot trigger to stop countdown mode.               |
| `countdown_add_time`     | boolean | One-shot trigger to add countdown time.                |
| `countdown_add_min`      | number  | Minutes to add when countdown extension is requested.  |
| `countdown_duration_min` | number  | Countdown duration requested by the dashboard.         |
| `timed_start_sec`        | number  | Timed start duration in seconds.                       |
| `emergency_stop`         | boolean | One-shot emergency stop request.                       |
| `clear_error`            | boolean | Acknowledge/clear error request.                       |
| `reset_stop`             | boolean | Clear emergency-stop latch request.                    |
| `bypass_level_sensor`    | boolean | Allow control without level-sensor gating.             |
| `bypass_flow_sensor`     | boolean | Allow control without flow-sensor dry-run gating.      |
| `reboot_request_id`      | number  | Reboot request counter or id.                          |


### 4.4 `/pump_system/status`

This branch is the controller's runtime snapshot.


| Group                   | Representative fields                                                                                                                               |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| Pump and mode state     | `is_running`, `run_mode`, `manual_desired`, `manual_runtime_warning`, `countdown_active`, `countdown_remaining_sec`, `pump_cooldown_remaining_sec`  |
| Safety and faults       | `is_error`, `is_flow_sensor_error`, `is_level_sensor_error`, `is_overflow_error`, `emergency_stop_latched`, `last_fault_code`, `last_fault_message` |
| Sensor and telemetry    | `flow_rate_lpm`, `flow_volume_added_l`, `remote_level_discard_count`, `level_fresh`, `level_last_valid_age_sec`, `level_sensor_health_pct`          |
| Connectivity and timing | `wifi_rssi`, `rs485_last_call_ms`, `cloud_last_control_call_ms`, `cloud_last_status_call_ms`, `cloud_last_cycle_ms`, `loop_max_ms`                  |
| Firebase health         | `firebase_consecutive_failures`, `firebase_timeout_count`, `firebase_auth_error_count`, `firebase_not_ready_skip_count`, `firebase_last_error`      |
| Resource health         | `free_heap_bytes`, `min_free_heap_bytes`, `min_free_heap_observed_bytes`, `max_alloc_heap_bytes`                                                    |
| Runtime counters        | `total_pump_cycles`, `total_pump_run_min`, `ultrasonic_cycles_ok`, `ultrasonic_cycles_timeout`, `flow_stuck_high_events`                            |


**New field added (M-19 fix):** `countdown_active` (boolean) — distinguishes "timer expired/stopped" from "timer never started". Dashboard should read this field alongside `countdown_remaining_sec`.

### 4.5 `/pump_system/audit/events`


| Field          | Meaning                                                                  |
| -------------- | ------------------------------------------------------------------------ |
| `action`       | Audit event type such as `control.set_mode` or `control.emergency_stop`. |
| `at`, `at_ms`  | Event timestamps.                                                        |
| `deviceId`     | Source device identifier.                                                |
| `email`, `uid` | Acting user identity.                                                    |
| `meta`         | Optional event-specific metadata.                                        |


### 4.6 `/pump_system/presence`


| Field          | Meaning                                            |
| -------------- | -------------------------------------------------- |
| Key format     | Usually `${uid}_${deviceId}`.                      |
| `at`           | Last presence timestamp.                           |
| `email`, `uid` | User identity associated with the presence marker. |


---

## 5. Cross-System Risks


| ID   | Severity | Risk                                                                                         | Impact                                                                                | Status                                                                                                                                   |
| ---- | -------- | -------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| X-01 | Critical | Any condition leaving pump ON during a communication fault.                                  | Most important safety failure mode.                                                   | Boot-window vulnerability (M-15/M-23) closed. Freshness gates enforce `levelLastUpdateMs > 0`. Must never regress.                       |
| X-02 | High     | Deprecated `FORCE_ON`/`FORCE_OFF` mode mapping to AUTO could override a safer current state. | Stale dashboard command causes unexpected mode transition.                            | Confirmed risk — open. Code maps FORCE modes to AUTO with `pendingModeWriteback`. Low risk but warrants documentation in operator guide. |
| X-03 | Medium   | Persistent NVS/Firebase state outlives a reboot. Stale flags affect startup behavior.        | Bypass flags, dry_run_err, or mode from a previous session can re-apply unexpectedly. | Confirmed — open                                                                                                                         |


---

## 6. Open Items to Watch

### Immediate Priority

- **NEW-01**: Replace raw cooldown addition in main WiFi reconnect path with `addMillisSaturated(...)`.
- **NEW-02**: Reclassify routine telemetry logs from ERROR to INFO in `main.cpp`.
- **NEW-03**: Harden sleep wake arithmetic with `addMillisSaturated(...)` in `main.cpp`.
- **S-11**: Keep dashboard/operator guide aligned that `remote_level_discard_count` is per-window and non-cumulative.
- **X-02**: Document FORCE_ON/FORCE_OFF mapping behavior in operator guide.

### Long-Term Hardening

- **M-03, M-04**: Improve safe-mode reachability — consider whether a minimal HTTP endpoint or Firebase presence write could survive safe mode for remote diagnosis.
- **M-06**: Evaluate persisting `pumpAutoStartMs` to NVS so overflow protection survives a reboot mid-run.
- **M-08**: Consider non-blocking WiFi connect pattern to eliminate 20s startup block.
- **M-16 (lastAddTime)**: If field feedback shows repeated missed add-time actions under poor connectivity, replace edge de-dup with command-id semantics.

### Field Validation

- Confirm emergency-stop and reset-stop behavior after repeated dashboard toggles and network faults (validates M-16, M-27 fixes).
- Validate flow sensor tuning (S-04, `FLOW_MIN_PULSE_INTERVAL_US = 5000µs`) against installed YF-G1 in field conditions.
- Monitor `remote_level_discard_count` behavior (S-11) against tank geometry and wave-reflection noise.
- Keep secrets files out of version control; ensure example files stay sanitized (M-09).

---

## 7. Traceability Notes

- This document is intentionally operational, not academic.
- Items marked as resolved remain listed so future regressions are easier to detect.
- Add new findings only after they are reproduced on hardware, seen in logs, or verified in code.
- All "Resolved — verified in source" entries were confirmed by direct code scan of the project files on 2026-04-02.

### Audit Summary

- **Date of latest scan**: 2026-04-03 (final firmware scan alignment)
- **Previously tracked fixes**: 20/20 confirmed present.
- **Safe timing patterns**: 11 patterns reviewed and classified non-issue.
- **New findings**: 3 (NEW-01..NEW-03), all low/cosmetic and non-blocking for flash.
- **Open safety-critical issues**: 0.
- **Flash recommendation**: Safe to flash now; patch NEW-01..NEW-03 in next maintenance update cycle.

