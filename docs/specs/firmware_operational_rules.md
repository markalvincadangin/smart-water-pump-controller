---
status: current
version: 1.0
last-reviewed: 2026-07-25
source: hand-authored
---

# SmartFlow Firmware Operational Rules (Current)

| Field | Value |
|---|---|
| Product | SmartFlow |
| Scope | `firmware/master_node/` (ESP32 master) |
| Status | Normative operational behavior |
| Last reviewed | 2026-07-25 |

For hardware architecture, build targets, RS-485 protocol, and RTDB schema see [`docs/specs/firmware.md`](./firmware.md).

This is the single source document for implemented firmware behavior: modes, safety, cloud connectivity, WiFi recovery, restart/safe-mode behavior, and command semantics.

## 1. Mode Model

- `pumpMode` is exclusive: exactly one policy mode at a time:
  - `AUTO`
  - `MANUAL`
  - `COUNTDOWN`
- MANUAL and COUNTDOWN cannot run concurrently.
- Certain mode transitions clear incompatible runtime state (for example, exiting active COUNTDOWN clears its timer state).
- Deprecated `FORCE_ON` and `FORCE_OFF` are mapped to `AUTO` for backward compatibility.

## 2. Decision Priority

Each loop effectively evaluates in this order:

1. Emergency stop latch
2. Hard safety lockouts (`DRY_RUN`, `OVERFLOW`)
3. Sensor freshness/stability gates
4. Mode policy logic (`MANUAL`, `COUNTDOWN`, `AUTO`)
5. Cloud and telemetry updates

Higher-priority OFF conditions always override lower-priority run intent.

## 3. Emergency Stop and Recovery

- `emergency_stop: true` is one-shot and immediate:
  - sets `emergency_stop_latched = true`
  - saves current mode for later restore
  - forces relay OFF
  - clears active countdown timer
- While latched:
  - mode apply is blocked
  - control processing continues for recovery commands (`reset_stop`, `clear_error`, bypass fields)
- `reset_stop: true`:
  - clears latch and restores saved mode
  - is blocked if hard lockout (`DRY_RUN` or `OVERFLOW`) is still active

## 4. Safety Rules (Mode-Independent)

- Safety always fails toward pump OFF.
- `is_error` (`DRY_RUN`) or `is_overflow_error` (`OVERFLOW`) forces pump OFF in all modes.
- Stale/unstable remote level data blocks starts and can stop running pump (unless bypass is enabled).
- Safety lockouts must be explicitly cleared (`clear_error`) before normal mode intent can drive ON again.

## 5. Mode-Specific Rules

### 5.1 MANUAL

- Intent-based control:
  - `manual_desired = true` requests ON
  - `manual_desired = false` requests OFF
- Fresh/stable level gates still apply when level bypass is OFF.
- Tank-full threshold can stop pump in MANUAL.
- Min off-time/cooldown still enforced.
- Overflow policy is Option B:
  - `manual_runtime_warning` at ~90% runtime
  - hard overflow stop at configured max runtime

### 5.2 COUNTDOWN

- Runs only when:
  - `pumpMode == "COUNTDOWN"`
  - timer is active
- `countdown_start` is one-shot start.
- `countdown_add_time` is validated and capped extension.
- `countdown_stop` clears timer but keeps mode COUNTDOWN (idle).
- On expiry:
  - pump OFF
  - mode remains COUNTDOWN idle until explicit restart

### 5.3 AUTO

- Start at/below `pump_start_level`.
- Stop at/above `pump_stop_level`.
- Min off-time enforced before restart.
- If level data is stale/unstable (and bypass OFF), AUTO start is blocked and running pump is stopped fail-safe.

## 6. Cooldown / Anti-Short-Cycle

- After OFF transition, minimum off-time is enforced before any restart.
- `run_mode` reflects cooldown:
  - `AUTO_COOLDOWN`
  - `MANUAL_COOLDOWN`

## 7. One-Shot Command Semantics

One-shot behavior (practical semantics):

- `emergency_stop`
- `reset_stop`
- `clear_error`
- `countdown_start`
- `countdown_stop`
- `countdown_add_time` (edge-detected extension command; firmware clears it when consumed)

These command paths are designed to avoid sticky replay during reconnect/retry windows.

## 8. Firebase Cloud Cycle Rules

- Cloud sync loop performs:
  1. control read (`/pump_system/control`)
  2. if control succeeds, status push (`/pump_system/status`)
- When cloud sync is due, RS-485 polling applies a short time budget so transport stalls do not starve Firebase updates.
- If control read fails, status push is skipped in that cycle to avoid compounding failures.
- Auth/timeouts increment dedicated counters and may trigger cooldown windows.
- Runtime telemetry includes connectivity health fields:
  - consecutive failures
  - timeout/auth counters
  - last error
  - per-call and per-cycle durations

## 9. WiFi Reconnection Rules

- Startup WiFi connect is bounded by retry count.
- Watchdog is registered before startup connect and fed in retry loop.
- Reconnect path uses backoff + jitter.
- On reconnect:
  - Firebase init/refresh logic executes
  - token refresh path runs
  - NTP resync attempted

## 10. Restart / Persistence / Safe Mode

- Crash-loop detection uses NVS boot counters.
- On threshold, safe mode is entered:
  - normal init path is constrained
  - pump remains fail-safe OFF behavior
  - auto-clear uses wall-clock when available, fallback timing otherwise
- Persisted state includes core mode/config and runtime counters.
- Some runtime timers remain volatile across reboot (documented operational limitation).

## 11. Operational Invariants

- Pump must not remain ON when freshness/stability safety gate fails (with bypass OFF).
- Emergency stop must always de-energize and latch.
- No MANUAL+COUNTDOWN concurrent run path exists.
- Hard safety lockouts always dominate mode intent until cleared.
