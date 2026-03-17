## Release Notes — v1.0.0

### Summary

This is the **first deployment-ready release** of the Smart Water Pump Controller, implementing a distributed architecture:

- **ESP32 master**: pump control + safety + cloud sync
- **ESP8266 (NodeMCU V2) sensor node**: tank sensing (ultrasonic distance + flow) over **RS‑485**
- **Dashboard**: Next.js PWA operator UI backed by Firebase Realtime Database

Core safety behavior is designed to **fail safe** under sensor faults, RS‑485 faults, and cloud/network faults.

---

### What’s included

#### Firmware (ESP32 master)

- **Modes (vNext)**:
  - `AUTO` (hysteresis)
  - `MANUAL` (intent-based ON/OFF using `manual_desired`)
  - `COUNTDOWN` (explicit start using `countdown_start`)
  - Separate **Emergency Stop** action (`emergency_stop` latch; `reset_stop` unlatch)
- **Safety invariants**:
  - Dry-run lockout (latched until `clear_error`)
  - Overflow protection (max runtime; latched until `clear_error`)
  - Emergency stop latch forces pump OFF until reset
  - RS‑485 data freshness/stability gates prevent unsafe starts; stale data stops a running pump (failsafe)
- **RS‑485 protocol hardening**:
  - STX/ETX framing
  - CRC16 (Modbus)
  - Sequence number for duplicate detection and robustness
  - Master prefers `DIST:` from node and computes level % from master calibration to avoid drift
- **Resilience**:
  - Crash-loop safe mode (deterministic boot counter + safe-mode duration handling)
  - Wi‑Fi reconnection with backoff and Firebase cooldown handling
  - NVS persistence for config and critical counters

#### Firmware (ESP8266 sensor node)

- **Flow**: interrupt pulse counting and time-aware conversion to L/min
- **Ultrasonic**: non-blocking measurement state machine, median filtering, plausibility checks
- **RS‑485**: responds to master with framed payload including `DIST`, `FLOW`, `ERR`, `SEQ`, `CRC`

#### Dashboard

- **Typed RTDB contract** in `dashboard/lib/types.ts` aligned to firmware
- **Safe control writers** (mode validation; one-shot patterns)
- **Clear safety-state UX**:
  - Emergency stop status
  - “Link unstable” and “Level stale” gates
  - Fault mapping with safe fallback for unknown fault codes

#### Firebase rules & operational hardening

- **No hardcoded UIDs** in `database.rules.json`; admin access is controlled via:
  - `pump_system/config/admins/{uid} = true`

---

### Breaking changes / compatibility notes

- Legacy policy modes `FORCE_ON` and `FORCE_OFF` are **not used** in this release.
- Legacy control one-shots (`manual_start`, `manual_stop`) are retained as **read-only compatibility** in the dashboard types but should not be used for new operation.

---

### Known limitations (release-time)

- **Hardware-in-loop testing** is required to validate RS‑485 noise margins, grounding, and power transient behavior on the real installation (this can’t be fully proven by builds/tests alone).
- Lighthouse CI is skipped on Windows during local validation due to a known temp-dir cleanup issue; it should be run in CI/Linux for a full performance regression gate.

