## System Overview — v1.0.0

### Purpose

The Smart Water Pump Controller automates a water pump feeding a storage tank by using:

- **Tank level** (ultrasonic distance → percent)
- **Flow rate** (pulse-based flow sensor)

It enforces safety behaviors (dry-run lockout, overflow cutoff, emergency stop latch) and provides a cloud-connected dashboard for monitoring and operator control.

---

### Architecture (distributed)

The system is intentionally split so long sensor cables do not directly drive the main controller IO:

- **ESP32 master (controller enclosure)**
  - Drives the relay module that energizes the contactor coil (pump ON/OFF).
  - Runs pump logic, safety logic, persistence (NVS), and cloud sync.
  - Polls the remote sensor node over RS‑485.

- **ESP8266 sensor node (tank enclosure)**
  - Reads the tank sensors locally:
    - JSN‑SR04T ultrasonic sensor (distance to water surface)
    - YF‑G1 flow sensor (pulse frequency)
  - Responds to RS‑485 requests from the master with a framed, CRC-protected payload.

- **Cloud + dashboard**
  - Firebase Realtime Database is the only channel between firmware and dashboard.
  - Dashboard is a Next.js PWA that displays status and writes control intents.

---

### End-to-end data flow (sensor → UI)

1. **Ultrasonic**: sensor node measures distance (cm) to water surface, filters samples, and keeps the last good distance.
2. **Flow**: sensor node counts pulses and converts them to flow (L/min) using time-aware math.
3. **RS‑485**: ESP32 master requests data; node responds with framed payload:

```text
STX (0x02)
LVL:<percent>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;SEQ:<n>;CRC:<hex>
ETX (0x03)
```

4. **Master compute**:
  - Validates frame framing + CRC.
  - Applies strict parsing and range checking.
  - Prefers `DIST` if present and computes `water_level_percent` from **master-side calibration** (`tank_empty_cm`, `tank_full_cm`).
  - Maintains **freshness** and **stability** latches used by safety logic.
5. **Cloud push**: ESP32 pushes `/pump_system/status` to Firebase.
6. **Dashboard**: subscribes to `/pump_system/status` and renders:
  - Level %, flow L/min, run state
  - Faults and safety gates (e-stop latch, comm stability, freshness)

---

### End-to-end control flow (UI → pump)

1. Operator uses the dashboard to write control intents under `/pump_system/control`.
2. ESP32 polls control and applies the highest-priority safety rules:
  - **Emergency stop latch** always wins.
  - **Hard lockouts** (dry-run / overflow) always win.
  - **Stale/unstable RS‑485 data** blocks starts and can stop a running pump (failsafe).
3. ESP32 drives the relay output to control the contactor coil.

---

### Safety model (layered)

This system is designed as **three independent protection layers**:

1. **Hardware layer**: MCB + contactor + TOR thermal overload relay protect the motor regardless of firmware.
2. **Firmware layer**: safety state machine enforces operational protections while powered.
3. **Operator/manual layer**: manual bypass (if installed) is physically possible but is treated as diagnostic-only because it bypasses software protections.

---

### Key operating modes (policy)

Policy mode is stored at `/pump_system/control/mode`:

- **AUTO**: hysteresis start/stop based on tank level thresholds.
- **MANUAL**: operator intent-based control:
  - `manual_desired=true` requests pump ON (safety still enforced).
  - `manual_desired=false` requests pump OFF (mode stays MANUAL).
- **COUNTDOWN**: timed run:
  - duration: `countdown_duration_min`
  - start: `countdown_start=true` one-shot

Emergency stop is separate from policy modes:

- `emergency_stop=true` one-shot latches stop.
- `reset_stop=true` one-shot clears the latch.

