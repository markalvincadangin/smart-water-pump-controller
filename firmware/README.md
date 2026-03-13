# Firmware — Smart Water Pump Controller
**Platform:** ESP32 DevKit V1 (38-pin)
**Framework:** Arduino (via Arduino IDE or VS Code + Arduino extension)
**Version:** 2.4.0

---

## Overview

The firmware runs a non-blocking state machine on the ESP32 that:

- Reads tank water level every **1 second** via JSN-SR04T ultrasonic sensor (5-sample median, EMA smoothing)
  - In long-run installations, the JSN-SR04T is driven by a small NodeMCU v3 (ESP8266) "tank node" near the tank and
    reported over RS-485 to the main enclosure.
- Reads pipe flow rate every **1 second** via YF-G1 hall-effect sensor (hardware interrupt)
- Syncs status to Firebase Realtime Database every **3 seconds**
- Reads control commands from Firebase every **3 seconds**
- Reads device config from Firebase every **30 seconds** (or uses NVS when offline)
- Automatically starts/stops the pump based on tank level thresholds (AUTO mode)
- Detects dry-run conditions (configurable threshold and timeout, default 30s of flow < 0.5 LPM)
- Overflow protection: max runtime cutoff in AUTO mode (default 120 min)
- Sensor failure detection: consecutive ultrasonic/flow errors → pump OFF in AUTO
- Scheduled sleep mode (optional): Light Sleep during idle hours to reduce power/heat
- Auto-reconnects to WiFi and Firebase if the router reboots

---

## File Structure

```
firmware/
├── smart_pump_controller/
│   ├── 01_config.ino               ← Includes, constants, globals
│   ├── 02_sensors.ino              ← Flow ISR + ultrasonic + flow calc
│   ├── 03_safety_pump.ino          ← Safety checks + pump state machine
│   ├── 04_persistence.ino          ← NVS config/state + crash loop helpers
│   ├── 05_connectivity_cloud.ino   ← WiFi/NTP/Firebase helpers
│   ├── smart_pump_controller.ino   ← Entry points only (open this in Arduino IDE)
│   ├── secrets.h.example           ← Template for credentials (copy to secrets.h)
│   └── secrets.h                  ← Your credentials (create from example, gitignored)
├── libraries.txt                   ← Required libraries and install instructions
└── README.md                       ← This file
```

> **Arduino IDE requirement:** The sketch must live in a folder named
> `smart_pump_controller/` and you should open `smart_pump_controller.ino`.
> Arduino concatenates all `.ino` tabs **alphabetically**, so the numeric prefixes
> (`01_...05_`) lock in the build order.

---

## Pin Mapping

| GPIO | Direction | Connected To | Notes |
|------|-----------|-------------|-------|
| 4 | OUTPUT | 5V Relay Module IN | Active LOW — LOW = pump ON |
| 5 | OUTPUT | JSN-SR04T TRIG | 10µs pulse to trigger ranging |
| 18 | INPUT | JSN-SR04T ECHO | Via 1kΩ/2kΩ voltage divider |
| 34 | INPUT | YF-G1 Signal (Yellow wire) | Via 1kΩ/2kΩ voltage divider; interrupt on RISING edge |

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

When using the **remote tank node**, the same calibration values are applied on the NodeMCU near the tank.
The main ESP32 only receives an already-calibrated percentage value over RS-485.

---

## Remote Ultrasonic Tank Node (RS-485)

For long runs (≈20–30m) between the main enclosure and the tank, the JSN-SR04T sensor is driven by a
small NodeMCU v3 (ESP8266) node mounted near the tank. This node:

- Drives JSN TRIG/ECHO locally (short wires, level-shifted ECHO).
- Applies the same 5-sample median + EMA smoothing and tank calibration.
- Sends a short ASCII status line over RS-485 every 1–2 seconds.

### Frame format

Each status update is a single line terminated by `\r\n`:

```text
LVL:<percent>;ERR:<flag>\r\n
```

- `LVL`: integer 0–100, current water level percentage.
- `ERR`: `0` = JSN healthy, `1` = JSN error (e.g. repeated timeouts, invalid echoes).

Examples:

```text
LVL:87;ERR:0\r\n
LVL:12;ERR:1\r\n
```

The main ESP32:

- Reads these lines from a UART connected via RS-485.
- Parses `LVL` into `waterLevelPct`.
- Treats `ERR` as the ultrasonic sensor health flag (sets/clears `isSensorError` accordingly).

All existing pump control, safety, and Firebase telemetry logic continues to use `waterLevelPct` and
`isSensorError` in exactly the same way as when the JSN was wired directly.

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
| `AUTO` | Pump starts at ≤30%, stops at ≥100% | Dashboard / default |
| `FORCE_ON` | Pump runs continuously regardless of level | Dashboard override |
| `FORCE_OFF` | Pump stays off regardless of level | Dashboard override |

> **Safety override:** If `isDryRunError` is active, the pump will not run in
> any mode until the error is acknowledged via the dashboard (`clear_error = true`).

### Smart manual & timed runs (Phase 7)

Phase 7 in `docs/ENHANCEMENT_PLAN.md` adds an extension where the ESP32 tracks a run mode (`AUTO`, `MANUAL`, `TIMED`, `OFF`) alongside the existing `mode` field:

- Dashboard can request:
  - A **manual run** (“run now until I press Stop”) via a `manual_start` control flag.
  - A **timed run** (“run for N minutes, then stop”) via a `timed_start_sec` control value.
- Firmware:
  - Starts/stops the pump according to the requested run, but **never bypasses** the existing protections (dry-run, overflow, sensor failure, sleep/emergency rules).
  - Reports `run_mode` and `run_remaining_sec` in status so the dashboard can show clear, organized controls and countdowns.

This keeps the current `AUTO` / `FORCE_ON` / `FORCE_OFF` contract intact while allowing smarter, safer manual operation.

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
  status/                        ← ESP32 writes every 3s
    water_level_percent: 85      (int,   0–100)
    is_running:          true    (bool)
    flow_rate_lpm:       12.4    (float, L/min)
    is_error:            false   (bool,  dry-run lockout)
    is_sensor_error:     false   (bool,  ultrasonic/flow sensor failure)
    is_overflow_error:   false   (bool,  max runtime exceeded)
    is_sleeping:         false   (bool,  scheduled sleep active)
    last_reboot_request_id: 0    (int,   last processed reboot request)
    last_reboot_at:      0       (int,   seconds since boot when last reboot acknowledged)
    wifi_rssi:           -65     (int,   dBm)
    last_boot_reason:    "Power-on" (string)
    uptime_minutes:      125     (int)
    ultrasonic_cycles_ok:      120  (int,   cycles with >=1 valid ultrasonic sample)
    ultrasonic_cycles_timeout: 0    (int,   cycles with 0 valid ultrasonic samples)
    ultrasonic_last_good_cm:   35.2 (float, last good median distance)
    flow_discard_max_sane:     0    (int,   discarded flow readings due to max-sane guard)
    flow_stuck_high_events:    0    (int,   stuck-high detections)
    free_heap_bytes:           182000 (int)
    min_free_heap_bytes:       175000 (int, ESP32)
    max_alloc_heap_bytes:      82000  (int, ESP32)
    min_free_heap_observed_bytes: 175000 (int)
    firebase_consecutive_failures: 0  (int)
    firebase_last_error:       ""   (string)

  control/                       ← Dashboard writes, ESP32 reads every 3s
    mode:        "AUTO"          (string: "AUTO" | "FORCE_ON" | "FORCE_OFF")
    clear_error: false           (bool:  set true to acknowledge errors)
    reboot_request_id: 0         (int:   bump to request a soft reboot)

  config/
    device/                      ← Dashboard writes, ESP32 reads every 30s
      tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level,
      dry_run_threshold_lpm, dry_run_timeout_sec, flow_calibration_factor,
      max_pump_runtime_min, sleep_enabled, sleep_start_hour, sleep_end_hour,
      sleep_emergency_level, sensor_failure_threshold, idle_sensor_interval_ms,
      idle_firebase_interval_ms
```

---

## Serial Monitor Output

Connect at **115200 baud** to see live debug output:

```
====================================
 Smart Water Pump Controller v1.0
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
[FIREBASE] Mode changed: AUTO -> FORCE_OFF
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
| Level always reads 0% or 100% | Calibration mismatch | Open Serial Monitor, note actual distance readings, update `TANK_EMPTY_CM`/`TANK_FULL_CM` |
| Flow reads 0 while pump is running | Wiring issue on GPIO 34 or YF-G1 | Check voltage divider (should read ~3.3V on GPIO 34 when signal is high); confirm YF-G1 VCC is 5V |
| Dry-run lockout triggers immediately | Flow calibration too high or sensor unpowered | Verify 5V on YF-G1 VCC pin; check CAT6 Orange pair connection |
| Pump runs non-stop in AUTO | `TANK_FULL_CM` too small (too close) | Re-calibrate with actual full-tank distance |
| ArduinoJson compile error | Wrong ArduinoJson version | Uninstall v7.x, install v6.21.5 specifically |

---

## Offline Behaviour

If WiFi is unavailable at boot, the ESP32 continues to:
- Load device config from **NVS** (last values saved when online), or use compiled defaults if NVS is empty
- Read sensors every 1 second
- Execute pump logic based on last known mode (defaults to `AUTO`) and last-known calibration/thresholds
- Attempt to push/pull Firebase (will log "Not ready. Skipping sync.")

When WiFi recovers, `Firebase.reconnectWiFi(true)` automatically re-establishes
the connection; on the next sync cycle the ESP32 reads `/pump_system/config/device` again and updates NVS if the database has config.

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

*Firmware version 2.4.0 — for end-to-end rollout (Firebase + Functions + Dashboard + ESP32), use `docs/DEPLOY_GUIDE.md`.*
