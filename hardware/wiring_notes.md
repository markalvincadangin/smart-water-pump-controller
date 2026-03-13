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

Some sensor outputs are 5V but the ESP32 GPIO pins are 3.3V maximum. Two separate voltage dividers are used:

- One for the **YF-G1** flow sensor signal (in the main enclosure)
- One for the **JSN-SR04T ECHO** signal (inside the tank-side node enclosure)

### Divider Formula
`V_out = V_in × R2 / (R1 + R2) = 5V × 20000 / (10000 + 20000) ≈ 3.33V ✓`

### D1. Flow Sensor Signal Divider (GPIO 34, main enclosure)

```
YF-G1 Signal wire (Yellow)
  → CAT6 Solid Brown pair (30–40m run)
    → At enclosure end:
      → 10 kΩ resistor (series, inline) ─┐
                                         ├──→ ESP32 GPIO 34
      → 20 kΩ resistor (shunt to GND) ───┘
```

Label this divider: **"G34 — Flow Signal"**

### D2. Ultrasonic ECHO Divider (Tank node, NodeMCU ESP8266)

```
JSN-SR04T ECHO pin (5V, at tank)
  → Local 10 kΩ resistor (series, inline) ─┐
                                           ├──→ NodeMCU GPIO (e.g. D2 / GPIO4)
  → Local 20 kΩ resistor (shunt to GND) ───┘
```

This divider now lives **inside the tank-side enclosure**, very close to the JSN-SR04T module and NodeMCU.
The long CAT6 run no longer carries the raw ECHO signal.

> **TRIG note:** JSN TRIG is a 3.3V output from the NodeMCU. It does not need a divider.

---

## Section E — CAT6 UTP Cable Pinout (30–40m Run)

Entry: **PG9-C gland** (bottom-right of enclosure).

The single outdoor CAT6 cable now carries:

- Shared **+5V** and **GND** rails for both sensors and the tank node electronics
- **YF-G1 flow signal** (to the main enclosure)
- **RS-485 A/B differential pair** between main enclosure and tank node

> Two pairs are paralleled for +5V and two for GND to reduce voltage drop over 30–40m.

### E1. Termination at Main Enclosure (panel) end

| CAT6 Pair | Wire Color | Signal | Termination at Enclosure End |
|-----------|-----------|--------|------------------------------|
| Power Pair | Solid Orange | +5V rail | Join with Orange/White → fused 5V branch from power adapter |
| Power Pair | Orange/White | +5V rail | Join with Solid Orange (same 5V rail) |
| Ground Pair | Solid Blue | GND rail | Join with Blue/White → ESP32 GND and RS-485 GND reference |
| Ground Pair | Blue/White | GND rail | Join with Solid Blue (same GND rail) |
| Flow Signal | Solid Brown | YF-G1 Signal | → 10 kΩ series → GPIO 34 → 20 kΩ → GND |
| (Spare) | Brown/White | — | Tape off both ends |
| RS-485 | Solid Green | RS-485 A | → Main RS-485 module A terminal |
| RS-485 | Green/White | RS-485 B | → Main RS-485 module B terminal |

### E2. Termination at Tank-Side Enclosure end

| Wire Color | Connects To |
|-----------|------------|
| Solid Orange | 5V busbar (+5V) inside tank box (feeds YF-G1, JSN, NodeMCU, RS-485) |
| Orange/White | 5V busbar (+5V) — tied together with Solid Orange |
| Solid Blue | GND busbar (0V) inside tank box |
| Blue/White | GND busbar (0V) — tied together with Solid Blue |
| Solid Brown | YF-G1 Yellow (Signal) |
| Brown/White | Spare (tape off) |
| Solid Green | RS-485 A on tank RS-485 module |
| Green/White | RS-485 B on tank RS-485 module |

> **Shared power note:** The existing 5V adapter in the main enclosure feeds this 5V/GND rail through a
> small inline fuse or polyfuse (~500 mA). All tank-side electronics (YF-G1, JSN-SR04T module, NodeMCU,
> and RS-485 module) share this rail.
>
> **JSN wiring note:** JSN-SR04T TRIG/ECHO are now short local wires inside the tank box between the JSN
> module and the NodeMCU. They do **not** run over CAT6 anymore.

---

## Section F — Remote Tank Sensor Node (RS-485)

The tank-side mini enclosure contains:

- **NodeMCU v3 (ESP8266)** (runs a small firmware that reads JSN-SR04T and sends water level over RS-485)
- **JSN-SR04T module PCB** (waterproof ultrasonic probe on a short cable)
- **RS-485 transceiver module** (3.3V logic)
- **CAT6 terminal block** (5V, GND, RS-485 A/B)
- Local **5V/GND busbars** and decoupling capacitors

### F1. Internal wiring (tank box)

```text
CAT6 Solid Orange + Orange/White
  → join at terminal block → 5V bus → NodeMCU VIN/5V, JSN VCC, RS-485 VCC

CAT6 Solid Blue + Blue/White
  → join at terminal block → GND bus → NodeMCU GND, JSN GND, RS-485 GND

CAT6 Solid Green   → RS-485 A terminal (tank module)
CAT6 Green/White   → RS-485 B terminal (tank module)

JSN-SR04T module:
  VCC  → 5V bus
  GND  → GND bus
  TRIG → NodeMCU GPIO (output, e.g. D1 / GPIO5)
  ECHO → 10 kΩ series → NodeMCU GPIO (input, e.g. D2 / GPIO4), with 20 kΩ from GPIO to GND

NodeMCU UART:
  TX   → RS-485 DI
  RX   → RS-485 RO
  GPIO (e.g. D5 / GPIO14) → RS-485 DE & RE (tied together)
```

The tank node sends a short ASCII status line (e.g. `LVL:87;ERR:0`) over RS-485 every 1–2 seconds. The main
ESP32 reads these lines, updates `waterLevelPct`, and treats `ERR` as the ultrasonic sensor health flag.

> **Flow sensor note:** YF-G1 remains a simple 5V → CAT6 → divider → ESP32 GPIO 34 path. Only the JSN
> ultrasonic sensor moved behind the tank-side NodeMCU node.

---

## Section G — Cable Gland Specifications

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
