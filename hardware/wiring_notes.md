# Wiring Notes
**Project:** Smart Water Pump Controller
**Standard:** Single-phase 220V AC, Philippines grid

---

## ⚠ Safety Rules — Read Before Touching Any Wire

- **Verify dead with a multimeter** before touching any terminal. Visual confirmation of MCB position is not sufficient.
- Work on the **low-voltage zone first** when assembling, **high-voltage zone last**.
- The TOR thermal protection is the **only protection that remains active during manual bypass mode**. All ESP32 software protections are bypassed when the manual switch is used.
- Never double-terminate two wires into one MCB screw terminal. Use the neutral busbar.
- The 50m pump output cable must be **3-conductor** (Live + Neutral + Earth). A 2-conductor cable is not acceptable — the pump casing must be earthed.

---

## Wire Color Convention

| Color | Meaning | Where Used |
|-------|---------|-----------|
| **Black** or Brown | 220V Live (Line) | All 220V live conductors inside enclosure |
| **Blue** | 220V Neutral | All 220V neutral conductors inside enclosure |
| **Green/Yellow** | Earth (PE) | All grounding/earth conductors |
| **Red** | +5V DC | Power adapter output → ESP32 VIN, Relay VCC |
| **Black** (thin) | DC GND | ESP32 GND, Relay GND, sensor GND |
| **Orange** | GPIO signal | Relay IN wire (GPIO 4 → Relay IN) |

> **Note on CAT6 pairs:** The CAT6 cable uses its own pair-color convention — see CAT6 Pinout section below.

---

## Section A — High Voltage Power Path (220V AC)

### A1. Grid Input → MCB

```
Grid Live  (Black) ──→ MCB Top Terminal L (left pole)
Grid Neutral (Blue) ──→ MCB Top Terminal N (right pole)
Grid Earth (Green/Yellow) ──→ DIN Rail Grounding Lug
```

Cable entry: **PG16-A gland** (bottom-left of enclosure).

### A2. MCB → Neutral Busbar + Contactor

```
MCB Bottom Live (L) ──→ Neutral Busbar Position 1  ──→ Branch A: Contactor L1 (top-left)
                                                    ──→ Branch B: 5V Power Adapter Live input
                                                    ──→ Branch C: Relay Module COM terminal

MCB Bottom Neutral (N) ──→ Neutral Busbar Position 2 ──→ Branch A: Contactor A2 (coil neutral)
                                                        ──→ Branch B: 5V Power Adapter Neutral input
                                                        ──→ Branch C: Pump output Neutral (TOR T2)
```

> **Critical:** Each of the three neutral branches must land on its **own terminal slot** in the busbar. Never share a slot between two wires.

### A3. MCB → Contactor Power Poles

```
MCB Bottom Live ──→ Contactor 1/L1 (top-left power terminal)
MCB Bottom Live ──→ Contactor 3/L2 (top-right power terminal)
```

Short 12 AWG wires, dressed against the DIN rail backplate.

### A4. Contactor Output → TOR → Pump

```
Contactor 2/T1 (bottom-left) ──→ TOR L1 (top-left)
Contactor 4/T2 (bottom-right)──→ TOR L2 (top-right)
[TOR L3 / T3 — CAPPED. Not connected. Single-phase install.]

TOR T1 (bottom-left)  ──→ Pump Live wire  (Black, 50m, via PG16-B)
TOR T2 (bottom-right) ──→ Pump Neutral wire (Blue, 50m, via PG16-B)
```

### A5. Earth / Grounding Path

```
Grid Earth (Green/Yellow, PG16-A) ──→ DIN Rail Grounding Lug
DIN Rail Grounding Lug ──→ Pump Motor Casing Earth Terminal (Green/Yellow, 50m, via PG16-B)
```

> The 50m pump cable must contain all three conductors: Live (Black), Neutral (Blue), Earth (Green/Yellow).

---

## Section B — Contactor Coil Trigger Circuit (220V Control)

This is the "kill-switch route" — a 220V control loop that energizes the contactor coil.

```
MCB Bottom Live
  ──→ Relay Module COM terminal
      ──→ [When Relay NO closes] Relay NO terminal
          ──→ TOR NC Pin 95
              ──→ [When TOR OK] TOR NC Pin 96
                  ──→ Contactor A1 (coil +)

MCB Bottom Neutral ──→ Contactor A2 (coil −)
```

**Manual bypass switch** (wired in **parallel** with the Relay NO–COM path):

```
MCB Bottom Live ──→ Manual Switch Terminal 1
Manual Switch Terminal 2 ──→ TOR NC Pin 95
```

When the manual switch is flipped ON, it feeds Live directly to TOR pin 95, energizing the contactor coil regardless of the relay or ESP32 state.

> **TOR protection note:** If the motor overheats at any time, the TOR physically opens the circuit between pin 95 and pin 96, collapsing the coil regardless of the switch or relay position.

---

## Section C — Low Voltage DC Wiring (5V)

### C1. Power Distribution

```
5V Power Adapter (+) ──→ ESP32 VIN pin
                    ──→ Relay Module VCC pin

5V Power Adapter (−) ──→ ESP32 GND pin
                    ──→ Relay Module GND pin
```

### C2. ESP32 → Relay Control

```
ESP32 GPIO 4 ──→ Relay Module IN pin
```

Single wire, any color distinct from power wires (suggested: orange).

---

## Section D — Voltage Dividers (5V → 3.3V)

Both sensor signal lines output 5V but the ESP32 GPIO pins are 3.3V maximum. Two independent voltage dividers are required — one per signal line.

### Divider Formula
`V_out = V_in × R2 / (R1 + R2) = 5V × 2000 / (1000 + 2000) = 3.33V ✓`

### D1. Flow Sensor Signal Divider (GPIO 34)

```
YF-G1 Signal wire (Yellow)
  → CAT6 Solid Brown pair (40m run)
    → At enclosure end:
      → 1kΩ resistor (series, inline) ─┐
                                        ├──→ ESP32 GPIO 34
      → 2kΩ resistor (shunt to GND) ───┘
```

Label this divider: **"G34 — Flow Signal"**

### D2. Ultrasonic ECHO Divider (GPIO 18)

```
JSN-SR04T ECHO pin
  → CAT6 Green/White pair (40m run)
    → At enclosure end:
      → 1kΩ resistor (series, inline) ─┐
                                        ├──→ ESP32 GPIO 18
      → 2kΩ resistor (shunt to GND) ───┘
```

Label this divider: **"G18 — Echo Signal"**

### D3. Ultrasonic TRIG (No Divider Needed)

```
ESP32 GPIO 5 ──→ CAT6 Solid Green pair ──→ JSN-SR04T TRIG pin
```

TRIG is an **output** from the ESP32 (3.3V), not an input. No voltage divider needed.

---

## Section E — CAT6 UTP Cable Pinout (40m Run)

Entry: **PG9-C gland** (bottom-right of enclosure).

| CAT6 Pair | Wire Color | Signal | Termination at Enclosure End |
|-----------|-----------|--------|------------------------------|
| Power Pair | Solid Orange | +5V | → YF-G1 VCC (Red wire) |
| Power Pair | Orange/White | +5V | → JSN-SR04T VCC |
| Ground Pair | Solid Blue | GND | → YF-G1 GND (Black wire) |
| Ground Pair | Blue/White | GND | → JSN-SR04T GND |
| Flow Signal | Solid Brown | YF-G1 Signal | → 1kΩ series → GPIO 34 → 2kΩ → GND |
| (Spare) | Brown/White | — | Tape off both ends |
| Ultrasonic TRIG | Solid Green | TRIG | → GPIO 5 (direct, no resistor) |
| Ultrasonic ECHO | Green/White | ECHO | → 1kΩ series → GPIO 18 → 2kΩ → GND |

**At the tank end**, connect:

| Wire Color | Connects To |
|-----------|------------|
| Solid Orange | YF-G1 Red (VCC) |
| Orange/White | JSN-SR04T VCC |
| Solid Blue | YF-G1 Black (GND) |
| Blue/White | JSN-SR04T GND |
| Solid Brown | YF-G1 Yellow (Signal) |
| Solid Green | JSN-SR04T TRIG |
| Green/White | JSN-SR04T ECHO |
| Brown/White | Leave unterminated (spare) |

> **YF-G1 wire colors:** Red = VCC, Black = GND, Yellow = Signal
> **JSN-SR04T connector:** Pin order on PCB is VCC, TRIG, ECHO, GND (left to right)

---

## Section F — Cable Gland Specifications

| Gland | Type | Cable OD | Location | Sealed With |
|-------|------|----------|----------|-------------|
| PG16-A | PG16 | 10–14mm | Bottom-left | PG nut + rubber seal (included) |
| PG16-B | PG16 | 10–14mm | Bottom-center | PG nut + rubber seal (included) |
| PG9-C | PG9 | 4–8mm | Bottom-right | PG nut + rubber seal (included) |

After routing cables, fully tighten the PG nut until the rubber seal grips the cable jacket firmly. Test by pulling the cable — it should not move.

---

## Pre-Energization Checklist

Complete every item before throwing the MCB.

- [ ] **Multimeter continuity:** No short between Live and Neutral at MCB input terminals
- [ ] **Tug test:** Pull every 220V wire firmly — nothing should move
- [ ] **TOR dial set:** Confirm dial is at motor FLA (typically 8–9A for 1.5HP 220V)
- [ ] **TOR T3/L3 capped:** Both unused terminals have terminal covers installed
- [ ] **Neutral busbar:** Each neutral wire has its own terminal slot (no shared slots)
- [ ] **Earth continuity:** Green/yellow wire measures <1Ω from DIN rail lug to pump casing
- [ ] **Voltage dividers verified:** Measure ~3.3V at GPIO 34 and GPIO 18 when sensor outputs 5V
- [ ] **CAT6 pinout:** Both ends documented and consistent (see Section E)
- [ ] **PG glands tight:** All three glands sealed and cable cannot pull through
- [ ] **ESP32 firmware flashed:** WiFi, Firebase, and pin constants confirmed before power-up
- [ ] **Enclosure lid seal:** IP65 gasket seated correctly before mounting on wall

---

*Wiring references: Hardware Documentation v1.0, Software & Firmware Documentation v1.0*
*All 220V work must be performed with grid power fully disconnected. Verify with multimeter.*
