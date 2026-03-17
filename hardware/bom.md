# Bill of Materials (BoM)
**Project:** Smart Water Pump Controller
**Location:** Leon, Iloilo
**Motor Load:** 1.5HP Lotus Jet Pump, 220V AC Single-Phase

---

## Electrical — High Voltage (220V AC)

| # | Component | Model / Spec | Qty | Role |
|---|-----------|-------------|-----|------|
| 1 | Miniature Circuit Breaker (MCB) | 20A, 2-Pole, 220V | 1 | Short-circuit & overcurrent protection for the entire panel |
| 2 | AC Magnetic Contactor | CJX2-2510, 25A rated, 40A thermal, **220V coil** | 1 | Heavy-duty switch that connects/disconnects the motor |
| 3 | Thermal Overload Relay (TOR) | LR2-D13, **7A–10A adjustable** | 1 | Protects motor from overheating and mechanical jamming |
| 4 | DIN Rail | 35mm wide, cut to fit enclosure | 1 | Mounting track for MCB and Contactor+TOR assembly |
| 5 | Power Wire | 10 AWG or 12 AWG THHN Copper | ~3m | All 220V interior wiring inside the enclosure |

> **TOR Calibration note:** The LR2-D13 must be set to the motor's Full Load Amperage (FLA).
> For a 1.5HP 220V single-phase motor, FLA is typically **8–9A**. Set the dial **before** first energization.
> The LR2-D13 is a 3-phase device — only **T1 and T2 / L1 and L2** are used. T3/L3 are capped.

---

## Electrical — Low Voltage (5V DC Logic)

| # | Component | Model / Spec | Qty | Role |
|---|-----------|-------------|-----|------|
| 6 | ESP32 Development Board | 38-Pin DevKit V1, 3.3V logic, Wi-Fi | 1 | Main controller — runs the firmware state machine |
| 7 | 5V Relay Module | 1-channel, 5V coil, 10A AC rated, opto-isolated | 1 | Bridges ESP32 GPIO to the 220V contactor coil trigger circuit |
| 8 | DC Power Adapter | **5V, 3A**, AC-to-DC | 1 | **UPDATED/CLARIFIED:** Main enclosure local supply (powers ESP32 + relay + local components) |
| 9 | Resistors — 1kΩ | 1/4W, through-hole | 2 | Series resistors for both voltage dividers (ECHO + Flow signal) |
| 10 | Resistors — 2kΩ | 1/4W, through-hole | 2 | Shunt resistors to GND for both voltage dividers |

> **UPDATED:** Voltage dividers are still required where 5V sensor signals enter 3.3V GPIO
> (ESP32 or NodeMCU). Exact divider values are documented in `hardware/wiring_notes.md`.

---

## Sensors

| # | Component | Model / Spec | Qty | Role |
|---|-----------|-------------|-----|------|
| 11 | Water Flow Sensor | YF-G1, 1-inch fitting, 5V DC, hall-effect | 1 | Detects dry-run condition — mounted inline on pump discharge pipe |
| 12 | Waterproof Ultrasonic Sensor | JSN-SR04T-2.0 with **separate waterproof probe** | 1 | Measures water level in 660L tank |

> **⚠ Part verification:** Confirm the ultrasonic sensor has a **separate torpedo-shaped probe on a cable**
> (JSN-SR04T-2.0), not two exposed PCB transducers (HC-SR04). Only the JSN-SR04T is waterproof.

---

## Enclosure & Cabling

| # | Component | Model / Spec | Qty | Role |
|---|-----------|-------------|-----|------|
| 13 | IP65 Enclosure | 30×40×20 cm, ABS plastic, wall-mount | 1 | Weatherproof housing for all control components |
| 14 | Cable Glands — Large | PG16 | 2 | Seal 220V input wire + 220V pump output wire entry points |
| 15 | Cable Glands — Small | PG9 | 1 | Seals CAT6 UTP sensor cable entry point |
| 16 | CAT6 UTP Cable | Outdoor-rated, Gigabit, **40m run** | 1 roll | Carries power + sensor signals from enclosure to tank |
| 17 | Terminal Block Strip | DIN-mount, WAGO 221 or screw-type, ≥4 position | 1 | Neutral busbar — separate termination for each neutral wire |
| 18 | DIN Rail Grounding Lug | 35mm DIN-compatible | 1 | Earth bonding point on the DIN rail |
| 19 | Power Cable to Pump | 3-conductor (L + N + Earth), 12 AWG, 50m | 1 run | Live, neutral, and **earth** from panel to pump motor |
| 20 | Earth Wire | 12 AWG, green/yellow, short | ~0.5m | Bonds DIN rail to house main earth terminal |
| 21 | Terminal caps / end covers | For TOR T3/L3 terminals | 2 | Caps unused 3-phase terminals — safety & inspection compliance |
| 22 | Standoffs or mounting tape | M3 nylon standoffs or foam adhesive | 4 | Secure ESP32 and relay module to enclosure base |

---

## Remote Tank Sensor Node (RS-485)

| # | Component | Model / Spec | Qty | Role |
|---|-----------|-------------|-----|------|
| 28 | NodeMCU V2 Development Board | **ESP8266 + CP2102**, 3.3V logic | 1 | **UPDATED:** Remote sensor node — reads ultrasonic + flow and sends over RS-485 |
| 29 | RS-485 to TTL Module (Main Box) | **MAX485** module | 1 | **UPDATED:** ESP32 UART ↔ RS-485 A/B (half-duplex) |
| 30 | RS-485 to TTL Module (Tank Box) | **MAX485** module | 1 | **UPDATED:** NodeMCU UART ↔ RS-485 A/B (half-duplex) |
| 31 | Tank-Side IP65 Enclosure | ~15×15×10 cm ABS, 1× PG gland for CAT6 | 1 | Weatherproof housing for NodeMCU, JSN-SR04T PCB, RS-485 module near the tank |
| 32 | DC Power Adapter (Remote) | **12V, 3A**, AC-to-DC | 1 | **NEW/CLARIFIED:** Injects 12V into CAT6 to power the tank enclosure |
| 33 | Buck Converter Module (Tank Box) | **LM2596** (12V → 5V adjustable) | 1 | **NEW/CLARIFIED:** Tank enclosure 12V→5V conversion (powers NodeMCU + sensors + MAX485) |
| 34 | DC Barrel Jack (Panel Mount) | Female socket | 1 | **NEW:** Clean 12V DC injection entry point (near main enclosure, or junction box) |
| 35 | DC Barrel Plug | Male plug | 1 | **NEW:** Mates with the 12V adapter lead / extension |
| 36 | Inline Fuse / Polyfuse | 500 mA @ 12V DC | 1 | **UPDATED:** Protects the 12V branch feeding CAT6 to tank box |
| 37 | Electrolytic Capacitors | Assorted (e.g. 47–470 µF), ≥16V | 2–4 | **UPDATED:** Bulk decoupling at LM2596 input/output and near MAX485 |
| 38 | Ceramic Capacitors | 100 nF | 4–6 | **UPDATED:** Local decoupling (NodeMCU, MAX485, buck output) |
| 39 | Termination Resistors | 120 Ω, 1/4W | 2 | **NEW:** RS-485 termination (one at each end of the bus) |
| 40 | Resistors — JSN ECHO divider | 10 kΩ and 20 kΩ, 1/4W | 2 each | On-board voltage divider to convert JSN ECHO 5V → ~3.3V for NodeMCU GPIO |
| 41 | Small terminal block strip | 4–8 way, screw or push-in | 1 | Terminates CAT6 pairs inside tank box (+12V, GND, RS-485 A/B) |

> **UPDATED/CLARIFIED power note:** There are two separate power systems:
> - **Main enclosure (local):** 5V adapter → ESP32 + relay + local components.
> - **Tank enclosure (remote):** 12V adapter injected into CAT6 → LM2596 in tank box → 5V rail for NodeMCU + sensors + MAX485.

---

## Consumables / Misc

| # | Item | Notes |
|---|------|-------|
| 23 | Heat shrink tubing | Assorted sizes — label resistor pairs (color-code dividers) |
| 24 | Wire ferrules | For all stranded wire terminations into screw terminals |
| 25 | Cable ties | Inside enclosure for wire management |
| 26 | Electrical tape / self-amalgamating tape | For outdoor junction protection |
| 27 | Label maker tape / Brady labels | Label all terminal connections |

---

## Summary Count

| Category | Items |
|----------|-------|
| High-voltage electrical | 5 |
| Low-voltage / logic | 6 |
| Sensors | 2 |
| Enclosure & cabling | 11 |
| Consumables | 5 |
| **Total line items** | **31** |

---

*Last reviewed against: Hardware Documentation v1.0, Master Manual v1.0*
