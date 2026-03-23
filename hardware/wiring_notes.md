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

## NEW — System Architecture (Updated: RS-485 + Remote Sensor Node + 12V Distribution)

**Master:** ESP32 in main enclosure  
**Remote sensor node (slave):** NodeMCU V2 (ESP8266, CP2102 USB) mounted near tank  

### Data flow (RS-485)

```text
JSN-SR04T + YF-G1  →  NodeMCU V2  →  MAX485  ⇄⇄  CAT6 (A/B pair)  ⇄⇄  MAX485  →  ESP32
```

### Power flow (CLARIFIED: two separate power systems)

```text
[Main Enclosure: Local 5V]
220V AC → 5V Adapter → ESP32 + relay + (local components)

[Tank Enclosure: Remote power over cable]
220V AC → 12V Adapter → CAT6 (+12V/GND) → [Tank Box] LM2596 (12V→5V) → NodeMCU + sensors + MAX485
```

**UPDATED rationale:** The long 30–40m run carries **12V** (not 5V) to reduce voltage drop and improve stability at the remote node.

---

## UPDATED — FINAL Pin Assignments (Production)

This section is the **single source of truth** for all GPIO assignments used for wiring.

### A) ESP32 (Main Enclosure) — MAX485 Link + Relay

| Function | GPIO | Notes |
|----------|------|------|
| Relay control (to relay IN) | GPIO 4 | Existing pump relay control (keep unchanged) |
| RS-485 UART TX (ESP32 → MAX485 DI) | GPIO 17 | UART2 TX (HardwareSerial) |
| RS-485 UART RX (MAX485 RO → ESP32) | GPIO 25 | UART2 RX (HardwareSerial) |
| RS-485 DE/RE (tied) | GPIO 5 | Output; LOW=RX mode, HIGH=TX mode |
| Debug UART (USB Serial) | GPIO 1/3 | UART0; used by Serial Monitor; do not use for RS-485 |

### B) NodeMCU V2 (ESP8266, Tank Enclosure) — Sensors + MAX485 Link

| Function | Pin Label | GPIO | Notes |
|----------|----------|------|------|
| RS-485 UART TX (NodeMCU → MAX485 DI) | TX | GPIO 1 | Hardware UART0 TX; **shares pins with USB flashing** |
| RS-485 UART RX (MAX485 RO → NodeMCU) | RX | GPIO 3 | Hardware UART0 RX; **shares pins with USB flashing** |
| RS-485 DE/RE (tied) | D5 | GPIO 14 | Output; LOW=RX mode, HIGH=TX mode |
| Flow sensor input (YF-G1 pulse) | D7 | GPIO 13 | Interrupt-capable; use local level shifting if sensor output is 5V |
| Ultrasonic TRIG (to JSN TRIG) | D1 | GPIO 5 | Output 10µs pulse |
| Ultrasonic ECHO (from JSN ECHO via divider) | D0 | GPIO 16 | Input; MUST be level-shifted to 3.3V |

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

## Section C — Power Distribution (CLARIFIED: 5V Local + 12V Distributed)

**CLARIFIED SOURCE OF TRUTH:** There are **two separate power systems**.

### C1. Main enclosure — Local 5V system (ESP32 side)

```text
5V Adapter (+) ──→ ESP32 VIN
             └──→ Relay Module VCC
             └──→ MAX485 VCC (if using 5V MAX485 module)

5V Adapter (−) ──→ ESP32 GND
             └──→ Relay Module GND
             └──→ MAX485 GND
```

### C2. Tank enclosure — 12V distributed system over CAT6

**12V injection point:** Near the main enclosure (inside a junction box or panel area), feed +12V and GND into the CAT6 power pairs.

```text
12V Adapter (+) ──→ Inline fuse (≈500 mA) ──→ CAT6 +12V pair(s)
12V Adapter (−) ───────────────────────────→ CAT6 GND pair(s)
```

> **SAFETY/RELIABILITY (CLARIFIED):** The CAT6 **GND** becomes the shared reference between the two power systems.
> Tie **ESP32 GND ↔ MAX485 GND ↔ CAT6 GND** in the main enclosure, and tie **NodeMCU GND ↔ sensors GND ↔ MAX485 GND ↔ LM2596 OUT−** in the tank enclosure.
> Without a shared reference, RS-485 common-mode voltage can drift and communication will become unreliable.

### C3. Tank enclosure — LM2596 conversion and 5V rail

```text
CAT6 +12V ──→ LM2596 (Tank Box) IN+
CAT6 GND  ──→ LM2596 (Tank Box) IN−

LM2596 OUT+ (5V rail) ──→ NodeMCU VIN / 5V pin
                      └──→ JSN-SR04T VCC
                      └──→ YF-G1 VCC
                      └──→ MAX485 VCC (if using 5V MAX485 module)

LM2596 OUT− (GND rail) ──→ NodeMCU GND
                      └──→ sensors GND
                      └──→ MAX485 GND
```

> **SAFETY (UPDATED):** Set LM2596 output to **5.00V** using a multimeter **before** connecting the NodeMCU or sensors.
> **Never** connect 12V directly to any MCU VIN/5V pin without the buck converter.

### C2. ESP32 → Relay Control

```
ESP32 GPIO 4 ──→ Relay Module IN pin
```

Single wire, any color distinct from power wires (suggested: orange).

---

## Section D — Voltage Dividers (5V → 3.3V)

Some sensor outputs are 5V but MCU GPIO pins are 3.3V maximum. In the upgraded design, dividers are placed **in the tank enclosure** (close to the NodeMCU and sensors):

- One for the **JSN-SR04T ECHO** signal (tank enclosure)
- One for the **YF-G1** flow sensor signal (tank enclosure, if the sensor outputs 5V pulses)

### Divider Formula
`V_out = V_in × R2 / (R1 + R2) = 5V × 20000 / (10000 + 20000) ≈ 3.33V ✓`

### D1. Flow Sensor Signal Divider (GPIO 34, main enclosure)

```
REMOVED: YF-G1 signal routed directly to main enclosure over long cable.

UPDATED: YF-G1 signal terminates at the tank-side NodeMCU sensor node. The main ESP32 receives flow over RS-485.
```

**REMOVED:** The main-enclosure flow divider is no longer required when flow is read by the NodeMCU.

### D2. Ultrasonic ECHO Divider (Tank node, NodeMCU ESP8266)

```
JSN-SR04T ECHO pin (5V, at tank)
  → Local 10 kΩ resistor (series, inline) ─┐
                                           ├──→ NodeMCU D0 (GPIO16)
  → Local 20 kΩ resistor (shunt to GND) ───┘
```

This divider now lives **inside the tank-side enclosure**, very close to the JSN-SR04T module and NodeMCU.
The long CAT6 run no longer carries the raw ECHO signal.

> **TRIG note:** JSN TRIG is a 3.3V output from the NodeMCU. It does not need a divider.

### D3. Flow Sensor Signal Divider (Tank node, NodeMCU ESP8266)

**UPDATED:** Many YF-series flow sensors output a 5V pulse. NodeMCU GPIO is 3.3V max.

```text
YF-G1 Signal (5V pulse)
  → Local 1 kΩ resistor (series) ─┐
                                  ├──→ NodeMCU D7 (GPIO13)
  → Local 2 kΩ / 2.2 kΩ to GND ───┘
```

Use the same divider concept as the ultrasonic ECHO divider.

---

## Section E — CAT6 UTP Cable Pinout (30–40m Run)

Entry: **PG9-C gland** (bottom-right of enclosure).

**UPDATED:** The single outdoor CAT6 cable now carries:

- **+12V and GND** distribution (remote adapter injected into CAT6)
- **RS-485 A/B differential pair** between main enclosure and tank node

> **UPDATED:** 12V is used over the long run to reduce voltage drop. The tank box converts 12V → 5V locally using LM2596.

### E1. Termination at Main Enclosure (panel) end

| CAT6 Pair | Wire Color | Signal | Termination at Enclosure End |
|-----------|-----------|--------|------------------------------|
| Power Pair | Solid Orange | +12V rail | Join with Orange/White → inline fuse → +12V injected into CAT6 |
| Power Pair | Orange/White | +12V rail | Join with Solid Orange (same +12V rail) |
| Ground Pair | Solid Blue | GND rail | Join with Blue/White → GND injected into CAT6 |
| Ground Pair | Blue/White | GND rail | Join with Solid Blue (same GND rail) |
| (Spare) | Solid Brown | — | Tape off both ends (reserved) |
| (Spare) | Brown/White | — | Tape off both ends (reserved) |
| RS-485 | Solid Green | RS-485 A | → Main RS-485 module A terminal |
| RS-485 | Green/White | RS-485 B | → Main RS-485 module B terminal |

### E2. Termination at Tank-Side Enclosure end

| Wire Color | Connects To |
|-----------|------------|
| Solid Orange | +12V input to tank box (LM2596 IN+) |
| Orange/White | +12V input (tied with Solid Orange) |
| Solid Blue | GND input to tank box (LM2596 IN−) |
| Blue/White | GND input (tied with Solid Blue) |
| Solid Brown | Spare (tape off) |
| Brown/White | Spare (tape off) |
| Solid Green | RS-485 A on tank RS-485 module |
| Green/White | RS-485 B on tank RS-485 module |

> **UPDATED power note:** In the tank box, LM2596 converts 12V → 5V and powers:
> YF-G1, JSN-SR04T module, NodeMCU, and the MAX485 module.
>
> **JSN wiring note:** JSN-SR04T TRIG/ECHO are now short local wires inside the tank box between the JSN
> module and the NodeMCU. They do **not** run over CAT6 anymore.

---

## Section F — Remote Tank Sensor Node (RS-485)

The tank-side mini enclosure contains:

**UPDATED:** NodeMCU V2 (ESP8266, CP2102 USB) is the remote sensor node.

- **NodeMCU V2 (ESP8266, CP2102)** (runs firmware that reads JSN-SR04T + YF-G1 and sends telemetry over RS-485)
- **JSN-SR04T module PCB** (waterproof ultrasonic probe on a short cable)
- **RS-485 transceiver module** (3.3V logic)
- **CAT6 terminal block** (+12V, GND, RS-485 A/B)
- Local **5V/GND busbars** and decoupling capacitors

### F1. Internal wiring (tank box)

```text
CAT6 Solid Orange + Orange/White
  → join at terminal block → +12V bus → LM2596 IN+

CAT6 Solid Blue + Blue/White
  → join at terminal block → GND bus → LM2596 IN−

LM2596:
  IN+  → +12V bus
  IN−  → GND bus
  OUT+ → 5V bus
  OUT− → GND bus

CAT6 Solid Green   → RS-485 A terminal (tank module)
CAT6 Green/White   → RS-485 B terminal (tank module)

JSN-SR04T module:
  VCC  → 5V bus
  GND  → GND bus
  TRIG → NodeMCU D1 (GPIO5)
  ECHO → 10 kΩ series → NodeMCU D0 (GPIO16), with 20 kΩ from GPIO to GND

YF-G1 flow sensor:
  VCC   → 5V bus
  GND   → GND bus
  SIG   → NodeMCU D7 (GPIO13) via local divider (required if sensor output is 5V)

NodeMCU UART (half-duplex):
  TX   → RS-485 DI (NodeMCU TX / GPIO1)
  RX   → RS-485 RO (NodeMCU RX / GPIO3)
  D5 (GPIO14) → RS-485 DE & RE (tied together)
```

The tank node sends a short ASCII status line (e.g. `LVL:87;ERR:0`) over RS-485 every 1–2 seconds. The main
ESP32 reads these lines, updates `waterLevelPct`, and treats `ERR` as the ultrasonic sensor health flag.

UPDATED: Both sensors (JSN-SR04T + YF-G1) terminate at the tank-side NodeMCU. The main ESP32 receives
both water level and flow telemetry via RS-485 only.

---

## NEW — RS-485 Implementation Notes (MAX485, Half-Duplex)

### Half-duplex overview

RS-485 is **two-wire** (A/B) differential. Only one side transmits at a time.

### DE/RE control

- MAX485 uses:
  - **DE** = Driver Enable (HIGH = transmit)
  - **RE** = Receiver Enable (LOW = receive)  
- Common wiring for simple nodes: **tie DE and RE together** and drive with one GPIO.

**Rule of thumb:**

```text
TX mode:  DE/RE = HIGH  (enable driver)
RX mode:  DE/RE = LOW   (enable receiver)
```

### FINAL DE/RE pins used in this system

- ESP32 DE/RE: **GPIO5**
- NodeMCU DE/RE: **D5 (GPIO14)**

### Termination (120 Ω)

**NEW:** Place **120 Ω** across A/B at **both physical ends** of the bus:

- One termination inside the **main enclosure** at the MAX485 module
- One termination inside the **tank box** at the MAX485 module

Do not add termination at intermediate nodes (if you later extend the bus).

### CAT6 twisted pair assignment

**UPDATED recommendation:** Use the **Green pair** for A/B (already documented in Section E) so the differential lines stay tightly coupled.

### Grounding strategy

**UPDATED requirement:** Always run a **shared GND reference** alongside A/B (Section E provides this).  
RS-485 is differential, but the transceivers still need a common reference to keep their common-mode voltage in range.

---

## NEW — Signal Integrity & Noise Handling

### Pump-induced electrical noise

- Keep **RS-485 wiring and NodeMCU/sensor wiring** physically separated from 220V paths (already covered by zone rules).
- Avoid running low-voltage wiring parallel to contactor/TOR wiring for long distances.

### Capacitor placement (required)

- **0.1 µF ceramic** close to each module power pin pair:
  - NodeMCU V2
  - MAX485 (both ends)
  - JSN-SR04T module (if possible)
- **Electrolytic bulk** near each conversion/load area:
  - At LM2596 input and output (both enclosures)
  - Near NodeMCU 5V rail inside tank box (e.g. 220–470 µF)

### Power vs signal separation

- Keep CAT6 A/B pair twisted and do not untwist more than needed at termination.
- Keep RS-485 A/B away from relay coil/control wiring inside the main box.

### Stable UART communication

- Use a fixed baud rate (e.g. 115200 8N1).
- Prefer short, line-based frames with `\r\n` terminators.
- Add a “frame timeout” in ESP32 logic: if no frames for N seconds, mark sensor as offline.

---

## NEW — Step-by-Step Hardware Assembly (Staged)

1) **Power system setup (bench)**
   - Set the **tank box** LM2596 output to **5.00V** (bench) before connecting the NodeMCU/sensors.
   - Verify the **main box** 5V adapter output is stable at ~5V under expected load.

2) **Main enclosure wiring**
   - Connect the **5V adapter** to ESP32 VIN/GND and relay VCC/GND.
   - Install the MAX485 module and connect:
     - ESP32 GPIO17 (TX2) → MAX485 DI
     - ESP32 GPIO25 (RX2) ← MAX485 RO
     - ESP32 GPIO5        → MAX485 DE and RE (tied)

3) **CAT6 distribution**
   - Connect the **12V adapter** (via inline fuse) to CAT6 +12V and GND pairs (Section E).
   - Connect RS-485 A/B to CAT6 Green pair.

4) **Tank box conversion**
   - Connect CAT6 +12V/GND to LM2596 (tank) input.
   - Confirm LM2596 tank output is stable at 5.00V under load.

5) **Tank node wiring**
   - Power NodeMCU from tank 5V rail.
   - Wire MAX485 to NodeMCU:
     - NodeMCU TX (GPIO1) → MAX485 DI
     - NodeMCU RX (GPIO3) ← MAX485 RO
     - NodeMCU D5 (GPIO14) → MAX485 DE and RE (tied)
   - Wire JSN-SR04T to NodeMCU (ECHO divider).
   - Wire YF-G1 to NodeMCU (signal divider if required).

6) **Termination**
   - Add 120 Ω across A/B at both ends if required (recommended for 30–40m).

---

## UPDATED — Testing & Validation (Staged)

### Stage 1 — Power validation

- Main box:
  - Measure 5V adapter output at ESP32 VIN/GND: **~5V**
- Tank box:
  - Measure CAT6 input: **~12V** at tank end (expect some drop)
  - Measure LM2596 output: **5.00V**

### Stage 2 — RS-485 link validation

- With both MAX485 modules powered:
  - Confirm A/B continuity end-to-end.
  - Confirm no short A↔B.
  - Confirm NodeMCU transmits and ESP32 receives frames (Serial logs should show periodic updates).

### Stage 3 — Sensor validation at tank node

- JSN-SR04T:
  - Confirm distance changes when water level changes.
  - Confirm timeouts set `ERR` in the outgoing frame.
- YF-G1:
  - Confirm pulses detected when water flows (bucket test).

---

## UPDATED — Troubleshooting (RS-485 + 12V Distribution)

### No RS-485 communication

- Swap A/B at one end (A↔B mismatch is common).
- Confirm MAX485 VCC and GND at both ends.
- Confirm DE/RE control toggles (stuck in TX can block reception).
- Add/verify 120 Ω termination at both ends.

---

## NEW — PIN VALIDATION CHECK (Sign-off)

### ESP32 (main enclosure)

- **Safe boot behaviour:** Uses GPIO25/17 (UART2) and GPIO5 for DE/RE. These do not interfere with flashing (UART0 remains on GPIO1/3 via USB).
- **No UART conflicts:** RS-485 uses UART2. Debug logs remain on UART0 (USB Serial).
- **No pin overlap:** GPIO4 remains dedicated to relay control.

### NodeMCU V2 (tank enclosure)

- **Safe boot behaviour:** Does not use GPIO0/GPIO2/GPIO15 for sensors/DE-RE. Uses D1/D0/D5/D7 which are stable.
- **UART choice justification:** RS-485 uses the hardware UART pins (GPIO1/3) for reliable timing at 115200 8N1.
- **Flashing consideration (REQUIRED):** Because RS-485 shares the hardware UART, **disconnect the MAX485 from NodeMCU TX/RX (or remove the NodeMCU from the circuit / use jumpers)** while flashing firmware over USB. Reconnect after flashing.

### Sensor timing

- **Ultrasonic timing:** TRIG on D1 and ECHO on D0 are stable pins; ECHO is level-shifted to 3.3V.
- **Flow pulses:** Flow input on D7 supports interrupt-based counting; ensure proper divider if sensor output is 5V.

### Unstable sensor readings (still noisy)

- Confirm ECHO divider is inside tank box (short wiring).
- Add/upgrade bulk capacitor near NodeMCU 5V rail.
- Ensure sensor grounds and MAX485 ground are tied to the same local GND bus.

### Voltage drop issues at tank

- Verify CAT6 power pairs are correctly paralleled for +12V and GND.
- Increase adapter capacity or reduce load.
- Confirm LM2596 input at tank stays above ~7V (typical for stable 5V output).

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
- [ ] **UPDATED/CLARIFIED voltage dividers verified:** Confirm 5V sensor outputs are level-shifted to 3.3V at the **tank-side NodeMCU** (JSN ECHO + flow signal). The main enclosure no longer expects ultrasonic ECHO on GPIO 18.
- [ ] **CAT6 pinout:** Both ends documented and consistent (see Section E)
- [ ] **PG glands tight:** All three glands sealed and cable cannot pull through
- [ ] **ESP32 firmware flashed:** WiFi, Firebase, and pin constants confirmed before power-up
- [ ] **Enclosure lid seal:** IP65 gasket seated correctly before mounting on wall

---

*Wiring references: Hardware Documentation v1.0, Software & Firmware Documentation v1.0*
*All 220V work must be performed with grid power fully disconnected. Verify with multimeter.*
