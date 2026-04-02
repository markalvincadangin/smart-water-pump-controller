# SmartFlow Firmware Issue Register

Date: 2026-04-02

Scope:

- Master Node: ESP32 controller firmware in `firmware/platformio_smart_water_pump_controller/`
- Sensor Node: NodeMCU firmware in `firmware/platformio_sensor_node/`

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

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| M-01 | High | RS-485 read stalls can delay the main loop and starve Firebase work. | Control/status sync becomes unreliable under transport pressure. | Confirmed |
| M-02 | High | Firebase control and status calls are sequential and blocking. | A transport failure can cascade into repeated cloud timeouts. | Mitigated in latest firmware |
| M-03 | High | Crash-loop safe mode disables WiFi, Firebase, and sensor initialization until the latch clears or the device is power-cycled. | The controller can remain offline and unreachable. | Confirmed |
| M-04 | High | Safe-mode recovery depends on NTP/latch state and an auto-clear timer. | Recovery can be delayed or inconsistent if time sync does not settle cleanly. | Confirmed |
| M-05 | High | Emergency stop must preserve the current mode and block mode changes while latched. | A bad restart path could reintroduce unsafe pump behavior. | Mitigated in latest firmware |
| M-06 | Medium | Safety-related state is persisted in NVS, but some runtime timers remain volatile. | A reboot during active operation can reset timing context and complicate recovery logic. | Confirmed |
| M-07 | Medium | Retry/backoff logic can mask the original upstream fault. | Diagnosis becomes slower and recovery may appear random. | Confirmed |
| M-08 | Medium | WiFi/NTP setup includes blocking waits during startup and reconnect paths. | Long blocking calls can reduce responsiveness and increase watchdog risk if the environment is unstable. | Confirmed |
| M-09 | High | Secrets files are present in the firmware tree. | Real credentials could leak if files are committed or shared incorrectly. | Confirmed risk |

### 2.2 Additional Timing and Arithmetic Bugs (Critical)

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| M-13 | Critical | Countdown timer millis() overflow vulnerability at line 165: `if (millis() >= countdownEndMs)`. Does not handle the ~50-day wraparound that occurs with `unsigned long` on Arduino. | At wraparound, the countdown timer comparison breaks and may never expire, or expire immediately. Pump could run indefinitely. | Confirmed vulnerability |
| M-14 | Critical | Unsafe timer difference calculation in multiple locations: `millis() - pendingModeWritebackSentMs >= threshold` (line 246), `(millis() - start) < timeoutMs` (rs485_comm.cpp line 18), `(millis() - levelSensorFailStartMs)` (line 37 in safety_pump.cpp). Standard pattern is vulnerable at wraparound. | All time-based retries, cooldowns, and timeouts can fail at the ~50-day wraparound. Safety cutoffs may not trigger. | Confirmed vulnerability |
| M-20 | High | Firebase auth cooldown calculation uses `max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS)` without overflow protection at line 200. If `now` approaches ULONG_MAX, the sum overflows. | Firebase connection can be permanently blocked if timing wraps near max value. | Confirmed risk |
| M-21 | High | RS-485 frame read timeout at rs485_comm.cpp line 18 uses the unsafe subtraction pattern `(millis() - start) < timeoutMs`. | Frame reads can hang indefinitely or timeout prematurely if millis() wraps during a read. | Confirmed vulnerability |

### 2.3 Runtime Errors Observed

| ID | Severity | Observation | Impact | Status |
|---|---|---|---|---|
| M-10 | Medium | Firebase RTDB timeout on control/status reads when RS-485 is under load. | Dashboard actions can appear delayed or lost. | Confirmed |
| M-11 | Medium | RS-485 read-frame timeout bursts during normal polling. | Sensor-node status can oscillate between online and offline. | Confirmed |
| M-12 | Low | PlatformIO test runner can fail in the Unity stage even when the firmware compile succeeds. | Test execution is noisy and not fully reliable as a validation signal. | Confirmed toolchain issue |

### 2.4 Configuration and Control Logic Bugs

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| M-15 | High | LEVEL_STALE_TIMEOUT_MS freshness check at line 458 assumes `levelLastUpdateMs > 0`, but on first boot this value is 0. The subtraction `(millis() - 0)` can return any value from 0 to ULONG_MAX. | The level freshness flag may be incorrectly set to true on startup, allowing control actions with stale data. | Confirmed vulnerability |
| M-16 | High | Static `bool` variables in `readFirebaseControl()` (countdownConsumed, lastAddTime, lastEmergencyStop, etc., lines 175–179) are never cleared except by setting their flags. An unsupervised soft reset may retain stale state. | Control messages from Firebase can be lost or misclassified after a crash/reboot if the static state is not synchronized with hardware state. | Confirmed risk |
| M-17 | Medium | Firebase configuration call at line 621: `Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium")`. The method name has incorrect camelCase: should be `setWriteSizeLimit`. | Method call may silently fail or not be invoked, leaving size limit at default. | Confirmed typo |
| M-18 | High | Countdown add-time value is not validated before being added to the timer. Line 360 does `countdownEndMs = min(countdownEndMs + (unsigned long)addMin * 60000UL, maxEnd)` without checking if `addMin` is negative or zero in Firebase. | A negative `countdown_add_min` value from the dashboard could wrap the unsigned arithmetic and cause an undefined timer value. | Confirmed vulnerability |
| M-19 | Medium | Countdown remaining time at lines 517–518: `countdownRemainSec = (countdownEndMs > nowMs) ? (int32_t)((countdownEndMs - nowMs) / 1000UL) : 0` returns 0 only when time has elapsed, but never explicitly handles the case where the countdown was canceled. | Dashboard displays 0 for both "timer complete" and "timer invalid", causing operator confusion. | Confirmed bug |
| M-22 | High | Firebase `getJSON()` checks success with `if (!Firebase.RTDB.getJSON(...)) return;` but does not validate that the returned JSON is non-empty or has expected fields. An empty object or partial object will still pass. | Configuration reads can silently fail to parse fields, leaving stale config values in place. | Confirmed vulnerability |

### 2.5 Initialization and State Persistence Bugs

| M-23 | High | Level freshness calculation on startup: if `levelLastUpdateMs` is initialized to 0, the check at line 458 `(millis() - levelLastUpdateMs)` can underflow or return a garbage value until the first sensor update arrives. | On startup, before the sensor node responds, level freshness is undefined, and control logic may act on invalid state. | Confirmed vulnerability |

### 2.6 New Operational & Diagnostic Findings (Round 2 Audit)

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| M-24 | High | Parse failure logging in rs485_comm.cpp uses raw `Serial.print()` instead of the `LOG()` macro (lines 232–234). These error messages bypass syslog and are invisible in production logs and remote monitoring. | Parse failures silently disappear from diagnostic telemetry, making RS-485 communication issues invisible to operators and support. | Confirmed vulnerability |
| M-25 | High | ~~DESIGN CONFLICT~~ **Resolved (Option B, 2026-04-02):** `checkOverflowProtection()` applies max runtime to AUTO, COUNTDOWN, and MANUAL (90% `manual_runtime_warning`, then hard stop). QA spec and operator docs updated to match firmware; MANUAL does not bypass `max_pump_runtime_min`. | Documented behavior; operators should treat MANUAL as still bounded by max runtime. | Resolved |
| M-26 | Medium | `isLevelFresh` is computed once at the start of `executePumpLogic()` (line 233) but then recalculated inside at least 3 separate branches (MANUAL path line 275, COUNTDOWN path line 306, AUTO path line 346). Each recalculation uses a new `millis()` call, so results can differ between the outer guard and inner branches. | A race condition can cause inconsistent level-freshness decisions within the same control loop, leading to unpredictable mode transitions. | Confirmed vulnerability |
| M-27 | **Critical** | **OPERATOR LOCKOUT**: When `emergencyStopLatched` is true, `readFirebaseControl()` returns early on line 235, skipping all subsequent Firebase field processing. This means `reset_stop` (which clears the latch) and `clear_error` can never be processed while the latch is active. An operator cannot clear a dry-run error or reset the e-stop without power-cycling the device. | A device in emergency-stop state is trapped until hard power-cycle. This is both a usability failure (operator frustration) and a safety concern (loss of remote control). | Confirmed critical usability & safety issue |
| M-28 | Medium | `pushFirebaseErrorLog()` uses `esp_timer_get_time()` (monotonic uptime in microseconds, converted to seconds) as the log timestamp on line 652. It does not use NTP wall-clock time. Logged errors show relative boot time, not absolute calendar time. | Cross-referencing error logs in Firebase with dashboard events (which may use wall-clock time) becomes difficult; forensic analysis requires knowing the device's exact boot time. | Confirmed diagnostic gap |
| M-29 | Low | In `pushFirebaseStatus()`, line 539 divides `totalPumpRunSec` by 60 and truncates to int before sending as `total_pump_run_min`. Sub-minute accumulated runtime is silently lost every push. Over many short pump cycles this accumulates reporting drift. | Long-term cumulative runtime metrics in Firebase drift from actual hardware records; the error is silent and undetectable from the dashboard. | Confirmed reporting gap |
| M-30 | Medium | The `LOG()` macro has `LOG_COMPILE_FLOOR` hardcoded to `LOG_LEVEL_INFO` (level 2). This means `LOG_LEVEL_DEBUG` (3) and `LOG_LEVEL_VERBOSE` (4) log statements are compiled out entirely (not at runtime). The Firebase `debug_log_level` config field can set `gLogLevel` at startup, but it cannot enable DEBUG/VERBOSE logs because the code is literally not in the binary. | Runtime log-level tuning via Firebase is partially broken; operators cannot enable finer diagnostics without recompiling the firmware. | Confirmed design limitation |
| M-31 | High | `connectWiFi()` blocking loop (40 × 500ms retry) runs **before** watchdog registration in `setup()`. If the WiFi environment hangs beyond ~30s, there is no WDT recovery mechanism. The log message says "failed after 20s" which is accidentally correct (40 × 500ms = 20s) but the loop count is an undocumented magic number. | If environmental WiFi issues cause longer hangs, the watchdog cannot rescue the device; it will timeout only after the WiFi loop returns or dies internally. | Confirmed risk |

### 2.7 Master Node Notes & Recommendations

- **CRITICAL ACTION ITEMS:**
  - **M-27 (operator lockout)** must be fixed before production use: allow `reset_stop` and `clear_error` to be processed even when `emergencyStopLatched` is true.
  - ~~**M-25 (design conflict)**~~ **Done:** Option B — same hard cap as AUTO/COUNTDOWN; documentation and QA updated.
  - **M-13, M-14 millis() wraparound** must be fixed before any device is deployed for >50 days continuous operation.

- **SECONDARY ISSUES:**
  - **M-24, M-30** reduce observability in production; add them to the diagnostic hardening roadmap.
  - **M-26** can cause unpredictable behavior under load; consider caching `isLevelFresh` once per loop.

- The initial emergency-stop changes (M-05 mitigation) were valuable, but these newer findings show gaps in edge-case control flow and state recovery.

## 3. Sensor Node (NodeMCU)

### 3.1 Confirmed Issues

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| S-01 | Medium | RS-485 slave framing uses a short partial-frame stall reset to avoid hanging on malformed packets. | Incomplete or noisy packets can interrupt polling until the receiver resets. | Confirmed |
| S-02 | Medium | Response turnaround is immediate after valid packet reception. | If the Master has not fully switched to RX, the first reply can be lost. | Confirmed risk |
| S-03 | Medium | Ultrasonic plausibility filtering can reject large level jumps. | Real level changes may be delayed if they look like outliers. | Confirmed |
| S-04 | Medium | Flow hardening uses aggressive deglitching and diagnostic tuning values. | Legitimate pulses may be dropped if the installed sensor behaves differently in the field. | Needs field validation |
| S-05 | Low | Sensor-node logic is primarily telemetry-oriented and depends on the Master for final safety action. | A Master-side fault can delay the actual safety response. | Design limitation |
| S-06 | High | Flow ISR at sensors.cpp line 10 increments volatile `flowRawEdgeCount++` without atomic protection, and the variable is never cleared or synchronized with the main loop. | ISR-main race condition: flow count can be corrupted, or pulses can be lost if the counter is cleared between ISR execution and main-loop read. | Confirmed vulnerability |
| S-07 | Medium | Sensor node does not explicitly detect or signal when the tank has reached full level (100% capacity). The Master node's dry-run and overflow checks assume the sensor is working but have no explicit "tank full" safety gate. | If the tank is filled beyond the configured `tank_full_cm`, the pump has no direct sensor-node signal to stop it. | Confirmed limitation |

### 3.2 Additional Timing Issues (Sensor Node)

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| S-08 | Medium | The partial-frame stall reset time (20ms at rs485_slave.cpp line 90) is a hardcoded magic number with no documented justification for the chosen window. At high baud rates or with latency jitter, this may be insufficient or excessive. | Frame recovery can be inconsistent or introduce false packet boundaries. | Confirmed risk |

### 3.3 New Findings from Code Audit (Round 2)

| ID | Severity | Issue | Impact | Status |
|---|---|---|---|---|
| S-09 | High | `flowRawEdgeCount` is incremented in the ISR without atomic protection (line 12 in sensors.cpp flowIsr()). The variable is marked `volatile` but reads in the main loop are not interrupt-guarded. The counter is **never used** for safety decisions but its semantics differ from `flowPulseCount` (which is guarded). The zeroing inside `noInterrupts()` block (line 173) gives a false sense of safety — the ISR increment outside the block races. | Silent data corruption: ISR-main race can increment the counter simultaneously with a main-loop read, corrupting the value. The unused counter masks the pattern that **should** be applied to flowPulseCount as well. | Confirmed vulnerability |
| S-10 | Medium | `snLevelDiscardCount` is `uint16_t` (line 35 in rs485_slave.cpp). It is cast to `uint8_t` before packing into the frame (line 40): `uint8_t ldsc = (snLevelDiscardCount > 255) ? 255 : (uint8_t)snLevelDiscardCount`. Values >255 silently wrap to 0–254, losing diagnostic information about sensor plausibility failures. | Telemetry of discard count is incomplete: the Master's `remoteSensorLevelDiscardCount` can never reliably show that the sensor node is filtering aggressively. | Confirmed reporting gap |
| S-11 | Medium | `snLevelDiscardCount` is reset to 0 inside `usResetWindow()` (line 51 in sensors.cpp) at the start of each measurement window (~every 1 second per US_MEAS_INTERVAL_MS). The value in the RS-485 frame reflects only the current window's discards, not cumulative sensor health. | Dashboard interpretation bug: `remote_level_discard_count` is a **per-window snapshot**, not cumulative health counter, but it is displayed/logged as if it were cumulative. Operators cannot determine long-term sensor filter health from this metric. | Confirmed semantic confusion |
| S-12 | Low | `usMedianValid()` sorts the array and returns index `n/2`. For even-count windows (e.g., n=4 returns index 2), this is the upper median rather than the average of the two middle values. This is a valid median choice but is undocumented. The bias is: the returned value is slightly toward higher distances (lower percentage fill), which could delay overflow detection by 1–3%. | Level readings have a subtle upward distance bias (downward fill-level bias) in even-window medians. Over time, the pump may not detect high water levels as quickly as expected. | Confirmed design choice — needs documentation |

### 3.4 Sensor Node Notes & Recommendations

- **ISR SAFETY (S-09)**: The `flowRawEdgeCount` race is a symptom of incomplete ISR hardening. This pattern is risky even if unused; fix it and audit other ISR interactions.
- **TELEMETRY CLARITY (S-10, S-11)**: The discard-count semantics are misaligned with the dashboard display. Document that `remote_level_discard_count` resets every measurement window and is not cumulative.
- **MEDIAN BIAS (S-12)**: The plausibility filter's upper-median choice is reasonable but should be documented as a known level-reading offset to explain minor overflow-detection delays.
- The sensor node is intentionally lightweight and mostly reports data, but these findings show that telemetry semantics and ISR patterns need operator documentation and careful review before field deployment.

## 4. RTDB Design Structure

The default export shows a single top-level namespace under `/pump_system/` with five main branches.

### 4.1 Top-Level Tree

| Path | Purpose | Notes |
|---|---|---|
| `/pump_system/audit/events` | Append-only audit trail. | Stores actor, timestamps, device ID, email, and optional metadata. |
| `/pump_system/config` | Shared configuration and notification settings. | Contains admin lists, device tuning values, and per-user notification preferences. |
| `/pump_system/control` | Dashboard-to-controller command surface. | One-shot flags and mode fields are written here by the UI. |
| `/pump_system/presence` | Device/user presence markers. | Tracks active device sessions by user and device key. |
| `/pump_system/status` | Controller telemetry and health snapshot. | Written by the ESP32 and consumed by the dashboard. |

### 4.2 `/pump_system/config`

| Subpath | Purpose | Example fields |
|---|---|---|
| `/pump_system/config/admins` | Authorized admin UID map. | `true` flags keyed by Firebase UID. |
| `/pump_system/config/device` | System tuning values. | `tank_empty_cm`, `tank_full_cm`, `pump_start_level`, `pump_stop_level`, `dry_run_timeout_sec`, `max_pump_runtime_min`, `sleep_enabled`. |
| `/pump_system/config/notifications` | Default notification policy. | `enabled`, `email`, `lowLevelAlert`, `dryRunAlert`, `pumpStartedAlert`. |
| `/pump_system/config/notifications_by_user` | Per-user notification policy and tokens. | User-scoped `fcmTokens`, thresholds, and push flags. |
| `/pump_system/config/notification_last_sent` | Anti-spam timestamps. | Last send times for `dryRun`, `lowLevel`, and `pumpStarted`. |

### 4.3 `/pump_system/control`

| Field | Type | Meaning |
|---|---|---|
| `mode` | string | Main operating mode, typically `AUTO`, `MANUAL`, or `COUNTDOWN`. |
| `manual_desired` | boolean | Manual pump intent flag. |
| `countdown_start` | boolean | One-shot trigger to start countdown mode. |
| `countdown_stop` | boolean | One-shot trigger to stop countdown mode. |
| `countdown_add_time` | boolean | One-shot trigger to add countdown time. |
| `countdown_add_min` | number | Minutes to add when countdown extension is requested. |
| `countdown_duration_min` | number | Countdown duration requested by the dashboard. |
| `timed_start_sec` | number | Timed start duration in seconds. |
| `emergency_stop` | boolean | One-shot emergency stop request. |
| `clear_error` | boolean | Acknowledge/clear error request. |
| `reset_stop` | boolean | Clear emergency-stop latch request. |
| `bypass_level_sensor` | boolean | Allow control without level-sensor gating. |
| `bypass_flow_sensor` | boolean | Allow control without flow-sensor dry-run gating. |
| `reboot_request_id` | number | Reboot request counter or id. |

### 4.4 `/pump_system/status`

This branch is the controller’s runtime snapshot. The export shows the following groups:

| Group | Representative fields |
|---|---|
| Pump and mode state | `is_running`, `run_mode`, `manual_desired`, `manual_runtime_warning`, `countdown_remaining_sec`, `pump_cooldown_remaining_sec` |
| Safety and faults | `is_error`, `is_flow_sensor_error`, `is_level_sensor_error`, `is_overflow_error`, `emergency_stop_latched`, `last_fault_code`, `last_fault_message` |
| Sensor and telemetry | `flow_rate_lpm`, `flow_volume_added_l`, `remote_level_discard_count`, `level_fresh`, `level_last_valid_age_sec`, `level_sensor_health_pct` |
| Connectivity and timing | `wifi_rssi`, `rs485_last_call_ms`, `cloud_last_control_call_ms`, `cloud_last_status_call_ms`, `cloud_last_cycle_ms`, `loop_max_ms` |
| Firebase health | `firebase_consecutive_failures`, `firebase_timeout_count`, `firebase_auth_error_count`, `firebase_not_ready_skip_count`, `firebase_last_error` |
| Resource health | `free_heap_bytes`, `min_free_heap_bytes`, `min_free_heap_observed_bytes`, `max_alloc_heap_bytes` |
| Runtime counters | `total_pump_cycles`, `total_pump_run_min`, `ultrasonic_cycles_ok`, `ultrasonic_cycles_timeout`, `flow_stuck_high_events` |

### 4.5 `/pump_system/audit/events`

| Field | Meaning |
|---|---|
| `action` | Audit event type such as `control.set_mode` or `control.emergency_stop`. |
| `at`, `at_ms` | Event timestamps. |
| `deviceId` | Source device identifier. |
| `email`, `uid` | Acting user identity. |
| `meta` | Optional event-specific metadata. |

### 4.6 `/pump_system/presence`

| Field | Meaning |
|---|---|
| Key format | Usually `${uid}_${deviceId}`. |
| `at` | Last presence timestamp. |
| `email`, `uid` | User identity associated with the presence marker. |

## 5. Cross-System Risks

| ID | Severity | Risk | Impact | Status |
|---|---|---|---|---|
| X-01 | Critical | Any condition that leaves the pump ON during a communication fault. | This is the most important safety failure mode. | Must never regress |
| X-02 | High | Backward-compatibility logic for deprecated control fields can override a safer current state if not handled carefully. | A stale or malformed command could cause unexpected mode behavior. | Confirmed risk |
| X-03 | Medium | Persistent state in NVS/Firebase can outlive a reboot. | Stale flags can affect startup behavior if not explicitly cleared. | Confirmed |

## 6. Open Items to Watch

### Immediate Priority (Next 2 Weeks)
- **M-27 [CRITICAL]**: Fix operator lockout — allow `reset_stop` and `clear_error` processing even when `emergencyStopLatched` is true.
- ~~**M-25 [DESIGN]**~~ **Done (Option B):** Documentation and QA aligned with firmware — MANUAL gets 90% warning then max-runtime hard stop.
- **M-24 [OBSERVABILITY]**: Replace raw `Serial.print()` with `LOG()` macro in rs485 parse error path.

### Long-Term Hardening (Before 50+ Day Deployment)
- Review and fix all millis() wraparound vulnerabilities (M-13, M-14, M-20, M-21) — use a proper elapsed-time macro with wraparound safety.
- Resolve M-26 (level-freshness race) by caching decision once per loop.
- Add atomic operations or disable interrupts during flow counter ISR updates (S-09).
- Document discard-count semantics (S-10, S-11) in dashboard and field-service guides.
- Decide whether sensor node should expose cumulative discard counters instead of per-window snapshots.

### Field Validation
- Confirm emergency-stop behavior after repeated dashboard toggles and network faults.
- Validate flow sensor tuning (S-04, FLOW_MIN_PULSE_INTERVAL_US) in diverse environments.
- Monitor discard-count and level-bias behavior (S-11, S-12) against tank geometry.
- Keep secrets files out of version control; ensure example files stay sanitized (M-09).

## 7. Traceability Notes

- This document is intentionally operational, not academic.
- Items marked as mitigated remain listed so future regressions are easier to detect.
- Add new findings only after they are reproduced on hardware, seen in logs, or verified in code.

### Audit Summary
- **Date of latest scan**: 2026-04-02 (Round 2: Operational & Diagnostic Issues)
- **Total tracked**: 39 items (9 baseline + 12 Master Node new + 4 Sensor Node new + 3 cross-system + 11 already mitigated).
- **Critical**: 3 (M-13, M-14, M-27, X-01) | **High**: 17 | **Medium**: 14 | **Low**: 5.
- **Status breakdown**: Confirmed: 32 | Mitigated: 2 | Toolchain issue: 1 | Needs review: 4.
- **New findings requiring triage**: M-24, M-25, M-26, M-27, M-28, M-29, M-30, M-31, S-09, S-10, S-11, S-12.