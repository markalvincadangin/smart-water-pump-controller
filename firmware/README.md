# Firmware — Smart Water Pump Controller
**Platform:** ESP32 DevKit V1 (38-pin)
**Framework:** Arduino (via Arduino IDE or VS Code + Arduino extension)  
**Notes:** This README describes current behavior and wiring at a high level. Source-of-truth behavior is the code in `firmware/`.

---

## Overview

The system is split into:

- **ESP32 master**: pump relay control, safety logic, cloud connectivity, persistence.
- **ESP8266 tank node** (NodeMCU): reads sensors near the tank and serves data over RS-485.

The ESP32 firmware runs a non-blocking control loop that:

- Polls remote tank data over **RS-485** (level + flow + sensor error code)
- Syncs status to Firebase Realtime Database every **3 seconds**
- Reads control commands from Firebase every **3 seconds**
- Reads device config from Firebase every **30 seconds** (or uses NVS when offline)
- Automatically starts/stops the pump based on tank level thresholds (AUTO mode)
- Detects dry-run conditions (configurable threshold and timeout, default 30s of flow < 0.5 LPM)
- Overflow protection: max runtime cutoff (default 120 min)
- Sensor/communications failsafe: stale/unstable remote data blocks starts; stale data stops a running pump
- Scheduled sleep mode (optional): Light Sleep during idle hours to reduce power/heat
- Auto-reconnects to WiFi and Firebase if the router reboots

---

## File Structure

```
firmware/
├── arduino_smart_water_pump_controller/
│   ├── 01_config.ino                      ← Includes, constants, globals
│   ├── 02_rs485_comm.ino                  ← RS-485 request/parse + freshness/stability latch
│   ├── 03_safety_pump.ino                 ← Safety checks + pump state machine (P0–P5)
│   ├── 04_persistence.ino                 ← NVS config/state + crash loop helpers
│   ├── 05_connectivity_cloud.ino          ← WiFi/NTP/Firebase helpers
│   ├── arduino_smart_water_pump_controller.ino ← Entry point (open this in Arduino IDE)
│   ├── smart_water_pump_controller_shared.h    ← Shared globals/constants header
│   ├── secrets.h.example                  ← Template for credentials (copy to secrets.h)
│   └── secrets.h                          ← Your credentials (create from example, gitignored)
├── arduino_sensor_node/                   ← Arduino sketch for NodeMCU V2 (ESP8266 tank node)
├── platformio_smart_water_pump_controller/     ← PlatformIO project (VS Code)
│   ├── platformio.ini
│   └── src/… (mirrors the Arduino tabs)
├── platformio_sensor_node/                ← PlatformIO project for NodeMCU V2 (ESP8266 tank node; see its README for OTA)
├── libraries.txt                          ← Required libraries and install instructions
└── README.md                              ← This file
```

> **Arduino IDE requirement:** Open `arduino_smart_water_pump_controller/arduino_smart_water_pump_controller.ino`.  
> Arduino concatenates all `.ino` tabs **alphabetically**, so the numeric prefixes
> (`01_...05_`) lock in the build order.

---

## Pin Mapping

| GPIO | Direction | Connected To | Notes |
|------|-----------|-------------|-------|
| 4 | OUTPUT | 5V Relay Module IN | Active LOW — LOW = pump ON |
| 17 | OUTPUT | RS-485 TX (UART2 TX) → MAX485 DI | **FINAL:** UART2 TX for RS-485 |
| 16 | INPUT | RS-485 RX (UART2 RX) ← MAX485 RO | **FINAL:** UART2 RX for RS-485 |
| 5 | OUTPUT | RS-485 DE/RE (tied) | **FINAL:** LOW=RX, HIGH=TX |

> **GPIO 34** is input-only on the ESP32 — it has no internal pull-up/pull-down
> and cannot be used as an output. This makes it ideal for the flow sensor interrupt.

---

## Before You Flash — Required Setup

### Step 1 — Install Arduino IDE
Download from: https://www.arduino.cc/en/software
Version 2.x recommended.

### Step 2 — Add ESP32 Board Support
1. Arduino IDE → **File → Preferences**
2. In *Additional Boards Manager URLs*, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager** → search `esp32` by Espressif Systems
4. Install version **2.0.11 or later**

### Step 3 — Install Required Libraries
See `libraries.txt` for full details. Quick summary:

| Library | Author | Version | Install via |
|---------|--------|---------|-------------|
| Firebase ESP Client | Mobizt | >= 4.4.14 | Library Manager |
| ArduinoJson | Benoit Blanchon | >= 6.21.5 (v6.x only) | Library Manager |

Both installed via: **Sketch → Include Library → Manage Libraries**

> ⚠ **ArduinoJson version warning:** Install v6.x specifically. v7.x has breaking
> API changes that cause Firebase ESP Client to fail to compile.

### Step 4 — Configure Your Credentials
1. Copy `secrets.h.example` to `secrets.h` (in the same folder as the `.ino` file)
2. Edit `secrets.h` and fill in your WiFi, Firebase, and Email/Password credentials:

```cpp
#define WIFI_SSID       "your_wifi_network_name"
#define WIFI_PASSWORD   "your_wifi_password"
#define API_KEY         "AIzaSy..."
#define DATABASE_URL    "https://your-project-default-rtdb.firebaseio.com"
#define FIREBASE_EMAIL    "your_email@example.com"
#define FIREBASE_PASSWORD "your_firebase_password"
```

Create the Email/Password user in Firebase Console → Authentication → Users → Add user.

> ⚠ **Never commit `secrets.h`** — it is gitignored. Use `secrets.h.example` as the template when setting up a new clone.

**Where to find Firebase credentials:**
1. Go to [console.firebase.google.com](https://console.firebase.google.com)
2. Select your project → ⚙ Project Settings → General tab
3. Scroll to "Your apps" → Web app → SDK setup
4. `API_KEY` = `apiKey` value
5. `DATABASE_URL` = found in Realtime Database → Data tab (the URL shown at top)

### Step 5 — Configure Board Settings
In Arduino IDE Tools menu, set exactly:

| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 115200 |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| PSRAM | Disabled |
| Port | Your COM port (check Device Manager) |

> **Partition scheme:** Use **Huge APP** — firmware is ~1.25MB and the default scheme is too tight.

### Step 6 — Flash
1. Connect ESP32 to laptop via USB
2. Select the correct COM port under **Tools → Port**
3. Click **Upload** (→ arrow button)
4. If upload fails with "Failed to connect": hold the **BOOT** button on the ESP32
   while clicking Upload, release once "Connecting..." appears in the console

---

## Sensor Calibration

Calibration and thresholds can be changed **without reflashing** via the dashboard: open the **Device config** (gear icon), edit values, and Save. The ESP32 reads `/pump_system/config/device` every **30 seconds** when online and persists to NVS. When offline it uses the last-saved config. If the database has no config yet, use **Seed defaults (if empty)** in the same modal, or the firmware uses the compiled defaults below.

### Ultrasonic Tank Level (Section 3 in firmware)

The default calibration is for **Bestank WT660** (660L, sensor on lid):

```cpp
#define TANK_EMPTY_CM   122   // Sensor-to-bottom when tank is empty
#define TANK_FULL_CM     8    // Sensor-to-water when tank is full
```

**How to calibrate for your actual tank:**
1. Mount the JSN-SR04T probe vertically, pointing down into the tank
2. With tank **empty**: open Serial Monitor (115200 baud), note the distance reading
3. Set **Tank empty (cm)** in dashboard Device config, or update `TANK_EMPTY_CM` and reflash
4. Fill the tank **completely**: note the distance reading
5. Set **Tank full (cm)** in dashboard Device config, or update `TANK_FULL_CM` and reflash

> The sensor measures distance to the water surface — closer distance = higher water level.
> `TANK_FULL_CM` will always be a smaller number than `TANK_EMPTY_CM`.

When using the **remote tank node**, the NodeMCU transmits the last good ultrasonic **distance (cm)** over RS‑485 (`DIST:<cm>`).
The ESP32 master computes level (%) using the configured tank calibration (`cfgTankEmptyCm/cfgTankFullCm`) to prevent drift.

---

## Remote Tank Sensor Node (RS-485)

For long runs (≈20–30m) between the main enclosure and the tank, sensors are driven by a
small NodeMCU V2 (ESP8266, CP2102) node mounted near the tank. This node:

- Drives JSN TRIG/ECHO locally (short wires, level-shifted ECHO).
- Runs a non-blocking acquisition window with median filtering.
- Reads YF-G1 flow via ISR pulse counting.
- Replies to the ESP32 over RS-485 on request (CRC framed).

### Frame format (RS‑485)

The tank node replies to `REQ\n` with a framed payload:

```text
STX (0x02)
LVL:<percent>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;SEQ:<n>;CRC:<hex>
ETX (0x03)
```

- `DIST` (cm) is the preferred primary measurement; the master computes `%` using configured calibration.
- `LVL` is retained for backward compatibility.
- `ERR` bitfield: bit0=ultrasonic error, bit1=flow signal error.
- `CRC` is CRC16 (Modbus) over the payload up to the trailing `;` after `SEQ`.

### Flow Rate (Section 4 in firmware)

```cpp
#define FLOW_CALIBRATION_FACTOR  7.5f   // Q (L/min) = F (Hz) / 7.5 per YF-G1 datasheet
```

The YF-G1 1-inch sensor datasheet cites ~7.5 pulses per L/min (some variants use 4.8). The default `7.5` is tunable via dashboard **Device config → Flow calibration factor** without reflashing. Verify with a bucket + stopwatch test.

### AUTO Mode Thresholds (Section 3 in firmware)

Pump start/stop levels are configurable via the dashboard **Device config** (gear icon). Defaults: start 30%, stop 100%. The ESP32 reads these from Firebase; compiled defaults apply only before the first successful read.

---

## System States

The firmware reads `/pump_system/control/mode` from Firebase every 3 seconds.

| Mode | Behaviour | Set By |
|------|-----------|--------|
| `AUTO` | Pump starts at ≤ start level, stops at ≥ stop level (hysteresis) | Dashboard / default |
| `MANUAL` | Operator policy mode; pump ON/OFF is controlled by `manual_desired` (full safety enforced) | Dashboard |
| `COUNTDOWN` | Timed run started via `countdown_start` (duration 1–120 min), then returns to AUTO | Dashboard |

> **Safety override:** If dry-run or overflow lockout is active, the pump will not run in any mode until the error is acknowledged via `clear_error = true`.
>
> **Emergency stop:** The dashboard can latch an immediate STOP via `emergency_stop=true` (one-shot). The pump remains stopped until `reset_stop=true`.

### Run modes (current)

The firmware publishes `run_mode` to describe the *actual* operational state of the pump (distinct from policy `mode`):

| `run_mode`    | Meaning |
|---------------|---------|
| `AUTO`        | AUTO mode, pump running |
| `AUTO_STANDBY`| AUTO mode, pump idle (above start level) |
| `MANUAL_ON`   | MANUAL mode, pump running (full safety) |
| `MANUAL_OFF`  | MANUAL mode selected, pump off (sticky MANUAL) |
| `COUNTDOWN`   | Timed run in progress (COUNTDOWN mode active) |
| `STOPPED`     | Pump stopped by Emergency Stop latch |
| `OFF`         | Pump off (idle or blocked by lockout) |

Dashboard can request:
- **MANUAL intent** via `mode="MANUAL"` and `manual_desired=true|false`.
- **COUNTDOWN run** via `mode="COUNTDOWN"`, `countdown_duration_min=N`, then `countdown_start=true` one-shot.
- **Add time** (+N minutes to a running countdown) via `countdown_add_min=N` and `countdown_add_time=true` one-shot.

All runs are subject to P1 safety (dry-run, overflow) — manual operation never bypasses safety interlocks.

---

## Safety Logic

### Dry-Run Lockout (Level 2 — Software)
- **Trigger:** Pump is running but flow rate below threshold (default < 0.5 LPM) for more than timeout (default 30s)
- **Action:** Relay opens (pump off), `is_error = true` pushed to Firebase
- **Reset:** Only via dashboard — user clicks ACK button, which sets
  `/pump_system/control/clear_error = true` in Firebase
- **Why it matters:** Running a jet pump dry destroys the pump head and impeller within minutes

### Overflow Protection (Level 2 — Software)
- **Trigger:** Pump runs in AUTO mode longer than `max_pump_runtime_min` (default 120 min) without reaching stop level
- **Action:** Relay opens, `is_overflow_error = true` pushed to Firebase
- **Reset:** Via dashboard `clear_error` or when pump stops normally
- **Why it matters:** Prevents tank overflow if the sensor fails during fill

### Sensor Failure (Level 2 — Software)
- **Trigger:** Consecutive ultrasonic timeouts (default 5) or flow sensor stuck (pump OFF + flow > 2 LPM for 5s)
- **Action:** Pump OFF in AUTO; `is_sensor_error = true` pushed to Firebase
- **Reset:** Auto-clears when valid readings resume

### TOR Thermal Overload (Level 1 — Hardware)
- Handled entirely in hardware by the LR2-D13 relay
- Trips if motor current exceeds the set FLA (~8–9A) for a sustained period
- **Firmware has no involvement** — the TOR physically breaks the contactor coil circuit
- Must be manually reset by pressing the reset button on the TOR

### Manual Bypass (Level 3 — Physical switch)
- When the enclosure manual switch is ON, the contactor is energized regardless of firmware state
- **All software protections (dry-run, level cutoff) are bypassed**
- TOR thermal protection remains active

---

## Firebase Data Structure

```
/pump_system/
  status/                            ← ESP32 writes every 3s
    water_level_percent: 85          (int,   0–100)
    is_running:          true        (bool)
    flow_rate_lpm:       12.4        (float, L/min)
    is_error:            false       (bool,  dry-run lockout)
    is_level_sensor_error: false     (bool,  ultrasonic sensor failure)
    is_flow_sensor_error:  false     (bool,  flow sensor stuck-high)
    is_overflow_error:   false       (bool,  max runtime exceeded)
    is_sleeping:         false       (bool,  scheduled sleep active)
    bypass_level_sensor: false       (bool,  manual/auto bypass active)
    auto_bypass_active:  false       (bool,  true when firmware auto-engaged bypass)
    wifi_rssi:           -65         (int,   dBm)
    last_boot_reason:    "Power-on"  (string)
    uptime_minutes:      125         (int)
    run_mode:            "AUTO"      (string: OFF|AUTO|AUTO_STANDBY|MANUAL|COUNTDOWN)
    countdown_remaining_sec: 0       (int,   seconds left in COUNTDOWN; 0 when inactive)
    last_fault_code:     ""          (string: DRY_RUN|OVERFLOW|LEVEL_SENSOR|FLOW_SENSOR|SAFE_MODE)
    last_fault_message:  ""          (string, human-readable detail)
    estimated_level_pct: -1          (int,   flow-based estimate; -1 = inactive)
    level_estimate_active: false     (bool,  true when using flow estimate)
    flow_volume_added_l: 0.0         (float, litres added since last good reading)
    level_last_valid_age_sec: 0      (int,   seconds since last valid ultrasonic)
    level_sensor_health_pct: 100     (int,   0–100 health score)
    total_pump_cycles:   42          (int,   lifetime pump start count)
    total_pump_run_min:  1280        (int,   lifetime runtime in minutes)
    ultrasonic_cycles_ok:      120   (int,   cycles with >=1 valid sample)
    ultrasonic_cycles_timeout: 0     (int,   cycles with 0 valid samples)
    ultrasonic_last_good_cm:   35.2  (float, last good median distance)
    flow_discard_max_sane:     0     (int,   discarded readings > max sane)
    flow_stuck_high_events:    0     (int,   stuck-high detections)
    free_heap_bytes:           182000 (int)
    min_free_heap_bytes:       175000 (int,  ESP32 SDK low-water mark)
    max_alloc_heap_bytes:      82000  (int,  largest allocatable block)
    min_free_heap_observed_bytes: 175000 (int, firmware-tracked minimum)
    firebase_consecutive_failures: 0  (int)
    firebase_last_error:       ""    (string)

  control/                           ← Dashboard writes, ESP32 reads every 3s
    mode:        "AUTO"              (string: AUTO|MANUAL|COUNTDOWN)
    clear_error: false               (bool:  set true to acknowledge errors)
    reboot_request_id: 0             (int:   bump to request soft reboot)
    manual_desired: false            (bool:  persistent MANUAL intent: true=run, false=stop)
    emergency_stop: false            (bool:  one-shot latch stop)
    reset_stop:     false            (bool:  one-shot reset stop latch)
    countdown_duration_min: 10       (int:   1–120, set before COUNTDOWN mode)
    countdown_start: false           (bool:  one-shot start countdown)
    countdown_add_min: 5             (int:   minutes to add)
    countdown_add_time: false        (bool:  one-shot to add time)
    bypass_level_sensor: false       (bool:  toggle level sensor bypass)

  config/
    device/                          ← Dashboard writes, ESP32 reads every 30s
      tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level,
      dry_run_threshold_lpm, dry_run_timeout_sec, flow_calibration_factor,
      max_pump_runtime_min, sleep_enabled, sleep_start_hour, sleep_end_hour,
      sleep_emergency_level, level_sensor_failure_threshold,
      idle_sensor_interval_ms, idle_firebase_interval_ms,
      auto_bypass_on_sensor_fail, auto_bypass_delay_sec
```

---

## Serial Monitor Output

Connect at **115200 baud** to see live debug output:

```
====================================
 Smart Water Pump Controller
====================================
[INIT] GPIO configured. Pump OFF.
[INIT] Flow sensor interrupt attached on GPIO 34.
[WIFI] Connecting to: YourNetwork..........
[WIFI] Connected! IP: 192.168.1.105
[FIREBASE] Initialized. Waiting for token...
[INIT] Boot complete. Entering main loop.

[SENSOR] Level: 42% | Flow: 0.00 LPM
[FIREBASE] Status pushed -> Level:42% | Flow:0.00 LPM | Running:NO | Error:NO
[AUTO] Water at 28%. Starting pump.
[SENSOR] Level: 28% | Flow: 13.20 LPM
[FIREBASE] Mode changed: AUTO -> MANUAL
[SENSOR] Level: 29% | Flow: 0.00 LPM
[WARN] Dry-run condition detected. Timer started.
```

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Upload fails — "Failed to connect" | ESP32 not in download mode | Hold BOOT button while clicking Upload |
| Compiles but WiFi never connects | Wrong SSID/password | Double-check credentials; ensure 2.4GHz network (ESP32 does not support 5GHz) |
| Firebase stays "Not ready" | Invalid API_KEY, DATABASE_URL, or credentials | Verify values in Firebase Console; ensure Email/Password user exists |
| ESP32 doesn't reconnect after power outage | Router boots slower than ESP32 | Normal — firmware retries WiFi with backoff (5–60s) and auto-inits Firebase on first connect |
| Dashboard restart fails to reconnect | WiFi briefly unavailable during restart | Firmware uses late-init path; check Serial Monitor for `[FIREBASE] Late init` message |
| Level always reads 0% or 100% | Calibration mismatch | Open Serial Monitor, note actual distance readings, update `TANK_EMPTY_CM`/`TANK_FULL_CM` |
| Flow reads 0 while pump is running | Wiring issue on GPIO 34 or YF-G1 | Check voltage divider (should read ~3.3V on GPIO 34 when signal is high); confirm YF-G1 VCC is 5V |
| Dry-run lockout triggers immediately | Flow calibration too high or sensor unpowered | Verify 5V on YF-G1 VCC pin; check CAT6 Orange pair connection |
| Pump runs non-stop in AUTO | `TANK_FULL_CM` too small (too close) | Re-calibrate with actual full-tank distance |
| ArduinoJson compile error | Wrong ArduinoJson version | Uninstall v7.x, install v6.21.5 specifically |

---

## Offline Behaviour & WiFi Recovery

If WiFi is unavailable at boot (common during power outages where the router takes
longer to start than the ESP32), the firmware:
- Loads device config from **NVS** (last values saved when online), or uses compiled defaults if NVS is empty
- Reads sensors every 1 second and executes pump logic based on last known mode
- Retries WiFi with exponential backoff (5s / 10s / 20s / 40s / 60s cap, with jitter)

When WiFi connects (first time or after a drop):
1. **Firebase late-init**: If `initFirebase()` was skipped at boot (no WiFi), it runs now.
2. **Token refresh**: If Firebase was already initialized, `Firebase.refreshToken()` renews the auth token.
3. **NTP re-sync**: Time is re-synced so scheduled sleep windows and countdown timers stay accurate.
4. Normal Firebase sync resumes on the next 3-second cycle.

For dashboard-initiated restarts (`ESP.restart()`), the same recovery path applies:
the ESP32 re-enters `setup()`, connects to WiFi, and initializes Firebase.

---

## Soak-test checklist (recommended)

- **Noise soak (2–6 hours, pump OFF)**:
  - Leave the 20–30m UTP sensor runs connected (ultrasonic + flow).
  - Watch Serial logs for `[TELEM]` once per minute.
  - Expect `ultrasonic_cycles_timeout` near 0 and `flow_discard_max_sane` near 0.
- **WiFi flap (3–5 reconnects)**:
  - Toggle router/AP or power-cycle it.
  - Confirm the firmware resumes Firebase sync without token loops.
  - During cooldown you may see `[FIREBASE] Cooling down...` but the loop should continue running sensors/safety.
- **Sensor fault injection**:
  - Disconnect ultrasonic ECHO to force timeouts; AUTO must fail-safe (pump OFF) and recover when reconnected.
  - With pump OFF, confirm no false “flow stuck-high” events; if you see any, check grounding/shielding on the long run.

---

*For current, canonical documentation: `docs/specs/firmware.md` and `docs/specs/dashboard.md`. Historical release docs live under `docs/archive/releases/`.*
