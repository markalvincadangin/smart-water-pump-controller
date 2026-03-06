# Firmware — Smart Water Pump Controller
**Platform:** ESP32 DevKit V1 (38-pin)
**Framework:** Arduino (via Arduino IDE or VS Code + Arduino extension)
**Version:** 1.0.0

---

## Overview

The firmware runs a non-blocking state machine on the ESP32 that:

- Reads tank water level every **1 second** via JSN-SR04T ultrasonic sensor
- Reads pipe flow rate every **1 second** via YF-G1 hall-effect sensor (hardware interrupt)
- Syncs status to Firebase Realtime Database every **3 seconds**
- Reads control commands from Firebase every **3 seconds**
- Automatically starts/stops the pump based on tank level thresholds (AUTO mode)
- Detects dry-run conditions and triggers a safety lockout after 30 seconds of low flow
- Auto-reconnects to WiFi and Firebase if the router reboots

---

## File Structure

```
firmware/
├── smart_pump_controller/
│   ├── smart_pump_controller.ino   ← Main firmware (open this in Arduino IDE)
│   ├── secrets.h.example           ← Template for credentials (copy to secrets.h)
│   └── secrets.h                  ← Your credentials (create from example, gitignored)
├── libraries.txt                   ← Required libraries and install instructions
└── README.md                       ← This file
```

> **Arduino IDE requirement:** The `.ino` file must be inside a folder of the
> exact same name. Do not rename or move the file out of `smart_pump_controller/`.

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
| Partition Scheme | Default 4MB with spiffs |
| PSRAM | Disabled |
| Port | Your COM port (check Device Manager) |

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

### Flow Rate (Section 4 in firmware)

```cpp
#define FLOW_CALIBRATION_FACTOR  1.0f   // Q (L/min) ≈ Pulses/second ÷ factor
```

The YF-G1 1-inch sensor specification states approximately 1 pulse per litre per minute.
If your measured flow rate differs from a reference measurement (e.g., bucket + stopwatch),
adjust this factor. A value of `0.8` means the sensor pulses slightly fast; `1.2` means slightly slow.

### AUTO Mode Thresholds (Section 3 in firmware)

```cpp
#define PUMP_START_LEVEL  30   // % — pump starts when level drops to this
#define PUMP_STOP_LEVEL  100   // % — pump stops when level reaches this
```

Adjust these to your preferred hysteresis range.

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

---

## Safety Logic

### Dry-Run Lockout (Level 2 — Software)
- **Trigger:** Pump is running but flow rate < 0.5 LPM for more than 30 continuous seconds
- **Action:** Relay opens (pump off), `is_error = true` pushed to Firebase
- **Reset:** Only via dashboard — user clicks ACK button, which sets
  `/pump_system/control/clear_error = true` in Firebase
- **Why it matters:** Running a jet pump dry destroys the pump head and impeller within minutes

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
    is_error:            false   (bool,  dry-run lockout flag)

  control/                       ← Dashboard writes, ESP32 reads every 3s
    mode:        "AUTO"          (string: "AUTO" | "FORCE_ON" | "FORCE_OFF")
    clear_error: false           (bool:  set true to acknowledge dry-run error)

  config/
    device/                      ← Dashboard writes, ESP32 reads every 30s (optional)
      tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level,
      dry_run_threshold_lpm, dry_run_timeout_sec, flow_calibration_factor
      (If present and valid, ESP32 uses these instead of compiled defaults; saves to NVS only when values change.)
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

*Firmware version 1.0.0 — generated from Hardware Documentation v1.0 and Software & Firmware Documentation v1.0*
