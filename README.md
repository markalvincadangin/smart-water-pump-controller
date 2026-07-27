# SmartFlow

**An industrial-grade IoT water pump controller** combining high-voltage motor control hardware with a Firebase-backed Next.js dashboard for remote monitoring and automation.

[![Build Status](https://img.shields.io/badge/build-CI%20configured-brightgreen)](.github/workflows/deploy.yml)
[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Firmware](https://img.shields.io/badge/firmware-v1.0.0-blue)](firmware/README.md)
[![Dashboard](https://img.shields.io/badge/dashboard-v1.0.0-blue)](dashboard/README.md)

---

## About

SmartFlow automates a 1.5 HP deep-well water pump system filling a 660L tank with real-time monitoring, remote control, and multi-layer safety protection. The system remains safe even if the cloud connection fails—hardware interlocks and firmware safeguards prevent damage.

**Deployed in:** Western Visayas, Philippines | **Status:** Production  
**Hardware:** ESP32 master + ESP8266 tank sensor + CJX2-2510 magnetic contactor + LR2-D13 thermal overload relay

> Safety-critical system: review [DEPLOYMENT_SAFETY.md](DEPLOYMENT_SAFETY.md) before first energization.

---

## Key Features

- **Three-layer safety architecture** — Hardware interlock, firmware lockouts, and manual bypass
- **Real-time monitoring** — Water level (ultrasonic), flow rate (hall-effect), WiFi signal, heap memory
- **Three operating modes** — AUTO (level-triggered), MANUAL (on/off), COUNTDOWN (timer-based)
- **Dry-run protection** — Detects pump cavitation and stops automatically
- **Overflow prevention** — Configurable max runtime limit
- **Sensor failure handling** — Auto-bypass or manual intervention options
- **Remote dashboard** — Next.js PWA, mobile-installable, works offline
- **Firebase integration** — Real-time RTDB, Email/Password + Google OAuth
- **OTA updates** — Over-the-air firmware updates on tank sensor node
- **Scheduled sleep mode** — Reduces power consumption during off-peak hours
- **Comprehensive audit log** — Every action (mode change, error, reboot) timestamped and searchable

---

## Quick Start

### Prerequisites

- Arduino IDE 2.x or PlatformIO + VS Code
- Node.js 22+
- Firebase project with Realtime Database, Email/Password, and Google authentication enabled
- Hardware: ESP32 DevKit V1, NodeMCU V2 (ESP8266), relay module, 40m CAT6 UTP cable, IP65 enclosure

### 1. Hardware Assembly (30 min)

See [hardware/bom.md](hardware/bom.md) for the complete bill of materials and [hardware/wiring_notes.md](hardware/wiring_notes.md) for detailed wiring.

```
Power chain (always active, independent of firmware):
  Grid (220V) → MCB → Magnetic Contactor → Thermal Overload Relay → 1.5 HP Pump
```

---

### 2. Firmware Setup (15 min)

**Install libraries** (Arduino Library Manager):
- Firebase ESP Client ≥ 4.4.14 by Mobizt
- ArduinoJson ≥ 6.21.5 by Benoit Blanchon (v6.x only)

**Flash ESP32 master:**
```bash
# Option 1: Arduino IDE
1. Open: firmware/arduino_smart_water_pump_controller/arduino_smart_water_pump_controller.ino
2. Board: ESP32 Dev Module | Speed: 115200 | Partition: Huge APP (3MB No OTA/1MB SPIFFS)
3. Click Upload

# Option 2: PlatformIO
cd firmware/master_node && pio run -t upload
```

**Flash ESP8266 tank sensor:**
```bash
# Arduino IDE
1. Open: firmware/arduino_sensor_node/arduino_sensor_node.ino
2. Board: NodeMCU 1.0 (ESP-12E Module) | Speed: 115200
3. Click Upload
```

**Configure credentials:**
- Copy `firmware/secrets.h.example` → `firmware/secrets.h`
- Add WiFi SSID, password, Firebase URL, and email/password credentials
- **Never commit secrets.h**

See [firmware/README.md](firmware/README.md) for full setup details and calibration.

---

### 3. Dashboard Setup (10 min)

```bash
cd dashboard
npm install
cp .env.local.example .env.local
# Edit .env.local with Firebase credentials
npm run dev
# Visit http://localhost:3000
```

**Firebase Console configuration** (one-time):
1. Enable Email/Password authentication (for ESP32)
2. Enable Google authentication (for users)
3. Create Realtime Database in test mode
4. Deploy security rules: `firebase deploy --only database`

See [dashboard/README.md](dashboard/README.md) for full setup and deployment to Vercel.

---

### 4. Pre-Energization Checklist

✅ Complete every item before powering the 220V circuit:

- [ ] Multimeter continuity check: no short between Live and Neutral
- [ ] Tug test: all 220V wires secure
- [ ] Thermal Overload Relay dial set to motor FLA (8–9A)
- [ ] TOR L3/T3 terminals capped
- [ ] Earth continuity verified (< 1Ω from enclosure to pump casing)
- [ ] Voltage dividers verified at tank node inputs (JSN ECHO and flow signal are MCU-safe at ~3.3V)
- [ ] CAT6 pinout verified at enclosure and tank ends
- [ ] All PG cable glands tightened
- [ ] Firmware flashed; Serial Monitor shows healthy boot
- [ ] Dashboard running and showing live telemetry
- [ ] IP65 enclosure lid gasket seated correctly

For a complete commissioning and operations checklist, use [DEPLOYMENT_SAFETY.md](DEPLOYMENT_SAFETY.md).

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ LAYER 3 — CLOUD / DASHBOARD                                 │
│ Next.js PWA  ↔  Firebase RTDB  ↔  ESP32 (every 3s)          │
│ View, control, configure, audit trail                       │
├─────────────────────────────────────────────────────────────┤
│ LAYER 2 — FIRMWARE (ESP32 + ESP8266)                        │
│ Poll tank node (RS-485) → Compute level → Apply logic       │
│ Emergency stop, dry-run lockout, overflow cutoff            │
├─────────────────────────────────────────────────────────────┤
│ LAYER 1 — HARDWARE (always active)                          │
│ 20A MCB → Contactor → Thermal Overload Relay → Pump        │
│ TOR trips on overcurrent (independent of software)          │
└─────────────────────────────────────────────────────────────┘
```

**Data flow (AUTO mode example):**
1. Tank sensor (ESP8266) reads ultrasonic distance + flow pulses
2. ESP32 polls via RS-485 every 1 second (CRC-protected)
3. ESP32 computes water level (%) and validates freshness
4. When level ≤ start threshold → relay closes → contactor energizes → pump starts
5. When level ≥ stop threshold → relay opens → pump stops
6. Live status syncs to Firebase every 3 seconds
7. Dashboard displays real-time metrics and audit log

---

## Project Structure

```
smart-water-pump-controller/
├── README.md                              ← You are here
├── firmware/
│   ├── arduino_smart_water_pump_controller/    ← ESP32 master (Arduino IDE)
│   ├── arduino_sensor_node/                    ← ESP8266 tank node (Arduino IDE)
│   ├── master_node/ ← ESP32 master (PlatformIO)
│   ├── sensor_node/                 ← ESP8266 tank node (PlatformIO)
│   └── README.md                               ← Hardware pinout & calibration
├── dashboard/
│   ├── app/                    ← Next.js App Router pages
│   ├── components/             ← UI components
│   ├── lib/                    ← Firebase client, hooks, types
│   └── README.md               ← Setup & deployment guide
├── functions/                  ← Firebase Cloud Functions (email alerts)
├── hardware/
│   ├── bom.md                  ← Bill of materials
│   ├── wiring_notes.md         ← Wiring reference & checklist
│   └── enclosure_layout.md     ← Component placement
├── docs/
│   ├── releases/               ← Release notes & deployment checklist
│   ├── specs/                  ← Firmware & dashboard specifications
│   ├── operations/             ← Troubleshooting & notifications setup
│   └── README.md               ← Documentation index
└── database.rules.json         ← Firebase RTDB security rules
```

**Full file structure & documentation index:** [docs/README.md](docs/README.md)

---

## Technology Stack

| Component | Technology |
|-----------|-----------|
| **Microcontroller** | ESP32 DevKit V1, NodeMCU V2 (ESP8266) |
| **Firmware** | Arduino framework (C/C++) |
| **Cloud database** | Firebase Realtime Database |
| **Authentication** | Firebase (Email/Password + Google OAuth) |
| **Frontend** | Next.js 15, TypeScript, Tailwind CSS |
| **Charts** | Recharts |
| **Switching** | CJX2-2510 magnetic contactor + LR2-D13 TOR |
| **Level sensor** | JSN-SR04T-2.0 ultrasonic (waterproof) |
| **Flow sensor** | YF-G1 1-inch hall-effect meter |
| **Enclosure** | IP65 ABS 30×40×20 cm |
| **Cable** | CAT6 UTP outdoor 40m |

---

## Safety Architecture

Three independent protection layers ensure the system fails safe:

| Layer | Mechanism | Always Active? | Covers |
|-------|-----------|---|---|
| **Hardware** | LR2-D13 TOR trips on motor current > 8–9A | ✅ Yes | Overcurrent, phase loss |
| **Firmware** | Dry-run lockout, overflow cutoff, sensor validity gate | ✅ Yes (when powered) | Cavitation, overflow, comm loss |
| **Manual** | Physical bypass switch energizes contactor directly | User-activated | Emergency start (diagnostics) |

> ⚠️ **Manual bypass mode:** All software protections are bypassed. Only the thermal overload relay remains active. Use only for diagnostics.

See [firmware/README.md](firmware/README.md#safety-architecture) for detailed safety specifications.

---

## Configuration

Device parameters are stored in Firebase and applied at runtime. No reflash required.

| Parameter | Default | Min | Max | Notes |
|-----------|---------|-----|-----|-------|
| Tank empty distance (cm) | 122 | 25 | 200 | Calibrate in field |
| Tank full distance (cm) | 30 | 25 | 150 | Calibrate in field |
| Pump start level (%) | 20 | 0 | 100 | Start filling at this level |
| Pump stop level (%) | 90 | 0 | 100 | Stop filling at this level |
| Dry-run threshold (L/min) | 0.5 | 0 | 60 | Flow must exceed this to run |
| Dry-run timeout (sec) | 30 | 1 | 300 | Time before lockout triggers |
| Max pump runtime (min) | 60 | 1 | 1440 | Overflow protection limit |

All parameters are tunable via the dashboard. See [docs/specs/README.md](docs/specs/README.md) for RTDB schema and protocol documentation.

---

## Troubleshooting

| Symptom | Cause | Solution |
|---------|-------|----------|
| Firebase not initialized | WiFi not connected or credentials wrong | Check ESP32 Serial Monitor; verify SSID/password in secrets.h |
| Ultrasonic reads 0% | Sensor timeout or out of range | Verify sensor power and CAT6 wiring; check tank distance is 20–600 cm |
| Pump not responding to dashboard commands | ESP32 not polling Firebase | Check cloud control poll interval in logs; verify ESP32 WiFi connection |
| Thermal overload keeps tripping | Motor overload or TOR dial set too low | Lower pump duty cycle; re-calibrate TOR dial to motor FLA |
| Dashboard shows stale data | Cloud control poll failure | Check ESP32 uptime; verify Firebase rules allow ESP32 UID to read |

Full troubleshooting guide: [docs/operations/troubleshooting.md](docs/operations/troubleshooting.md)

---

## Deployment

For complete end-to-end deployment and go-live checklist, see [docs/releases/v1.0.0/deploy.md](docs/releases/v1.0.0/deploy.md).

**Deploy dashboard to Vercel:**
```bash
cd dashboard
npx vercel
# Add NEXT_PUBLIC_FIREBASE_* variables when prompted
```

**Enable push notifications (optional):**
See [docs/operations/notifications_setup.md](docs/operations/notifications_setup.md) for Firebase Cloud Functions setup and FCM token management.

> **Note on calibration:** If you're experiencing water level reading discrepancies, see [docs/operations/calibration_fix_integration.md](docs/operations/calibration_fix_integration.md) for diagnosis and adjustment procedures.

---

## Maintenance

| Interval | Task |
|----------|------|
| Monthly | Verify ultrasonic accuracy vs. physical dipstick |
| Quarterly | Inspect IP65 enclosure gasket and PG cable glands |
| Quarterly | Run `npm audit` in `dashboard/` and `functions/`; address high-severity findings |
| Bi-annually | Tug-test all high-current terminal connections (thermal cycling loosens screws) |
| As needed | Re-calibrate TOR if motor is replaced or rewound |

---

## Contributing

We welcome contributions focused on safety, reliability, and maintainability.

- Start here: [CONTRIBUTING.md](CONTRIBUTING.md)
- Key requirement: preserve fail-safe behavior (pump OFF on fault/ambiguity)
- Validate before PR:
  - Firmware: `pio run -d firmware/master_node` and `pio run -d firmware/sensor_node`
  - Dashboard: `cd dashboard && npm run validate`

---

## Security

- Vulnerability reporting and disclosure policy: [SECURITY.md](SECURITY.md)
- Dashboard requires authenticated access.
- Never commit secrets (`dashboard/.env.local`, firmware `secrets.h`).
- Apply least-privilege Firebase rules for control and status paths.

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE).

## Trademark Notice

Third-party product names, logos, and brands (for example, ESP32, ESP8266, Firebase, Next.js, and hardware model names) are the property of their respective owners. Their use in this repository is for identification and compatibility reference only and does not imply affiliation or endorsement.

---

## About This Project

**SmartFlow** was created by Mark Alvin Cadangin with AI assistance for automating water pump systems in Leon, Iloilo. The project combines custom hardware interfacing, real-time cloud integration, and a full-stack web application.

**Technology credits:**
- Firebase (Real-time database and authentication)
- Next.js & Tailwind (Dashboard framework and styling)
- Arduino & PlatformIO (Microcontroller development)

---

## Support

- **Documentation:** [docs/README.md](docs/README.md)
- **Release documentation:** See [docs/README.md](docs/README.md)
- **Issues:** Use the repository Issues tab on GitHub
- **Discussions:** Use the repository Discussions tab on GitHub

---

**SmartFlow** — Industrial IoT automation for reliable water systems.  
*Built in the Philippines. Deployed in production. Battle-tested. Yours to use, modify, and improve.*
