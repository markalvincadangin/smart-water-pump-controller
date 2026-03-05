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
| 8 | DC Power Adapter | 5V, 3A, AC-to-DC | 1 | Powers ESP32 and relay module from the 220V panel |
| 9 | Resistors — 1kΩ | 1/4W, through-hole | 2 | Series resistors for both voltage dividers (ECHO + Flow signal) |
| 10 | Resistors — 2kΩ | 1/4W, through-hole | 2 | Shunt resistors to GND for both voltage dividers |

> **Voltage divider purpose:** The YF-G1 and JSN-SR04T both output 5V signals.
> The ESP32 GPIO pins are **3.3V max**. Each signal line needs a 1kΩ/2kΩ divider.
> `V_out = 5V × (2kΩ / (1kΩ + 2kΩ)) = 3.33V` ✓

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
| Low-voltage / logic | 5 |
| Sensors | 2 |
| Enclosure & cabling | 10 |
| Consumables | 5 |
| **Total line items** | **27** |

---

*Last reviewed against: Hardware Documentation v1.0, Master Manual v1.0*
