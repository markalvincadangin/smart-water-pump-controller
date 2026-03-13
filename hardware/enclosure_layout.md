# Enclosure Layout
**Enclosure:** 30×40×20 cm IP65 ABS Plastic, wall-mount
**Orientation:** Portrait (30cm wide × 40cm tall)

---

## Zone Overview

The enclosure interior is divided into two horizontal zones separated by the DIN rail.

```
┌─────────────────────────────────────────┐  ← Top of enclosure (40cm wide)
│                                         │
│  ┌──────────┐  ┌──────────────────────┐ │
│  │  20A MCB │  │  Contactor (CJX2)    │ │
│  │  2-Pole  │  │  + TOR (LR2-D13)     │ │  ← HIGH VOLTAGE ZONE
│  │          │  │  attached below      │ │    (DIN rail mounted)
│  └──────────┘  └──────────────────────┘ │
│  ┌────────────────────────────────────┐ │
│  │         ════ DIN Rail ════          │ │  ← DIN RAIL (divider)
│  └────────────────────────────────────┘ │
│                                         │
│  ┌──────────────┐  ┌─────────┐          │
│  │ 5V 3A DC     │  │ Neutral │          │  ← TRANSITION ZONE
│  │ Power Adapter│  │ Busbar  │          │    (line voltage in,
│  └──────────────┘  └─────────┘          │     DC out)
│                                         │
│  ┌──────────┐  ┌──────────────┐         │
│  │  ESP32   │  │  5V Relay    │         │  ← LOW VOLTAGE ZONE
│  │ DevKit   │  │  Module      │         │    (3.3V / 5V logic)
│  └──────────┘  └──────────────┘         │
│                                         │
├─────────────────────────────────────────┤  ← Bottom wall
│  [PG16]  [PG16]  [PG9]                  │  ← Cable glands
└─────────────────────────────────────────┘
  220V IN  Pump OUT  CAT6 Sensor
```

---

## Zone 1 — High Voltage (Top Section, DIN Rail)

**Rule:** Nothing 5V/3.3V in this zone. No sensor wires routed through here.

| Component | Position on DIN Rail | Notes |
|-----------|---------------------|-------|
| 20A MCB (2-pole) | Left side of rail | Input terminals face UP toward top wall |
| CJX2-2510 Contactor | Right side of rail | Slides onto rail; input L1/L2 face UP |
| LR2-D13 TOR | Plugs into **bottom** of Contactor | Copper prongs insert into T1, T2, T3; TOR hangs below contactor |
| DIN Rail Grounding Lug | Far right end of rail | Green/yellow earth wire bonds here |

**MCB → Contactor wiring path:**
Short 12 AWG wires run horizontally from MCB bottom terminals directly to Contactor L1/L2 top terminals. Keep these short and dressed against the DIN rail backplate.

**Terminal Block (Neutral Busbar) placement:**
Mount a 4-position DIN screw terminal strip immediately to the left of the MCB on the DIN rail. This serves as the neutral distribution point. Never double-wire into a single MCB terminal.

---

## Zone 2 — Transition (Middle, below DIN Rail)

| Component | Position | Notes |
|-----------|----------|-------|
| 5V 3A DC Power Adapter | Left-center, mounted with standoffs or adhesive bracket | 220V input wires come down from neutral busbar and MCB; 5V output wires go down to ESP32 zone |
| Neutral Busbar terminal strip | Right of power adapter | 3 branches: Contactor A2 coil, pump output neutral, power adapter neutral |

**Wire segregation:** Use a plastic cable duct or cable ties to keep the 220V wires on the left side of the enclosure and DC wires on the right side as they descend from this zone.

---

## Zone 3 — Low Voltage (Bottom Section)

**Rule:** No 220V wires in this zone. All wiring is 5V DC or 3.3V signal level.

| Component | Position | Mounting | Notes |
|-----------|----------|----------|-------|
| ESP32 DevKit V1 (38-pin) | Bottom-left | M3 nylon standoffs, 5mm height | USB port faces left wall for potential laptop access |
| 5V Relay Module | Bottom-center | M3 nylon standoffs | IN pin faces toward ESP32; NO/COM/NC terminals face right toward HV zone edge |
| Voltage divider resistors | On small perfboard or terminal strip, mounted adjacent to ESP32 | Solder or terminal-mount | Label: "G34-Flow" (flow sensor signal divider) |
| RS-485 Module (Tank Link) | Bottom-right, near CAT6 entry | M3 standoffs or adhesive | DI/RO/DE/RE wired to ESP32 UART & GPIO; A/B wired to CAT6 Green pair |

---

## Bottom Wall — Cable Glands

```
Left edge ──────────────── Right edge
  [PG16-A]    [PG16-B]    [PG9-C]
  220V Grid   Pump 50m    CAT6 Sensor
  Input       Output      Cable (40m)
```

| Gland | Type | Cable | Direction |
|-------|------|-------|-----------|
| PG16-A | PG16 | 220V Live + Neutral from grid (2-conductor 12AWG + earth) | Enters from outside power source |
| PG16-B | PG16 | 50m pump cable (Live + Neutral + **Earth**, 3-conductor 12AWG) | Exits to pump motor |
| PG9-C | PG9 | 40m CAT6 UTP outdoor cable | Exits to water tank |

> **Earth wire routing:** The green/yellow earth from PG16-A bonds to the DIN rail grounding lug.
> The green/yellow earth in PG16-B connects TOR T2 output → pump motor casing earth terminal.
> These are **two separate runs** — both originate from the DIN rail grounding lug.

---

## Tank-Side Mini Enclosure — Remote Ultrasonic Node

A small IP65 box is mounted near the tank to host the remote ultrasonic node.

### Layout

```text
┌──────────────────────────────┐
│  JSN-SR04T probe cable  ─────┼──→ Down into tank
│                              │
│  [ Terminal Block ]          │  ← CAT6 5V/GND/A/B
│                              │
│  [ RS-485 Module ]           │
│                              │
│  [ NodeMCU v3 (ESP8266) ]    │
│                              │
│  [ 5V / GND bus + caps ]     │
└──────────────────────────────┘
```

- CAT6 enters through a small gland at the bottom.
- The terminal block breaks out:
  - Paralleled +5V (Orange/Orange-White)
  - Paralleled GND (Blue/Blue-White)
  - RS-485 A/B (Green/Green-White)
- The NodeMCU, JSN module PCB, and RS-485 module are mounted on standoffs or a small perfboard.
- A 100–470 µF electrolytic capacitor and 100 nF ceramic capacitor are placed close to the NodeMCU
  between 5V and GND.

## Internal Wire Routing Guidelines

1. **220V wires** (black Live, blue Neutral, green/yellow Earth): Route along the LEFT side of the enclosure, dressed with cable ties against the backplate.
2. **5V DC wires** (red +5V, black GND): Route along the RIGHT side of the enclosure descending from the power adapter to ESP32/relay.
3. **Signal wires** (resistor divider connections, relay IN wire): Shortest possible path; keep away from 220V runs.
4. **CAT6 pairs from PG9 gland**: Route directly to the bottom-right low-voltage zone. Terminate resistors immediately at the cable end — do not run the raw 5V sensor signals across the enclosure.
5. **Minimum separation:** Maintain at least **25mm** between 220V and DC wiring runs where they must be parallel.

---

## Physical Assembly Sequence

Follow this order to avoid having to move already-mounted components:

1. Mount DIN rail (drill and screw to backplate)
2. Snap MCB, Contactor+TOR, and neutral busbar onto DIN rail
3. Install DIN rail grounding lug on rail
4. Drill and install PG16-A, PG16-B, PG9-C glands in bottom wall
5. Mount power adapter in transition zone (adhesive or bracket)
6. Mount ESP32 and relay module in low-voltage zone with standoffs
7. Prepare and mount voltage divider perfboard adjacent to ESP32
8. Run all 220V wiring (High Voltage Zone first — top to bottom)
9. Run neutral busbar wiring
10. Run 5V DC wiring (power adapter → ESP32 → relay)
11. Run signal wiring (relay IN, voltage divider connections)
12. Terminate and dress CAT6 pairs
13. Install cable gland seal nuts and tighten
14. Final tug test on all 220V connections before closing

---

*Enclosure dimensions: 300mm W × 400mm H × 200mm D (IP65, ABS)*
*DIN rail height from bottom wall: approximately 280mm*
