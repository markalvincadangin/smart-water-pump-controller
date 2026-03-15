# Smart Water Pump Controller
**Location:** Leon, Iloilo
**Motor:** 1.5HP Lotus Jet Pump, 220V AC Single-Phase
**Tank:** 660L (Bestank WT660)

An industrial-grade IoT pump controller that automates a deep well water system.
It combines high-voltage motor control hardware (magnetic contactor, thermal overload relay)
with an ESP32 microcontroller and a Firebase-backed web dashboard for remote monitoring
and control from any device.

---

## How It Works

The system operates in three independent layers so that a failure in software never
compromises the physical safety of the motor:

```
┌─────────────────────────────────────────────────────────────────┐
│  LAYER 3 — CLOUD / DASHBOARD                                    │
│  Next.js web app  ←→  Firebase RTDB  ←→  ESP32 (every 3s)      │
│  View level & flow, switch modes, acknowledge errors            │
├─────────────────────────────────────────────────────────────────┤
│  LAYER 2 — FIRMWARE (ESP32)                                     │
│  Reads sensors every 1s · Runs AUTO / FORCE_ON / FORCE_OFF      │
│  Dry-run lockout after 30s of zero flow while pump is running   │
├─────────────────────────────────────────────────────────────────┤
│  LAYER 1 — HARDWARE (always active)                             │
│  20A MCB  →  Contactor (CJX2-2510)  →  TOR (LR2-D13)  →  Pump │
│  TOR trips on overcurrent regardless of ESP32 or dashboard      │
└─────────────────────────────────────────────────────────────────┘
```

**Normal AUTO operation:**
1. JSN-SR04T ultrasonic sensor measures water level in the tank
2. YF-G1 flow sensor monitors water flow in the discharge pipe
3. When tank drops to configured start level (default ≤30%) → ESP32 closes relay → contactor energizes → pump starts
4. When tank reaches configured stop level (default ≥100%) → ESP32 opens relay → contactor releases → pump stops
5. Live status streams to Firebase and appears on the dashboard in real time

---

## Project Structure

```
smart-water-pump-controller/
│
├── README.md                          ← You are here
├── .gitignore
├── firebase.json                      ← Firebase CLI config (functions, rules)
├── database.rules.json                 ← RTDB security rules
│
├── docs/
│   ├── README.md                       ← Docs index
│   ├── releases/
│   │   ├── v2.0/
│   │   │   ├── firmware-rtdb-spec.md   ← Firmware & RTDB contract (v2.0)
│   │   │   ├── dashboard-ux-spec.md    ← Dashboard UI/UX spec (v2.0)
│   │   │   ├── deploy.md               ← Deploy guide for v2.0
│   │   │   └── changelog.md            ← Summary of v2 changes
│   │   └── v3.0/
│   │       ├── firmware-spec.md        ← Firmware behavior (v3.0)
│   │       └── migration-from-v2.md    ← Notes for upgrading from v2 → v3
│   ├── operations/                     ← Troubleshooting, safety (future)
│   ├── assets/
│   │   ├── diagrams/                   ← System diagrams
│   │   └── manuals/                    ← PDFs and manuals
│   └── archive/                        ← Historical plans and notes
│
├── firmware/
│   ├── arduino_smart_water_pump_controller/
│   │   ├── arduino_smart_water_pump_controller.ino  ← Open this in Arduino IDE and flash to ESP32
│   │   ├── 01_config.ino
│   │   ├── 02_sensors.ino
│   │   ├── 03_safety_pump.ino
│   │   ├── 04_persistence.ino
│   │   ├── 05_connectivity_cloud.ino
│   │   └── smart_water_pump_controller_shared.h
│   ├── platformio_smart_water_pump_controller/      ← PlatformIO project (VS Code)
│   │   ├── platformio.ini
│   │   ├── src/
│   │   └── include/
│   ├── libraries.txt                                ← Required Arduino libraries
│   └── README.md                                    ← Full flash & calibration guide
│
├── dashboard/
│   ├── app/                           ← Next.js App Router pages
│   ├── components/                    ← UI components
│   ├── lib/                           ← Firebase client, types, data hook
│   ├── .env.local.example             ← Firebase credentials template
│   ├── package.json
│   └── README.md                      ← Full dashboard setup & deploy guide
│
├── functions/                         ← Firebase Cloud Functions (email notifications)
│   ├── src/index.ts
│   └── package.json
│
└── hardware/
    ├── bom.md                         ← Full bill of materials (27 items)
    ├── enclosure_layout.md            ← Component placement & zone map
    └── wiring_notes.md                ← Complete wiring reference & checklist
```

**Firmware & dashboard design:** The authoritative specifications live under `docs/releases/` — start with:

- [docs/releases/v2.0/firmware-rtdb-spec.md](docs/releases/v2.0/firmware-rtdb-spec.md)
- [docs/releases/v2.0/dashboard-ux-spec.md](docs/releases/v2.0/dashboard-ux-spec.md)

---

## Safety Architecture

There are three independent protection layers. They are additive — a failure
in an outer layer does not disable the inner layers.

| Priority | Layer | Mechanism | Always Active? |
|----------|-------|-----------|----------------|
| 1 (highest) | Hardware | LR2-D13 TOR trips if motor current > FLA (~8–9A) | ✅ Yes |
| 2 | Firmware | Dry-run lockout (configurable, default 30s of flow < 0.5 LPM); overflow cutoff (max runtime); sensor failure detection | ✅ Yes (when powered) |
| 3 | Manual | Physical bypass switch energizes contactor directly | User-activated |

> ⚠ When the **manual bypass switch** is ON, all software protections (dry-run
> detection, tank level cutoff) are bypassed. The TOR thermal protection is the
> only protection that remains active. Use manual mode only for diagnostics.

---

## Quick Start

Complete these four phases in order. Do not power the 220V circuit until
Phase 4 is fully checked off.

---

### Phase 1 — Hardware Build

**Prerequisites:** All items from `hardware/bom.md` on hand. Power fully disconnected.

1. Mount DIN rail inside the IP65 enclosure
2. Snap MCB, Contactor, and TOR onto the DIN rail
3. Drill and install cable glands: PG16 × 2 (power), PG9 × 1 (CAT6)
4. Mount ESP32, relay module, and power adapter in the low-voltage zone
5. Wire the 220V power path: Grid → MCB → Terminal Block → Contactor → TOR → Pump
6. Wire the neutral busbar (separate terminal for each neutral wire)
7. Wire earth: DIN rail grounding lug → pump motor casing (green/yellow, 3-conductor cable)
8. Wire the coil trigger circuit: MCB Live → Relay COM → Relay NO → TOR pin 95 → TOR pin 96 → Contactor A1
9. Wire the manual bypass switch in parallel with the Relay NO–COM path
10. Wire 5V logic: Power Adapter → ESP32 VIN + Relay VCC; GND rail to both
11. Wire GPIO 4 → Relay IN
12. Build and label voltage dividers: `1kΩ + 2kΩ` for GPIO 34 (Flow) and GPIO 18 (ECHO)
13. Terminate CAT6 pairs at both enclosure and tank ends (see `hardware/wiring_notes.md` Section E)
14. Set TOR dial to motor FLA (typically **8–9A** for 1.5HP 220V)
15. Cap TOR L3/T3 terminals with terminal covers

> 📄 Full detail: `hardware/wiring_notes.md` and `hardware/enclosure_layout.md`

---

### Phase 2 — Firmware

**Prerequisites:** Arduino IDE 2.x installed, ESP32 board support added, libraries installed.

**Required libraries** (install via Arduino Library Manager):
| Library | Author | Version |
|---------|--------|---------|
| Firebase ESP Client | Mobizt | ≥ 4.4.14 |
| ArduinoJson | Benoit Blanchon | ≥ 6.21.5 **(v6.x only — not v7)** |

1. Open `firmware/arduino_smart_water_pump_controller/arduino_smart_water_pump_controller.ino` in Arduino IDE (or use PlatformIO: `firmware/platformio_smart_water_pump_controller/`)
2. Copy `secrets.h.example` to `secrets.h` and fill in WiFi, Firebase, and Email/Password credentials (see `firmware/README.md`)
3. Set board: **Tools → Board → ESP32 Dev Module**
4. Set board: **ESP32 Dev Module**; upload speed **115200**; Partition Scheme **Huge APP (3MB No OTA/1MB SPIFFS)**; Flash Mode **QIO**; PSRAM **Disabled**
5. Click **Upload**
6. Open Serial Monitor at **115200 baud** — confirm boot output shows WiFi connected
   and Firebase initialized before closing

> 📄 Full detail: `firmware/README.md`

---

### Phase 3 — Dashboard

**Prerequisites:** Node.js 18+, a Firebase project with Realtime Database, Email/Password, and Google Auth enabled.

```bash
# 1. Enter the dashboard directory
cd dashboard

# 2. Install dependencies
npm install

# 3. Set up Firebase credentials
cp .env.local.example .env.local
# Edit .env.local with your Firebase project values

# 4. Run locally
npm run dev
# Visit http://localhost:3000
```

**Firebase Console setup** (one-time):
1. [console.firebase.google.com](https://console.firebase.google.com) → your project
2. **Realtime Database** → Create database → Start in test mode
3. **Authentication** → Sign-in methods → **Email/Password** (for ESP32) and **Google** (for dashboard) → Enable both
4. **Realtime Database → Rules** — use `database.rules.json` and replace `YOUR_GOOGLE_UID` with your UID from Authentication → Users, or deploy: `firebase deploy --only database`

**Deploy to Vercel (optional):**
```bash
npx vercel
# Add each NEXT_PUBLIC_FIREBASE_* variable when prompted (same as .env.local)
```

**Install as app (PWA):** Once deployed over HTTPS, users can add the dashboard to the home screen on mobile (Add to Home Screen / Install app) for an app-like experience.

> 📄 Full detail: `dashboard/README.md`

---

### Phase 4 — Pre-Energization Checklist

Complete every item before switching the MCB on for the first time.

- [ ] Multimeter continuity check: no short between Live and Neutral at MCB input
- [ ] Tug test: pull every 220V wire — nothing moves
- [ ] TOR dial confirmed at motor FLA (8–9A)
- [ ] TOR L3/T3 terminals capped
- [ ] Neutral busbar: each wire on its own terminal slot (no shared slots)
- [ ] Earth continuity: < 1Ω from DIN rail lug to pump motor casing
- [ ] Voltage dividers verified: ~3.3V measured at GPIO 34 and GPIO 18 when sensor outputs 5V
- [ ] CAT6 pinout verified at both ends (enclosure and tank)
- [ ] All PG glands tightened — cables cannot pull through
- [ ] Firmware flashed and Serial Monitor shows healthy boot
- [ ] Dashboard running and showing live data from ESP32
- [ ] IP65 enclosure lid gasket seated correctly

---

## Full deployment

For a single, end-to-end deployment guide (Firebase, Functions, Dashboard, ESP32, and smoke tests), use **`docs/releases/v2.0/deploy.md`**.

## Notifications (Optional)

Email and **push notifications** (to phone/browser, like YouTube or Facebook) for dry-run lockout, low tank level, pump started, and overflow protection. See `docs/operations/NOTIFICATIONS_SETUP.md`.

## Implemented Enhancements

The system includes safety, resilience, power-saving, and UX features:

- **Phases 1–5:** Sensor reliability, overflow protection, NVS persistence, scheduled sleep, uptime counter, dynamic dashboard labels
- **Phase 6:** Settings tooltips (InfoTooltip), push notifications (FCM), installable PWA (Add to Home Screen), Firebase optimization assessment

See `docs/ENHANCEMENT_PLAN.md` and `docs/IMPLEMENTATION_VERIFICATION.md` for the full list and status.

---

## Firebase Data Structure

```
/pump_system/
  status/                          ← ESP32 writes every 3 seconds
    water_level_percent: 85        int    0–100
    is_running:          true      bool
    flow_rate_lpm:       12.4      float  L/min
    is_error:            false     bool   true = dry-run lockout active
    is_sensor_error:     false     bool   ultrasonic/flow sensor failure
    is_overflow_error:   false     bool   max runtime exceeded (overflow protection)
    is_sleeping:         false     bool   scheduled sleep mode active
    wifi_rssi:           -65       int    dBm, signal strength
    last_boot_reason:    "Power-on" string e.g. Power-on, Task watchdog
    uptime_minutes:      125       int    minutes since boot
    ultrasonic_cycles_ok:      120  int    cycles with ≥1 valid ultrasonic sample
    ultrasonic_cycles_timeout: 0    int    cycles with 0 valid ultrasonic samples
    ultrasonic_last_good_cm:   35.2 float  last good median distance (cm)
    flow_discard_max_sane:     0    int    discarded flow readings due to max-sane guard
    flow_stuck_high_events:    0    int    stuck-high detections
    free_heap_bytes:           182000 int
    min_free_heap_bytes:       175000 int    (ESP32)
    max_alloc_heap_bytes:      82000  int    (ESP32)
    min_free_heap_observed_bytes: 175000 int
    firebase_consecutive_failures: 0  int
    firebase_last_error:       ""     string

  control/                         ← Dashboard writes, ESP32 reads every 3 seconds
    mode:        "AUTO"            string  AUTO | FORCE_ON | FORCE_OFF
    clear_error: false             bool    set true to acknowledge dry-run/sensor/overflow errors

  config/
    device/                        ← Dashboard writes, ESP32 reads every 30 seconds
      tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level,
      dry_run_threshold_lpm, dry_run_timeout_sec, flow_calibration_factor,
      max_pump_runtime_min, sleep_enabled, sleep_start_hour, sleep_end_hour,
      sleep_emergency_level, sensor_failure_threshold, idle_sensor_interval_ms,
      idle_firebase_interval_ms
    notifications_by_user/        ← Per-user notification settings (Dashboard ↔ Cloud Function)
      $uid/
        enabled, email, fcmTokens, dryRunAlert, lowLevelAlert, lowLevelThreshold,
        pumpStartedAlert, overflowAlert
    notification_last_sent/       ← Functions only (throttling); no client access
```

---

## Pin Reference

| GPIO | Direction | Connected To | Protection |
|------|-----------|-------------|-----------|
| 4 | OUTPUT | 5V Relay IN | — (active LOW) |
| 5 | OUTPUT | JSN-SR04T TRIG | — (3.3V output, direct) |
| 18 | INPUT | JSN-SR04T ECHO | 1kΩ + 2kΩ voltage divider |
| 34 | INPUT | YF-G1 Signal | 1kΩ + 2kΩ voltage divider |

---

## Maintenance Schedule

| Interval | Task |
|----------|------|
| Monthly | Verify ultrasonic sensor accuracy against a physical dipstick measurement |
| Quarterly | Inspect PG16/PG9 cable glands for insect ingress or seal degradation |
| Quarterly | **Dependency review:** Run `npm audit` in `dashboard/` and `functions/`; follow `docs/operations/DEPENDENCY_PATCHING_PLAN.md` for high-severity fixes |
| Bi-annually | Tug test all high-current terminal connections (thermal cycling loosens screws) |
| As needed | Re-calibrate TOR dial if motor is replaced or rewound |

---

## Security

- **Dashboard:** Google sign-in required. Unauthorized users are redirected to `/login`. Firebase rules restrict control writes to your Google UID.
- **ESP32:** Uses Firebase Email/Password Auth. Credentials are in `secrets.h` (gitignored).
- **Firebase rules:** Only your UID can write to `/pump_system/control/`; ESP32 can still read control and write status.

---

## Repository Conventions

- `docs/` — read-only reference. Never edit PDFs; update the source `.drawio` and re-export.
- `firmware/` — use either Arduino IDE with `arduino_smart_water_pump_controller/` or PlatformIO with `platformio_smart_water_pump_controller/`. Use `secrets.h.example` → `secrets.h` for credentials (never commit `secrets.h`).
- `dashboard/` — never commit `.env.local`. It is gitignored. Use `.env.local.example` as the template.
- `hardware/` — plain markdown. Update these files when physical changes are made to the build.

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Microcontroller | ESP32 DevKit V1 (38-pin), Arduino framework |
| Cloud database | Firebase Realtime Database |
| Authentication | Firebase (Email/Password for ESP32, Google for dashboard) |
| Frontend framework | Next.js 14 (App Router), TypeScript |
| Styling | Tailwind CSS |
| Charts | Recharts |
| High-voltage switching | CJX2-2510 Magnetic Contactor + LR2-D13 TOR |
| Level sensing | JSN-SR04T-2.0 waterproof ultrasonic |
| Flow sensing | YF-G1 1-inch hall-effect |
| Enclosure | IP65 ABS, 30×40×20cm |
| Sensor cable | CAT6 UTP outdoor, 40m |

---

*Smart Water Pump Controller — Leon, Iloilo*
*Documentation v2.5 — aligned with ENHANCEMENT_PLAN Phases 1–6*
