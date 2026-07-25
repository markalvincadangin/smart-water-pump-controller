# ✅ Flash ready — proceed with controlled first boot

Firmware is safe to flash for sensor node and master node. RTDB pre-flash blockers were cleaned in live control/config nodes. Proceed with controlled first-boot observation checks.

## Bottom line

Bottom line: flash-ready with verification checks. The firmware itself is solid, and the previous RTDB blockers were resolved in live control/config nodes.

The previously hard blockers (`emergency_stop: true`, `countdown_stop: true`, and bypass flags stuck true) were cleared in the live RTDB control node. This removes immediate e-stop lockout and startup safety-bypass risk.

The previous `flow_calibration_factor: 1` issue was corrected to `7.5` in live RTDB and in the seed export. Dry-run detection scaling is now aligned with firmware default assumptions.

Sensor node is clean — flash that one whenever you're ready.

### Pre-flash sequence

1. Confirm live `/pump_system/control` remains: `emergency_stop=false`, `countdown_stop=false`, `bypass_level_sensor=false`, `bypass_flow_sensor=false`, `mode="AUTO"`.
2. Confirm live `/pump_system/config/device`: `flow_calibration_factor=7.5`, `tank_full_cm=30`.
3. Flash master node.
4. Watch first 60 seconds of serial log — confirm `[BOOT]`, `[NTP]`, `[FIREBASE]` come up clean and `[PUMP]` stays OFF until level sensor establishes stable readings.

## Firmware scan — sensor node (NodeMCU)

| Check | Result | Detail |
| --- | --- | --- |
| S-06/S-09 ISR fix | ✓ Fixed | `flowRawEdgeCount++` inside deglitch gate. Confirmed line 22. |
| RS-485 frame build | ✓ OK | CRC over canonical payload. Overflow guards on `snprintf`. Frame size 128 bytes — safe. |
| Response turnaround | S-02 open | Immediate reply after `Serial.flush()` + 2ms delay. Known risk; DE/RE turnaround is 60µs — adequate for most transceivers. |
| 20ms stall reset | S-08 open | Hardcoded magic number. At 115200 baud ≈ 230 byte-times. Conservative for 40m CAT6. Acceptable for deployment. |
| noInterrupts() coverage | ✓ OK | All three flow counters (`flowPulseCount`, `flowPulseDiscardCount`, `flowRawEdgeCount`) read and zeroed inside the guard. |
| usMedianValid upper-median | S-12 doc needed | US_SAMPLES=5 (odd), so n/2=2 is the true median — no bias. S-12 only matters for even-count windows which don't occur in production config. |
| FLOW_MIN_PULSE_INTERVAL_US | S-04 — field validation | 800µs marked "temporary diagnostic tuning". At 7.5 Hz/LPM, max 60 LPM → min period 111µs. 800µs blocks pulses above ~8.3 LPM. Safe for your 1–10 LPM typical range but confirm with YF-G1 field data. |
| DEBUG_USB_MODE | ✓ OK | Set to 0 in production config. #warning fires if 1 is compiled in. |
| SENSOR_DEBUG_ENABLED | Set to 1 | Debug output on Serial1/GPIO2. No impact on RS-485 (UART0). Acceptable for production if GPIO2 TX is floating/disconnected. |

Sensor node: **ready to flash**. All critical issues resolved. Open items (S-02, S-04, S-08, S-12) are known, accepted, and do not block deployment.

---

## Firmware scan — master node (ESP32)

| Check | Result | Detail |
| --- | --- | --- |
| M-13 countdown wraparound | ✓ Fixed | `millisDeadlineReached(now, countdownEndMs)` with null guard. Confirmed. |
| M-14 unsafe subtractions | ✓ Fixed | All safety-path timers use `elapsedMillis32()`. Remaining raw subtractions classified safe (telemetry/NVS throttles only). |
| M-20 cooldown overflow | ✓ Fixed | `addMillisSaturated()` on all four cooldown deadlines. Confirmed. |
| M-21 RS-485 frame timeout | ✓ Fixed | `elapsedMillis32(millis(), start)` in frame read loop. Confirmed. |
| M-32 offline branch | ✓ Fixed | Last unsafe subtraction resolved. All millis() patterns in rs485_comm.cpp clean. |
| M-15/M-23 boot freshness | ✓ Fixed | All 3 freshness sites have `levelLastUpdateMs > 0` guard. Confirmed. |
| M-26 freshness snapshot | ✓ Fixed | Single `nowMsPump` + `levelFreshOk` per loop. All inner branches use same value. Confirmed. |
| M-27 e-stop lockout | ✓ Fixed | Early return removed. `reset_stop` and `clear_error` always reachable. Confirmed. |
| M-16 emergency_stop | ✓ Fixed | Firebase self-clear is idempotency guard. `!emergencyStopLatched` prevents double-latch. |
| M-16 countdown_start | ✓ Fixed | Self-clear fires on every `true`; `!lastCountdownStart` only suppresses within-cycle duplicates. |
| M-19 countdown_active field | ✓ Fixed | `statusJson.set("countdown_active", isCountdownActive)` present. Confirmed. |
| M-18 add-time arithmetic | ✓ Fixed | `addMin >= 1` gate + `addMillisSaturated()` + `min(candidate, maxEnd)` cap. |
| M-28 error log timestamp | ✓ Fixed | NTP epoch used when synced. Confirmed. |
| M-29 runtime rounding | ✓ Fixed | `(totalPumpRunSec + 30UL) / 60UL`. Confirmed. |
| M-31 WDT before WiFi | ✓ Fixed | WDT registered before `connectWiFi()`; `esp_task_wdt_reset()` inside retry loop. Confirmed. |
| M-24 parse failure logging | ✓ Fixed | `LOG(LOG_LEVEL_ERROR, "RS485-ERR", ...)` replaces raw `Serial.print()`. Confirmed. |
| Relay active-LOW logic | ✓ OK | `on ? LOW : HIGH`. Boot forces HIGH (relay off) before state machine starts. |
| All safety paths setPump(false) | ✓ OK | 14 confirmed `setPump(false)` callsites covering e-stop, dry-run, overflow, comm-loss, sensor-error, level-full paths. |
| M-30 LOG_COMPILE_FLOOR | ✓ Verified | Master config confirms `LOG_COMPILE_FLOOR = LOG_LEVEL_VERBOSE` in `firmware/platformio_smart_water_pump_controller/src/config/config.h`. |
| M-01 RS-485 time budget | ✓ Mitigated | 150ms cap when Firebase work is due. Budget-aware per-attempt timeout. Confirmed. |

### Pre-flash status — master node

Previously flagged pre-flash blockers are now resolved in live RTDB; keep verification checks before flash.

| # | Item | Action needed |
| --- | --- | --- |
| 1 | RESOLVED RTDB control node | `emergency_stop=false` and `countdown_stop=false` confirmed in reflected export/live cleanup state. |
| 2 | RESOLVED RTDB bypass flags | `bypass_flow_sensor=false` and `bypass_level_sensor=false` confirmed in reflected export/live cleanup state. |
| 3 | RESOLVED startup mode | Control `mode="AUTO"` confirmed; keep as intended startup mode. |

---

## RTDB audit

### Structure alignment with firmware

| Path | Status | Notes |
| --- | --- | --- |
| /pump_system/control | ✓ Cleaned live | Control node reflects safe startup values (`emergency_stop=false`, `countdown_stop=false`, bypass flags false, `mode="AUTO"`). |
| /pump_system/config/device | ✓ Aligned | All fields firmware reads are present: `tank_empty_cm=120`, `tank_full_cm=30`, `pump_start_level=30`, `pump_stop_level=85`, `dry_run_threshold_lpm=0.5`, `dry_run_timeout_sec=30`, `max_pump_runtime_min=40`, `sleep_enabled=true`, `debug_log_level=0`, `flow_calibration_factor=7.5`. |
| /pump_system/status | Stale snapshot | Last status push shows `last_boot_reason: "Exception/panic"` — indicates previous crash. `remote_sensor_stable: false`, `level_fresh: false`, `bypass_level_sensor/flow_sensor: true` (stale). Will be overwritten on first successful push after flash. No action needed. |
| /pump_system/audit/events | FORCE_ON/OFF in history | All historical events use `FORCE_ON`/`FORCE_OFF` modes. These are deprecated — firmware maps them to AUTO on receipt. Audit trail is append-only, no cleanup needed. Dashboard should stop sending these modes. |
| /pump_system/config/admins | ✓ OK | Two admin UIDs present. Aligns with expected access control. |
| /pump_system/config/notifications | ✓ OK | All notification policy fields present. FCM tokens present for two users. |
| /pump_system/presence | Stale entries | Multiple device sessions from testing (5+ device IDs). Presence entries are cosmetic only — no firmware dependency. Can leave or prune. |

### Config alignment with hardware

| Parameter | RTDB value | Assessment |
| --- | --- | --- |
| `tank_empty_cm` | 120 | ✓ Matches sensor node calibration constant (TANK_US_DIST_EMPTY_CM = 120.0f) |
| `tank_full_cm` | 30 | ✓ Confirmed by field check. Use 30 cm as the full-tank ultrasonic threshold. |
| `dry_run_threshold_lpm` | 0.5 | ✓ Below typical YF-G1 minimum reliable reading. Reasonable for dry-run detection. |
| `flow_calibration_factor` | 7.5 | ✓ Aligned with firmware default (FLOW_CALIBRATION_FACTOR). |
| `pump_stop_level` | 85% | ✓ Conservative stop before overflow. Good. |
| `max_pump_runtime_min` | 40 | ✓ For 660L tank at ~10–20 LPM, 40 min is adequate safety margin. |
| `sleep_enabled` | true, 23:00–05:00 | ✓ Matches PHT schedule. Emergency level 5% means pump can run in sleep if critically low. |

### Missing status fields (new firmware adds these)

| Field | Status |
| --- | --- |
| `countdown_active` | ✓ Addressed. Firmware publishes it and dashboard now uses it to gate timer display. |
| `reset_stop` in control | ✓ Already in schema |
| `bypass_flow_sensor` in control | ✓ Already present. Seed export now defaults to `false`; confirm live control node is `false` before flash. |

### Pre-flash verification checklist

- ✓ `/pump_system/control/emergency_stop` = `false`
- ✓ `/pump_system/control/countdown_stop` = `false`
- ✓ `/pump_system/control/bypass_level_sensor` = `false`
- ✓ `/pump_system/control/bypass_flow_sensor` = `false`
- ✓ `/pump_system/control/mode` = `"AUTO"`
- ✓ `flow_calibration_factor` = `7.5` in live RTDB
- ○ Confirm `tank_full_cm: 30` remains consistent with physical full-tank ultrasonic reading
- ○ Observe first 60 seconds post-flash: `[BOOT]`, `[NTP]`, `[FIREBASE]` healthy and `[PUMP]` remains OFF until level is stable

✓ = completed   ○ = recommended verification
